#ifndef _DC_QUEUE_H
#define _DC_QUEUE_H

#include "libdc.h"

DC_CPP (extern "C" {)

typedef long long   obj_t;

#define QZERO   ((obj_t)-1)

typedef struct _DC_queue {
    unsigned int size;
    obj_t    *PRI (buffer);
    obj_t    *PRI (tail_ptr);
    obj_t    *PRI (head_ptr);
}DC_queue_t;


extern int DC_queue_init (DC_queue_t *queue, unsigned int size);

extern int DC_queue_add (DC_queue_t *queue, obj_t obj, int overwrite);

extern int DC_queue_is_empty (const DC_queue_t *queue);

extern int DC_queue_is_full (const DC_queue_t *queue);

extern int DC_queue_get_length (DC_queue_t *queue);

extern obj_t DC_queue_fetch (DC_queue_t *queue);

extern void DC_queue_clear (DC_queue_t *queue);

extern void DC_queue_destroy (DC_queue_t *queue);

DC_CPP (})
#endif
