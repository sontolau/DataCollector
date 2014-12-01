#include "signal.h"

#define __SIG_EXIT ((DC_sig_t)0xFFFFFFFE)

struct _DC_signal {
    DC_sig_t __sig;
    void (*__sig_callback)(DC_sig_t, void*);
    void *__data;
    long  __timedwait;
    DC_list_t      __sig_async_func;
    pthread_mutex_t __sig_mutex;
    pthread_cond_t  __sig_cond;
};


static void *__wait_cb (void *data)
{
    struct _DC_signal *sig = (struct _DC_signal*)data;
    int    signo;

    while ((signo = DC_signal_wait (sig, sig->__timedwait)) != INVALID_SIGNAL) {
        if (signo == __SIG_EXIT) {
            break;
        } else {
            if (sig->__sig_callback) {
                sig->__sig_callback (signo, sig->__data);
            }
        }
    }

    printf ("%lu exit \n", pthread_self ());
    return NULL;
}

static void __sig_free (void*);
HDC DC_signal_alloc ()
{
    struct _DC_signal *sig;

    sig = (struct _DC_signal*)malloc (sizeof (struct _DC_signal));
    if (sig == NULL) {
        return NULL;
    }

    memset (sig, '\0', sizeof (struct _DC_signal));

    if (pthread_mutex_init (&sig->__sig_mutex, NULL) < 0) {
        free (sig);
        return NULL;
    }

    if (pthread_cond_init (&sig->__sig_cond, NULL) < 0) {
        pthread_mutex_destroy (&sig->__sig_mutex);
        free (sig);
        return NULL;
    }

    if (DC_list_init (&sig->__sig_async_func, __sig_free, NULL) < 0) {
        pthread_mutex_destroy (&sig->__sig_mutex);
        pthread_cond_destroy (&sig->__sig_cond);
        free (sig);
        return NULL;
    }

    return (HDC)sig;
}        

DC_sig_t DC_signal_wait (HDC hsig, long time)
{
    struct _DC_signal *sig = (struct _DC_signal*)hsig;
    struct timespec timedwait = {(long)(time/1000), (time%1000)*1000000};
    int ret;
    DC_sig_t signal;

    if (pthread_mutex_lock (&sig->__sig_mutex) < 0) {
        return INVALID_SIGNAL;
    }

    if (time) {
        ret = pthread_cond_timedwait (&sig->__sig_cond, &sig->__sig_mutex, &timedwait);
        if (errno == ETIMEDOUT) {
            sig->__sig = 0;
        }
    } else {
        ret = pthread_cond_wait (&sig->__sig_cond, &sig->__sig_mutex);
    }

    signal = sig->__sig;
    pthread_mutex_unlock (&sig->__sig_mutex);

    return ret<0?INVALID_SIGNAL:signal;
}


int      DC_signal_wait_asyn (HDC hsig,
                              long milliseconds,
                              void (*sig_cb)(DC_sig_t, void*), 
                              void *data)
{
    struct _DC_signal *sig = (struct _DC_signal*)hsig;
    pthread_t        *asyn_func = NULL;

    sig->__timedwait    = milliseconds;
    sig->__sig_callback = sig_cb;
    sig->__data         = data;

    asyn_func = (pthread_t*)calloc (1, sizeof (pthread_t));
    if (asyn_func == NULL || pthread_mutex_lock (&sig->__sig_mutex) < 0) {
        return -1;
    }
    printf ("%p\n", asyn_func);
    if (pthread_create (asyn_func, NULL, __wait_cb, hsig) < 0) {
        pthread_mutex_unlock (&sig->__sig_mutex);
        free (asyn_func);
        return -1;
    }

    DC_list_insert_object_at_index (&sig->__sig_async_func, asyn_func, 0);
    pthread_mutex_unlock (&sig->__sig_mutex);
    return 0;
}


int     DC_signal_send (HDC sig, DC_sig_t s)
{
    if (pthread_mutex_lock (&((struct _DC_signal*)sig)->__sig_mutex) < 0) {
        return -1;
    }

    ((struct _DC_signal*)sig)->__sig = s;

    pthread_mutex_unlock (&((struct _DC_signal*)sig)->__sig_mutex);
    return pthread_cond_broadcast (&((struct _DC_signal*)sig)->__sig_cond);
}

static void __sig_free (void *obj)
{
    pthread_t *thrd = (pthread_t*)obj;
    printf ("xxxx %p\n", obj);
    pthread_join (*thrd, NULL);
    free (thrd);
}

void     DC_signal_free (HDC hsig)
{
    struct _DC_signal *sig = (struct _DC_signal*)hsig;

    DC_signal_send (sig, __SIG_EXIT);
    //DC_list_loop (&sig->__sig_async_func, __sig_free);
    DC_list_destroy (&sig->__sig_async_func);
    pthread_mutex_destroy (&sig->__sig_mutex);
    pthread_cond_destroy (&sig->__sig_cond);
    free (sig);
}

