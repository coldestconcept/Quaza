#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "pkg_server.h"
#include "browser_launch.h"
#include "self_install.h"

/*
 * Quaza — PS5 PKG Tool
 * ====================
 * Startup sequence — notification fires FIRST so the user always gets
 * visual feedback even if server / thread init fails.
 *
 *  1. Resolve LAN URL (getifaddrs — fallback to 127.0.0.1)
 *  2. Send on-screen notification → user sees URL immediately
 *  3. Init HTTP server on port 4242 (non-fatal if socket fails)
 *  4. Spawn server thread (non-fatal if create fails)
 *  5. Launch PS5 browser (non-fatal)
 *  6. Auto-install tile on background thread (non-fatal)
 *  7. Block until SIGINT / SIGTERM
 */

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
        printf("[install] Tile installed.\n");
        break;
    case INSTALL_SKIPPED:
        printf("[install] Skipped.\n");
        break;
    case INSTALL_ERROR:
        fprintf(stderr, "[install] Failed.\n");
        break;
    }
    return NULL;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* ── 1. Resolve LAN URL ──────────────────────────────────────────── */
    char url[128];
    browser_get_local_url(url, sizeof(url));
    printf("[main] Quaza URL: %s\n", url);

    /* ── 2. Notification FIRST — user sees this even if server fails ─── */
    browser_notify(url);

    /* ── 3. HTTP server ─────────────────────────────────────────────── */
    int server_ok = 0;
    if (pkg_server_init() >= 0) {
        pthread_t srv_tid;
        if (pthread_create(&srv_tid, NULL, server_thread, NULL) == 0) {
            pthread_detach(srv_tid);
            server_ok = 1;
            printf("[main] Server running on %s\n", url);
        } else {
            fprintf(stderr, "[main] Could not spawn server thread.\n");
        }
    } else {
        fprintf(stderr, "[main] Could not bind port %d — server disabled.\n",
                PKG_SERVER_PORT);
    }

    /* ── 4. Launch browser ──────────────────────────────────────────── */
    browser_launch_app(url);

    /* ── 5. Install tile on background thread ───────────────────────── */
    pthread_t inst_tid;
    if (pthread_create(&inst_tid, NULL, install_thread, NULL) == 0)
        pthread_detach(inst_tid);
    else
        fprintf(stderr, "[main] Could not spawn install thread.\n");

    /* ── 6. Block until signal ──────────────────────────────────────── */
    printf("[main] Running. server=%s\n", server_ok ? "yes" : "no");
    while (g_running) sleep(1);

    printf("[main] Shutdown.\n");
    return 0;
}
