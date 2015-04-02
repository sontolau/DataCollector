#include "libdc.h"

#define __set_thread_status(_thrd, _status) \
do {\
    _thrd->PRI (thread_status) = _status;\
    if (_thrd->PRI (thread_status_cb)) {\
        _thrd->PRI (thread_status_cb) (_thrd, _thrd->user_data, _status);\
    }\
} while (0)

#ifdef OS_WINDOWS
#else
static void *__thread_cb (void *data)
#endif
{
    DC_thread_t *thread = (DC_thread_t*)data;
    int ret;

    do {
        DC_mutex_lock (&thread->PRI (thread_lock), 0, 1);
        ret = DC_notifier_wait (&thread->PRI (notif_object), 0, 1);
        if (ret == 0 && (!thread->PRI (exit_flag))) {
            __set_thread_status (thread, THREAD_STAT_RUNNING);
            if (thread->PRI (thread_cb)) {
                thread->PRI (thread_cb) ((struct _DC_thread*)thread, thread->user_data);
            }
            __set_thread_status (thread, THREAD_STAT_IDLE);
        } 
        DC_mutex_unlock (&thread->PRI (thread_lock));
    } while (!thread->PRI (exit_flag));
    __set_thread_status (thread, THREAD_STAT_EXITED);
    return __thread_cb;
}

int DC_thread_init (DC_thread_t *thread,
                    DC_thread_status_func_t status_cb,
                    DC_error_t *error)
{
    int ret = ERR_OK;

    memset (thread, '\0', sizeof (DC_thread_t));

    thread->error = error;
    thread->PRI (thread_status) = THREAD_STAT_IDLE;
    thread->PRI (thread_status_cb) = status_cb;
    if ((ret = DC_notifier_init (&thread->PRI (notif_object), 
                                 &thread->PRI (thread_lock), 
                                 error))) {
        return ret;
    }
#ifdef OS_WINDOWS
#else
    if (pthread_create (&thread->PRI (thread_handle), NULL, __thread_cb, thread)) {
        DC_error_set (error, ERR_SYSTEM, STRERR(ERR_SYSTEM));
        ret = ERR_SYSTEM;
        DC_notifier_destroy (&thread->PRI (notif_object));
        return  ret;
    } else {
        usleep (100);
    }
#endif

    return ERR_OK;
}

int DC_thread_run (DC_thread_t *thread,
                   DC_thread_func_t  thread_cb,
                   void *data)
{
    int retcode = ERR_OK;

    if ((retcode = DC_mutex_lock (&thread->PRI (thread_lock), 0, 0))) {
        return retcode;
    }
    thread->PRI (thread_cb) = thread_cb;
    thread->user_data       = data;
    DC_mutex_unlock (&thread->PRI (thread_lock));
    DC_notifier_notify (&thread->PRI (notif_object));

    return retcode;
}
                  
void DC_thread_destroy (DC_thread_t *thread)
{
    __set_thread_status (thread, THREAD_STAT_EXITING);
    while (DC_thread_get_status (thread) != THREAD_STAT_EXITED) {
        thread->PRI (exit_flag) = 1;
        DC_notifier_notify_all (&thread->PRI (notif_object));
        usleep (1000);
    }
#ifdef OS_WINDOWS
#else
    pthread_join (thread->PRI (thread_handle), NULL);
#endif
    DC_notifier_destroy (&thread->PRI (notif_object));
}


// the thread pool manager.

static void __thread_pool_status_cb (DC_thread_t *thread, void *userdata, int status)
{
    switch (status) {
        case THREAD_STAT_IDLE:
        case THREAD_STAT_RUNNING:
        case THREAD_STAT_EXITING:
        case THREAD_STAT_EXITED:
        case THREAD_STAT_WAIT_TIMEOUT:
        default:
            break;
    }
}

static void __thread_task (DC_thread_t *thread, void *data)
{
    //int num_wait = 0;
    DC_task_t *task = (DC_task_t*)data;
    DC_thread_pool_manager_t *manager = task->PRI (pool_manager);
    if (task->will_process) {
        task->will_process (task, task->task_data);
    }

    if (task->task) {
        task->task (task, task->task_data);
    }

    if (task->did_process) {
        task->did_process (task, task->task_data);
    }

    DC_mutex_lock (&manager->PRI (pool_mutex), 0, 1);
    //num_wait = manager->PRI (num_wait);
    DC_queue_push (&manager->PRI (thread_queue), (qobject_t)task->PRI (task_thread), 0);
    DC_mutex_unlock (&manager->PRI (pool_mutex));
    /*
    while (num_wait && num_wait >= manager->PRI (num_wait)) {
        fprintf (stderr, "[libdc] notify other listeners to run.\n");
        DC_notifier_notify (&manager->PRI (task_notifier));
        usleep (100);//make other threads run.
        //DC_mutex_lock (&manager->PRI (pool_mutex), 0, 1);
    }
*/
    DC_notifier_notify (&manager->PRI (task_notifier));
    //DC_mutex_unlock (&manager->PRI (pool_mutex));
}


static void __pool_manager_cb (DC_thread_t *thread, void *data)
{
    DC_thread_pool_manager_t *pool_manager = data;
    DC_task_t                *task         = NULL;

    do {
        DC_mutex_lock (&pool_manager->PRI (pool_mutex), 0, 1);
        task = (DC_task_t*)DC_queue_pop (&pool_manager->PRI (task_queue));
        DC_mutex_unlock (&pool_manager->PRI (pool_mutex));
        if (!task) break;

        DC_thread_run (task->PRI (task_thread),
                       __thread_task,
                       task);
    } while (1);
}

