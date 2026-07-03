#ifndef BROWSER_LAUNCH_H
#define BROWSER_LAUNCH_H

/*
 * browser_launch
 * Open the PS5 WebKit browser pointed at the given URL.
 * Call this after the HTTP server is confirmed listening.
 *
 * Returns 0 on success, -1 if the system call fails.
 *
 * Implementation uses sceLncUtilLaunchWebBrowser2 from libSceLncUtil,
 * available via the PS5 Payload SDK.
 */
int browser_launch(const char *url);

#endif /* BROWSER_LAUNCH_H */
