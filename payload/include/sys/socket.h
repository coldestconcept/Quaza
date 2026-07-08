/* sys/socket.h — FreeBSD/PS5 constants for Termux cross-compilation (-nostdinc mode)
 *
 * FreeBSD values — NOT Linux.  The PS5 runs Orbis OS (FreeBSD-derived).
 * Linux and FreeBSD differ substantially for SOL_SOCKET, SO_*, AF_INET6,
 * MSG_* etc.  Wrong values silently cause setsockopt/bind/send to fail.
 */
#ifndef _SYS_SOCKET_H_STUB
#define _SYS_SOCKET_H_STUB

#include <sys/types.h>

struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};

struct sockaddr_storage {
    sa_family_t ss_family;
    char        __ss_pad[128 - sizeof(sa_family_t)];
};

struct iovec {
    void  *iov_base;
    size_t iov_len;
};

struct msghdr {
    void        *msg_name;
    socklen_t    msg_namelen;
    struct iovec *msg_iov;
    size_t       msg_iovlen;
    void        *msg_control;
    size_t       msg_controllen;
    int          msg_flags;
};

int      socket(int domain, int type, int protocol);
int      bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int      listen(int sockfd, int backlog);
int      accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int      connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t  send(int sockfd, const void *buf, size_t len, int flags);
ssize_t  recv(int sockfd, void *buf, size_t len, int flags);
ssize_t  sendto(int sockfd, const void *buf, size_t len, int flags,
                const struct sockaddr *dest, socklen_t destlen);
ssize_t  recvfrom(int sockfd, void *buf, size_t len, int flags,
                  struct sockaddr *src, socklen_t *srclen);
int      setsockopt(int sockfd, int level, int optname,
                    const void *optval, socklen_t optlen);
int      getsockopt(int sockfd, int level, int optname,
                    void *optval, socklen_t *optlen);
int      shutdown(int sockfd, int how);
int      getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int      getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/* socket type — same on FreeBSD and Linux */
#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3

/* address families — AF_INET6 differs! Linux=10, FreeBSD=28 */
#define AF_UNSPEC  0
#define AF_UNIX    1
#define AF_INET    2
#define AF_INET6   28   /* FreeBSD/PS5 */
#define PF_INET    AF_INET
#define PF_INET6   AF_INET6

/* SOL_SOCKET: Linux=1, FreeBSD=0xffff */
#define SOL_SOCKET    0xffff

/* SO_* options: FreeBSD uses bitmask style, very different from Linux */
#define SO_DEBUG      0x0001
#define SO_ACCEPTCONN 0x0002
#define SO_REUSEADDR  0x0004  /* Linux=2,  FreeBSD=0x0004 */
#define SO_KEEPALIVE  0x0008  /* Linux=9,  FreeBSD=0x0008 */
#define SO_DONTROUTE  0x0010
#define SO_BROADCAST  0x0020  /* Linux=6,  FreeBSD=0x0020 */
#define SO_LINGER     0x0080
#define SO_OOBINLINE  0x0100
#define SO_REUSEPORT  0x0200  /* Linux=15, FreeBSD=0x0200 */
#define SO_NOSIGPIPE  0x0800
#define SO_TYPE       0x1008  /* Linux=3,  FreeBSD=0x1008 */
#define SO_ERROR      0x1007  /* Linux=4,  FreeBSD=0x1007 */
#define SO_SNDBUF     0x1001  /* Linux=7,  FreeBSD=0x1001 */
#define SO_RCVBUF     0x1002  /* Linux=8,  FreeBSD=0x1002 */
#define SO_SNDLOWAT   0x1003
#define SO_RCVLOWAT   0x1004
#define SO_SNDTIMEO   0x1005  /* Linux=21, FreeBSD=0x1005 */
#define SO_RCVTIMEO   0x1006  /* Linux=20, FreeBSD=0x1006 */

#define SHUT_RD    0
#define SHUT_WR    1
#define SHUT_RDWR  2

/* MSG_* flags — MSG_DONTWAIT and MSG_NOSIGNAL differ */
#define MSG_OOB       0x0001
#define MSG_PEEK      0x0002
#define MSG_DONTROUTE 0x0004
#define MSG_DONTWAIT  0x0080  /* Linux=0x40,   FreeBSD=0x80 */
#define MSG_NOSIGNAL  0x20000 /* Linux=0x4000, FreeBSD=0x20000 */

#endif
