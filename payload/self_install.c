#include "self_install.h"
#include "kstuff_lite.h"
#include "sfo_embed.h"      /* g_param_sfo_data / g_param_sfo_size  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

/* ── App identity ────────────────────────────────────────────────────── */
#define QUAZA_TITLE_ID   "QUAZ00001"
#define QUAZA_CONTENT_ID "EP0000-QUAZ00001_00-QUAZAPKGTOOL0001"
#define INSTALL_BASE     "/user/app/" QUAZA_TITLE_ID

/* ── sceAppInstUtil bindings ─────────────────────────────────────────── */
/*
 * These are resolved at link time via libSceAppInstUtil from the
 * PS5 Payload SDK.  Prototypes sourced from ps5-payload-dev headers.
 */
extern int sceAppInstUtilInitialize(void);
extern int sceAppInstUtilGetAppList(char **list_out, int *count_out);
extern int sceAppInstUtilAppPrepareInstall(const char *content_id,
                                           const char *title_id,
                                           int        flags);
extern int sceAppInstUtilAppInstall(const char *title_id);
extern int sceAppInstUtilAppRecover(const char *title_id);   /* triggers DB refresh */
extern int sceUserServiceInitialize(void *param);            /* NULL for defaults  */

/* ── Helpers ─────────────────────────────────────────────────────────── */

/* Create a directory (and parents) — ignores EEXIST. */
static int mkdirs(const char *path) {
    char tmp[512];
    strncpy(tmp, path, sizeof(tmp) - 1);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) < 0 && errno != EEXIST) {
                fprintf(stderr, "[install] mkdir(%s): %s\n", tmp, strerror(errno));
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) < 0 && errno != EEXIST) {
        fprintf(stderr, "[install] mkdir(%s): %s\n", tmp, strerror(errno));
        return -1;
    }
    return 0;
}

/* Write a byte buffer to a file, creating or overwriting it. */
static int write_file(const char *path,
                      const void *data, size_t size) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "[install] open(%s): %s\n", path, strerror(errno));
        return -1;
    }
    ssize_t written = write(fd, data, size);
    close(fd);
    if (written != (ssize_t)size) {
        fprintf(stderr, "[install] short write to %s\n", path);
        return -1;
    }
    return 0;
}

/* Copy the running ELF to dest_path for use as eboot.bin. */
static int copy_self(const char *dest_path) {
    /* On PS5 (FreeBSD) the running process image is at /proc/self/exe */
    const char *self_path = "/proc/self/exe";
    int src = open(self_path, O_RDONLY);
    if (src < 0) {
        fprintf(stderr, "[install] Cannot open self (%s): %s\n",
                self_path, strerror(errno));
        return -1;
    }

    int dst = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (dst < 0) {
        fprintf(stderr, "[install] Cannot create eboot.bin: %s\n", strerror(errno));
        close(src);
        return -1;
    }

    char buf[65536];
    ssize_t n;
    int ok = 1;
    while ((n = read(src, buf, sizeof(buf))) != 0) {
        if (n < 0) {
            fprintf(stderr, "[install] Read error from %s: %s\n",
                    self_path, strerror(errno));
            ok = 0;
            break;
        }
        if (write(dst, buf, (size_t)n) != n) {
            fprintf(stderr, "[install] Write error for eboot.bin: %s\n",
                    strerror(errno));
            ok = 0;
            break;
        }
    }
    close(src);
    close(dst);
    if (!ok) {
        unlink(dest_path);   /* remove partial file so it can't be used */
        return -1;
    }
    return 0;
}

/* ── Sentinel file ───────────────────────────────────────────────────── */
/* We write a small stamp file after a successful install so subsequent
 * runs skip the whole process without calling sceAppInstUtil again.    */
#define INSTALL_STAMP  INSTALL_BASE "/sce_sys/.quaza_installed"

int self_install_is_done(void) {
    return access(INSTALL_STAMP, F_OK) == 0;
}

