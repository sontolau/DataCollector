#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "errcode.h"

CPP(extern "C" {)

#ifdef OS_WINDOWS
typedef HANDLE DC_thread_t;
typedef CRITICAL_SECTION DC_thread_mutex_t;
typedef SRWLOCK DC_thread_rwlock_t;
typedef CONDITION_VARIABLE DC_thread_cond_t;
typedef LPTHREAD_START_ROUTINE DC_thread_proc_t;

#define DC_thread_create(t, func, param) ((t=CreateThread(NULL, 0, func, param, 0, NULL))?E_OK:E_ERROR)
#define DC_thread_join(t) (void)WaitForSingleObject(t, 0)
#define DC_thread_destroy(t) (void)CloseHandleEx(t)
#define DC_thread_cancel(t)
#define DC_thread_mutex_init(t) InitializeCriticalSection(&t)
#define DC_thread_mutex_lock(t) EnterCriticalSection(&t)
#define DC_thread_mutex_unlock(t) LeaveCriticalSection(&t)
#define DC_thread_mutex_trylock(t) TryEnterCriticalSection(&t)
#define DC_thread_mutex_destroy(t) DeleteCriticalSection(&t)

#else

#include <pthread.h>

typedef pthread_t DC_thread_t;
typedef pthread_mutex_t DC_thread_mutex_t;
typedef pthread_rwlock_t DC_thread_rwlock_t;
typedef pthread_cond_t DC_thread_cond_t;
typedef void*(*DC_thread_proc_t);
#define __sys(x) (x)

#define DC_thread_create(t, func, arg) __sys(pthread_create(&t, NULL, func, arg))
#define DC_thread_join(t) __sys(pthread_join(&t, NULL))
#define DC_thread_cancel(t) __sys(pthread_cancel(t))
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

CPP(})
#endif
