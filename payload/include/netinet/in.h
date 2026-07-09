/* netinet/in.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _NETINET_IN_H_STUB
#define _NETINET_IN_H_STUB

#include <sys/types.h>
#include <sys/socket.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct in_addr {
    in_addr_t s_addr;
};

/* FreeBSD/PS5 sockaddr_in — MUST include sin_len before sin_family.
 * The PS5 kernel reads byte 0 as the struct length and byte 1 as the
 * address family.  A Linux-style struct (no sin_len, sa_family_t as
 * uint16_t) puts AF_INET=2 at byte 0, so the kernel sees sin_len=2
 * and sin_family=AF_UNSPEC — every sendto/bind silently fails.
 * Both fields are uint8_t (as in FreeBSD's <netinet/in.h>). */
struct sockaddr_in {
    uint8_t        sin_len;     /* total length = sizeof(struct sockaddr_in) = 16 */
    uint8_t        sin_family;  /* AF_INET = 2 */
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr_in6 {
    uint8_t         sin6_len;       /* sizeof(struct sockaddr_in6) = 28 */
    uint8_t         sin6_family;    /* AF_INET6 = 28 */
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};

/* Compile-time layout guards — catch any future struct drift immediately. */
_Static_assert(sizeof(struct sockaddr_in)  == 16, "sockaddr_in must be 16 bytes on PS5/FreeBSD");
_Static_assert(sizeof(struct sockaddr_in6) == 28, "sockaddr_in6 must be 28 bytes on PS5/FreeBSD");

#define INADDR_ANY       ((in_addr_t)0x00000000)
#define INADDR_BROADCAST ((in_addr_t)0xffffffff)
#define INADDR_LOOPBACK  ((in_addr_t)0x7f000001)
#define INADDR_NONE      ((in_addr_t)0xffffffff)

#define IPPROTO_IP   0
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17
#define IPPROTO_IPV6 41

#define SOL_TCP         6
#define TCP_NODELAY     1
#define TCP_KEEPIDLE    4
#define TCP_KEEPINTVL   5
#define TCP_KEEPCNT     6

#define IPV6_V6ONLY     26

/* Byte-order conversions — implemented as static inlines using compiler
 * builtins so no library symbol is needed in either stub or SDK mode.
 * PS5/x86-64 is little-endian; network order is big-endian. */
static inline uint16_t htons(uint16_t v) { return __builtin_bswap16(v); }
static inline uint16_t ntohs(uint16_t v) { return __builtin_bswap16(v); }
static inline uint32_t htonl(uint32_t v) { return __builtin_bswap32(v); }
static inline uint32_t ntohl(uint32_t v) { return __builtin_bswap32(v); }

#endif
