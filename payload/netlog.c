/*
 * netlog.c — UDP broadcast log for live runtime debugging.
 *
 * Every netlog_printf() call sends a UDP datagram to 255.255.255.255:9020.
 * On the dev machine run:
 *
 *   nc -ul 9020          # Linux / Termux
 *   nc -ul 9020          # macOS
 *
 * All datagrams arrive in real-time without any TCP connection staying open.
 * Works alongside the elfldr stdout pipe and USB file logging.
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
    g_dest.sin_family      = AF_INET;
    g_dest.sin_port        = htons(NETLOG_PORT);
    g_dest.sin_addr.s_addr = 0xFFFFFFFFu; /* 255.255.255.255 */
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
