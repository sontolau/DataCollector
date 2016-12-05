#ifndef _DC_QUEUE_H
#define _DC_QUEUE_H

#include "libdc.h"
#include "thread.h"


CPP (extern "C" {)

typedef struct _DC_queue {
    uint32_t size;
    OBJ_t init_value;
    OBJ_t *buf_ptr;
    OBJ_t *tail_ptr;
    OBJ_t *head_ptr;
    DC_thread_mutex_t t_mutex;
    DC_thread_cond_t t_cond;
} DC_queue_t;

extern err_t DC_queue_init(__in__ DC_queue_t *queue,
                           uint32_t size,
                           OBJ_t init_value);

extern err_t DC_queue_add(__in__ DC_queue_t *queue,
                          __in__ OBJ_t obj,
                          uint8_t block,
                          uint32_t timeout);

extern bool_t DC_queue_is_empty(__in__ DC_queue_t *queue);

extern bool_t DC_queue_is_full(__in__ DC_queue_t *queue);

extern int DC_queue_get_length(__in__ DC_queue_t *queue);

extern OBJ_t DC_queue_fetch(__in__ DC_queue_t *queue,
                            uint8_t block,
                            uint32_t timeout);

extern void DC_queue_clear(__in__ DC_queue_t *queue);

extern void DC_queue_destroy(__in__ DC_queue_t *queue);

CPP (})
#endif
