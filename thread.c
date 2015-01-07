#include "thread.h"

#define DC_thread_set_status(_thrd, _stat) \
    do {\
        if (_thrd->thrd_status_cb) {\
            _thrd->thrd_status_cb (_thrd, DC_thread_user_data(_thrd), _stat);\
        }\
        _thrd->thrd_status = _stat;\
    } while (0)

static void *__thrd_core (void *data)
{
    DC_thread_t  *thrd = (DC_thread_t*)data;
    DC_sig_t     sig;

    pthread_rwlock_unlock (&thrd->thrd_lock);

    while ((sig = DC_signal_wait (thrd->thrd_signal, 0)) > 0) {
        pthread_rwlock_wrlock (&thrd->thrd_lock);
        DC_thread_set_status (thrd, THREAD_STAT_RUNNING);

        if (thrd->thrd_handler) {
            thrd->thrd_handler (thrd, thrd->user_data);
        }
        DC_thread_set_status (thrd, THREAD_STAT_IDLE);
        pthread_rwlock_unlock (&thrd->thrd_lock);
    }

    return __thrd_core;
}

int DC_thread_init (DC_thread_t *thread,
                    void (*status_cb)(DC_thread_t*, void*, int))
{
    memset (thread, '\0', sizeof (DC_thread_t));

    thread->thrd_status = THREAD_STAT_IDLE;
    thread->thrd_status_cb = status_cb;

    if (!(thread->thrd_signal = DC_signal_alloc ())) {
        return -1;
    }

    if (pthread_rwlock_init (&thread->thrd_lock, NULL) < 0) {
        DC_signal_free (thread->thrd_signal);
        return -1;
    }

    if (pthread_create (&thread->thrd_handle, NULL, __thrd_core, thread) < 0) {
        pthread_rwlock_destroy (&thread->thrd_lock);
        DC_signal_free (thread->thrd_signal);
        return -1;
    }

    pthread_rwlock_wrlock (&thread->thrd_lock);
    return 0;
}

void DC_thread_set (DC_thread_t *thread,
                    void (*handler) (struct _thread*, void *user),
                    void *userdata)
{
    pthread_rwlock_wrlock (&thread->thrd_lock);
    thread->thrd_handler = handler;
    thread->user_data    = userdata;
    pthread_rwlock_unlock (&thread->thrd_lock);
}

void DC_thread_run (DC_thread_t *thread)
{
    DC_signal_send (thread->thrd_signal, 1);
}

int DC_thread_get_status (DC_thread_t *thread)
{
    int status;

    pthread_rwlock_rdlock (&thread->thrd_lock);
    status = thread->thrd_status;
    pthread_rwlock_unlock (&thread->thrd_lock);

    return status;
}

void DC_thread_destroy (DC_thread_t *thread)
{
    DC_signal_free (thread->thrd_signal);
    pthread_rwlock_destroy (&thread->thrd_lock);
}

static void __mgr_thread_status_cb (DC_thread_t *thrd, void *data, int status)
{
    DC_thread_manager_t *mgr = (DC_thread_manager_t*)data;

    switch (status) {
        case THREAD_STAT_RUNNING:
        {
            DC_list_remove_object (&mgr->idle_list, thrd);
            DC_list_add_object (&mgr->running_list, thrd);
        }
            break;
        case THREAD_STAT_IDLE:
        {
            DC_signal_send (mgr->sig_wait, 1);
            DC_list_remove_object (&mgr->running_list, thrd);
            DC_list_add_object (&mgr->idle_list, thrd);
            break;
        }
        default:
            break;
    }
}

int DC_thread_manager_init (DC_thread_manager_t *manager, int numthreads)
{
    int i;

    memset (manager, '\0', sizeof (DC_thread_manager_t));

    manager->thread_pool = (DC_thread_t*)calloc (numthreads, sizeof (DC_thread_t));
    if (!manager->thread_pool) {
L0:
        return -1;
    }

    if (DC_list_init (&manager->running_list, NULL) < 0) {
        free (manager->thread_pool);
L1:
        goto L0;
    }

    if (DC_list_init (&manager->idle_list, NULL) < 0) {
        DC_list_destroy (&manager->idle_list);
        goto L1;
    }

    for (i=0; i<numthreads; i++) {
        if (!DC_thread_init (&manager->thread_pool[i], __mgr_thread_status_cb)) {
            DC_list_add_object (&manager->idle_list, (void*)&manager->thread_pool[i]);
        }
    }

    return 0;
}

static DC_thread_t *DC_thread_manager_alloc(DC_thread_manager_t *mgr)
{
    DC_thread_t *thrd = NULL;

    do {
        thrd = (DC_thread_t*)DC_list_get_object_at_index (&mgr->idle_list, 0);
        if (thrd) {
            break;
        } else {
            if (DC_signal_wait (mgr->sig_wait, 0) <= 0) {
                break;
            }
        }
    } while (1);

    return thrd;
}
    
void  DC_thread_manager_run (DC_thread_manager_t *manager,
                             int (*cond_cb) (DC_thread_manager_t*, void*),
                             void (*func)(DC_thread_t*, void*),
                             void *data)
{
    DC_thread_t *thrd = NULL;

    manager->private_data = data;

    while ((cond_cb?cond_cb(manager, data):1) 
           &&  (thrd = DC_thread_manager_alloc (manager))) {
        if (func) {
            DC_thread_set (thrd, func, manager);
            DC_thread_run (thrd);
        }
    }
}

void DC_thread_manager_destroy (DC_thread_manager_t *manager)
{
    if (manager) {
    	DC_signal_free (manager->sig_wait);
    	DC_list_destroy (&manager->idle_list);
    	DC_list_destroy (&manager->running_list);
    	free (manager->thread_pool);
    }
}
