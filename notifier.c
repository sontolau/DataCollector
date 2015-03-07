#include <sys/time.h>
#include "notifier.h"

int DC_notifier_init (DC_notifier_t *notif, DC_mutex_t *obj, DC_error_t *error)
{
    notif->error = error;

    if (pthread_cond_init (&notif->PRI(notif_cond), NULL)) {
        DC_error_set (error, ERR_SYSTEM, STRERR(ERR_SYSTEM));
        return ERR_SYSTEM;
    }

   
    if (DC_mutex_init (obj, 0, NULL, NULL)) {
        pthread_cond_destroy (&notif->PRI (notif_cond));
        DC_error_set (error, ERR_SYSTEM, STRERR(ERR_SYSTEM));
        return ERR_SYSTEM;
    }
    
    notif->PRI (notif_object) = obj;
    return ERR_OK;
}

int DC_notifier_wait (DC_notifier_t *notif, long time)
{
    struct timeval now;
    struct timespec timedwait = {0,0};
    int ret = 0;

    if (DC_mutex_lock (notif->PRI(notif_object), 0, 0) < 0) {
        fprintf (stderr, "The lock has been locked.\n");
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
   
    if (ret) {
        if (ret == ETIMEDOUT) {
            ret = ERR_TIMEDOUT;
        } else {
            ret = ERR_SYSTEM;
        }
    } else {
        ret = ERR_OK;
    }

    DC_error_set (notif->error, ret, STRERR(ret));
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
