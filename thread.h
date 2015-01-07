#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "list.h"
#include "signal.h"

#include <pthread.h>

DC_CPP (extern "C"  {)

enum {
    THREAD_STAT_IDLE    = 0,
    THREAD_STAT_RUNNING = 1,
};

typedef struct _thread {
    int thrd_status;
    HDC  thrd_signal;
    pthread_rwlock_t  thrd_lock;
    pthread_t         thrd_handle;
    void (*thrd_handler) (struct _thread*, void *user);
    void (*thrd_status_cb) (struct _thread*, void *user, int status);
    void *user_data;
} DC_thread_t;

#define DC_thread_user_data(thread) (thread->user_data)
extern int DC_thread_init (DC_thread_t *thread,
                           void (*status_cb) (DC_thread_t*, void*, int));
extern void DC_thread_set (DC_thread_t *thread, 
                           void (*handler) (struct _thread*, void *user),
                           void *userdata);
extern void DC_thread_run (DC_thread_t *thread);

extern int DC_thread_get_status (DC_thread_t *thread);

extern void DC_thread_destroy (DC_thread_t *thread);


typedef struct _thread_manager {
    HDC          sig_wait;
    DC_thread_t  *thread_pool;
    DC_list_t    running_list;
    DC_list_t    idle_list;
    void         *private_data;
} DC_thread_manager_t;

extern int DC_thread_manager_init (DC_thread_manager_t *manager, int numthreads);
extern void  DC_thread_manager_run (DC_thread_manager_t *manager,
                             int (*cond_cb) (DC_thread_manager_t*, void*),
                             void (*func)(DC_thread_t*, void*),
                             void *data);
extern void DC_thread_manager_destroy (DC_thread_manager_t *manager);

DC_CPP (})

#endif
