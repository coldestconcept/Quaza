/*
 * browser_launch.c — PS5 notification + browser open for Quaza.
 *
 * Reference: elf-arsenal DT_NEEDED list uses libkernel_sys.sprx +
 * libSceSystemService.sprx. game-compressor uses libSceNotification.sprx.
 * Neither uses libSceLncUtil — removed entirely.
 *
 * Function split:
 *   browser_notify()     — send on-screen toast (call FIRST, before socket init)
 *   browser_launch_app() — open PS5 browser via sceSystemServiceLaunchApp
 */

#include "browser_launch.h"
#include "pkg_server.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

/* ── PS5 API — resolved from DT_NEEDED libraries at runtime ─────────── */
extern int sceUserServiceInitialize(void *param);
extern int sceSystemServiceLaunchApp(const char *title_id,
                                      const char **argv, void *options);

/* sceKernelSendNotificationRequest — libkernel_web.sprx */
typedef struct {
    int  type;        /* 0 = normal toast */
    int  req_id;
    int  priority;
    int  msg_id;
    int  target_id;   /* -1 = all users */
    int  user_id;
    int  unk1;
    int  unk2;
    char icon_uri[1024];
    char message[1024];
} SceNotificationRequest;

extern int sceKernelSendNotificationRequest(int,
                                             SceNotificationRequest *,
                                             size_t, int);

/* ── Get LAN IP ──────────────────────────────────────────────────────── */
static int get_local_ip(char *buf, size_t buflen) {
    struct ifaddrs *ifap = NULL;
    if (getifaddrs(&ifap) != 0) return -1;
    int found = 0;
    for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        unsigned char *b = (unsigned char *)&sa->sin_addr.s_addr;
        if (b[0] == 127) continue;                   /* loopback */
        if (b[0] == 169 && b[1] == 254) continue;   /* link-local */
        inet_ntop(AF_INET, &sa->sin_addr, buf, (unsigned int)buflen);
        found = 1;
        break;
    }
    freeifaddrs(ifap);
    return found ? 0 : -1;
}

/* ── Public API ──────────────────────────────────────────────────────── */

int browser_get_local_url(char *buf, size_t buflen) {
    char ip[64] = "127.0.0.1";
    get_local_ip(ip, sizeof(ip));
    snprintf(buf, buflen, "http://%s:%d", ip, PKG_SERVER_PORT);
    return 0;
}

int browser_notify(const char *url) {
    SceNotificationRequest notif;
    memset(&notif, 0, sizeof(notif));
    notif.type      = 0;
    notif.target_id = -1;
    snprintf(notif.message, sizeof(notif.message),
             "Quaza running\n%s", url);

    int rc = sceKernelSendNotificationRequest(0, &notif, sizeof(notif), 0);
    if (rc != 0)
        fprintf(stderr, "[notify] rc=0x%08x\n", (unsigned int)rc);
    else
        printf("[notify] Sent: %s\n", url);
    return rc;
}

int browser_launch_app(const char *url) {
    if (!url || !url[0]) return -1;

    /* Initialize user service — non-fatal if already up */
    int rc = sceUserServiceInitialize(NULL);
    if (rc != 0)
        fprintf(stderr, "[browser] sceUserServiceInitialize rc=0x%08x\n",
                (unsigned int)rc);

    sleep(1); /* let server thread start */

    /* Try known PS5 Internet Browser title IDs (firmware-dependent) */
    static const char *ids[] = {
        "NPXS40106",  /* FW 3.x+ */
        "NPXS40113",
        "NPXS40104",
        "NPXS40103",
        "NPXS40037",
        NULL
    };

    for (int i = 0; ids[i]; i++) {
        rc = sceSystemServiceLaunchApp(ids[i], NULL, NULL);
        printf("[browser] LaunchApp(%s) rc=0x%08x\n",
               ids[i], (unsigned int)rc);
        if (rc == 0) {
            printf("[browser] Opened via %s\n", ids[i]);
            return 0;
        }
    }

    fprintf(stderr, "[browser] All LaunchApp attempts failed.\n");
    return -1;
}
