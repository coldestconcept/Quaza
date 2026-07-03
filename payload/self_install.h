#ifndef SELF_INSTALL_H
#define SELF_INSTALL_H

/*
 * self_install — Quaza auto-installation into the PS5 game library.
 *
 * When the ELF is run for the first time (via a payload injector like
 * ps5-payload-elfldr), self_install_run() writes the app structure to
 * /user/app/QUAZ00001/, registers it with the PS5 content database via
 * sceAppInstUtil, and triggers a library refresh — so Quaza appears as
 * a game icon exactly like ELF Arsenal, Kuro, and PS5 Payload Manager.
 *
 * Subsequent runs detect the existing install and skip straight to
 * starting the HTTP server + browser launch.
 */

/*
 * self_install_is_done
 * Returns 1 if Quaza is already installed in the game library, 0 if not.
 */
int self_install_is_done(void);

/*
 * self_install_run
 * Performs the full installation using kstuff-lite + sceAppInstUtil.
 * Safe to call every run — exits early if already installed.
 * Returns 0 on success, -1 on failure.
 */
int self_install_run(void);

#endif /* SELF_INSTALL_H */
