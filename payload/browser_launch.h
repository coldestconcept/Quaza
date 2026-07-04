#ifndef BROWSER_LAUNCH_H
#define BROWSER_LAUNCH_H

#include <stddef.h>

/*
 * browser_get_local_url
 * Fills buf with "http://<LAN-IP>:4242". Falls back to 127.0.0.1.
 */
int browser_get_local_url(char *buf, size_t buflen);

/*
 * browser_notify
 * Sends an on-screen PS5 notification containing the URL.
 * Call this FIRST — before server init, before browser launch.
 * Always returns immediately; never crashes even if the syscall fails.
 */
int browser_notify(const char *url);

/*
 * browser_launch_app
 * Opens the PS5 WebKit browser at url via sceSystemServiceLaunchApp,
 * falling back to sceLncUtilLaunchWebBrowser2.
 * Non-fatal — returns -1 on failure, 0 on success.
 */
int browser_launch_app(const char *url);

#endif /* BROWSER_LAUNCH_H */
