#!/bin/bash
# mk_shims.sh — create all x86_64 asm shim headers needed to cross-compile
# the Quaza payload on Termux ARM64 without the PS5 Payload SDK.
# Run from payload/:   bash mk_shims.sh
set -e
D=include
mkdir -p $D/asm $D/asm-generic

# ── asm/types.h ──────────────────────────────────────────────────────────────
cat > $D/asm/types.h << 'HEOF'
#ifndef _ASM_TYPES_H_STUB
#define _ASM_TYPES_H_STUB
typedef unsigned char       __u8;  typedef signed char       __s8;
typedef unsigned short      __u16; typedef signed short      __s16;
typedef unsigned int        __u32; typedef signed int        __s32;
typedef unsigned long long  __u64; typedef signed long long  __s64;
typedef __u16 __le16; typedef __u16 __be16;
typedef __u32 __le32; typedef __u32 __be32;
typedef __u64 __le64; typedef __u64 __be64;
typedef __u16 __sum16; typedef __u32 __wsum;
#endif
HEOF

# ── asm/posix_types.h ────────────────────────────────────────────────────────
cat > $D/asm/posix_types.h << 'HEOF'
#ifndef _ASM_POSIX_TYPES_H_STUB
#define _ASM_POSIX_TYPES_H_STUB
typedef unsigned long    __kernel_ulong_t;
typedef long             __kernel_long_t;
typedef long             __kernel_suseconds_t;
typedef long long        __kernel_time64_t;
typedef long             __kernel_old_time_t;
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t  __kernel_ssize_t;
typedef __kernel_long_t  __kernel_ptrdiff_t;
typedef long long        __kernel_loff_t;
typedef long             __kernel_time_t;
typedef long             __kernel_clock_t;
typedef int              __kernel_timer_t;
typedef int              __kernel_clockid_t;
typedef int              __kernel_pid_t;
typedef unsigned int     __kernel_uid32_t;
typedef unsigned int     __kernel_gid32_t;
typedef unsigned int     __kernel_old_uid_t;
typedef unsigned int     __kernel_old_gid_t;
typedef unsigned long    __kernel_old_dev_t;
typedef unsigned long    __kernel_ino_t;
typedef unsigned short   __kernel_mode_t;
typedef unsigned short   __kernel_nlink_t;
typedef long             __kernel_off_t;
typedef int              __kernel_daddr_t;
typedef char *           __kernel_caddr_t;
typedef unsigned short   __kernel_uid16_t;
typedef unsigned short   __kernel_gid16_t;
typedef int              __kernel_ipc_pid_t;
typedef unsigned long    __kernel_sigset_t;
#endif
HEOF

# ── asm/bitsperlong.h ────────────────────────────────────────────────────────
cat > $D/asm/bitsperlong.h << 'HEOF'
#ifndef _ASM_BITSPERLONG_H_STUB
#define _ASM_BITSPERLONG_H_STUB
#define __BITS_PER_LONG    64
#define __BITS_PER_LONG_LONG 64
#endif
HEOF

# ── asm/byteorder.h ──────────────────────────────────────────────────────────
cat > $D/asm/byteorder.h << 'HEOF'
#ifndef _ASM_BYTEORDER_H_STUB
#define _ASM_BYTEORDER_H_STUB
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BYTE_ORDER
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#endif
HEOF

# ── asm/fcntl.h ──────────────────────────────────────────────────────────────
cat > $D/asm/fcntl.h << 'HEOF'
#ifndef _ASM_FCNTL_H_STUB
#define _ASM_FCNTL_H_STUB
#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002
#define O_CREAT     00000100
#define O_EXCL      00000200
#define O_NOCTTY    00000400
#define O_TRUNC     00001000
#define O_APPEND    00002000
#define O_NONBLOCK  00004000
#define O_DSYNC     00010000
#define O_DIRECT    00040000
#define O_LARGEFILE 00100000
#define O_DIRECTORY 00200000
#define O_NOFOLLOW  00400000
#define O_NOATIME   01000000
#define O_CLOEXEC   02000000
#define O_SYNC      04010000
#define O_NDELAY    O_NONBLOCK
#define O_ACCMODE   00000003
#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4
#define F_GETLK     5
#define F_SETLK     6
#define F_SETLKW    7
#define F_SETOWN    8
#define F_GETOWN    9
#define FD_CLOEXEC  1
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif
#endif
HEOF

