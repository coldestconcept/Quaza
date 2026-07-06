/* asm/fcntl.h — x86_64 shim for cross-compilation on Termux ARM64.
 * Provides the O_* and F_* constants that stdio.h / fcntl.h need.       */
#ifndef _ASM_FCNTL_H_STUB
#define _ASM_FCNTL_H_STUB

/* open() flags */
#define O_RDONLY        00000000
#define O_WRONLY        00000001
#define O_RDWR          00000002
#define O_CREAT         00000100
#define O_EXCL          00000200
#define O_NOCTTY        00000400
#define O_TRUNC         00001000
#define O_APPEND        00002000
#define O_NONBLOCK      00004000
#define O_DSYNC         00010000
#define O_DIRECT        00040000
#define O_LARGEFILE     00100000
#define O_DIRECTORY     00200000
#define O_NOFOLLOW      00400000
#define O_NOATIME       01000000
#define O_CLOEXEC       02000000
#define O_SYNC          04010000
#define O_PATH          010000000
#define O_TMPFILE       020200000
#define O_NDELAY        O_NONBLOCK

/* fcntl() commands */
#define F_DUPFD         0
#define F_GETFD         1
#define F_SETFD         2
#define F_GETFL         3
#define F_SETFL         4
#define F_GETLK         5
#define F_SETLK         6
#define F_SETLKW        7
#define F_SETOWN        8
#define F_GETOWN        9
#define F_SETSIG        10
#define F_GETSIG        11
#define F_DUPFD_CLOEXEC 1030

/* file descriptor flags */
#define FD_CLOEXEC      1

/* access mode mask */
#define O_ACCMODE       00000003

#ifndef SEEK_SET
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#endif

#endif /* _ASM_FCNTL_H_STUB */
