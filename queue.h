#ifndef _DC_QUEUE_H
#define _DC_QUEUE_H

#include "libdc.h"

DC_CPP (extern "C" {)

typedef long long   qobject_t;

typedef struct _DC_queue {
    unsigned int queue_size;
    unsigned int length;
    qobject_t    zero_type;
    qobject_t    *__queue_buffer;
    qobject_t    *__tail_ptr;
    qobject_t    *__head_ptr;
}DC_queue_t;


extern int DC_queue_init (DC_queue_t *queue, unsigned int queue_size, qobject_t zero);

extern int DC_queue_push (DC_queue_t *queue, qobject_t obj, int overwrite);

extern int DC_queue_is_empty (const DC_queue_t *queue);

extern int DC_queue_is_full (const DC_queue_t *queue);

extern qobject_t DC_queue_pop (DC_queue_t *queue);

extern void DC_queue_destroy (DC_queue_t *queue);

#define QTYPEOF(type, obj)   ((type*)obj)

DC_CPP (})
#endif
