#ifndef BROWSER_LAUNCH_H
#define BROWSER_LAUNCH_H

#include <stddef.h>

/*
 * browser_get_local_url
 * ─────────────────────
 * Fills `buf` with the PS5's LAN URL, e.g. "http://192.168.1.42:4242".
 * Falls back to "http://127.0.0.1:4242" if no LAN IP is detected.
 * Call this before browser_launch() and before displaying the URL to the user.
 */
int browser_get_local_url(char *buf, size_t buflen);

/*
 * browser_launch
 * ──────────────
 * Open the PS5 WebKit browser at `url`.
 * Tries sceSystemServiceLaunchWebBrowser first, then sceLncUtilLaunchWebBrowser2.
 * Call after pkg_server_init() so the server is ready before the browser opens.
 */
int browser_launch(const char *url);

#endif /* BROWSER_LAUNCH_H */
