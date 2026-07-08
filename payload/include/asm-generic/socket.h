/* asm-generic/socket.h — FreeBSD/PS5 constants for Termux cross-compilation.
 *
 * FreeBSD values — NOT Linux.  See sys/socket.h for rationale.
 */
#ifndef _ASM_GENERIC_SOCKET_H_STUB
#define _ASM_GENERIC_SOCKET_H_STUB

#define SOL_SOCKET      0xffff  /* Linux=1,  FreeBSD=0xffff */

#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004  /* Linux=2,  FreeBSD=0x0004 */
#define SO_KEEPALIVE    0x0008  /* Linux=9,  FreeBSD=0x0008 */
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020  /* Linux=6,  FreeBSD=0x0020 */
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100
#define SO_REUSEPORT    0x0200  /* Linux=15, FreeBSD=0x0200 */
#define SO_NOSIGPIPE    0x0800
#define SO_TYPE         0x1008  /* Linux=3,  FreeBSD=0x1008 */
#define SO_ERROR        0x1007  /* Linux=4,  FreeBSD=0x1007 */
#define SO_SNDBUF       0x1001  /* Linux=7,  FreeBSD=0x1001 */
#define SO_RCVBUF       0x1002  /* Linux=8,  FreeBSD=0x1002 */
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_SNDTIMEO     0x1005  /* Linux=21, FreeBSD=0x1005 */
#define SO_RCVTIMEO     0x1006  /* Linux=20, FreeBSD=0x1006 */

#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_SEQPACKET  5

#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_INET         2
#define AF_INET6        28      /* Linux=10, FreeBSD=28 */
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6

#define SHUT_RD         0
#define SHUT_WR         1
#define SHUT_RDWR       2

#define MSG_OOB         0x0001
#define MSG_PEEK        0x0002
#define MSG_DONTROUTE   0x0004
#define MSG_DONTWAIT    0x0080  /* Linux=0x40,   FreeBSD=0x80 */
#define MSG_NOSIGNAL    0x20000 /* Linux=0x4000, FreeBSD=0x20000 */

#endif
