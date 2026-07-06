/* asm-generic/stat.h — x86_64 shim for cross-compilation on Termux ARM64.
 * Defines the raw kernel stat structure (not the libc struct stat).      */
#ifndef _ASM_GENERIC_STAT_H_STUB
#define _ASM_GENERIC_STAT_H_STUB

#include <asm/types.h>

struct stat {
    unsigned long  st_dev;
    unsigned long  st_ino;
    unsigned int   st_mode;
    unsigned int   st_nlink;
    unsigned int   st_uid;
    unsigned int   st_gid;
    unsigned long  st_rdev;
    long           st_size;
    long           st_blksize;
    long           st_blocks;
    unsigned long  st_atime;
    unsigned long  st_atime_nsec;
    unsigned long  st_mtime;
    unsigned long  st_mtime_nsec;
    unsigned long  st_ctime;
    unsigned long  st_ctime_nsec;
    unsigned long  __unused[2];
};

#endif
