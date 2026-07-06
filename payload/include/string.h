/* string.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _STRING_H_STUB
#define _STRING_H_STUB

#include <stddef.h>

void  *memset(void *s, int c, size_t n);
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
int    memcmp(const void *s1, const void *s2, size_t n);
void  *memchr(const void *s, int c, size_t n);

int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
int    strcasecmp(const char *s1, const char *s2);
int    strncasecmp(const char *s1, const char *s2, size_t n);
char  *strcpy(char *dst, const char *src);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strncat(char *dst, const char *src, size_t n);
size_t strlen(const char *s);
char  *strdup(const char *s);
char  *strndup(const char *s, size_t n);
char  *strstr(const char *haystack, const char *needle);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strtok(char *str, const char *delim);
char  *strtok_r(char *str, const char *delim, char **saveptr);
char  *strerror(int errnum);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char  *strpbrk(const char *s, const char *accept);

#endif
