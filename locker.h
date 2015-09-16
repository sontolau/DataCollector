#ifndef _DC_MUTEX_H
#define _DC_MUTEX_H

#include "libdc.h"

typedef struct _DC_attr {

} DC_attr_t;


typedef struct _DC_locker {    
#ifndef OS_WINDOWS
    int PRI (rwflag);
    union {
        pthread_rwlock_t   rwlock;
        pthread_mutex_t     mutex;
    } PRI (lock);
    pthread_cond_t     PRI (cond);
#else

#endif
} DC_locker_t;

enum {
    LOCK_IN_NORMAL= 0,
    LOCK_IN_WRITE = 1,
    LOCK_IN_READ  = 2,
};

extern int DC_locker_init (DC_locker_t *locker,
                          int rwflag, 
                          DC_attr_t *attr);

extern int DC_locker_lock (DC_locker_t *locker, int type, int wait);

extern int DC_locker_wait (DC_locker_t *locker, int block, int ms);

extern void DC_locker_notify (DC_locker_t *locker, int all);

extern void DC_locker_unlock (DC_locker_t *locker);

extern void DC_locker_destroy (DC_locker_t *locker);

#endif