int DC_thread_pool_manager_init (DC_thread_pool_manager_t *pool, 
                                 int maxthreads,
                                 DC_error_t *error)
{
    int i;
    int ret = ERR_OK;

    memset (pool, '\0', sizeof (DC_thread_pool_manager_t));

    pool->max_threads = maxthreads;
    pool->error       = error;

    pool->PRI (num_wait) = 0;
    if ((ret = DC_queue_init (&pool->PRI (thread_queue), maxthreads, 0))) {
L0:
        return ret;
    }

    if ((ret = DC_queue_init (&pool->PRI (task_queue), maxthreads, 0))) {

L1:
        DC_queue_destroy (&pool->PRI (thread_queue));
        goto L0;
    }

    if ((ret = DC_notifier_init (&pool->PRI (task_notifier), 
                                 &pool->PRI (pool_mutex),
                                 pool->error))) {
L2:
        DC_queue_destroy (&pool->PRI (task_queue));
        goto L1;
    }

    if ((ret = DC_thread_init (&pool->PRI (manager_thread), 
                               __thread_pool_status_cb,
                               error))) {
L3:
        DC_notifier_destroy (&pool->PRI (task_notifier));
        goto L2;
    } /*else {
        DC_thread_run (&pool->PRI (manager_thread),
                       __pool_manager_cb,
                       (void*)pool);
    }*/

    if (!(pool->PRI (thread_pool) = (DC_thread_t*)calloc (maxthreads, sizeof (DC_thread_t)))) {
        DC_error_set (error, (ret = ERR_SYSTEM), STRERR(ERR_SYSTEM));
        goto L3;
    }
    
    for (i=0; i<maxthreads; i++) {
        DC_thread_init (&pool->PRI (thread_pool)[i], 
                        __thread_pool_status_cb,
                        error);
        DC_queue_push (&pool->PRI (thread_queue), (qobject_t)&pool->PRI (thread_pool)[i], 1);
    }

    return 0;
}


static int __wait_for_idle_thread (DC_thread_pool_manager_t *pool,
                                   long timeout)
{
    int ret = ERR_OK;
    
    while (DC_queue_is_empty (&pool->PRI (thread_queue))) {
        Dlog ("[libdc] WARN: thread pool is full, waiting for a task to be idle[%d tasks in progress] ...\n", pool->PRI (num_wait));
        pool->PRI (num_wait)++;
        ret = DC_notifier_wait (&pool->PRI (task_notifier), timeout, 1);
        pool->PRI (num_wait)--;
        if (ret != ERR_OK) {
            break;
        }
        Dlog ("[libdc] INFO: a idle task has been fetched\n");
    }

    return ret;
}

int DC_thread_pool_manager_run_task (DC_thread_pool_manager_t *pool,
                                     void (*task_core) (DC_task_t*, void *arg),
                                     void (*will_process) (DC_task_t*, void*),
                                     void (*did_process) (DC_task_t*, void*),
                                     void *userdata,
                                     long wait)
{
    int ret = ERR_OK;
    DC_thread_t *thrdptr = NULL;
    DC_task_t   *task    = NULL;

    if (!(task = (DC_task_t*)calloc (1, sizeof (DC_task_t)))) {
        DC_error_set (pool->error, ERR_SYSTEM, STRERR (ERR_SYSTEM));
        return ERR_SYSTEM;
    }
   
    
    DC_mutex_lock (&pool->PRI (pool_mutex), 0, 1);
    ret = __wait_for_idle_thread (pool, wait);
    if (ret == ERR_OK) {
        thrdptr = (DC_thread_t*)DC_queue_pop (&pool->PRI (thread_queue));
        Dlog ("[libdc] INFO: there are still %d idle threads.\n", pool->PRI(thread_queue).length);
        task->PUB (task)        = task_core;
        task->PUB (will_process)= will_process;
        task->PUB (did_process) = did_process;
        task->PUB (task_data)   = userdata;
        
        task->PRI (task_thread) = thrdptr;
        task->PRI (pool_manager)= pool;

        DC_queue_push (&pool->PRI (task_queue), (qobject_t)task, 0);
        DC_mutex_unlock (&pool->PRI (pool_mutex));
        if (DC_thread_get_status (((DC_thread_t*)&pool->PRI (manager_thread)))
                                                     != THREAD_STAT_RUNNING) {
            DC_thread_run (&pool->PRI (manager_thread),
                           __pool_manager_cb,
                           pool);
        }
    } else {
        free (task);
        DC_mutex_unlock (&pool->PRI (pool_mutex));
    }
    return ret;
}

int DC_thread_pool_manager_is_full (DC_thread_pool_manager_t *pool)
{
    int full = 0;

    DC_mutex_lock (&pool->PRI (pool_mutex), 0, 1);
    full = DC_queue_is_empty (&pool->PRI (thread_queue));
    DC_mutex_unlock (&pool->PRI (pool_mutex));

    return full;
}

void DC_thread_pool_manager_destroy (DC_thread_pool_manager_t *pool)
{
    int i;
    DC_task_t *task;

    DC_mutex_lock (&pool->PRI (pool_mutex), 0, 1);
    DC_thread_destroy (&pool->PRI (manager_thread));
    for (i=0; i<pool->max_threads; i++) {
        DC_thread_destroy (&pool->PRI (thread_pool)[i]);
    }
    DC_notifier_destroy (&pool->PRI (task_notifier));
    while ((task = (DC_task_t*)DC_queue_pop (&pool->PRI (task_queue)))) {
        free (task);
    }
   DC_queue_destroy (&pool->PRI (task_queue));
   DC_queue_destroy (&pool->PRI (thread_queue));
   free (pool->PRI (thread_pool));
}

