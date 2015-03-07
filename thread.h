#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "list.h"
#include "notifier.h"
#include "error.h"
#include "mutex.h"
#include "queue.h"

DC_CPP (extern "C"  {)

enum {
    THREAD_STAT_IDLE    = 0,
    THREAD_STAT_RUNNING = 1,
    THREAD_STAT_WAIT_TIMEOUT = 2,
    THREAD_STAT_EXITING,
    THREAD_STAT_EXITED,
};

struct _DC_thread;
typedef void (*DC_thread_func_t)(struct _DC_thread*, void *userdata);
typedef void  (*DC_thread_status_func_t) (struct _DC_thread*, void *userdata, int status);

typedef struct _DC_thread {
    DC_error_t      PUB (*error);
    void            PUB (*user_data);
    
    //private fields.
    DC_notifier_t   PRI (notif_object);
    int             PRI (thread_status);
    volatile int    PRI (exit_flag);
    DC_mutex_t      PRI (thread_lock);
#ifdef OS_WINDOWS
#else
    pthread_t       PRI (thread_handle);
    pthread_attr_t  PRI (thread_attr);
#endif
    DC_thread_func_t PRI (thread_cb);
    DC_thread_status_func_t PRI (thread_status_cb);
} DC_thread_t;

#define DC_thread_user_data(thread) (thread->user_data)

#define DC_thread_set_user_data(thread, data) do {thread->user_data = data;}while(0)


extern int DC_thread_init (DC_thread_t *thread,
                           DC_thread_status_func_t status_cb,
                           DC_error_t *error);

extern int DC_thread_run (DC_thread_t *thread,
                          DC_thread_func_t  thread_cb, 
                          void *userdata);

#define DC_thread_get_status(thread) (thread->PRI(thread_status))

extern void DC_thread_destroy (DC_thread_t *thread);

struct _DC_thread_pool_manager;
typedef struct _DC_task {
    void          (*will_process) (struct _DC_task*, void*);
    void          (*task)         (struct _DC_task*, void*);
    void          (*did_process)  (struct _DC_task*, void*);
    void          *task_data;

    void          *PRI (task_data);
    DC_thread_t   *PRI (task_thread);
    struct _DC_thread_pool_manager *PRI (pool_manager);
} DC_task_t;

typedef struct _DC_thread_pool_manager {
    int           PUB (max_threads);
    DC_error_t    *PUB (error);
    int           PRI (num_wait);
    DC_notifier_t PRI (task_notifier);
    DC_queue_t    PRI (thread_queue);
    DC_queue_t    PRI (task_queue);
    DC_mutex_t    PRI (pool_mutex);
    DC_thread_t   PRI (manager_thread);
    DC_thread_t   *PRI (thread_pool);
} DC_thread_pool_manager_t;

extern int DC_thread_pool_manager_init (DC_thread_pool_manager_t *pool, int maxthreads, DC_error_t *error);

extern int DC_thread_pool_manager_run_task (DC_thread_pool_manager_t *pool,
                                            void (*task) (struct _DC_task*, void *arg),
                                            void (*will_process) (struct _DC_task*, void*),
                                            void (*did_process)  (struct _DC_task*, void*),
                                            void *userdata,
                                            long wait);

extern void DC_thread_pool_manager_destroy (DC_thread_pool_manager_t *pool);

DC_CPP (})

#endif
