#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "pkg_server.h"
#include "browser_launch.h"
#include "self_install.h"
#include "netlog.h"

/*
 * Quaza — PS5 PKG Tool
 * ====================
 * Startup sequence — notification fires FIRST so the user always gets
 * visual feedback even if server / thread init fails.
 *
 *  0. UDP netlog init   — broadcast every log line to dev machine (nc -ul 9020)
 *  1. USB/file log init — dup2 stdout+stderr to quaza.log if USB present
 *  2. Resolve LAN URL   — getifaddrs, fallback to 127.0.0.1
 *  3. Notification      — user sees URL on PS5 immediately
 *  4. HTTP server       — port 4242 (non-fatal if socket fails)
 *  5. Server thread     — (non-fatal)
 *  6. Browser launch    — (non-fatal)
 *  7. Tile install      — background thread (non-fatal)
 *  8. Block            — until SIGINT / SIGTERM
 *
 * --- RUNTIME LOG CHANNELS ---
 *
 *  A. elfldr TCP pipe  — push_to_ps5.py stays connected and streams back
 *                         all stdout/stderr printed before USB redirect.
 *                         Works automatically; no extra setup needed.
 *
 *  B. UDP broadcast    — every NL_LOG() line goes to 255.255.255.255:9020.
 *                         On the dev machine (Termux / PC):
 *                           nc -ul 9020
 *                         Works even after elfldr closes the pipe, and even
 *                         when USB logging is active.
 *
 *  C. USB / file log   — if /mnt/usb0 or /mnt/usb1 is mounted a quaza.log
 *                         is written there.  Falls back to /data/quaza/.
 */

/* Log to both stdout (→ elfldr pipe) and UDP netlog */
#define NL_LOG(fmt, ...) \
    do { \
        printf(fmt "\n", ##__VA_ARGS__); \
        fflush(stdout); \
        netlog_printf("[quaza] " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#define NL_ERR(fmt, ...) \
    do { \
        fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
        fflush(stderr); \
        netlog_printf("[quaza][ERR] " fmt "\n", ##__VA_ARGS__); \
    } while (0)

static volatile int g_running = 1;

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
    pkg_server_stop();
}

static void *server_thread(void *arg) {
    (void)arg;
    pkg_server_run();
    return NULL;
}

static void *install_thread(void *arg) {
    (void)arg;
    switch (self_install_run()) {
    case INSTALL_OK:
        NL_LOG("[install] Tile installed.");
        break;
    case INSTALL_SKIPPED:
        NL_LOG("[install] Tile already present — skipped.");
        break;
    case INSTALL_ERROR:
        NL_ERR("[install] Failed to install tile.");
        break;
    }
    return NULL;
}

/* ── USB / file log redirect ─────────────────────────────────────────────────
 * Opens quaza.log on the first writable USB or internal path, then dup2()'s
 * stdout and stderr to it.
 *
 * NOTE: after this point, printf/fprintf no longer reach the elfldr pipe.
 * Use NL_LOG / netlog_printf for output that must reach the dev machine.
 * Returns the log path, or NULL if no writable path found.
 */
static const char *usb_log_init(void) {
    static const char *candidates[] = {
        "/mnt/usb0/quaza.log",
        "/mnt/usb1/quaza.log",
        "/data/quaza/quaza.log",
        NULL
    };

    for (int i = 0; candidates[i]; i++) {
        int fd = open(candidates[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) continue;

        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        printf("=== Quaza log — started ===\n");
        printf("Log path: %s\n\n", candidates[i]);
        fflush(stdout);

        netlog_printf("[quaza] USB log active: %s\n", candidates[i]);
        return candidates[i];
    }
    return NULL;   /* no file — output stays on elfldr pipe */
}

int main(int argc, char **argv, char **envp) {
    (void)argc; (void)argv; (void)envp;

    /* ── 0. UDP netlog — before ANYTHING so early crashes are visible ─── */
    netlog_init();
    netlog_printf("[quaza] === BOOT ===\n");

    /* ── 1. USB log ──────────────────────────────────────────────────── */
    const char *log_path = usb_log_init();
    if (!log_path)
        NL_LOG("[main] No USB/file log — stdout on elfldr pipe.");
    else
        NL_LOG("[main] File log: %s", log_path);

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* ── 2. Resolve LAN URL ──────────────────────────────────────────── */
    char url[128];
    browser_get_local_url(url, sizeof(url));
    NL_LOG("[main] URL: %s", url);

    /* ── 3. Notification FIRST ───────────────────────────────────────── */
    int notif_rc = browser_notify(url);
    NL_LOG("[main] browser_notify rc=0x%08x", (unsigned int)notif_rc);

    /* ── 4 + 5. HTTP server ──────────────────────────────────────────── */
    int server_ok = 0;
    int srv_rc = pkg_server_init();
    NL_LOG("[main] pkg_server_init rc=%d", srv_rc);

    if (srv_rc >= 0) {
        pthread_t srv_tid;
        int thr_rc = pthread_create(&srv_tid, NULL, server_thread, NULL);
        NL_LOG("[main] server thread create rc=%d", thr_rc);
        if (thr_rc == 0) {
            pthread_detach(srv_tid);
            server_ok = 1;
        } else {
            NL_ERR("[main] Could not spawn server thread.");
        }
    } else {
        NL_ERR("[main] Could not bind port %d — server disabled.", PKG_SERVER_PORT);
    }

    /* ── 6. Launch browser ──────────────────────────────────────────── */
    NL_LOG("[main] Launching browser...");
    browser_launch_app(url);
    NL_LOG("[main] browser_launch_app returned.");

    /* ── 7. Install tile ────────────────────────────────────────────── */
    pthread_t inst_tid;
    int inst_rc = pthread_create(&inst_tid, NULL, install_thread, NULL);
    NL_LOG("[main] install thread create rc=%d", inst_rc);
    if (inst_rc == 0)
        pthread_detach(inst_tid);
    else
        NL_ERR("[main] Could not spawn install thread.");

    /* ── 8. Block ───────────────────────────────────────────────────── */
    NL_LOG("[main] Running. server=%s", server_ok ? "yes" : "no");
    while (g_running) sleep(1);

    NL_LOG("[main] Shutdown.");
    return 0;
}
