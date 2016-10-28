#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "errcode.h"

#ifdef WINDOWS
#else
#include <pthread.h>
#define DC_thread_t pthread_t
#define DC_thread_mutex_t pthread_mutex_t
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

#define DC_thread_cond_init(t) __sys(pthread_cond_init(&t, NULL))
#define DC_thread_cond_wait(t, m) __sys(pthread_cond_wait(&t, &m))

static int DC_thread_cond_timedwait(DC_thread_cond_t t, 
                                    DC_thread_mutex_t m, 
                                    uint32_t ms) {
    struct timespec __time_to_wait;
    struct timeval __now;

    gettimeofday (&__now, NULL);
    __time_to_wait.tv_sec = __now.tv_sec+((int)(ms/1000));
    __time_to_wait.tv_nsec = __now.tv_usec * 1000+1000*1000*(ms%1000);
    __time_to_wait.tv_sec += __time_to_wait.tv_nsec/(1000*1000*1000);
    __time_to_wait.tv_nsec %= (1000*1000*1000);

    if (ISERR (pthread_cond_timedwait (&t, &m, &__time_to_wait))) {
        if (errno == ETIMEDOUT) {
            return E_TIMEDOUT;
        }

        return E_SYSTEM;
    }
    
    return E_OK;
}

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
