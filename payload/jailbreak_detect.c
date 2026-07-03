#include "jailbreak_detect.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * Detection strategy (FW 12.xx / FreeBSD, no kstuff APIs called)
 * ──────────────────────────────────────────────────────────────
 * kstuff-lite  — kstuff_init() connects to a UNIX-domain socket or a
 *                device node.  We check both known paths before ever
 *                calling kstuff_init(), so calling it afterwards is safe.
 *
 *   /dev/kstuff          — device node exposed by recent kstuff-lite builds
 *   /tmp/kstuff.sock     — UNIX socket used by older kstuff-lite builds
 *
 * etaHEN       — creates /data/etaHEN/ on first launch for config/logs.
 *
 * GoldHEN      — creates /data/GoldHEN/ on first launch.
 *
 * Process scanning (kinfo_getproc / /proc/*/comm) is intentionally omitted:
 *   - on PS5 /proc is not always mounted
 *   - the directory checks are faster, sufficient, and panic-safe
 */

/* ── kstuff-lite probe paths ─────────────────────────────────────────── */
static const char *const KSTUFF_PATHS[] = {
    "/dev/kstuff",
    "/tmp/kstuff.sock",
    NULL
};

/* ── etaHEN / GoldHEN marker directories ────────────────────────────── */
#define ETAHEN_DIR  "/data/etaHEN"
#define GOLDHEN_DIR "/data/GoldHEN"

/* ── Helpers ─────────────────────────────────────────────────────────── */
static inline int path_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static inline int is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* ── Public API ──────────────────────────────────────────────────────── */

jb_type_t jailbreak_detect(void) {
    int result = JB_NONE;

    /* kstuff-lite: any of the known bridge paths */
    for (int i = 0; KSTUFF_PATHS[i] != NULL; i++) {
        if (path_exists(KSTUFF_PATHS[i])) {
            printf("[jb] kstuff-lite detected via %s\n", KSTUFF_PATHS[i]);
            result |= JB_KSTUFF;
            break;
        }
    }

    /* etaHEN */
    if (is_dir(ETAHEN_DIR)) {
        printf("[jb] etaHEN detected (%s)\n", ETAHEN_DIR);
        result |= JB_ETAHEN;
    }

    /* GoldHEN */
    if (is_dir(GOLDHEN_DIR)) {
        printf("[jb] GoldHEN detected (%s)\n", GOLDHEN_DIR);
        result |= JB_GOLDHEN;
    }

    if (result == JB_NONE)
        printf("[jb] No jailbreak detected.\n");

    return (jb_type_t)result;
}

const char *jailbreak_name(jb_type_t jb) {
    /*
     * Static buffer — fine for a one-time startup message.
     * Not re-entrant, but this is only called from main thread.
     */
    static char buf[128];
    buf[0] = '\0';

    if (jb == JB_NONE)
        return "none";

    if (jb & JB_KSTUFF)  { if (buf[0]) strncat(buf, " + ", sizeof(buf)-strlen(buf)-1);
                            strncat(buf, "kstuff-lite", sizeof(buf)-strlen(buf)-1); }
    if (jb & JB_ETAHEN)  { if (buf[0]) strncat(buf, " + ", sizeof(buf)-strlen(buf)-1);
                            strncat(buf, "etaHEN",      sizeof(buf)-strlen(buf)-1); }
    if (jb & JB_GOLDHEN) { if (buf[0]) strncat(buf, " + ", sizeof(buf)-strlen(buf)-1);
                            strncat(buf, "GoldHEN",     sizeof(buf)-strlen(buf)-1); }
    return buf;
}

int jailbreak_can_install(jb_type_t jb) {
    /*
     * kstuff-lite provides the kernel bridge we route through.
     * etaHEN and GoldHEN on FW ≤9.xx also load kstuff internally,
     * so /dev/kstuff is typically present alongside those flags too.
     */
    return (jb & JB_KSTUFF) != 0;
}
