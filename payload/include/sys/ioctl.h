/* sys/ioctl.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _SYS_IOCTL_H_STUB
#define _SYS_IOCTL_H_STUB

int ioctl(int fd, unsigned long request, ...);

#define FIONBIO  0x5421
#define FIONREAD 0x541b
#define TIOCGWINSZ 0x5413

#endif
