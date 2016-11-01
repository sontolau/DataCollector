#include "queue.h"
#include "mm.h"


FASTCALL unsigned long long __get_queue_offset(OBJ_t *curpos, OBJ_t *startpos, int size) {
    uint32_t offset = (unsigned char *) curpos - (unsigned char *) startpos;

    return ((int) (offset / sizeof(OBJ_t))) % size;
}

FASTCALL  OBJ_t *__recalc_address(DC_queue_t *queue, OBJ_t *ptr) {
    return (queue->buf_ptr + __get_queue_offset(++ptr,
                                                queue->buf_ptr,
                                                queue->size));
}

err_t DC_queue_init(DC_queue_t *queue,
                    uint32_t size,
                    OBJ_t zero) {
    int i = 0;


    queue->buf_ptr = (OBJ_t *) DC_calloc (size, sizeof(OBJ_t));
    if (!queue->buf_ptr) {
        return E_SYSTEM;
    }

    queue->init_value = zero;
    queue->size = size;
    for (i = 0; i < size; i++) {
        queue->buf_ptr[i] = zero;
    }

    queue->tail_ptr = queue->head_ptr = queue->buf_ptr;

    if (ISERR (DC_thread_mutex_init(&queue->t_mutex))) {
        DC_free (queue->buf_ptr);
        return E_ERROR;
    }


    if (ISERR (DC_thread_cond_init(&queue->t_cond))) {
        DC_free (queue->buf_ptr);
        DC_thread_mutex_destroy(&queue->t_mutex);
        return E_ERROR;
    }


    return E_OK;
}

#define QUEUE_EMPTY(queue) ((queue->tail_ptr == queue->head_ptr &&\
           *queue->head_ptr== queue->init_value)?TRUE:FALSE)

#define QUEUE_FULL(queue) ((queue->tail_ptr == queue->head_ptr &&\
             *queue->head_ptr != queue->init_value)?TRUE:FALSE)

bool_t DC_queue_is_empty(DC_queue_t *queue) {
    bool_t s;
    int err = E_OK;

    DC_syn_run (queue->t_mutex, err, {
        s = QUEUE_EMPTY(queue);
    });

    return err == E_OK ? s : FALSE;
}


bool_t DC_queue_is_full(DC_queue_t *queue) {
    bool_t s;
    int err = E_OK;

    DC_syn_run (queue->t_mutex, err, {
        s = QUEUE_FULL(queue);
    });

    return err == E_OK ? s : FALSE;
}


err_t DC_queue_add(DC_queue_t *queue,
                   OBJ_t obj,
                   bool_t block,
                   uint32_t timeout) {
    if (ISERR (DC_thread_mutex_lock(&queue->t_mutex))) {
        return E_ERROR;
    }

    if (QUEUE_FULL (queue)) {
        if (!block) {
            DC_thread_mutex_unlock(&queue->t_mutex);
            DC_set_errcode (E_FULL);
            return E_ERROR;
        } else {
            if (ISERR(DC_thread_cond_wait(&queue->t_cond,
                                          &queue->t_mutex,
                                          timeout))) {
                DC_thread_mutex_unlock(&queue->t_mutex);
                return E_ERROR;
            }
        }
    }


    (*queue->head_ptr) = obj;
    queue->head_ptr = __recalc_address(queue, queue->head_ptr);
    DC_thread_cond_signal(&queue->t_cond, TRUE);

    DC_thread_mutex_unlock(&queue->t_mutex);

    return E_OK;
}

int DC_queue_get_length(DC_queue_t *queue) {
    int len = 0;
    err_t err = E_OK;

    if (DC_queue_is_empty(queue)) return 0;
    if (DC_queue_is_full(queue)) return queue->size;

    DC_syn_run(queue->t_mutex, err, {
        if (queue->head_ptr > queue->tail_ptr) {
            len = queue->head_ptr - queue->tail_ptr;
            DC_syn_break(queue->t_mutex);
        }

        len = queue->size - (queue->tail_ptr - queue->head_ptr);
    });

    return err == E_OK ? len : E_ERROR;
}

OBJ_t DC_queue_fetch(DC_queue_t *queue,
                     bool_t block,
                     uint32_t timeout) {
    OBJ_t obj;

    if (ISERR (DC_thread_mutex_lock(&queue->t_mutex))) {
        return queue->init_value;
    }

    if (QUEUE_EMPTY (queue)) {
        if (!block) {
            DC_thread_mutex_unlock(&queue->t_mutex);
            DC_set_errcode (E_EMPTY);
            return queue->init_value;
        } else {
            if (ISERR(DC_thread_cond_wait(&queue->t_cond,
                                          &queue->t_mutex,
                                          timeout))) {
                DC_thread_mutex_unlock(&queue->t_mutex);
                return queue->init_value;
            }
        }
    }


    obj = *queue->tail_ptr;
    (*queue->tail_ptr) = queue->init_value;
    queue->tail_ptr = __recalc_address(queue, queue->tail_ptr);
    DC_thread_cond_signal(&queue->t_cond, TRUE);
    DC_thread_mutex_unlock(&queue->t_mutex);

    return obj;
}

void DC_queue_clear(DC_queue_t *queue) {
    while (DC_queue_fetch(queue, FALSE, 0) != queue->init_value);
}

void DC_queue_destroy(DC_queue_t *queue) {
    DC_queue_clear(queue);

    if (queue->buf_ptr) {
        DC_free (queue->buf_ptr);
    }

    DC_thread_mutex_destroy(&queue->t_mutex);
    DC_thread_cond_destroy(&queue->t_cond);
}
