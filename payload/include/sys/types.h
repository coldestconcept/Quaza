/* sys/types.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _SYS_TYPES_H_STUB
#define _SYS_TYPES_H_STUB

#include <stddef.h>  /* size_t, NULL — from clang resource dir */
#include <stdint.h>  /* uint*_t, int*_t */

typedef long               ssize_t;
typedef long               off_t;
typedef long long          off64_t;
typedef unsigned int       uid_t;
typedef unsigned int       gid_t;
typedef int                pid_t;
typedef unsigned long      ino_t;
typedef unsigned long      dev_t;
typedef unsigned int       mode_t;
typedef unsigned int       nlink_t;
typedef long               blksize_t;
typedef long               blkcnt_t;
typedef unsigned int       socklen_t;
typedef unsigned short     sa_family_t;
typedef long               time_t;
typedef long               clock_t;
typedef long               suseconds_t;
typedef unsigned long      useconds_t;
typedef int                clockid_t;

#endif
