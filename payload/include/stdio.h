/* stdio.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _STDIO_H_STUB
#define _STDIO_H_STUB

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

#define EOF    (-1)
#define BUFSIZ 8192

/* Opaque FILE — defined by the C runtime we link against at PS5 runtime */
typedef struct __sFILE FILE;

/* PS5 uses FreeBSD ABI: file globals are exported as __stdinp/__stdoutp/__stderrp.
 * Using these names ensures sceKernelDlsym() can resolve them from
 * libSceLibcInternal at runtime. */
extern FILE *__stdinp;
extern FILE *__stdoutp;
extern FILE *__stderrp;
#define stdin  __stdinp
#define stdout __stdoutp
#define stderr __stderrp

int    printf(const char *fmt, ...);
int    fprintf(FILE *stream, const char *fmt, ...);
int    sprintf(char *buf, const char *fmt, ...);
int    snprintf(char *buf, size_t n, const char *fmt, ...);
int    vprintf(const char *fmt, va_list ap);
int    vfprintf(FILE *stream, const char *fmt, va_list ap);
int    vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
int    sscanf(const char *str, const char *fmt, ...);
int    fscanf(FILE *stream, const char *fmt, ...);

int    fflush(FILE *stream);
void   perror(const char *s);
int    puts(const char *s);
int    fputs(const char *s, FILE *stream);
int    fputc(int c, FILE *stream);
int    fgetc(FILE *stream);
int    ungetc(int c, FILE *stream);
char  *fgets(char *s, int n, FILE *stream);

FILE  *fopen(const char *path, const char *mode);
FILE  *fdopen(int fd, const char *mode);
int    fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
long   ftell(FILE *stream);
int    fseek(FILE *stream, long offset, int whence);
void   rewind(FILE *stream);
int    feof(FILE *stream);
int    ferror(FILE *stream);
void   clearerr(FILE *stream);
int    fileno(FILE *stream);

int    rename(const char *old, const char *newpath);
int    remove(const char *path);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif
