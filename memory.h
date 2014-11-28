#ifndef _DC_MEMORY_H
#define _DC_MEMORY_H

#include "libdc.h"
#include "list.h"

DC_CPP(extern "C" {)


typedef struct _DC_buf {
    unsigned long size;
    void          *buffer;
}DC_buf_t;


typedef struct DC_buf_pool {
    unsigned long szalloc;
    DC_list_t   __buflist;
}DC_buf_pool_t;

extern int DC_buf_pool_init (DC_buf_pool_t*);

extern DC_buf_t  *DC_buf_alloc (DC_buf_pool_t*, unsigned long size);

extern void      DC_buf_free (DC_buf_pool_t*, DC_buf_t*);

extern void      DC_buf_pool_destroy (DC_buf_pool_t*);

DC_CPP(})

#endif