/* ── Main install routine ────────────────────────────────────────────── */
int self_install_run(void) {
    if (self_install_is_done()) {
        printf("[install] Already installed — skipping.\n");
        return 0;
    }

    printf("[install] First run detected — installing Quaza to game library...\n");

    /* ── Step 1: kstuff-lite — escalate privileges ───────────────────── */
    printf("[install] Initialising kstuff-lite...\n");
    if (kstuff_init() != 0) {
        fprintf(stderr, "[install] kstuff_init() failed — cannot write to /user/app/\n");
        fprintf(stderr, "[install] Ensure the jailbreak (GoldHEN / etaHEN) is active.\n");
        return -1;
    }
    printf("[install] kstuff-lite ready.\n");

    /*
     * 0x80990085 — sceAppInstUtil "already prepared / title already exists"
     * This is the only non-zero code from AppPrepareInstall we treat as benign.
     * All other non-zero codes are genuine failures.
     */
#define SCE_APPINSTUTIL_ERROR_ALREADY_EXISTS  0x80990085u

    /* ── Step 2: initialise user/app-install services ───────────────── */
    int rc;
    rc = sceUserServiceInitialize(NULL);
    if (rc != 0) {
        fprintf(stderr, "[install] sceUserServiceInitialize() rc=0x%08x\n",
                (unsigned)rc);
        return -1;
    }
    rc = sceAppInstUtilInitialize();
    if (rc != 0) {
        fprintf(stderr, "[install] sceAppInstUtilInitialize() rc=0x%08x\n",
                (unsigned)rc);
        return -1;
    }

    /* ── Step 3: reserve the TITLE_ID slot in the content database ──── */
    printf("[install] Reserving title slot %s...\n", QUAZA_TITLE_ID);
    rc = sceAppInstUtilAppPrepareInstall(QUAZA_CONTENT_ID, QUAZA_TITLE_ID, 0);
    if (rc != 0 && (unsigned)rc != SCE_APPINSTUTIL_ERROR_ALREADY_EXISTS) {
        fprintf(stderr, "[install] sceAppInstUtilAppPrepareInstall() "
                "rc=0x%08x — aborting.\n", (unsigned)rc);
        return -1;
    }
    if ((unsigned)rc == SCE_APPINSTUTIL_ERROR_ALREADY_EXISTS)
        printf("[install] Title slot already reserved — continuing.\n");

    /* ── Step 4: write the app directory structure ───────────────────── */
    printf("[install] Creating app directory structure...\n");

    if (mkdirs(INSTALL_BASE "/sce_sys") != 0) return -1;

    /* param.sfo — embedded at build time by gen_param_sfo.py */
    printf("[install] Writing param.sfo...\n");
    if (write_file(INSTALL_BASE "/sce_sys/param.sfo",
                   g_param_sfo_data, g_param_sfo_size) != 0) return -1;

    /* icon0.png — look alongside the ELF; skip (default icon) if absent */
    const char *icon_src = "/data/pkg_tool/sce_sys/icon0.png";
    if (access(icon_src, R_OK) == 0) {
        printf("[install] Copying icon0.png...\n");
        int isrc = open(icon_src, O_RDONLY);
        if (isrc >= 0) {
            struct stat st;
            if (fstat(isrc, &st) == 0 && st.st_size > 0) {
                char *ibuf = malloc((size_t)st.st_size);
                if (ibuf) {
                    ssize_t nr = read(isrc, ibuf, (size_t)st.st_size);
                    if (nr == (ssize_t)st.st_size)
                        write_file(INSTALL_BASE "/sce_sys/icon0.png",
                                   ibuf, (size_t)st.st_size);
                    else
                        fprintf(stderr, "[install] icon0.png read short — skipping\n");
                    free(ibuf);
                }
            }
            close(isrc);
        }
    } else {
        printf("[install] icon0.png not found at %s — "
               "game library will show default icon.\n", icon_src);
    }

    /* eboot.bin — self-copy so clicking the icon re-launches us */
    printf("[install] Writing eboot.bin (self-copy)...\n");
    if (copy_self(INSTALL_BASE "/eboot.bin") != 0) return -1;

    /* ── Step 5: register with the PS5 content database ─────────────── */
    printf("[install] Registering with PS5 content database...\n");
    rc = sceAppInstUtilAppInstall(QUAZA_TITLE_ID);
    if (rc != 0) {
        fprintf(stderr, "[install] sceAppInstUtilAppInstall() rc=0x%08x\n",
                (unsigned)rc);
        /* Fall through to AppRecover — it can succeed even when AppInstall
         * returns a soft error on already-staged installs.              */
    }

    rc = sceAppInstUtilAppRecover(QUAZA_TITLE_ID);
    if (rc != 0) {
        fprintf(stderr, "[install] sceAppInstUtilAppRecover() rc=0x%08x "
                "— DB refresh may not have taken effect.\n", (unsigned)rc);
        /* Still considered a failed install; do NOT write sentinel. */
        return -1;
    }

    /* ── Step 6: write sentinel ONLY on confirmed success ────────────── */
    if (write_file(INSTALL_STAMP, "1", 1) != 0) {
        fprintf(stderr, "[install] Warning: could not write install stamp — "
                "will re-run install next launch (harmless).\n");
    }

    printf("[install] ✓ Quaza installed! "
           "It will appear in your PS5 game library.\n");
    printf("[install]   You can now launch it from the library "
           "instead of injecting the ELF.\n");
    return 0;
}
