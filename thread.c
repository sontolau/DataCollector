#include "thread.h"


#ifdef OS_WINDOWS
#else
static void *__thread_cb (void *data)
#endif
{
    DC_thread_t *thread = (DC_thread_t*)data;
    int ret;

    do {
        thread->PRI (exit_flag)     = 0;
        thread->PRI (thread_status) = THREAD_STAT_READY;
        ret = DC_locker_wait (&thread->PRI (notify_obj), 1, 0);
        if (thread->PRI (exit_flag)) {
            break;
        } else {
            if (ret == ERR_OK) {
                thread->PRI (thread_status) = THREAD_STAT_RUNNING;
                if (thread->PRI (thread_callback)) {
                    thread->PRI (thread_callback) (thread->PRI (user_data));
                }
            } else if (ret != ERR_TIMEDOUT) {
                break;
            }
        }

        //DC_locker_unlock (&thread->PRI (notify_obj));
        //DC_locker_lock (&thread->PRI (notify_obj), LOCK_IN_NORMAL, 1);
    } while (1);
    //Dlog ("The thread[%u] is exiting...\n", pthread_self ());
    thread->PRI (thread_status) = THREAD_STAT_EXITING;
    DC_locker_unlock (&thread->PRI (notify_obj));

    return __thread_cb;
}

int DC_thread_init (DC_thread_t *thread)
{
    int ret = ERR_OK;

    thread->PRI (thread_status)    = 0;

    if ((ret = DC_locker_init (&thread->PRI (notify_obj), 0, NULL)) != ERR_OK) {
        return ret;
    } else {
        DC_locker_lock (&thread->PRI (notify_obj), LOCK_IN_NORMAL, 1);
    }

    
#ifdef OS_WINDOWS
#else
    if (pthread_create (&thread->PRI (thread_handle), NULL, __thread_cb, thread)) {
        return ERR_FAILURE;
    } else {
        DC_locker_lock (&thread->PRI (notify_obj), LOCK_IN_NORMAL, 1);
        DC_locker_unlock (&thread->PRI (notify_obj));
    }
#endif

    return ERR_OK;
}

int DC_thread_run (DC_thread_t *thread,
                   DC_thread_func_t  thread_cb,
                   void  *data)
{
    int retcode = ERR_OK;
    if ((retcode = DC_locker_lock (&thread->PRI (notify_obj), LOCK_IN_NORMAL, 0)) != ERR_OK) {
        return retcode;
    }
    thread->PRI (thread_callback) = thread_cb;
    thread->PRI (user_data)       = data;
    DC_locker_unlock (&thread->PRI (notify_obj));

    DC_locker_notify (&thread->PRI (notify_obj), 1);
    return retcode;
}

int DC_thread_get_status (DC_thread_t *thread)
{
    int retcode = ERR_OK;

    retcode = DC_locker_lock (&thread->PRI (notify_obj), LOCK_IN_NORMAL, 0);
    switch (retcode) {
        case ERR_OK:
            retcode = thread->PRI (thread_status);
            DC_locker_unlock (&thread->PRI (notify_obj));
            break;
        case ERR_BUSY:
            retcode = THREAD_STAT_RUNNING;
            break;
        default:
            break;
    }

    return retcode;
}
 
void DC_thread_destroy (DC_thread_t *thread)
{
    pthread_cancel (thread->PRI (thread_handle));
    DC_locker_destroy (&thread->PRI (notify_obj));
}

static void __task_core (void *data)
{
    DC_task_t *task = (DC_task_t*)data;
    DC_task_manager_t *manager = task->PRI (task_manager);
    
    if (task->PRI (task)) {
        task->PRI (task) (task->PRI (task_data));
    }

    DC_locker_lock (&manager->PRI (manager_lock), LOCK_IN_NORMAL, 1);
    DC_queue_add (&manager->PRI (task_queue), (obj_t)task, 0);
    DC_locker_notify (&manager->PRI (manager_lock), 1);
    DC_locker_unlock (&manager->PRI (manager_lock));
}


int DC_task_manager_init (DC_task_manager_t *manager, 
                          int maxtasks)
{
    int i;
    int ret = ERR_OK;

    DC_task_t *task;

    memset (manager, '\0', sizeof (DC_task_manager_t));

    manager->max_tasks   = maxtasks;
    if ((ret = DC_queue_init (&manager->PRI (task_queue), maxtasks)) != ERR_OK) {
        return ret;
    }

    if (!(manager->PRI (task_pool) = (DC_task_t*)calloc (maxtasks, sizeof (DC_task_t)))) {
        return ERR_NOMEM;
    } else {
        for (i=0; i<maxtasks; i++) {
            task = &manager->PRI (task_pool)[i];
            task->PRI (task_manager) = manager;
            task->PRI (task_data)    = NULL;
            task->PRI (task)         = NULL;
            DC_thread_init (&task->PRI (task_thread));
            DC_queue_add (&manager->PRI (task_queue), (obj_t)task, 0);
        }
    }

    return ERR_OK;
}


int DC_task_manager_run_task (DC_task_manager_t *manager,
                              void (*task_core) (void *arg),
                              void *userdata,
                              long wait)
{
    DC_task_t   *task    = NULL;

    do {
        DC_locker_lock (&manager->PRI (manager_lock), LOCK_IN_NORMAL, 1);
        if (DC_queue_is_empty (&manager->PRI (task_queue))) {
            if (wait) {
                DC_locker_wait (&manager->PRI (manager_lock), 1, 0);
            } else {
                DC_locker_unlock (&manager->PRI (manager_lock));
                return ERR_BUSY;
            }
        }

        if ((task = (DC_task_t*)DC_queue_fetch (&manager->PRI (task_queue)))) {
            task->PRI (task) = task_core;
            task->PRI (task_data) = userdata;
        }

        DC_locker_unlock (&manager->PRI (manager_lock));
    } while (!task);

    DC_thread_run (&task->PRI (task_thread), __task_core, task);

    return ERR_OK;
}


void DC_task_manager_destroy (DC_task_manager_t *manager)
{
    DC_task_t *task;

    if (manager) {
        while ((task = (DC_task_t*)DC_queue_fetch (&manager->PRI (task_queue))) != (DC_task_t*)QZERO) {
            DC_thread_destroy (&task->PRI (task_thread));
        }

        DC_queue_destroy (&manager->PRI (task_queue));
        if (manager->PRI (task_pool)) {
            free (manager->PRI (task_pool));
        }
    }
}

