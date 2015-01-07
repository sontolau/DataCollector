#include "queue.h"

DC_INLINE int __get_queue_offset (qobject_t *curpos, qobject_t *startpos, int size)
{
    unsigned int offset = (unsigned char*)curpos - (unsigned char*)startpos;

    return ((int)(offset / sizeof (qobject_t)))%size;
}

int DC_queue_init (DC_queue_t *queue, unsigned int queue_size, qobject_t zero)
{
    int i = 0;

    queue->__queue_buffer = (qobject_t*)calloc (queue_size, sizeof (qobject_t));
    if (!queue->__queue_buffer) {
        return -1;
    }

    queue->queue_size       = queue_size;
    queue->length           = 0;
    queue->zero_type        = zero;

    for (i=0; i<queue_size; i++) {
        queue->__queue_buffer[i] = zero;
    }

    queue->__tail_ptr = queue->__head_ptr = queue->__queue_buffer;
    return 0;
}


int DC_queue_is_empty (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr== queue->zero_type;
}


int DC_queue_is_full (const DC_queue_t *queue)
{
    return queue->__tail_ptr == queue->__head_ptr &&\
           *queue->__head_ptr != queue->zero_type;
}


int DC_queue_push (DC_queue_t *queue, qobject_t obj, int overwrite)
{
    if (!overwrite && DC_queue_is_full (queue)) {
        return -1;
    }

    *queue->__head_ptr = obj;
    queue->__head_ptr  = (qobject_t*)(queue->__queue_buffer + \
                         __get_queue_offset (++queue->__head_ptr, \
                                             queue->__queue_buffer, \
                                             queue->queue_size));
    queue->length = (++queue->length>queue->queue_size?queue->queue_size:queue->length);
    return 0;
}

qobject_t DC_queue_pop (DC_queue_t *queue)
{
    qobject_t   obj;

    if (DC_queue_is_empty (queue)) {
        return queue->zero_type;
    }

    obj = *queue->__tail_ptr;
    *queue->__tail_ptr = queue->zero_type;
    queue->__tail_ptr  = (qobject_t*)(queue->__queue_buffer + \
                         __get_queue_offset (++queue->__tail_ptr, \
                                             queue->__queue_buffer, \
                                             queue->queue_size));
    queue->length--;
    return obj;
}

void DC_queue_destroy (DC_queue_t *queue)
{
    if (queue->__queue_buffer) {
        free (queue->__queue_buffer);
    }
}
