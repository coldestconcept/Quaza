/* time.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _TIME_H_STUB
#define _TIME_H_STUB

#include <sys/types.h>

struct tm {
    int tm_sec;   /* seconds [0,60]    */
    int tm_min;   /* minutes [0,59]    */
    int tm_hour;  /* hours   [0,23]    */
    int tm_mday;  /* day     [1,31]    */
    int tm_mon;   /* month   [0,11]    */
    int tm_year;  /* year - 1900       */
    int tm_wday;  /* weekday [0,6]     */
    int tm_yday;  /* day of year [0,365] */
    int tm_isdst; /* DST flag          */
};

struct timespec {
    time_t tv_sec;
    long   tv_nsec;
};

struct timeval {
    time_t      tv_sec;
    suseconds_t tv_usec;
};

#define CLOCKS_PER_SEC 1000000L
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 1

time_t time(time_t *tloc);
clock_t clock(void);
double  difftime(time_t time1, time_t time0);
time_t  mktime(struct tm *tm);
struct tm *localtime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
size_t  strftime(char *s, size_t max, const char *format, const struct tm *tm);
int     nanosleep(const struct timespec *req, struct timespec *rem);
int     clock_gettime(clockid_t clk_id, struct timespec *tp);
int     gettimeofday(struct timeval *tv, void *tz);

#endif
