#ifndef _DC_MUTEX_H
#define _DC_MUTEX_H

#include "libdc.h"
#include "error.h"

typedef struct _DC_attr {

} DC_attr_t;


typedef struct _DC_mutex {
    DC_error_t  PUB (*error);
#ifndef OS_WINDOWS
    int PRI (rwflag);
    union {
        pthread_rwlock_t   PRI (rwlock);
        pthread_mutex_t    PRI (mutex);
    } PRI (mutex_lock);
#else

#endif
} DC_mutex_t;

enum {
    LOCK_IN_NORMAL= 0,
    LOCK_IN_WRITE = 1,
    LOCK_IN_READ  = 2,
};

extern int DC_mutex_init (DC_mutex_t *mutex,
                          int rwflag, 
                          DC_attr_t *attr,
                          DC_error_t *error);

extern int DC_mutex_lock (DC_mutex_t *mutex, int type, int wait);

extern void DC_mutex_unlock (DC_mutex_t *mutex);

extern void DC_mutex_destroy (DC_mutex_t *mutex);

#endif
