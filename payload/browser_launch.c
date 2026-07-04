/*
 * browser_launch.c — open the PS5 WebKit browser at the Quaza web UI.
 *
 * ELF Arsenal reverse-engineered approach:
 *   1. Detect the PS5's local LAN IP via getifaddrs().
 *   2. Send a sceKernelSendNotificationRequest so the URL appears as an
 *      on-screen pop-up (visible even if the browser launch fails).
 *   3. Call sceSystemServiceLaunchApp() trying each known PS5 browser
 *      title ID in order — ELF Arsenal does NOT use
 *      sceSystemServiceLaunchWebBrowser at all.
 *   4. Fall back to sceLncUtilLaunchWebBrowser2 as a last resort.
 */

#include "browser_launch.h"
#include "pkg_server.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>

/* ── PS5 system APIs ────────────────────────────────────────────────────
 * All resolved at runtime by ps5-payload-elfldr.
 */
extern int sceUserServiceInitialize(void *param);
extern int sceSystemServiceInitialize(void *param);
extern int sceSystemServiceLaunchApp(const char *title_id,
                                      const char **argv, void *options);
extern int sceLncUtilLaunchWebBrowser2(const char *url, size_t url_len,
                                        unsigned long long flags);

/* ── Notification request (matches PS5 kernel ABI) ──────────────────── */
typedef struct {
    int   type;             /* 0 = normal notification                  */
    int   req_id;           /* 0                                        */
    int   priority;         /* 0                                        */
    int   msg_id;           /* 0                                        */
    int   target_id;        /* -1 = all users                          */
    int   user_id;          /* 0                                        */
    int   unk1;
    int   unk2;
    char  icon_uri[1024];   /* empty = default icon                     */
    char  message[1024];    /* visible text                             */
} SceNotificationRequest;   /* total ≈ 0x830 bytes                      */

extern int sceKernelSendNotificationRequest(int, SceNotificationRequest *,
                                             size_t, int);

/* ── Get the PS5's LAN IP ───────────────────────────────────────────── */
static int get_local_ip(char *buf, size_t buflen) {
    struct ifaddrs *ifap = NULL;
    if (getifaddrs(&ifap) != 0) return -1;

    int found = 0;
    for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        uint32_t addr = ntohl(sa->sin_addr.s_addr);

        /* Skip loopback (127.x.x.x) and link-local (169.254.x.x) */
        if ((addr >> 24) == 127) continue;
        if ((addr >> 16) == 0xa9fe) continue;

        inet_ntop(AF_INET, &sa->sin_addr, buf, (socklen_t)buflen);
        found = 1;
        break;
    }
    freeifaddrs(ifap);
    return found ? 0 : -1;
}

/* ── Public API ─────────────────────────────────────────────────────── */

int browser_get_local_url(char *buf, size_t buflen) {
    char ip[INET_ADDRSTRLEN] = "127.0.0.1";
    get_local_ip(ip, sizeof(ip));
    snprintf(buf, buflen, "http://%s:%d", ip, PKG_SERVER_PORT);
    return 0;
}

int browser_launch(const char *url) {
    if (!url || url[0] == '\0') {
        fprintf(stderr, "[browser] NULL or empty URL\n");
        return -1;
    }

    /* Initialize services — non-fatal if already initialised */
    int rc = sceUserServiceInitialize(NULL);
    if (rc != 0)
        fprintf(stderr, "[browser] sceUserServiceInitialize rc=0x%08x\n",
                (unsigned int)rc);

    rc = sceSystemServiceInitialize(NULL);
    if (rc != 0)
        fprintf(stderr, "[browser] sceSystemServiceInitialize rc=0x%08x\n",
                (unsigned int)rc);

    /* ── Step 1: send on-screen notification with the URL ────────────
     * This is ELF Arsenal's primary user-feedback mechanism.
     * The URL appears as a PS5 system notification even if the
     * automatic browser launch fails.
     */
    SceNotificationRequest notif;
    memset(&notif, 0, sizeof(notif));
    notif.type      = 0;
    notif.target_id = -1;
    snprintf(notif.message, sizeof(notif.message),
             "Quaza PKG Tool\nOpen: %s", url);

    rc = sceKernelSendNotificationRequest(0, &notif, sizeof(notif), 0);
    if (rc != 0)
        fprintf(stderr, "[browser] sceKernelSendNotificationRequest "
                "rc=0x%08x\n", (unsigned int)rc);
    else
        printf("[browser] Notification sent → %s\n", url);

    /* Give the HTTP server a moment to start listening */
    sleep(2);

    /* ── Step 2: launch the PS5 Internet Browser by title ID ─────────
     * ELF Arsenal tries these title IDs in order (firmware-dependent).
     * sceSystemServiceLaunchApp opens the app; the browser will land
     * on its home page — the user can then tap the notification URL.
     */
    static const char *browser_title_ids[] = {
        "NPXS40106",   /* Internet Browser — PS5 FW 3.x+       */
        "NPXS40113",   /* Internet Browser — newer FW variant   */
        "NPXS40104",   /* alternate browser slot                */
        "NPXS40103",   /* alternate browser slot                */
        "NPXS40000",   /* System UI / shell (last resort)       */
        "NPXS40037",   /* another known browser slot            */
        NULL
    };

    fprintf(stdout, "[browser] Attempting sceSystemServiceLaunchApp...\n");
    int launched = 0;
    for (int i = 0; browser_title_ids[i]; i++) {
        rc = sceSystemServiceLaunchApp(browser_title_ids[i], NULL, NULL);
        fprintf(stdout, "[browser] LaunchApp(%s) rc=0x%08x\n",
                browser_title_ids[i], (unsigned int)rc);
        if (rc == 0) {
            printf("[browser] ✓ Browser launched via %s\n",
                   browser_title_ids[i]);
            launched = 1;
            break;
        }
    }

    if (launched) return 0;

    /* ── Step 3: last resort — sceLncUtilLaunchWebBrowser2 ───────────
     * Older payloads use this; may work on some firmware versions.
     */
    fprintf(stderr, "[browser] All LaunchApp attempts failed — "
            "trying sceLncUtilLaunchWebBrowser2...\n");
    rc = sceLncUtilLaunchWebBrowser2(url, strlen(url), 0ULL);
    if (rc != 0)
        fprintf(stderr, "[browser] sceLncUtilLaunchWebBrowser2 "
                "rc=0x%08x\n", (unsigned int)rc);

    return (rc == 0) ? 0 : -1;
}
