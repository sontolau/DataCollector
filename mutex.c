#include "mutex.h"

#define PMUTEX(x)   ((pthread_mutex_t*)&x)
#define RWLOCK(x)   ((pthread_rwlock_t*)&x)

int DC_mutex_init (DC_mutex_t *mutex,
                   int rwflag,
                   DC_attr_t *attr,
                   DC_error_t *error)
{
    memset (mutex, '\0', sizeof (DC_mutex_t));

    mutex->error = error;
    mutex->PRI (rwflag) = rwflag;
#ifdef OS_WINDOWS
#else
    if (rwflag) {
        if (pthread_rwlock_init (RWLOCK(mutex->PRI (mutex_lock)), NULL)) {
            DC_error_set (error, ERR_SYSTEM, ERRSTR(ERR_SYSTEM));
            return ERR_SYSTEM;
        }
    } else {
        if (pthread_mutex_init (PMUTEX(mutex->PRI (mutex_lock)), NULL)) {
            DC_error_set (error, ERR_SYSTEM, ERRSTR(ERR_SYSTEM));
            return ERR_SYSTEM;
        }
    }
#endif
    return 0;
}

int DC_mutex_lock (DC_mutex_t *mutex, int type, int wait)
{
    int ret = ERR_OK;
#ifdef OS_WINDOWS
#else
    if (mutex->PRI (rwflag)) {
        if (type == LOCK_IN_READ) {
            if (wait) ret = pthread_rwlock_rdlock (RWLOCK(mutex->PRI (mutex_lock)));
            else      ret = pthread_rwlock_tryrdlock (RWLOCK(mutex->PRI (mutex_lock)));
        } else if (type == LOCK_IN_WRITE || type == LOCK_IN_NORMAL) {
            if (wait) ret = pthread_rwlock_wrlock (RWLOCK(mutex->PRI (mutex_lock)));
            else      ret = pthread_rwlock_trywrlock (RWLOCK(mutex->PRI (mutex_lock)));
        }
    }  else {
        if (type != LOCK_IN_NORMAL) {
            DC_error_set (mutex->error, -1, "Unsupported mutex type.");
            ret = -1;
        } else {
            if (wait) ret = pthread_mutex_lock (PMUTEX(mutex->PRI (mutex_lock)));
            else      ret = pthread_mutex_trylock (PMUTEX(mutex->PRI (mutex_lock)));
        }
    }
#endif
    if (ret) {
        ret = ERR_SYSTEM;
        DC_error_set (mutex->error, ERR_SYSTEM, ERRSTR (ERR_SYSTEM));
    } else {
        ret = ERR_OK;
    }

    return ret;
}

void DC_mutex_unlock (DC_mutex_t *mutex)
{
#ifdef OS_WINDOWS
#else
    if (mutex->PRI (rwflag)) {
        pthread_rwlock_unlock (RWLOCK(mutex->PRI (mutex_lock)));
    } else {
        pthread_mutex_unlock (PMUTEX(mutex->PRI (mutex_lock)));
    }
#endif
}

extern void DC_mutex_destroy (DC_mutex_t *mutex)
{
#ifdef OS_WINDOWS
#else
    if (mutex->PRI (rwflag)) {
        pthread_rwlock_destroy (RWLOCK(mutex->PRI (mutex_lock)));
    } else {
        pthread_mutex_destroy (PMUTEX(mutex->PRI (mutex_lock)));
    }
#endif
}
