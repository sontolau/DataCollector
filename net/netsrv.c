#include "netsrv.h"
#include "netudp.h"
#include "nettcp.h"

#define __SIG__   (10)
#define __SIG_REQUEST (__SIG__ + 1)
#define __SIG_REPLY   (__SIG__ + 2)
#define __SIG_PING    (__SIG__ + 3)

#define WakeUpQueueProc(serv, sig)   do{ DC_signal_send (serv->sig_handle, sig);}while (0)
#define WakeUpRequestProc(serv) WakeUpQueueProc(serv, __SIG_REQUEST)
#define WakeUpReplyProc(serv)    WakeUpQueueProc(serv, __SIG_REPLY)
#define WakeUpPingProc(serv)     WakeUpQueueProc(serv, __SIG_PING)


static NetProtocolHandler_t  NET_DEFAULT_PROTOCOL_HANDLER[] = {
    [NET_TYPE_TCP] = {
        .createNetIO = TCPCreateNetIO,
        .eventCallBack = TCPEventCallBack,
        .closeNetIO    = TCPCloseNetIO,
    },

    [NET_TYPE_UDP] = {
        .createNetIO = UDPCreateNetIO,
        .eventCallBack = UDPEventCallBack,
        .closeNetIO    = UDPCloseNetIO,
    },
};




static void RequestQueueProc (DC_sig_t sig, void *data)
{
    NetBuffer_t  *req_buf = NULL;
    Server_t *serv = (Server_t*)data;
    int       status = 0;
    NEv_t     ret;

    switch (sig) {
        case __SIG_REQUEST:
        {
            while ((req_buf = GetNetBufferFromQueue (serv, &serv->request_queue))) {
                //TODO: add code here to process request from remote.
            }
        }
            break;
        default:
            break;
    }
}

static void ReplyQueueProc (DC_sig_t sig, void *data)
{
    NetBuffer_t  *req_buf = NULL;
    Server_t  *serv = (Server_t*)data;
    int       ret = 0;

    switch (sig) {
        case __SIG_REPLY:
        {
            while ((req_buf = GetNetBufferFromQueue (serv, &serv->reply_queue))) {
            }
        }
            break;
        default:
            break;
    }
}

static void PingProc (DC_sig_t sig, void *data)
{
    Server_t *srv = (Server_t*)data;

    switch (sig) {
        case __SIG_PING:
        {
            
        }
            break;
        default:
            break;
    }
}


DC_INLINE int PutNetBufferIntoQueue (Server_t *serv, NetBuffer_t *buf, DC_queue_t *queue)
{
    int ret;

    pthread_rwlock_wrlock (&serv->serv_lock);
    ret = DC_queue_push (queue, (qobject_t)buf, 1);
    pthread_rwlock_unlock (&serv->serv_lock);

    return ret;
}
#define PutNetBufferIntoRequestQueue(serv, buf, queue)\
    PutNetBufferIntoQueue(serv, buf, ((DC_queue_t*)&serv->request_queue))

#define PutNetBufferIntoReplyQueue(serv, buf, queue) \
    PutNetBufferIntoReplyQueue(serv, buf, ((DC_queue_t*)&serv->reply_queue))


DC_INLINE NetBuffer_t *GetNetBufferFromQueue (Server_t *serv, DC_queue_t *queue)
{
    NetBuffer_t *nbuf;

    pthread_rwlock_wrlock (&serv->serv_lock);
    nbuf = (NetBuffer_t*)DC_queue_pop (queue);
    pthread_rwlock_unlock (&serv->serv_lock);

    return nbuf;
}
#define GetNetBufferFromRequestQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->request_queue))

#define GetNetBufferFromReplyQueue(serv) \
    GetNetBufferFromQueue(serv, ((DC_queue_t*)&serv->reply_queue)


DC_INLINE void NetIOEventCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    NetIO_t *io   = (NetIO_t*)w->data;
    Server_t *srv = ev_userdata (ev);

    io->io_handler->eventCallBack (srv, io);
}

DC_INLINE int CreateSocketIO (Server_t *serv)
{
    int i;
    
    serv->net_io = (NetIO_t*)calloc (serv->config->num_net_io, sizeof (NetIO_t));
    if (serv->net_io == NULL) {
        return -1;
    }

    for (i=0; i<serv->config->num_net_io &&
        serv->delegate &&
        serv->delegate->getNetInfoWithIndex; i++) {
        serv->delegate->getNetInfoWithIndex (serv, &serv->net_io[i].io_net, i);
        serv->net_io[i].io_handler = &serv->handler_map[serv->net_io[i].io_net.net_type];
        if (serv->net_io[i].io_handler->createNetIO (serv, &serv->net_io[i].io_net, &serv->net_io[i]) < 0) {
            return -1;
        } else {
            ev_io_init (&serv->net_io[i].io_ev, NetIOEventCallBack, serv->net_io[i].io_fd, EV_READ);
            ev_io_start(serv->ev_loop, &serv->net_io[i].io_ev);
            serv->net_io[i].io_ev.data = &(serv->net_io[i]);
        }
    }

    return 0;
}

