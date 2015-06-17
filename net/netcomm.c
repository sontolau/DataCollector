
#define BufferAlloc(srv, bufptr, type, pool, sz) \
    do {\
        DC_buffer_t *__buf = NULL;\
        __buf = DC_buffer_pool_alloc (&pool, sz);\
        bufptr = (__buf?((type*)__buf->data):NULL);\
    } while (0)

#define BufferFree(srv, buf, pool) \
    do {\
        DC_buffer_t *__buf = (DC_buffer_t*)DC_buffer_from (buf);\
        DC_buffer_pool_free (&pool, __buf);\
    } while (0)

NetBuffer_t *NetAllocBuffer (Net_t *srv)
{
    NetBuffer_t *buf = NULL;

    if (srv->config->num_sockbufs) {
        NetLockContext (srv);
        BufferAlloc (srv, buf, NetBuffer_t, srv->net_buffer_pool, srv->config->max_sockbuf_size);
        if (buf) {
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                                RES_BUFFER, 
                                DC_buffer_pool_get_usage (&srv->net_buffer_pool));
            }
        }
        NetUnlockContext (srv);
    } else {
        buf = (NetBuffer_t*)calloc (1, 
              sizeof (NetBuffer_t) + srv->config->max_sockbuf_size);
    }

    if (buf) {
        buf->io            = NULL;
        buf->size   = srv->config->max_sockbuf_size;
        buf->data_length = 0;
        buf->offset = 0;
        //buf->buffer = (unsigned char*)buf+sizeof (NetBuffer_t);
        memset (buf->buffer, '\0', buf->size);
    }
    Dlog ("[libdc] allocate system buffer: %p\n", buf);

    return buf;
}

void NetFreeBuffer (Net_t *srv, NetBuffer_t *buf)
{
    if (srv->config->num_sockbufs) {
        NetLockContext (srv);
        BufferFree (srv, buf, srv->net_buffer_pool);
        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                          RES_BUFFER, 
                          DC_buffer_pool_get_usage (&srv->net_buffer_pool));
        }
        NetUnlockContext (srv);
    } else {
        free (buf);
    }

    Dlog ("[libdc] free system buffer: %p\n", buf);
}

NetIO_t *NetAllocIO (Net_t *srv)
{
    NetIO_t *iobuf = NULL;

    if (srv->config->num_sock_conns) {
        NetLockContext (srv);
        BufferAlloc (srv, iobuf, NetIO_t, srv->net_io_pool, sizeof (NetIO_t));
        if (iobuf) {
            memset (iobuf, '\0', sizeof (NetIO_t));
            if (srv->delegate->resourceUsage) {
                srv->delegate->resourceUsage (srv, 
                                          RES_IO, 
                                          DC_buffer_pool_get_usage (&srv->net_io_pool));
            }
        }
        NetUnlockContext (srv);
    } else {
       iobuf = (NetIO_t*)calloc (1, sizeof (NetIO_t));
    }
  
    Dlog ("[libdc] allocate IO : %p\n", iobuf);
    return iobuf;
}

void NetFreeIO (Net_t *srv, NetIO_t *io)
{
    if (srv->config->num_sock_conns) {
        NetLockContext (srv);
        BufferFree (srv, io, srv->net_io_pool);
        if (srv->delegate->resourceUsage) {
            srv->delegate->resourceUsage (srv, 
                                   RES_BUFFER, 
                                   DC_buffer_pool_get_usage (&srv->net_io_pool));
        }
        NetUnlockContext (srv);
    } else {
        free (io);
    }

    Dlog ("[libdc] free IO: %p\n", io);
}


int InitQueueManager (NetQueueManager_t *mgr,
                      int queue_size,
                      int num_threads,
                      void (*handler)(NetQueueManager_t*, qobject_t, void*),
                      void *data)
{
    if (DC_mutex_init (&mgr->lock, 0, NULL, NULL) < 0) {
        Dlog ("[libdc] ERROR: initialized mutex failed at line:%d.\n", __LINE__);
        return -1;
    }

    if (DC_thread_pool_manager_init (&mgr->task_pool, num_threads, NULL) < 0) {
        Dlog ("[libdc] ERROR: initialized thread pool manager failed at line:%d.\n", __LINE__);
L0:
        DC_mutex_destroy (&mgr->lock);
        return -1;
    }

    if (DC_thread_init (&mgr->manager_thread, NULL, NULL) < 0) {
        Dlog ("[libdc] ERROR: allocate threads failed at line: %d.\n", __LINE__);
L1:
        DC_thread_pool_manager_destroy (&mgr->task_pool);
        goto L0;

    }

    if (DC_queue_init (&mgr->queue, queue_size, 0) < 0) {
        DC_thread_destroy (&mgr->manager_thread);
        goto L1;
    }

    mgr->handler = handler;
    mgr->data    = data;
    return 0;
}

static qobject_t FetchObjectFromQueue (NetQueueManager_t *q)
{
    qobject_t obj = 0;
    int empty = 0;

    DC_mutex_lock (&q->lock, 0, 1);
    empty = DC_queue_is_empty (&q->queue);
    if (!empty) {
        obj = DC_queue_fetch (&q->queue);
    }
    DC_mutex_unlock (&q->lock);

    return obj;
}
static void __task_core (DC_task_t *task, void *data)
{
    NetQueueManager_t *queue = (NetQueueManager_t*)data;
    qobject_t obj;

    while ((obj = FetchObjectFromQueue (queue))) {
        if (queue->handler) {
            queue->handler (queue, obj, queue->data);
        }
    }
}

static void __queue_manager_func (DC_thread_t *thread, void *data)
{
    NetQueueManager_t *queue = (NetQueueManager_t*)data;
    int       empty = 0;

    do {
        DC_mutex_lock (&queue->lock, 0, 1);
        empty = DC_queue_is_empty (&queue->queue);
        DC_mutex_unlock (&queue->lock);
        if (!empty) {
            DC_thread_pool_manager_run_task (&queue->task_pool,
                                             __task_core,
                                             NULL,
                                             NULL,
                                             queue,
                                             1);
        }
    } while (!empty);
}

int RunTaskInQueue (NetQueueManager_t *mgr,
                    qobject_t task)
{
    DC_mutex_lock (&mgr->lock, 0, 1);
    DC_queue_add (&mgr->queue, task, 1);
    if (DC_thread_get_status (((DC_thread_t*)&mgr->manager_thread)) != THREAD_STAT_RUNNING) {
        DC_thread_run (&mgr->manager_thread, __queue_manager_func, mgr);
    }
    DC_mutex_unlock (&mgr->lock);

    return 0;
}

void DestroyQueueManager (NetQueueManager_t *mgr)
{
    if (mgr) {
        DC_mutex_destroy (&mgr->lock);
        DC_thread_pool_manager_destroy (&mgr->task_pool);
        DC_thread_destroy (&mgr->manager_thread);
        DC_queue_destroy (&mgr->queue);
    }
}


NetBuffer_t *NetIOBufferFetch (NetIO_t *io)
{
    register DC_link_t *linkptr = NULL;
    NetBuffer_t        *buf = NULL;

    NetIOLock (io);
    linkptr = io->PRI(buffer_link).next;
    if (linkptr == &io->PRI (buffer_link)) {
        NetIOUnlock (io);
        return NULL;
    } 

    buf = DC_link_container_of(linkptr, NetBuffer_t, PRI(io_link));
    DC_link_remove (linkptr);
    NetIOUnlock (io);

    return buf;
}

void NetIODestroy (NetIO_t *io)
{
    DC_mutex_destroy (&io->io_lock);
}

