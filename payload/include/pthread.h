/* pthread.h — minimal stub for Termux cross-compilation (-nostdinc mode) */
#ifndef _PTHREAD_H_STUB
#define _PTHREAD_H_STUB

#include <sys/types.h>
#include <time.h>

typedef unsigned long pthread_t;
typedef unsigned int  pthread_key_t;

typedef struct { unsigned int __val; }            pthread_mutex_t;
typedef struct { unsigned int __val; }            pthread_cond_t;
typedef struct { unsigned int __val[2]; }         pthread_rwlock_t;
typedef struct { unsigned int __val; }            pthread_spinlock_t;
typedef struct { unsigned int __val[8]; }         pthread_attr_t;
typedef struct { unsigned int __val[2]; }         pthread_mutexattr_t;
typedef struct { unsigned int __val[2]; }         pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER   {0}
#define PTHREAD_COND_INITIALIZER    {0}
#define PTHREAD_RWLOCK_INITIALIZER  {0}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval) __attribute__((noreturn));
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);
int pthread_cancel(pthread_t thread);

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                            const struct timespec *abstime);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

#define PTHREAD_CREATE_JOINABLE  0
#define PTHREAD_CREATE_DETACHED  1

#endif
