#ifndef JAILBREAK_DETECT_H
#define JAILBREAK_DETECT_H

/*
 * jailbreak_detect — safe pre-flight check for active PS5 jailbreak tools.
 *
 * On FW 12.xx (the latest jailbreakable firmware as of mid-2026), kstuff-lite
 * is the standard kernel bridge.  etaHEN and GoldHEN remain in use on older
 * firmware.
 *
 * IMPORTANT: this module deliberately does NOT call any kstuff, etaHEN, or
 * GoldHEN APIs.  It only inspects filesystem paths and directory entries so
 * it is safe to call before any kernel-level work — calling kstuff_init()
 * without a live kstuff bridge will kernel-panic the PS5.
 *
 * Detected flags are ORed together so you can test for multiple tools
 * simultaneously (e.g. kstuff-lite loaded inside etaHEN).
 */

/* Bitmask of detected jailbreak environments */
typedef enum {
    JB_NONE    = 0,   /* No known jailbreak found — do NOT call kstuff APIs */
    JB_KSTUFF  = 1,   /* kstuff-lite bridge active (/dev/kstuff or socket)  */
    JB_ETAHEN  = 2,   /* etaHEN present (/data/etaHEN/)                     */
    JB_GOLDHEN = 4,   /* GoldHEN present (/data/GoldHEN/)                   */
} jb_type_t;

/*
 * jailbreak_detect
 * ────────────────
 * Returns an ORed bitmask of jb_type_t values for each tool whose presence
 * is confirmed.  Returns JB_NONE if nothing is detected.
 *
 * Must be called before any kstuff/etaHEN/GoldHEN API.  Safe on any FW.
 */
jb_type_t jailbreak_detect(void);

/*
 * jailbreak_name
 * ──────────────
 * Returns a human-readable string listing every active tool, e.g.
 * "kstuff-lite" or "kstuff-lite + etaHEN".  Caller must NOT free the result.
 */
const char *jailbreak_name(jb_type_t jb);

/*
 * jailbreak_can_install
 * ─────────────────────
 * Returns 1 if the detected environment provides the kernel write access
 * needed to install to /user/app/, 0 if not.
 *
 * Currently: kstuff-lite (JB_KSTUFF) is sufficient.
 * etaHEN / GoldHEN also provide this via their own kernel primitives, but
 * we always route through kstuff_init() — if etaHEN/GoldHEN ship kstuff
 * internally, /dev/kstuff will be present and JB_KSTUFF will be set too.
 */
int jailbreak_can_install(jb_type_t jb);

#endif /* JAILBREAK_DETECT_H */
