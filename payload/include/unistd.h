/* unistd.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _UNISTD_H_STUB
#define _UNISTD_H_STUB

#include <sys/types.h>

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int     close(int fd);
off_t   lseek(int fd, off_t offset, int whence);
int     dup(int fd);
int     dup2(int oldfd, int newfd);
int     pipe(int pipefd[2]);

int     access(const char *path, int mode);
int     unlink(const char *path);
int     rmdir(const char *path);
int     symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
int     rename(const char *oldpath, const char *newpath);
char   *getcwd(char *buf, size_t size);
int     chdir(const char *path);

pid_t   getpid(void);
uid_t   getuid(void);
uid_t   geteuid(void);

unsigned int sleep(unsigned int seconds);
int     usleep(useconds_t usec);

long    sysconf(int name);
int     isatty(int fd);

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define _SC_PAGESIZE    30
#define _SC_PAGE_SIZE   _SC_PAGESIZE
#define _SC_NPROCESSORS_ONLN 84

#endif
