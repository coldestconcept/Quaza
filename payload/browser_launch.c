#include "browser_launch.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * PS5 system service function to open the WebKit browser.
 * Resolved at link time via libSceLncUtil from the PS5 Payload SDK.
 *
 * Signature from ps5-payload-dev SDK:
 *   int sceLncUtilLaunchWebBrowser2(const char *url, size_t url_len,
 *                                   uint64_t flags);
 *
 * flags = 0 for default behaviour (foreground browser, no dev tools).
 */
extern int sceLncUtilLaunchWebBrowser2(const char *url, size_t url_len,
                                       unsigned long long flags);

int browser_launch(const char *url) {
    if (!url || url[0] == '\0') {
        fprintf(stderr, "[browser] NULL or empty URL\n");
        return -1;
    }

    /* Give the HTTP server a moment to start listening before we redirect
     * the user's browser, otherwise the first load may fail. */
    sleep(1);

    size_t url_len = strlen(url);
    fprintf(stdout, "[browser] Launching PS5 WebKit → %s\n", url);

    int rc = sceLncUtilLaunchWebBrowser2(url, url_len, 0ULL);
    if (rc != 0) {
        fprintf(stderr, "[browser] sceLncUtilLaunchWebBrowser2 returned 0x%08x\n",
                (unsigned int)rc);
        return -1;
    }
    return 0;
}
