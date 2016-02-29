#include "locker.h"

#define PMUTEX(x)   ((pthread_mutex_t*)&x)
#define RWLOCK(x)   ((pthread_rwlock_t*)&x)

int DC_locker_init (DC_locker_t*locker,
                   int rwflag,
                   DC_attr_t *attr)
{
    locker->PRI (rwflag) = rwflag;
#ifdef OS_WINDOWS
#else
    if (rwflag) {
        if (pthread_rwlock_init (RWLOCK(locker->PRI (lock)), NULL)) {
            return ERR_FAILURE;
        }
    } else {
        if (pthread_mutex_init (PMUTEX(locker->PRI (lock)), NULL)) {
            return ERR_FAILURE;
        }
        if (pthread_cond_init (&locker->PRI (cond), NULL) < 0) {
            return ERR_FAILURE;
        }

    }

#endif
    return ERR_OK;
}

int DC_locker_lock (DC_locker_t*locker, int type, int wait)
{
    int ret = ERR_OK;
#ifdef OS_WINDOWS
#else
    if (locker->PRI (rwflag)) {
        if (type == LOCK_IN_READ) {
            if (wait) ret = pthread_rwlock_rdlock (RWLOCK(locker->PRI (lock)));
            else      ret = pthread_rwlock_tryrdlock (RWLOCK(locker->PRI (lock)));
        } else if (type == LOCK_IN_WRITE || type == LOCK_IN_NORMAL) {
            if (wait) ret = pthread_rwlock_wrlock (RWLOCK(locker->PRI (lock)));
            else      ret = pthread_rwlock_trywrlock (RWLOCK(locker->PRI (lock)));
        }
    }  else {
        if (type != LOCK_IN_NORMAL) {
            ret = ERR_FAILURE;
        } else {
            if (wait) ret = pthread_mutex_lock (PMUTEX(locker->PRI (lock)));
            else      ret = pthread_mutex_trylock (PMUTEX(locker->PRI (lock)));
        }
    }
#endif

    switch (ret) {
        case 0:
            return ERR_OK;
        case EBUSY:
            return ERR_BUSY;
        default:
            return ERR_FAILURE;
    }

    return ret;
}

DC_INLINE int __cond_mutex_wait (DC_locker_t *locker,  int block, long time)
{
    struct timeval now;
    struct timespec timedwait = {0,0};
    int ret = ERR_OK;

    if (time) {
        gettimeofday(&now, NULL);
        timedwait.tv_sec = now.tv_usec*1000;

#define NS(ms) (ms * 1000000)
#define ABS_NS_TIME(now, delay)   (now.tv_usec * 1000 + delay)

        timedwait.tv_sec = now.tv_sec + ABS_NS_TIME(now,
                               NS(time))/1000000000;
        timedwait.tv_nsec = ABS_NS_TIME(now, NS(time)) % 1000000000;
        ret = pthread_cond_timedwait (&locker->PRI (cond),
                                      PMUTEX (locker->PRI (lock)),
                                      &timedwait);
    } else {
        ret = pthread_cond_wait (&locker->PRI (cond),
                                 PMUTEX (locker->PRI (lock)));
    }

    if (ret) {
        if (ret == ETIMEDOUT) {
            ret = ERR_TIMEDOUT;
        } else {
            ret = ERR_FAILURE;
        }
    }

    return ret;
}


int DC_locker_wait (DC_locker_t *locker, int block, int ms)
{
    assert (locker->PRI (rwflag) == 0);
    return __cond_mutex_wait (locker, block, ms);
}

void DC_locker_notify (DC_locker_t *locker, int all)
{
    assert (locker->PRI (rwflag) == 0);

    if (all) {
        pthread_cond_broadcast (&locker->PRI (cond));
    } else {
        pthread_cond_signal (&locker->PRI (cond));
    }
}


void DC_locker_unlock (DC_locker_t*locker)
{
#ifdef OS_WINDOWS
#else
    if (locker->PRI (rwflag)) {
        pthread_rwlock_unlock (RWLOCK(locker->PRI (lock)));
    } else {
        pthread_mutex_unlock (PMUTEX(locker->PRI (lock)));
    }
    //usleep (100);
#endif
}

void DC_locker_destroy (DC_locker_t*locker)
{
#ifdef OS_WINDOWS
#else
    if (locker->PRI (rwflag)) {
        pthread_rwlock_destroy (RWLOCK(locker->PRI (lock)));
    } else {
    	pthread_cond_destroy (&locker->PRI (cond));
        pthread_mutex_destroy (PMUTEX(locker->PRI (lock)));
    }

#endif
}
