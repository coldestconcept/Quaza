/* sys/select.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _SYS_SELECT_H_STUB
#define _SYS_SELECT_H_STUB

#include <sys/types.h>
#include <time.h>

typedef struct {
    unsigned long fds_bits[16]; /* 1024 bits */
} fd_set;

#define FD_SETSIZE 1024
#define FD_ZERO(s)    __builtin_memset(s, 0, sizeof(fd_set))
#define FD_SET(d,s)   ((s)->fds_bits[(d)/8/sizeof(long)] |=  (1UL<<((d)%(8*sizeof(long)))))
#define FD_CLR(d,s)   ((s)->fds_bits[(d)/8/sizeof(long)] &= ~(1UL<<((d)%(8*sizeof(long)))))
#define FD_ISSET(d,s) ((s)->fds_bits[(d)/8/sizeof(long)] &   (1UL<<((d)%(8*sizeof(long)))))

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout);

#endif