# ── asm/errno.h / asm-generic/errno-base.h / asm-generic/errno.h ─────────────
cat > $D/asm/errno.h << 'HEOF'
#ifndef _ASM_ERRNO_H_STUB
#define _ASM_ERRNO_H_STUB
#include <asm-generic/errno.h>
#endif
HEOF

cat > $D/asm-generic/errno-base.h << 'HEOF'
#ifndef _ASM_GENERIC_ERRNO_BASE_H_STUB
#define _ASM_GENERIC_ERRNO_BASE_H_STUB
#define EPERM 1  #define ENOENT 2  #define ESRCH 3   #define EINTR 4
#define EIO   5  #define ENXIO  6  #define E2BIG 7   #define ENOEXEC 8
#define EBADF 9  #define ECHILD 10 #define EAGAIN 11 #define ENOMEM 12
#define EACCES 13 #define EFAULT 14 #define ENOTBLK 15 #define EBUSY 16
#define EEXIST 17 #define EXDEV 18  #define ENODEV 19  #define ENOTDIR 20
#define EISDIR 21 #define EINVAL 22 #define ENFILE 23  #define EMFILE 24
#define ENOTTY 25 #define ETXTBSY 26 #define EFBIG 27  #define ENOSPC 28
#define ESPIPE 29 #define EROFS 30   #define EMLINK 31 #define EPIPE 32
#define EDOM 33   #define ERANGE 34
#endif
HEOF

cat > $D/asm-generic/errno.h << 'HEOF'
#ifndef _ASM_GENERIC_ERRNO_H_STUB
#define _ASM_GENERIC_ERRNO_H_STUB
#include <asm-generic/errno-base.h>
#define EDEADLK 35 #define ENAMETOOLONG 36 #define ENOLCK 37 #define ENOSYS 38
#define ENOTEMPTY 39 #define ELOOP 40 #define EWOULDBLOCK EAGAIN
#define ENOMSG 42  #define EIDRM 43   #define ENOSTR 60 #define ENODATA 61
#define ETIME 62   #define ENOSR 63   #define ENOLINK 67 #define EPROTO 71
#define EMULTIHOP 72 #define EBADMSG 74 #define EOVERFLOW 75 #define EILSEQ 84
#define ENOTSOCK 88  #define EDESTADDRREQ 89 #define EMSGSIZE 90 #define EPROTOTYPE 91
#define ENOPROTOOPT 92 #define EPROTONOSUPPORT 93 #define EOPNOTSUPP 95
#define EAFNOSUPPORT 97 #define EADDRINUSE 98  #define EADDRNOTAVAIL 99
#define ENETDOWN 100 #define ENETUNREACH 101 #define ECONNABORTED 103
#define ECONNRESET 104 #define ENOBUFS 105 #define EISCONN 106 #define ENOTCONN 107
#define ETIMEDOUT 110 #define ECONNREFUSED 111 #define EHOSTUNREACH 113
#define EALREADY 114  #define EINPROGRESS 115 #define ECANCELED 125
#define EOWNERDEAD 130 #define ENOTRECOVERABLE 131
#endif
HEOF

# ── asm/sigcontext.h ─────────────────────────────────────────────────────────
cat > $D/asm/sigcontext.h << 'HEOF'
#ifndef _ASM_SIGCONTEXT_H_STUB
#define _ASM_SIGCONTEXT_H_STUB
#include <asm/types.h>
struct sigcontext {
  __u64 r8,r9,r10,r11,r12,r13,r14,r15;
  __u64 rdi,rsi,rbp,rbx,rdx,rax,rcx,rsp,rip,eflags;
  __u16 cs,gs,fs,ss;
  __u64 err,trapno,oldmask,cr2,fpstate;
  __u64 reserved[8];
};
#endif
HEOF