DC_INLINE void DestroySocketIO (Server_t *serv)
{
    int i;

    for (i=0; i<serv->config->num_net_io &&
                serv->net_io   && 
                serv->delegate && 
                serv->delegate->closeNetIO; i++) {
        ev_io_stop (serv->ev_loop, &serv->net_io[i].io_ev);
        serv->delegate->closeNetIO (serv, &serv->net_io[i]);
    }

    if (serv->net_io) free (serv->net_io);
}


DC_INLINE int InitNet (Server_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;

    if (delegate && delegate->willInitNet && delegate->willInitNet (serv) < 0) {
        return -1;
    }

    if (DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->max_buffers,
                             config->buffer_size + sizeof (NetBuffer_t)) < 0 ||
        DC_buffer_pool_init (&serv->net_peer_pool,
                             config->max_peers,
                             sizeof (NetPeer_t)) < 0) {
        fprintf (stderr, "DC_buffer_pool_init failed.\n");
        return -1;
    }

    if (DC_hash_init (&srv->peer_hash_table, 
                      65536,
                      peer_udp_hash, 
                      peer_compare_func, NULL) < 0) {
        fprintf (stderr, "DC_hash_init failed.\n");
    }

    if (DC_queue_init (&serv->request_queue,
                       config->queue_size,
                       0) < 0 ||
        DC_queue_init (&serv->reply_queue,
                       config->queue_size,
                       0) < 0) {
        fprintf (stderr, "DC_queue_init failed.\n");
        return -1;
    }

    if (!(serv->sig_handle = DC_signal_alloc ())) {
        fprintf (stderr, "DC_signal_alloc failed.\n");
        return -1;
    }

    if (pthread_rwlock_init (&serv->serv_lock, NULL) < 0) {
        fprintf (stderr, "pthread_rwlock_init failed.\n");
        return -1;
    }

    if (DC_signal_wait_asyn (serv->sig_handle, 1000, RequestQueueProc, serv) < 0 ||
        DC_signal_wait_asyn (serv->sig_handle, 1000, ReplyQueueProc, serv) < 0 ||
        DC_signal_wait_asyn (serv->sig_handle, 1000, PingProc, serv) < 0) {
        fprintf (stderr, "DC_signal_wait_asyn failed.\n");
        return -1;
    }

    serv->handler_map = DEFAULT_PROTOCOL_HANDLER;

    if (serv->delegate && serv->delegate->didInitNet) {
         serv->delegate->didInitNet (serv);
    }
    return 0;
}

DC_INLINE void TimerCallBack (struct ev_loop *ev, ev_timer *w, int revents)
{
    Server_t *srv = (Server_t*)ev_userdata (ev);

    srv->timer++;
    if (srv->delegate && srv->delegate->timerIsUp) {
        srv->delegate->timerIsUp (srv, srv->timer);
    }

    if (srv->exit_flag) {
        ev_break (ev, EVBREAK_ALL);
    }
}

DC_INLINE void SignalCallBack (struct ev_loop *ev, ev_signal *w, int revents)
{
    ev_break (ev, EVBREAK_ALL);
}

DC_INLINE void RunNet (Server_t *serv)
{
    ev_timer  timer;
    ev_signal sig_handler;

    if (CreateSocketIO (serv) < 0) {
        fprintf (stderr, "CreateSocketIO failed due to %s\n", strerror (errno));
        return;
    }

    timer.data = serv;
    ev_timer_init (&timer, TimerCallBack, 1, 1);
    ev_timer_start (serv->ev_loop, &timer);

    sig_handler.data = serv;
    ev_signal_init (&sig_handler, SignalCallBack, SIGINT);
    ev_signal_start(serv->ev_loop, &sig_handler);

    if (serv->delegate && serv->delegate->willRunServer) {
        serv->delegate->willRunServer (serv);
    }

    
    ev_loop (serv->ev_loop, 0);

    ev_timer_stop (serv->ev_loop, &timer);
    ev_signal_stop (serv->ev_loop &sig_handler);

    DestroySocketIO (serv);
}

DC_INLINE void ReleaseNet (Server_t *serv)
{
    if (serv->delegate && serv->delegate->willStopNet) {
        serv->delegate->willStopNet (serv);
    }

    ev_loop_destroy (serv->ev_loop);
    DC_signal_free (serv->sig_handle);
    DC_queue_destroy (&serv->request_queue);
    DC_queue_destroy (&serv->reply_queue);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_buffer_pool_destroy (&serv->net_peer_pool);
    DC_hash_destroy (&serv->peer_hash_table);
    pthread_rwlock_destroy (&serv->serv_lock);
}

int ServerRun (Server_t *serv, NetConfig_t *config, NetDelegate_t *delegate)
{
    int ret;
    
    memset (serv, '\0', sizeof (Server_t));
    serv->config   = config;
    serv->delegate = delegate;

    if (!(ret = InitNet (serv))) {
        RunNet (serv);
    }

    ReleaseNet (serv);

    return ret;
}

void ServerStop (Server_t *serv)
{
    serv->exit_flag = 1;
    kill (0, SIGINT);
}
