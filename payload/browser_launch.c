/*
 * browser_launch.c — PS5 notification + browser open for Quaza.
 *
 * Split into two functions so main.c can fire the notification
 * BEFORE attempting socket/thread init — the user always sees
 * something on screen even if the server fails to start.
 */

#include "browser_launch.h"
#include "pkg_server.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>

/* ── PS5 system API declarations ─────────────────────────────────────── */
extern int sceUserServiceInitialize(void *param);
extern int sceSystemServiceLaunchApp(const char *title_id,
                                      const char **argv, void *options);
extern int sceLncUtilLaunchWebBrowser2(const char *url, size_t url_len,
                                        unsigned long long flags);

/* ── Notification struct (matches PS5 kernel ABI) ────────────────────── */
typedef struct {
    int  type;           /* 0 = normal toast notification */
    int  req_id;
    int  priority;
    int  msg_id;
    int  target_id;      /* -1 = all users */
    int  user_id;
    int  unk1;
    int  unk2;
    char icon_uri[1024];
    char message[1024];
} SceNotificationRequest;

extern int sceKernelSendNotificationRequest(int,
                                             SceNotificationRequest *,
                                             size_t, int);

/* ── Get LAN IP via getifaddrs ───────────────────────────────────────── */
static int get_local_ip(char *buf, size_t buflen) {
    struct ifaddrs *ifap = NULL;
    if (getifaddrs(&ifap) != 0) return -1;

    int found = 0;
    for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        unsigned int addr = (unsigned int)__builtin_bswap32(
                                (unsigned int)sa->sin_addr.s_addr);
        if ((addr >> 24) == 127)   continue; /* loopback */
        if ((addr >> 16) == 0xa9fe) continue; /* link-local */
        inet_ntop(AF_INET, &sa->sin_addr, buf, (unsigned int)buflen);
        found = 1;
        break;
    }
    freeifaddrs(ifap);
    return found ? 0 : -1;
}

/* ── Public API ──────────────────────────────────────────────────────── */

int browser_get_local_url(char *buf, size_t buflen) {
    char ip[64];
    ip[0] = '\0';
    if (get_local_ip(ip, sizeof(ip)) != 0 || ip[0] == '\0')
        snprintf(ip, sizeof(ip), "127.0.0.1");
    snprintf(buf, buflen, "http://%s:%d", ip, PKG_SERVER_PORT);
    return 0;
}

/*
 * browser_notify — send on-screen toast with the Quaza URL.
 * Called BEFORE server init so the user always gets visual feedback.
 */
int browser_notify(const char *url) {
    SceNotificationRequest notif;
    memset(&notif, 0, sizeof(notif));
    notif.type      = 0;
    notif.target_id = -1;
    snprintf(notif.message, sizeof(notif.message),
             "Quaza running\n%s", url);

    int rc = sceKernelSendNotificationRequest(0, &notif, sizeof(notif), 0);
    if (rc != 0)
        fprintf(stderr, "[notify] sceKernelSendNotificationRequest "
                "rc=0x%08x\n", (unsigned int)rc);
    else
        printf("[notify] Notification sent: %s\n", url);
    return rc;
}

/*
 * browser_launch_app — open the PS5 Internet Browser.
 * Tries known browser title IDs; falls back to sceLncUtil.
 * Non-fatal — caller ignores return value.
 */
int browser_launch_app(const char *url) {
    if (!url || !url[0]) return -1;

    /* Small delay so server thread has time to start listening */
    sleep(1);

    static const char *browser_ids[] = {
        "NPXS40106",   /* Internet Browser FW 3.x+ */
        "NPXS40113",   /* newer FW variant          */
        "NPXS40104",
        "NPXS40103",
        "NPXS40037",
        NULL
    };

    for (int i = 0; browser_ids[i]; i++) {
        int rc = sceSystemServiceLaunchApp(browser_ids[i], NULL, NULL);
        printf("[browser] LaunchApp(%s) rc=0x%08x\n",
               browser_ids[i], (unsigned int)rc);
        if (rc == 0) {
            printf("[browser] Browser opened via %s\n", browser_ids[i]);
            return 0;
        }
    }

    /* Last resort */
    printf("[browser] Trying sceLncUtilLaunchWebBrowser2...\n");
    int rc = sceLncUtilLaunchWebBrowser2(url, strlen(url), 0ULL);
    printf("[browser] sceLncUtilLaunchWebBrowser2 rc=0x%08x\n",
           (unsigned int)rc);
    return (rc == 0) ? 0 : -1;
}