# ── asm/signal.h ─────────────────────────────────────────────────────────────
cat > $D/asm/signal.h << 'HEOF'
#ifndef _ASM_SIGNAL_H_STUB
#define _ASM_SIGNAL_H_STUB
#define SIGHUP 1  #define SIGINT 2  #define SIGQUIT 3 #define SIGILL 4
#define SIGTRAP 5 #define SIGABRT 6 #define SIGBUS 7  #define SIGFPE 8
#define SIGKILL 9 #define SIGUSR1 10 #define SIGSEGV 11 #define SIGUSR2 12
#define SIGPIPE 13 #define SIGALRM 14 #define SIGTERM 15 #define SIGCHLD 17
#define SIGCONT 18 #define SIGSTOP 19 #define SIGTSTP 20 #define SIGTTIN 21
#define SIGTTOU 22 #define SIGURG 23  #define SIGXCPU 24 #define SIGXFSZ 25
#define SIGVTALRM 26 #define SIGPROF 27 #define SIGWINCH 28 #define SIGIO 29
#define SIGPWR 30  #define SIGSYS 31
#define _NSIG 64
#define SIG_DFL ((void(*)(int))0)
#define SIG_IGN ((void(*)(int))1)
#define SIG_ERR ((void(*)(int))-1)
#define SA_NOCLDSTOP 0x00000001 #define SA_SIGINFO 0x00000004
#define SA_ONSTACK   0x08000000 #define SA_RESTART 0x10000000
#define SA_NODEFER   0x40000000 #define SA_RESETHAND 0x80000000
#endif
HEOF

# ── asm/mman.h / asm-generic/mman.h ─────────────────────────────────────────
cat > $D/asm/mman.h << 'HEOF'
#ifndef _ASM_MMAN_H_STUB
#define _ASM_MMAN_H_STUB
#include <asm-generic/mman.h>
#endif
HEOF

cat > $D/asm-generic/mman.h << 'HEOF'
#ifndef _ASM_GENERIC_MMAN_H_STUB
#define _ASM_GENERIC_MMAN_H_STUB
#define PROT_READ  0x1 #define PROT_WRITE 0x2 #define PROT_EXEC  0x4 #define PROT_NONE 0x0
#define MAP_SHARED 0x01 #define MAP_PRIVATE 0x02 #define MAP_FIXED 0x10 #define MAP_ANONYMOUS 0x20
#define MAP_ANON MAP_ANONYMOUS #define MAP_FAILED ((void*)-1)
#endif
HEOF

# ── asm/socket.h / asm-generic/socket.h ─────────────────────────────────────
cat > $D/asm/socket.h << 'HEOF'
#ifndef _ASM_SOCKET_H_STUB
#define _ASM_SOCKET_H_STUB
#include <asm-generic/socket.h>
#endif
HEOF

cat > $D/asm-generic/socket.h << 'HEOF'
#ifndef _ASM_GENERIC_SOCKET_H_STUB
#define _ASM_GENERIC_SOCKET_H_STUB
#define SOL_SOCKET 1
#define SO_DEBUG 1 #define SO_REUSEADDR 2 #define SO_TYPE 3   #define SO_ERROR 4
#define SO_SNDBUF 7 #define SO_RCVBUF 8  #define SO_KEEPALIVE 9 #define SO_REUSEPORT 15
#define SO_RCVTIMEO 20 #define SO_SNDTIMEO 21 #define SO_ACCEPTCONN 30
#define SOCK_STREAM 1 #define SOCK_DGRAM 2 #define SOCK_RAW 3
#define SOCK_NONBLOCK 0x800 #define SOCK_CLOEXEC 0x80000
#define AF_UNSPEC 0 #define AF_UNIX 1 #define AF_INET 2 #define AF_INET6 10
#define PF_INET AF_INET #define PF_INET6 AF_INET6
#define SHUT_RD 0 #define SHUT_WR 1 #define SHUT_RDWR 2
#define MSG_OOB 1 #define MSG_PEEK 2 #define MSG_DONTWAIT 64 #define MSG_NOSIGNAL 0x4000
#endif
HEOF

