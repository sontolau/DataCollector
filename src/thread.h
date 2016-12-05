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
#else
#include <pthread.h>
typedef pthread_t DC_thread_t;
typedef pthread_mutex_t DC_thread_mutex_t;
typedef pthread_rwlock_t DC_thread_rwlock_t;
typedef pthread_cond_t DC_thread_cond_t;

typedef void *(*DC_thread_proc_t)(void *);
#endif

extern err_t DC_thread_create(__out__ DC_thread_t *t, __in__ DC_thread_proc_t proc, __in__ void *param);

extern void DC_thread_cancel(__in__ DC_thread_t t);

extern void DC_thread_exit(__in__ uint32_t code);

extern err_t DC_thread_join(__in__ DC_thread_t t, __out__ uint32_t *exitcode);

extern err_t DC_thread_mutex_init(__in__ DC_thread_mutex_t *t);

extern err_t DC_thread_mutex_lock(__in__ DC_thread_mutex_t *t);

extern err_t DC_thread_mutex_trylock(__in__ DC_thread_mutex_t *t);

extern void DC_thread_mutex_unlock(__in__ DC_thread_mutex_t *t);

extern void DC_thread_mutex_destroy(__in__ DC_thread_mutex_t *t);

extern err_t DC_thread_rwlock_init(__out__ DC_thread_rwlock_t *t);

extern err_t DC_thread_rwlock_rdlock(__in__ DC_thread_rwlock_t *t);

extern err_t DC_thread_rwlock_wrlock(__in__ DC_thread_rwlock_t *t);

extern err_t DC_thread_rwlock_tryrdlock(__in__ DC_thread_rwlock_t *t);

extern err_t DC_thread_rwlock_trywrlock(__in__ DC_thread_rwlock_t *t);

extern void DC_thread_rwlock_unlock(__in__ DC_thread_rwlock_t *t);

extern void DC_thread_rwlock_destroy(__in__ DC_thread_rwlock_t *t);

extern err_t DC_thread_cond_init(__out__ DC_thread_cond_t *t);

extern void DC_thread_cond_signal(__in__ DC_thread_cond_t *t, __in__ bool_t all);

extern void DC_thread_cond_destroy(__in__ DC_thread_cond_t *t);

extern err_t DC_thread_cond_wait(DC_thread_cond_t *t,
                                 DC_thread_mutex_t *m,
                                 uint32_t ms);
#define DC_syn_break(t) {\
    DC_thread_mutex_unlock (&t);\
    break;\
}

#define DC_syn_run(t, err, block) \
do {\
    if (ISERR ((err = DC_thread_mutex_lock (&t)))) {\
        DC_syn_break(t);\
    }\
    block;\
    DC_syn_break(t);\
} while (0)


CPP(})
#endif
