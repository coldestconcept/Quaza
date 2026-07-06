/* arpa/inet.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _ARPA_INET_H_STUB
#define _ARPA_INET_H_STUB

#include <sys/types.h>
#include <netinet/in.h>

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int         inet_pton(int af, const char *src, void *dst);
in_addr_t   inet_addr(const char *cp);
char       *inet_ntoa(struct in_addr in);

#endif
