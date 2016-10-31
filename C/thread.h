#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "errcode.h"

#ifdef WINDOWS
#else

#include <pthread.h>

#define DC_thread_t pthread_t
#define DC_thread_mutex_t pthread_mutex_t
#define DC_thread_rwlock_t pthread_rwlock_t
#define DC_thread_cond_t pthread_cond_t

#define __sys(x) (x)

#define DC_thread_create(t, func, arg) __sys(pthread_create(&t, NULL, func, arg))
#define DC_thread_join(t, status) __sys(pthread_join(&t, status))
#define DC_thread_destroy(t)

#define DC_thread_mutex_init(t) __sys(pthread_mutex_init(&t, NULL))
#define DC_thread_mutex_lock(t) __sys(pthread_mutex_lock(&t))
#define DC_thread_mutex_unlock(t) __sys(pthread_mutex_unlock(&t))
#define DC_thread_mutex_trylock(t) __sys(pthread_mutex_trylock(&t))
#define DC_thread_mutex_destroy(t) __sys(pthread_mutex_destroy(&t))
#define DC_thread_rwlock_init(t) __sys(pthread_rwlock_init (&t, NULL))
#define DC_thread_rwlock_rdlock(t) __sys(pthread_rwlock_rdlock (&t))
#define DC_thread_rwlock_wrlock(t) __sys(pthread_rwlock_wrlock (&t))
#define DC_thread_rwlock_unlock(t) __sys(pthread_rwlock_unlock(&t))
#define DC_thread_rwlock_tryrdlock(t) __sys(pthread_rwlock_tryrdlock (&t))
#define DC_thread_rwlock_trywrlock(t) __sys(pthread_rwlock_trywrlock (&t))
#define DC_thread_rwlock_destroy(t) __sys(pthread_rwlock_destroy (&t))

#define DC_thread_cond_init(t) __sys(pthread_cond_init(&t, NULL))
#define DC_thread_cond_wait(t, m) __sys(pthread_cond_wait(&t, &m))

extern err_t DC_thread_cond_timedwait(DC_thread_cond_t t,
                                      DC_thread_mutex_t m,
                                      uint32_t ms);

#define DC_thread_cond_signal(t) pthread_cond_signal(&t)
#define DC_thread_cond_signal_all(t) pthread_cond_broadcast(&t)
#define DC_thread_cond_destroy(t) pthread_cond_destroy(&t)
#endif

#define DC_syn_break(t) {\
    DC_thread_mutex_unlock (t);\
    break;\
}

#define DC_syn_run(t, err, block) \
do {\
    if (ISERR ((err = DC_thread_mutex_lock (t)))) {\
        DC_syn_break(t);\
    }\
    block;\
    DC_syn_break(t);\
} while (0)

#endif
