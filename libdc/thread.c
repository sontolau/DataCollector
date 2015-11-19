#include "thread.h"
#include "log.h"

#ifdef OS_WINDOWS
#else
static void *__thread_cb (void *data)
#endif
{
    DC_thread_t *thread = (DC_thread_t*)data;
    int ret = 0;

    do {
        thread->PRI (exit_flag)     = 0;
        thread->PRI (thread_status) = THREAD_STAT_READY;
        ret = DC_locker_wait (&thread->PRI (notify_obj), 
                              1, thread->PRI (every_ms));
        if (thread->PRI (exit_flag)) {
            break;
        } else {
            //if (ret == ERR_OK) {
            thread->PRI (thread_status) = THREAD_STAT_RUNNING;
            if (thread->PRI (thread_callback)) {
                thread->PRI (thread_callback) (
                       thread->PRI (user_data));
            }
/*
            } else if (ret != ERR_TIMEDOUT) {
                break;
            }
*/
        }

    } while (1);
    thread->PRI (thread_status) = THREAD_STAT_EXITING;
    DC_locker_unlock (&thread->PRI (notify_obj));

    return __thread_cb;
}

int DC_thread_init (DC_thread_t *thread,
                    unsigned int ms)
{
    int ret = ERR_OK;

    thread->PRI (thread_status)    = 0;
    thread->PRI (every_ms)         = ms;

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
    if (DC_locker_lock (&thread->PRI (notify_obj), 
                        LOCK_IN_NORMAL, 0) != ERR_OK) {
        return ERR_BUSY;
    }
    thread->PRI (thread_callback) = thread_cb;
    thread->PRI (user_data)       = data;
    DC_locker_notify (&thread->PRI (notify_obj), 1);
    DC_locker_unlock (&thread->PRI (notify_obj));
    usleep (1);

    return ERR_OK;
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

int DC_task_manager_init (DC_task_manager_t *manager, 
                          int maxtasks)
{
    int i;
    int ret = ERR_OK;

    DC_task_t *task;

    memset (manager, '\0', sizeof (DC_task_manager_t));

    manager->max_tasks   = maxtasks;
    if ((ret = DC_queue_init (&manager->PRI (task_queue), 
                              maxtasks)) != ERR_OK) {
        return ret;
    }

    DC_locker_init (&manager->PRI (qlock), 0, NULL);

    if (!(manager->PRI (task_pool) = (DC_task_t*)calloc (maxtasks, 
				          sizeof (DC_task_t)))) {
        return ERR_NOMEM;
    } else {
        for (i=0; i<maxtasks; i++) {
            task = &manager->PRI (task_pool)[i];
            task->PRI (task_manager) = manager;
            task->PRI (task_data)    = NULL;
            task->PRI (task)         = NULL;
            DC_thread_init (&task->PRI (task_thread), 0);
            DC_queue_add (&manager->PRI (task_queue), (obj_t)task, 0);
        }
    }

    return ERR_OK;
}

static void __task_core (void *data)
{
    DC_task_t *task = (DC_task_t*)data;
    DC_task_manager_t *manager = task->PRI (task_manager);

    if (task->PRI (task)) {
        task->PRI (task) (task->PRI (task_data));
    }

    DC_locker_lock (&manager->PRI (qlock), LOCK_IN_NORMAL, 1);
    DC_queue_add (&manager->PRI (task_queue), (obj_t)task, 0);
    DC_locker_notify (&manager->PRI (qlock), 1);
    DC_locker_unlock (&manager->PRI (qlock));
}


int DC_task_manager_run_task (DC_task_manager_t *manager,
                              void (*task_core) (void *arg),
                              void *userdata,
                              long wait)
{
    DC_task_t   *task    = NULL;

    do {
	DC_locker_lock (&manager->PRI (qlock), 0, wait);
        if (DC_queue_is_empty (&manager->PRI (task_queue))) {
            if (wait) {
                DC_locker_wait (&manager->PRI (qlock), 1, 0);
            } else {
                DC_locker_unlock (&manager->PRI (qlock));
                return ERR_BUSY;
            }
        }


        if ((task = (DC_task_t*)(DC_task_t*)DC_queue_fetch 
            (&manager->PRI (task_queue))) != (DC_task_t*)QZERO) {
            task->PRI (task) = task_core;
            task->PRI (task_data) = userdata;
            DC_thread_run (&task->PRI (task_thread), __task_core, task);
        }

        DC_locker_unlock (&manager->PRI (qlock));
    } while ((obj_t)task == QZERO);

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

DC_INLINE void __do_task (void *data)
{
    DC_task_queue_t *qtask = (DC_task_queue_t*)data;
    obj_t objdata;

    //Dlog ("[task] task %u is running.", pthread_self ());
    do {
        DC_locker_lock (&qtask->locker, 0, 1);
        if (DC_queue_get_length (&qtask->queue) > 0) {
            objdata = DC_queue_fetch (&qtask->queue);
            DC_locker_unlock (&qtask->locker);
            qtask->task_func (qtask->data, (void*)objdata);
        } else {
            DC_locker_unlock (&qtask->locker);
            break;
        }
    } while (1);
    //Dlog ("[task] task %u is stoped.", pthread_self ());
}


DC_INLINE void __master_cb (void *data)
{
    DC_task_queue_t *qtask = (DC_task_queue_t*)data;
    int ret;

    do {
        //do {
            DC_locker_lock (&qtask->locker, 0, 1);
            if (DC_queue_is_empty (&qtask->queue)) {
                DC_locker_unlock (&qtask->locker);
                break;
                //ret = DC_locker_wait (&qtask->locker, 1, 0);
            } else {
                ret = ERR_OK;
            }

            if (ret == ERR_OK) {
                DC_locker_unlock (&qtask->locker);
               // break;
            }
        //} while (1);
        

        DC_task_manager_run_task (&qtask->task_manager,
                                  __do_task,
                                  qtask,
                                  1);
        
    } while (1);
}

int DC_task_queue_init (DC_task_queue_t *qtask,
                        int size,
                        int numtasks,
                        void (*task_func)(void*, void*),
                        void *userdata)
{
    qtask->dirty = 0;
    if (DC_locker_init (&qtask->locker, 0, NULL) < 0) {
        return -1;
    }

    if (DC_thread_init (&qtask->master_thread, 1) != ERR_OK) {
L0:
        DC_locker_destroy (&qtask->locker);
        return -1;
    } else {
    }

    if (DC_queue_init (&qtask->queue, size) < 0) {
L1:
        DC_thread_destroy (&qtask->master_thread);
        goto L0;
    }

    if (DC_task_manager_init (&qtask->task_manager, numtasks) < 0) {
        DC_queue_destroy (&qtask->queue);
        goto L1;
    }

    qtask->data = userdata;
    qtask->task_func = task_func;

    DC_thread_run (&qtask->master_thread, __master_cb, qtask);

    return 0;
}


int DC_task_queue_run_task (DC_task_queue_t *qtask, void *object, int wait)
{
    int ret = ERR_OK;

    if (object) {
        if (DC_locker_lock (&qtask->locker, 0, 1) != ERR_OK) {
            return ERR_BUSY;
        }
        DC_locker_notify (&qtask->locker, 1);
	if (!DC_queue_is_full (&qtask->queue)) {
            
            DC_queue_add (&qtask->queue, (obj_t)object, 1);
            DC_locker_notify (&qtask->locker, 1);
/*
    	    DC_thread_run (&qtask->master_thread, 
                           __master_cb, 
                           qtask);
*/
        } else {
            ret = ERR_FULL;
        }
        DC_locker_unlock (&qtask->locker);
    }
    return ret;
}

void DC_task_queue_destroy (DC_task_queue_t *qtask)
{
    DC_task_manager_destroy (&qtask->task_manager);
    DC_thread_destroy (&qtask->master_thread);
    DC_queue_destroy (&qtask->queue);
    DC_locker_destroy (&qtask->locker);
}
