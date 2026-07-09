/*
 * netlog.c — UDP log for live runtime debugging.
 *
 * Destination selection (automatic, in priority order):
 *
 *   1. UNICAST to pusher's IP — if stdin is the elfldr TCP socket,
 *      getpeername(stdin) reveals the dev machine's IP and we send
 *      directly there.  This works through WiFi AP/client isolation
 *      that silently drops broadcast packets between wireless clients.
 *
 *   2. BROADCAST to 255.255.255.255 — fallback when getpeername fails
 *      (stdin is not the TCP socket, or payload loaded differently).
 *
 * On the dev machine:
 *   nc -ul 9020          # Linux / Termux / macOS
 *   python3 push_to_ps5.py listens on 0.0.0.0:9020 automatically.
 */

#include "netlog.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int g_fd = -1;
static struct sockaddr_in g_dest;

void netlog_init(void) {
    g_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_fd < 0) return;

    int yes = 1;
    setsockopt(g_fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));

    memset(&g_dest, 0, sizeof(g_dest));
    g_dest.sin_len         = (uint8_t)sizeof(g_dest); /* FreeBSD/PS5 requires this */
    g_dest.sin_family      = AF_INET;
    g_dest.sin_port        = htons(NETLOG_PORT);
    g_dest.sin_addr.s_addr = 0xFFFFFFFFu; /* default: broadcast */

    /* ── Unicast detection ──────────────────────────────────────────────
     * If elfldr dup2'd its accepted TCP socket onto stdin (fd 0), then
     * getpeername(0) gives us the dev machine's IP.  Use it for unicast
     * delivery so logs arrive even when the router has AP isolation on. */
    struct sockaddr_in peer;
    socklen_t plen = (socklen_t)sizeof(peer);
    if (getpeername(STDIN_FILENO, (struct sockaddr *)&peer, &plen) == 0 &&
        peer.sin_family      == AF_INET &&
        peer.sin_addr.s_addr != 0u &&
        peer.sin_addr.s_addr != 0xFFFFFFFFu) {
        g_dest.sin_addr.s_addr = peer.sin_addr.s_addr; /* unicast to pusher */
    }
}

void netlog_printf(const char *fmt, ...) {
    if (g_fd < 0) return;

    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    if (n <= 0) return;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
    buf[n] = '\0';

    sendto(g_fd, buf, (size_t)n, 0,
           (struct sockaddr *)&g_dest, (unsigned int)sizeof(g_dest));
}
