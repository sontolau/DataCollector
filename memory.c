#include "memory.h"

int DC_buf_pool_init (DC_buf_pool_t *bp)
{
    memset (bp, '\0', sizeof (DC_buf_pool_t));
    return DC_list_init (&bp->__buflist, NULL);
}

DC_buf_t *DC_buf_alloc (DC_buf_pool_t *bp, unsigned long size)
{
    DC_buf_t    *buf;

    buf = (DC_buf_t*)calloc (1, sizeof (DC_buf_t));
    if (!buf) {
        goto __init_failed;
    }

    buf->buffer = (void*)malloc (size);
    if (!buf->buffer) {
        goto __init_failed;
    }
    buf->size   = size;

    DC_list_add_object (&bp->__buflist, buf);
    bp->szalloc += size;
    
    return buf;

__init_failed:
    if (buf) {
        if (buf->buffer) free (buf->buffer);
        free (buf);
    }
    return NULL;
}

void DC_buf_free (DC_buf_pool_t *bp, DC_buf_t *buf)
{
    if (bp && buf) {
        DC_list_remove_object (&bp->__buflist, buf);
        bp->szalloc -= buf->size;
        free (buf->buffer);
        free (buf);
    }
}

static void __free_buf (void *buf)
{
    DC_buf_t *bf = (DC_buf_t*)buf;

    free (bf->buffer);
}

void DC_buf_pool_destroy (DC_buf_pool_t *bp)
{
    if (bp) {
        DC_list_destroy (&bp->__buflist, __free_buf);
    }
}
