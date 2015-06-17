#ifndef _DC_NOTIFIER_H
#define _DC_NOTIFIER_H

#include "libdc.h"
#include "error.h"
#include "mutex.h"

DC_CPP (extern "C" {)

typedef struct _notifier {
    DC_error_t      PUB (*error);
    DC_mutex_t*    PRI (notif_object);
    pthread_cond_t  PRI (notif_cond);
} DC_notifier_t;


extern int DC_notifier_init (DC_notifier_t *notif, DC_mutex_t *obj, DC_error_t *error);

extern int DC_notifier_wait (DC_notifier_t *notif, long ms, int lockflag);

extern void DC_notifier_notify (DC_notifier_t *notif);

extern void DC_notifier_notify_all (DC_notifier_t *notif);

extern void DC_notifier_destroy (DC_notifier_t *notif);

DC_CPP (})

#endif //_DC_NOTIFIER_H
