#include "libdc.h"
#include <sys/time.h>
//#include "notifier.h"

int DC_notifier_init (DC_notifier_t *notif, DC_mutex_t *obj, DC_error_t *error)
{
    notif->error = error;

    if (pthread_cond_init (&notif->PRI(notif_cond), NULL)) {
        DC_error_set (error, -1, ERRSTR);
        return ERR_FAILURE;
    }

   
    if (DC_mutex_init (obj, 0, NULL, error)) {
        pthread_cond_destroy (&notif->PRI (notif_cond));
        return ERR_FAILURE;
    }
    
    notif->PRI (notif_object) = obj;
    return ERR_OK;
}

int DC_notifier_wait (DC_notifier_t *notif, long time, int lockflag)
{
    struct timeval now;
    struct timespec timedwait = {0,0};
    int ret = 0;

    if (!lockflag && DC_mutex_lock (notif->PRI(notif_object), 0, 1) < 0) {
        return ERR_FAILURE;
    }

    if (time) {
        gettimeofday(&now, NULL);
        timedwait.tv_sec = now.tv_usec*1000;

#define NS(ms) (ms * 1000000)
#define ABS_NS_TIME(now, delay)   (now.tv_usec * 1000 + delay)

        timedwait.tv_sec = now.tv_sec + ABS_NS_TIME(now,
                               NS(time))/1000000000;
        timedwait.tv_nsec = ABS_NS_TIME(now, NS(time)) % 1000000000;
        ret = pthread_cond_timedwait (&notif->PRI(notif_cond),
                                      &notif->PRI(notif_object)->PRI (mutex_lock).PRI (mutex),
                                      &timedwait);
    } else {
        ret = pthread_cond_wait (&notif->PRI(notif_cond), 
                                 &notif->PRI(notif_object)->PRI(mutex_lock).PRI(mutex));
    }
  
    if (!lockflag) {
        DC_mutex_unlock (notif->PRI (notif_object));
    }

    if (ret) {
        DC_error_set (notif->error, ERRNO, ERRSTR);
        if (ret == ETIMEDOUT) {
            ret = time;
        } else {
            ret = -1;
        }
    }

    return ret;
}

void DC_notifier_notify (DC_notifier_t *notif)
{
    //DC_mutex_lock (notif->PRI (notif_object), 0, 1);
    pthread_cond_signal (&notif->PRI(notif_cond));
    //usleep (10);
    //DC_mutex_unlock (notif->PRI (notif_object));
}

void DC_notifier_notify_all (DC_notifier_t *notif)
{
    //DC_mutex_lock (notif->PRI (notif_object), 0, 1);
    pthread_cond_broadcast (&notif->PRI(notif_cond));
    //usleep (10);
    //DC_mutex_unlock (notif->PRI (notif_object));
}

void DC_notifier_destroy (DC_notifier_t *notif)
{
    //pthread_mutex_lock (&notif->PRI (notif_mutex));
    pthread_cond_destroy (&notif->PRI(notif_cond));
    DC_mutex_destroy (notif->PRI (notif_object));
    //pthread_mutex_destroy (&notif->PRI(notif_mutex));
}