# ── asm/ioctl.h / asm-generic/ioctl.h ───────────────────────────────────────
cat > $D/asm/ioctl.h << 'HEOF'
#ifndef _ASM_IOCTL_H_STUB
#define _ASM_IOCTL_H_STUB
#include <asm-generic/ioctl.h>
#endif
HEOF

cat > $D/asm-generic/ioctl.h << 'HEOF'
#ifndef _ASM_GENERIC_IOCTL_H_STUB
#define _ASM_GENERIC_IOCTL_H_STUB
#define _IOC_NRBITS 8 #define _IOC_TYPEBITS 8 #define _IOC_SIZEBITS 14 #define _IOC_DIRBITS 2
#define _IOC_NRSHIFT 0
#define _IOC_TYPESHIFT (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT  (_IOC_SIZESHIFT+_IOC_SIZEBITS)
#define _IOC_NONE 0U #define _IOC_WRITE 1U #define _IOC_READ 2U
#define _IOC(dir,type,nr,size) \
  (((dir)<<_IOC_DIRSHIFT)|((type)<<_IOC_TYPESHIFT)|((nr)<<_IOC_NRSHIFT)|((size)<<_IOC_SIZESHIFT))
#define _IO(type,nr)        _IOC(_IOC_NONE,(type),(nr),0)
#define _IOR(type,nr,size)  _IOC(_IOC_READ,(type),(nr),sizeof(size))
#define _IOW(type,nr,size)  _IOC(_IOC_WRITE,(type),(nr),sizeof(size))
#define _IOWR(type,nr,size) _IOC(_IOC_READ|_IOC_WRITE,(type),(nr),sizeof(size))
#endif
HEOF

# ── asm/stat.h / asm-generic/stat.h ─────────────────────────────────────────
cat > $D/asm/stat.h << 'HEOF'
#ifndef _ASM_STAT_H_STUB
#define _ASM_STAT_H_STUB
#include <asm-generic/stat.h>
#endif
HEOF

cat > $D/asm-generic/stat.h << 'HEOF'
#ifndef _ASM_GENERIC_STAT_H_STUB
#define _ASM_GENERIC_STAT_H_STUB
#include <asm/types.h>
struct stat {
  unsigned long st_dev; unsigned long st_ino;
  unsigned int  st_mode; unsigned int  st_nlink;
  unsigned int  st_uid;  unsigned int  st_gid;
  unsigned long st_rdev; long          st_size;
  long st_blksize; long st_blocks;
  unsigned long st_atime; unsigned long st_atime_nsec;
  unsigned long st_mtime; unsigned long st_mtime_nsec;
  unsigned long st_ctime; unsigned long st_ctime_nsec;
  unsigned long __unused[2];
};
#endif
HEOF

# ── asm-generic passthrough stubs ────────────────────────────────────────────
cat > $D/asm-generic/types.h      << 'HEOF'
#ifndef _ASM_GENERIC_TYPES_H
#define _ASM_GENERIC_TYPES_H
#include <asm/types.h>
#endif
HEOF

cat > $D/asm-generic/posix_types.h << 'HEOF'
#ifndef _ASM_GENERIC_POSIX_TYPES_H
#define _ASM_GENERIC_POSIX_TYPES_H
#include <asm/posix_types.h>
#endif
HEOF

cat > $D/asm-generic/bitsperlong.h << 'HEOF'
#ifndef _ASM_GENERIC_BITSPERLONG_H
#define _ASM_GENERIC_BITSPERLONG_H
#define __BITS_PER_LONG 64
#define __BITS_PER_LONG_LONG 64
#endif
HEOF

cat > $D/asm-generic/int-ll64.h << 'HEOF'
#ifndef _ASM_GENERIC_INT_LL64_H
#define _ASM_GENERIC_INT_LL64_H
#ifndef __ASSEMBLY__
typedef signed char __s8; typedef unsigned char __u8;
typedef signed short __s16; typedef unsigned short __u16;
typedef signed int __s32; typedef unsigned int __u32;
typedef signed long long __s64; typedef unsigned long long __u64;
#endif
#endif
HEOF

echo "✓ All shim headers written to $D/"
echo "Run 'make' to build the payload."
