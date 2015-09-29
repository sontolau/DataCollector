#ifndef _BUFFER_H
#define _BUFFER_H

#include "libdc.h"
#include "queue.h"

typedef struct _Buffer {    
    unsigned int size;
    unsigned int length;
    unsigned char buffer[0];
} DC_buffer_t;


typedef struct _BufferPool {
    unsigned int unit_size;
    int          num_total;
    int          num_left;
    DC_buffer_t*    PRI(buf_ptr);
    DC_queue_t      PRI (buf_queue);
} DC_buffer_pool_t;

#define DC_buffer_from(addr)   CONTAINER_OF(addr, DC_buffer_t, buffer)
                               /*((DC_buffer_t*)((long long)addr-\
                               ((long long)((DC_buffer_t*)0)->data)))*/

extern int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size);

extern DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool, unsigned int size);

extern void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf);

extern void DC_buffer_pool_destroy (DC_buffer_pool_t *pool);

#endif
