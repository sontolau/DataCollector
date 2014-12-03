#ifndef _BUFFER_H
#define _BUFFER_H

#include "link.h"

typedef struct _Buffer {
    unsigned int size;
    unsigned int length;
    DC_link_t    __link;
    unsigned char data[0];
} Buffer_t;


typedef struct _BufferPool {
    int          numbufs;
    unsigned int bufsize;

    Buffer_t     *__netbuf;
    DC_link_t    __free_link;
    DC_link_t    __engaged_link;
} BufferPool_t;

#define ADDR2BUFF(addr)   ((Buffer_t*)((long long)addr-((long long)((Buffer_t*)0)->data)))

extern int BufferPoolInit (BufferPool_t *pool, int num, unsigned int size);

extern Buffer_t *BufferAlloc (BufferPool_t *pool);

extern void     BufferFree (BufferPool_t *pool, Buffer_t *buf);

extern void BufferPoolDestroy (BufferPool_t *pool);
#endif
