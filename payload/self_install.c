/*
 * self_install.c — install Quaza into the PS5 game library via a fake PKG.
 *
 * ELF Arsenal reverse-engineered approach:
 *   1. Use our own pkg_builder to create a minimal tile PKG in /data/quaza/.
 *   2. Call sceAppInstUtilAppInstallPkg() on that PKG file.
 *      This is the same function ELF Arsenal uses — it installs a fake/unsigned
 *      PKG directly on the jailbroken console's content database.
 *   3. Write a sentinel file so subsequent runs skip the whole process.
 *
 * The old approach (AppPrepareInstall → write files → AppInstall) was wrong:
 *   AppPrepareInstall is a PSN download-staging call and always failed for
 *   direct installs, causing an early abort before any files were written.
 */

#include "self_install.h"
#include "jailbreak_detect.h"
#include "kstuff_lite.h"
#include "tile_pkg_embed.h"     /* g_tile_pkg_data / g_tile_pkg_size — built at compile time */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ── App identity ────────────────────────────────────────────────────── */
#define QUAZA_TITLE_ID    "QUAZ00001"
#define QUAZA_CONTENT_ID  "EP0000-QUAZ00001_00-QUAZAPKGTOOL0001"

/* Working paths (writable on jailbroken PS5) */
#define QUAZA_DATA_DIR    "/data/quaza"
#define TILE_PKG_PATH     QUAZA_DATA_DIR "/quaza-tile.pkg"
#define INSTALL_STAMP     QUAZA_DATA_DIR "/.installed"

/* ── sceAppInstUtil bindings ─────────────────────────────────────────── */
extern int sceAppInstUtilInitialize(void);
/*
 * sceAppInstUtilAppInstallPkg — installs a (fake) PKG file.
 * This is what ELF Arsenal uses.  The second arg is an options struct;
 * NULL is accepted and means "use defaults".
 */
extern int sceAppInstUtilAppInstallPkg(const char *pkg_path, void *options);
extern int sceAppInstUtilAppRecover(const char *title_id);
extern int sceUserServiceInitialize(void *param);

/* ── Helpers ─────────────────────────────────────────────────────────── */

static int make_dir(const char *path) {
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        fprintf(stderr, "[install] mkdir(%s): %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int write_file(const char *path, const void *data, size_t size) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "[install] open(%s): %s\n", path, strerror(errno));
        return -1;
    }
    ssize_t w = write(fd, data, size);
    close(fd);
    if (w != (ssize_t)size) {
        fprintf(stderr, "[install] short write to %s\n", path);
        return -1;
    }
    return 0;
}

/* ── Public API ──────────────────────────────────────────────────────── */

int self_install_is_done(void) {
    return access(INSTALL_STAMP, F_OK) == 0;
}

install_result_t self_install_run(void) {
    if (self_install_is_done()) {
        printf("[install] Already installed — skipping.\n");
        return INSTALL_OK;
    }

    printf("[install] First run — installing Quaza tile to game library...\n");

    /* ── Pre-flight: confirm jailbreak bridge is active ────────────── */
    jb_type_t jb = jailbreak_detect();
    printf("[install] Jailbreak environment: %s\n", jailbreak_name(jb));

    if (!jailbreak_can_install(jb)) {
        printf("[install] No kstuff bridge found — skipping install.\n"
               "[install]   Inject kstuff first, then re-inject Quaza.\n"
               "[install]   All server features still work without the tile.\n");
        return INSTALL_SKIPPED;
    }

    if (kstuff_init() != 0) {
        fprintf(stderr, "[install] kstuff_init() failed — bridge may have crashed.\n");
        return INSTALL_ERROR;
    }
    printf("[install] kstuff-lite ready.\n");

    /* ── Step 1: write the embedded tile PKG to disk ───────────────────
     *
     * g_tile_pkg_data / g_tile_pkg_size come from tile_pkg_embed.h,
     * generated at compile time by:
     *   payload/tools/pkg_builder_main.c  (native PKG builder)
     *   payload/tools/gen_embed_pkg.py    (C header generator)
     *
     * Fully offline — no runtime PKG assembly, no internet needed.
     */
    printf("[install] Writing tile PKG (%zu bytes) → %s\n",
           g_tile_pkg_size, TILE_PKG_PATH);

    if (make_dir(QUAZA_DATA_DIR) != 0) return INSTALL_ERROR;
    if (write_file(TILE_PKG_PATH, g_tile_pkg_data, g_tile_pkg_size) != 0)
        return INSTALL_ERROR;

    /* ── Step 2: install the PKG ────────────────────────────────────── */
    printf("[install] Initialising sceAppInstUtil...\n");
    int rc;
    rc = sceUserServiceInitialize(NULL);
    if (rc != 0)
        fprintf(stderr, "[install] sceUserServiceInitialize rc=0x%08x "
                "(may be already init)\n", (unsigned)rc);

    rc = sceAppInstUtilInitialize();
    if (rc != 0)
        fprintf(stderr, "[install] sceAppInstUtilInitialize rc=0x%08x\n",
                (unsigned)rc);

    printf("[install] Installing PKG via sceAppInstUtilAppInstallPkg...\n");
    rc = sceAppInstUtilAppInstallPkg(TILE_PKG_PATH, NULL);
    if (rc != 0) {
        fprintf(stderr, "[install] sceAppInstUtilAppInstallPkg rc=0x%08x\n",
                (unsigned)rc);
        return INSTALL_ERROR;
    }

    /* AppRecover triggers the shell UI refresh so the icon appears now */
    rc = sceAppInstUtilAppRecover(QUAZA_TITLE_ID);
    if (rc != 0)
        fprintf(stderr, "[install] sceAppInstUtilAppRecover rc=0x%08x "
                "(icon may appear after reboot)\n", (unsigned)rc);

    /* ── Step 4: write sentinel ──────────────────────────────────────── */
    if (write_file(INSTALL_STAMP, "1", 1) != 0)
        fprintf(stderr, "[install] Warning: no sentinel — will retry next run.\n");

    printf("[install] ✓ Quaza tile installed — check your PS5 game library!\n");
    return INSTALL_OK;
}
