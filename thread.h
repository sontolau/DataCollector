#ifndef _DC_THREAD_H
#define _DC_THREAD_H

#include "libdc.h"
#include "list.h"
#include "locker.h"
#include "queue.h"

DC_CPP (extern "C"  {)

enum {
    THREAD_STAT_READY   = 1,
    THREAD_STAT_RUNNING = 2,
    THREAD_STAT_EXITING,
};

struct _DC_thread;
typedef void (*DC_thread_func_t)(void *userdata);

typedef struct _DC_thread {
    DC_thread_func_t PRI (thread_callback);
    
    //private fields.
    DC_locker_t   PRI (notify_obj);
    void*         PRI (user_data);
    int           PRI (thread_status);
    volatile int  PRI (exit_flag);
    
    
#ifdef OS_WINDOWS
#else
    pthread_t       PRI (thread_handle);
#endif
} DC_thread_t;


extern int DC_thread_init (DC_thread_t *thread);

extern int DC_thread_run (DC_thread_t *thread,
                          DC_thread_func_t  thread_cb, 
                          void *userdata);

extern int DC_thread_get_status(DC_thread_t *thread);

extern void DC_thread_destroy (DC_thread_t *thread);


struct _DC_task_manager;
typedef struct _DC_task {
    void     (*PRI (task)) (void*);
    void     *PRI (task_data);
    DC_thread_t PRI (task_thread);
    struct _DC_task_manager *PRI (task_manager);
} DC_task_t;

typedef struct _DC_task_manager {
    int           max_tasks;

    DC_queue_t    PRI (task_queue);
    DC_locker_t   PRI (manager_lock);
    DC_task_t     *PRI (task_pool);
} DC_task_manager_t;

extern int DC_task_manager_init (DC_task_manager_t *pool, int maxthreads);


extern int DC_task_manager_run_task (DC_task_manager_t *pool,
                                     void (*task) (void *arg),
                                     void *userdata,
                                     long wait);
extern void DC_task_manager_destroy (DC_task_manager_t *pool);


DC_CPP (})

#endif
