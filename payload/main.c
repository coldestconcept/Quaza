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
 * Entry point for the PS5 ELF payload.
 *
 * Startup sequence
 * ────────────────
 *  1. Auto-install  — on first run, write the app to /user/app/QUAZ00001/
 *                     via kstuff-lite + sceAppInstUtil so Quaza appears in
 *                     the PS5 game library (same as ELF Arsenal / Kuro / PM).
 *                     Subsequent runs detect the stamp file and skip this step.
 *
 *  2. HTTP server   — bind port 4242, serve embedded www/ assets from memory
 *                     (offline mode — no files on disk required).
 *
 *  3. Browser launch — call sceLncUtilLaunchWebBrowser2 to open the PS5
 *                      browser at http://127.0.0.1:4242 automatically.
 *
 *  4. Run forever   — block until SIGINT / SIGTERM.
 *
 * Offline web assets
 * ──────────────────
 * All HTML/JS files are compiled into the ELF via payload/www_embed.h.
 * Regenerate with: python3 payload/tools/gen_embed.py
 *
 * Icon / fPKG
 * ───────────
 * param.sfo is embedded via payload/sfo_embed.h (auto-generated).
 * Place icon0.png at /data/pkg_tool/sce_sys/icon0.png before first run,
 * or provide your own at payload/sce_sys/icon0.png and rebuild.
 * See docs/INSTALL.md for the full build + install flow.
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

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("╔══════════════════════════════════════╗\n");
    printf("║          Quaza  PS5 PKG Tool         ║\n");
    printf("║       Native Homebrew Payload        ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* ── 1. Auto-install to game library (first run only) ────────────── */
    switch (self_install_run()) {
    case INSTALL_OK:
        printf("[main] Game library install complete.\n");
        break;
    case INSTALL_SKIPPED:
        /* kstuff not yet active — expected on first injection without kstuff */
        printf("[main] Install skipped (no kstuff bridge). "
               "Re-inject after loading kstuff to auto-install to game library.\n");
        break;
    case INSTALL_ERROR:
        fprintf(stderr, "[main] Install attempted but failed — "
                "continuing as injected payload.\n");
        break;
    }

    /* ── 2. Initialise HTTP server ────────────────────────────────────── */
    printf("[main] Starting HTTP server on port %d...\n", PKG_SERVER_PORT);
    if (pkg_server_init() < 0) {
        fprintf(stderr, "[main] Fatal: could not bind port %d.\n", PKG_SERVER_PORT);
        return 1;
    }

    /* ── 3. Serve on background thread ───────────────────────────────── */
    pthread_t srv_tid;
    if (pthread_create(&srv_tid, NULL, server_thread, NULL) != 0) {
        fprintf(stderr, "[main] Fatal: could not spawn server thread.\n");
        return 1;
    }

    /* ── 4. Auto-open PS5 browser ─────────────────────────────────────── */
    char url[64];
    snprintf(url, sizeof(url), "http://127.0.0.1:%d", PKG_SERVER_PORT);
    printf("[main] Opening PS5 browser → %s\n", url);
    browser_launch(url);

    /* ── 5. Block until signal ────────────────────────────────────────── */
    printf("[main] Quaza running.  Press PS button to switch apps.\n");
    while (g_running) sleep(1);

    printf("[main] Shutting down...\n");
    pthread_join(srv_tid, NULL);
    printf("[main] Done.\n");
    return 0;
}
