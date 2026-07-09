/* arpa/inet.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _ARPA_INET_H_STUB
#define _ARPA_INET_H_STUB

#include <sys/types.h>
#include <netinet/in.h>

/* inet_ntop — IPv4-only inline implementation; no library symbol needed.
 * Writes "a.b.c.d\0" into dst (must be >= 16 bytes for AF_INET).
 * Returns dst on success, NULL if af != AF_INET or size too small. */
static inline const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (af != 2 /* AF_INET */ || size < 16) return (const char *)0;
    const unsigned char *b = (const unsigned char *)src;
    /* Write decimal octets separated by '.' without using sprintf. */
    char *p = dst;
    for (int i = 0; i < 4; i++) {
        unsigned v = b[i];
        if (v >= 100) { *p++ = '0' + v / 100; v %= 100; *p++ = '0' + v / 10; v %= 10; }
        else if (v >= 10) { *p++ = '0' + v / 10; v %= 10; }
        *p++ = '0' + v;
        if (i < 3) *p++ = '.';
    }
    *p = '\0';
    return dst;
}

int       inet_pton(int af, const char *src, void *dst);
in_addr_t inet_addr(const char *cp);
char     *inet_ntoa(struct in_addr in);

#endif
