/* ifaddrs.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _IFADDRS_H_STUB
#define _IFADDRS_H_STUB

#include <sys/socket.h>

struct ifaddrs {
    struct ifaddrs   *ifa_next;
    char             *ifa_name;
    unsigned int      ifa_flags;
    struct sockaddr  *ifa_addr;
    struct sockaddr  *ifa_netmask;
    struct sockaddr  *ifa_broadaddr;
    void             *ifa_data;
};

int  getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);

#endif
