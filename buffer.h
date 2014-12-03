#ifndef _BUFFER_H
#define _BUFFER_H

#include "link.h"

typedef struct _Buffer {
    unsigned int size;
    unsigned int length;
    DC_link_t    __link;
    unsigned char data[0];
} DC_buffer_t;


typedef struct _BufferPool {
    int          numbufs;
    unsigned int bufsize;

    DC_buffer_t     *__netbuf;
    DC_link_t    __free_link;
    DC_link_t    __engaged_link;
} DC_buffer_pool_t;

#define DC_buffer_from(addr)   ((DC_buffer_t*)((long long)addr-((long long)((DC_buffer_t*)0)->data)))

extern int DC_buffer_pool_init (DC_buffer_pool_t *pool, int num, unsigned int size);

extern DC_buffer_t *DC_buffer_pool_alloc (DC_buffer_pool_t *pool);

extern void     DC_buffer_pool_free (DC_buffer_pool_t *pool, DC_buffer_t *buf);

extern void DC_buffer_pool_destroy (DC_buffer_pool_t *pool);
#endif
