/* asm-generic/socket.h — x86_64 shim for cross-compilation on Termux ARM64. */
#ifndef _ASM_GENERIC_SOCKET_H_STUB
#define _ASM_GENERIC_SOCKET_H_STUB

#define SOL_SOCKET      1
#define SO_DEBUG        1
#define SO_REUSEADDR    2
#define SO_TYPE         3
#define SO_ERROR        4
#define SO_DONTROUTE    5
#define SO_BROADCAST    6
#define SO_SNDBUF       7
#define SO_RCVBUF       8
#define SO_KEEPALIVE    9
#define SO_OOBINLINE    10
#define SO_NO_CHECK     11
#define SO_PRIORITY     12
#define SO_LINGER       13
#define SO_BSDCOMPAT    14
#define SO_REUSEPORT    15
#define SO_RCVLOWAT     18
#define SO_SNDLOWAT     19
#define SO_RCVTIMEO     20
#define SO_SNDTIMEO     21
#define SO_ACCEPTCONN   30
#define SO_SNDBUFFORCE  32
#define SO_RCVBUFFORCE  33

#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_SEQPACKET  5
#define SOCK_NONBLOCK   0x800
#define SOCK_CLOEXEC    0x80000

#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_INET         2
#define AF_INET6        10
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6

#define SHUT_RD         0
#define SHUT_WR         1
#define SHUT_RDWR       2

#define MSG_OOB         1
#define MSG_PEEK        2
#define MSG_DONTROUTE   4
#define MSG_DONTWAIT    64
#define MSG_NOSIGNAL    0x4000

#endif
