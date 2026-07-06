/* stdlib.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _STDLIB_H_STUB
#define _STDLIB_H_STUB

#include <stddef.h>
#include <sys/types.h>

void  *malloc(size_t size);
void  *calloc(size_t nmemb, size_t size);
void  *realloc(void *ptr, size_t size);
void   free(void *ptr);

int    atoi(const char *s);
long   atol(const char *s);
double atof(const char *s);
long   strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
double strtod(const char *s, char **end);

void   exit(int status)  __attribute__((noreturn));
void   abort(void)       __attribute__((noreturn));
void   qsort(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *));
void  *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *));
int    abs(int j);
long   labs(long j);
int    rand(void);
void   srand(unsigned int seed);
char  *getenv(const char *name);
int    setenv(const char *name, const char *value, int overwrite);
int    unsetenv(const char *name);
int    system(const char *cmd);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     0x7fffffff

#endif
