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
 * Startup sequence — browser + server fire FIRST so the user sees
 * immediate feedback; install happens on a background thread.
 *
 *  1. Resolve LAN URL (getifaddrs)
 *  2. Init HTTP server + background thread (port 4242)
 *  3. Send on-screen notification with URL
 *  4. Launch PS5 browser (sceSystemServiceLaunchApp)
 *  5. Auto-install tile to game library (background, non-blocking)
 *  6. Block until SIGINT / SIGTERM
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
        printf("[install] ✓ Tile installed — check your PS5 game library.\n");
        break;
    case INSTALL_SKIPPED:
        printf("[install] Skipped (no kstuff bridge yet).\n");
        break;
    case INSTALL_ERROR:
        fprintf(stderr, "[install] Failed — continuing as injected payload.\n");
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

    /* ── 2. Initialise HTTP server ────────────────────────────────────── */
    if (pkg_server_init() < 0) {
        fprintf(stderr, "[main] Fatal: could not bind port %d.\n",
                PKG_SERVER_PORT);
        return 1;
    }

    pthread_t srv_tid;
    if (pthread_create(&srv_tid, NULL, server_thread, NULL) != 0) {
        fprintf(stderr, "[main] Fatal: could not spawn server thread.\n");
        return 1;
    }

    /* ── 3. Show URL on screen ────────────────────────────────────────── */
    printf("\n");
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║           Quaza  PS5 PKG Tool  v1.0         ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  Web UI:  %-34s  ║\n", url);
    printf("╚══════════════════════════════════════════════╝\n\n");

    /* ── 4. Notification + browser — happens immediately ─────────────── */
    browser_launch(url);

    /* ── 5. Auto-install on background thread (non-blocking) ─────────── *
     * Install runs AFTER browser launch so the user gets immediate
     * feedback even on first run.  kstuff_init() can be slow; putting
     * it here prevents it from blocking browser notification.
     */
    pthread_t inst_tid;
    if (pthread_create(&inst_tid, NULL, install_thread, NULL) == 0)
        pthread_detach(inst_tid);   /* fire-and-forget; don't join */
    else
        fprintf(stderr, "[main] Warning: could not spawn install thread.\n");

    /* ── 6. Block until signal ────────────────────────────────────────── */
    printf("[main] Quaza running — press PS button to switch apps.\n");
    while (g_running) sleep(1);

    printf("[main] Shutting down...\n");
    pthread_join(srv_tid, NULL);
    printf("[main] Done.\n");
    return 0;
}
