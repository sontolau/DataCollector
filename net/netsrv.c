#include "netsrv.h"

#define __SIG__   (10)
#define __SIG_REQUEST (__SIG__ + 1)
#define __SIG_REPLY   (__SIG__ + 2)
#define __SIG_PING    (__SIG__ + 3)

#define WakeUpQueueProc(serv, sig)   do{ DC_signal_send (serv->sig_handle, sig);}while (0)
#define WakeUpRequestProc(serv) WakeUpQueueProc(serv, __SIG_REQUEST)
#define WakeUpReplyProc(serv)    WakeUpQueueProc(serv, __SIG_REPLY)
#define WakeUpPingProc(serv)     WakeUpQueueProc(serv, __SIG_PING)

DC_INLINE long __recv (int sock, unsigned char *buf, unsigned int size)
{
    long szread;

    while (1) {
        szread = recv (sock, buf, size, 0);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}

DC_INLINE long __send (int sock, unsigned char *buf, unsigned int size)
{
    long szwrite;

    while (1) {
        szwrite = send (sock, buf, size, 0);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}

DC_INLINE long __recvfrom (int sock, unsigned char *buf, unsigned int size,
                           struct sockaddr *addr, socklen_t *sklen)
{
    long szread;

    while (1) {
        szread = recvfrom (sock, buf, size, 0, addr, sklen);
        if (szread < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szread;
}


DC_INLINE long __sendto (int sock, unsigned char *buf, unsigned int size,
                         struct sockaddr *addr, socklen_t sklen)
{
    long szwrite;

    while (1) {
        szwrite = sendto (sock, buf, size, 0, addr, sklen);
        if (szwrite < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }

    return szwrite;
}

DC_INLINE long SendNetBuffer (NetBuffer_t *buf)
{
    double szsend = 0;
    struct sockaddr_in caddr;


    if (buf->net_io->net_info.net_type == NET_TYPE_TCP) {
        szsend = __send (buf->net_io->sock_fd, 
                         buf->buffer,
                         SZNETHEADER + buf->net_buf.data_len);
    } else if (buf->net_io->net_info.net_type == NET_TYPE_UDP){
        caddr.sin_family = AF_INET;
        caddr.sin_port   = htons (buf->net_io->net_info.net_port);
        caddr.sin_addr.s_addr = buf->net_io->net_info.net_addr;

        szsend = __sendto (buf->net_io->sock_fd, 
                           buf->buffer, 
                           SZNETHEADER + buf->net_buf.data_len,
                           (struct sockaddr*)&caddr, sizeof (caddr));
    }
    
    if (szsend > 0) {
        //TODO
    }

    return szsend;
}

DC_INLINE long RecvNetBuffer (NetBuffer_t *buf)
{
    socklen_t sklen = sizeof (struct sockaddr);
    double     szread= 0;
    struct sockaddr_in caddr;

    if (buf->net_io->net_info.net_type == NET_TYPE_TCP) {
        szread  = __recv (buf->net_io->sock_fd, buf->buffer, buf->buffer_size);
    } else if (buf->net_io->net_info.net_type == NET_TYPE_UDP){
        szread  = __recvfrom (buf->net_io->sock_fd, buf->buffer, buf->buffer_size,
                              (struct sockaddr*)&caddr, &sklen);
        if (szread > 0) {
            buf->net_io->net_info.net_port = ntohs (caddr.sin_port);
            buf->net_io->net_info.net_addr = caddr.sin_addr.s_addr;
        }
    }

    if (szread > 0) {
        //TODO: validate data buffer.

    }


   return szread;
}

DC_INLINE void SetNonblockFD (int fd)
{
    int flag = 0;

    if ((flag = fcntl (fd, F_GETFL)) < 0) {
        return;
    }

    flag |= O_NONBLOCK;
   
    fcntl (fd, F_SETFL, &flag);
}

DC_INLINE void FreeNetBuffer (Server_t*, NetBuffer_t*);
DC_INLINE int PutNetBufferIntoQueue (Server_t*, NetBuffer_t*, DC_queue_t*);
DC_INLINE NetBuffer_t *GetNetBufferFromQueue (Server_t*, DC_queue_t*);
DC_INLINE void ClosePeer (Server_t*, NetIO_t*);
//DC_INLINE void SetPeerFlag (Server_t*, NetIO_t*, int);
//#define ClosePeer(srv, io)  do {SetPeerFlag(srv, io, PEER_FLAG_CLOSE);} while (0)



DC_INLINE void SetNetIOStatus (Server_t *srv, NetIO_t *io, int status)
{
    DC_link_t *link = NULL;

    switch (status) {
        case PEER_FLAG_WAITING:
        {
            link = GetActiveLink (srv);
        }
            break;
        case PEER_FLAG_CONN:
        {
            link = GetInactiveLink (srv);
        }
        default:
            break;
    }

    if (link) {
        pthread_rwlock_wrlock (&srv->serv_lock);
        if (srv->delegate && srv->delegate->willChangePeerStatus) {
            srv->delegate->willChangePeerStatus (srv, io, NetIOGetStatus (io), status);
        }

        if (NetIOGetStatus(io)) {
            DC_link_remove (&io->link);
        }
        DC_link_add (link, &io->link);
        NetIOSetStatus (io, status);
        pthread_rwlock_unlock (&srv->serv_lock);
    }
}

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
                if (serv->delegate && serv->delegate->processRequest) {
                    ret = serv->delegate->processRequest (serv, req_buf, &status);
                    if (status) SetNetIOStatus (serv, req_buf->net_io, status);
                    switch (ret) {
                        case NEV_CLOSE:
                            ClosePeer (serv, req_buf->net_io);
                            break;
                        case NEV_REPLY:
                            PutNetBufferIntoQueue (serv, req_buf, &serv->reply_queue);
                            WakeUpReplyProc (serv);
                            return;
                        default:
                            break;
                    }
                }

                FreeNetBuffer (serv, req_buf);
/*
*/
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
                ret = SendNetBuffer (req_buf);
                if (serv->delegate && serv->delegate->didSendReply) {
                    serv->delegate->didSendReply (serv, req_buf, ret>0?S_OK:S_ERROR);
                }

                
                if (ret <= 0) {
                    ClosePeer (serv, req_buf->net_io);
                }
                FreeNetBuffer (serv, req_buf);
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

DC_INLINE void CleanAllPeersOnIO (Server_t *srv, NetIO_t *io)
{
}



DC_INLINE int PutNetBufferIntoQueue (Server_t *serv, NetBuffer_t *buf, DC_queue_t *queue)
{
    int ret;

    pthread_rwlock_wrlock (&serv->serv_lock);
    ret = DC_queue_push (queue, (qobject_t)buf, 1);
    pthread_rwlock_unlock (&serv->serv_lock);

    return ret;
}


DC_INLINE NetBuffer_t *GetNetBufferFromQueue (Server_t *serv, DC_queue_t *queue)
{
    NetBuffer_t *nbuf;

    pthread_rwlock_wrlock (&serv->serv_lock);
    nbuf = (NetBuffer_t*)DC_queue_pop (queue);
    pthread_rwlock_unlock (&serv->serv_lock);

    return nbuf;
}


DC_INLINE int CreateSocketIO (Server_t *serv, int flag)
{
    int i, xflag = 1;
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;
    NetIO_t       *io       = NULL;
    NetInfo_t   *net_info   = NULL;
    struct sockaddr_in addr;
    int  szbuf = config->buffer_size;

    if (flag) {
         if (!(delegate && delegate->getNetInfoWithIndex)) {
             return -1;
         }
         serv->net_io = (NetIO_t*)calloc (serv->config->num_sock_io,
                                          sizeof (NetIO_t));
         if (!serv->net_io) {
             fprintf (stderr, "Can't allocate memory for socket IO.\n");
             return -1;
         }
    }

    for (i = 0; i < config->num_sock_io; i++) {
        io  = &serv->net_io[i];
        net_info = &io->net_info;

        if (!flag) {
            close (io->sock_fd);
            continue;
        }

        delegate->getNetInfoWithIndex (serv, net_info, i);
        if (net_info->net_type == NET_TYPE_TCP) {
            io->sock_fd = socket (AF_INET, SOCK_STREAM, 0);
        } else if (net_info->net_type == NET_TYPE_UDP) {
            io->sock_fd = socket (AF_INET, SOCK_DGRAM, 0);
        }

        SetNonblockFD (io->sock_fd);
        setsockopt (io->sock_fd, SOL_SOCKET, SO_REUSEADDR, &xflag, sizeof (int));
        setsockopt (io->sock_fd, SOL_SOCKET, SO_SNDBUF, &szbuf, sizeof (int));
        setsockopt (io->sock_fd, SOL_SOCKET, SO_RCVBUF, &szbuf, sizeof (int));

        addr.sin_family = AF_INET;
        addr.sin_port   = htons (net_info->net_port);
        addr.sin_addr.s_addr = net_info->net_addr;

        if (bind (io->sock_fd, (struct sockaddr*)&addr, sizeof (addr)) < 0) {
            close (io->sock_fd);
            return -1;
        }

        if (net_info->net_type == NET_TYPE_TCP) {
            listen (io->sock_fd, 1000);
        }
    }
    if (!flag) {
        free (serv->net_io);
    }

    return 0;
}

DC_INLINE int ValidateRequestData (NetBuffer_t *buf)
{
    return S_OK;
}


DC_INLINE void UpdatePeer (Server_t *serv, Remote_t *peer)
{
    //pthread_mutex
}

DC_INLINE void ProcessRemoteRequest (Server_t *serv, NetBuffer_t *buf)
{
    switch (buf->net_buf.command) {
        case NET_COM_CONN:
        {
            
        }
            break;
        case NET_COM_TRANSACTION:
        {
            UpdatePeer (serv, buf->net_io);
            PutNetBufferIntoQueue (serv, buf, &serv->request_queue);
            WakeUpRequestProc (serv);
        }
            break;
        case NET_COM_PING:
        {
            if (NET_REPLY (buf->net_buf)) {
                UpdatePeer (serv, buf->net_io);
            } else {
                buf->net_buf.flag = 0;
                if (SendNetBuffer (buf) <= 0) {
                    ClosePeer (serv, buf->net_io);
                }
            }

            FreeNetBuffer (serv, buf);
        }
            break;
        case NET_COM_DISCONN:
        {

        }
            break;
        default:
            break;
    }
}

DC_INLINE void DisconnectPeer (Server_t *serv, NetIO_t *io)
{
    register DC_link_t *linkptr,*linktmp;
    
    
    pthread_rwlock_wrlock (&serv->serv_lock);
    while (linkptr) {
        linkptr = linkptr->next;
    }
    pthread_rwlock_unlock (&serv->serv_lock);
}

DC_INLINE void ReadEventCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    NetBuffer_t *buf = NULL;
    Server_t    *serv = (Server_t*)ev_userdata(ev);
    double        ret = 0;
    NetIO_t     *cio= (NetIO_t*)w->data;

    if ((buf = AllocNetBuffer (serv))) {
        buf->net_io = cio;

        ret = RecvNetBuffer (buf);
        if (ret > 0 && ValidateRequestData (buf) == S_OK) {
            ProcessRemoteRequest (serv, buf);
        } else if (ret <= 0) {
            ClosePeer (serv, cio);
            FreeNetBuffer (serv, buf);
        }
    }
}

DC_INLINE int TCPAcceptClient (Server_t *serv, ev_io *w)
{
    NetIO_t *io     = NULL;
    NetIO_t *servio = (NetIO_t*)w->data;

    if (!(io = AllocNetIO (serv))) {
        return -1;
    } else {
        io->io.data =io;
        io->net_info.net_type = servio->net_info.net_type;
        io->trans_time   = 0;
        io->link.next    = NULL;
        io->flag         = 0;
    }

    do {
        io->sock_fd = accept (servio->sock_fd, NULL, NULL);
        if (io->sock_fd > 0 || (io->sock_fd < 0 && errno != EINTR)) {
            break;
        }
    } while (1);
    if (io->sock_fd < 0) {
        FreeNetIO (serv, io);
        return -1;
    }

    ev_io_init (&io->io, ReadEventCallBack, io->sock_fd, EV_READ);
    ev_io_start(serv->ev_loop, &io->io);

    pthread_rwlock_wrlock (&serv->serv_lock);
    io->expire_time = serv->timer;
    DC_link_add (NetGetInactiveLink (serv), &io->link);
    pthread_rwlock_unlock (&serv->serv_lock);
    return 0;
}

DC_INLINE void ClosePeer (Server_t *serv, NetIO_t *io)
{
    pthread_rwlock_wrlock (&serv->serv_lock);
    if (serv->delegate && serv->delegate->willClosePeer) {
        serv->delegate->willClosePeer (serv, io);
    }
    ev_io_stop (serv->ev_loop, &io->io);
    close (io->sock_fd);
    if (io->link.next || io->link.prev) {
        DC_link_remove (&io->link);
    }

    FreeNetIO (serv, io);
    pthread_rwlock_unlock (&serv->serv_lock);
}

DC_INLINE void ServerEventCallBack (struct ev_loop *ev, ev_io *w, int revents)
{
    Server_t *serv = (Server_t*)ev_userdata (ev);
    NetIO_t  *netio= (NetIO_t*)w->data;

    switch (NetIOType (netio)) {
        case NET_TYPE_TCP:
        {
            if (TCPAcceptClient (serv, servio) < 0) {
                return;
            }
        }
            break;
        case NET_TYPE_UDP:
        {
            ReadEventCallBack (ev, w, revents);
        }
            break;
        default:
            break;
    }
}

DC_INLINE void SetEventIO (Server_t *serv)
{
    int i;
    
    serv->ev_loop = ev_loop_new (0);
    ev_set_userdata (serv->ev_loop, serv);

    for (i=0 ;i<serv->config->num_sock_io; i++) {
        ev_io_init (&serv->net_io[i].io, ServerEventCallBack, serv->net_io[i].sock_fd, EV_READ);

        ev_io_start (serv->ev_loop, &serv->net_io[i].io);
        serv->net_io[i].io.data = &serv->net_io[i];
    }
}

DC_INLINE int InitServer (Server_t *serv)
{
    NetConfig_t *config     = serv->config;
    NetDelegate_t *delegate = serv->delegate;

    if (delegate && delegate->willInitServer && delegate->willInitServer (serv) < 0) {
        return -1;
    }

    serv->net_io = (NetIO_t*)calloc (serv->config->num_sock_io,
                                         sizeof (NetIO_t));
    if (!serv->net_io) {
        fprintf (stderr, "Can't allocate memory for socket IO.\n");
        return -1;
    }

    if (DC_buffer_pool_init (&serv->net_buffer_pool, 
                             config->max_buffers,
                             config->buffer_size + sizeof (NetBuffer_t)) < 0 ||
        DC_buffer_pool_init (&serv->net_io_pool,
                             config->max_peers,
                             sizeof (NetIO_t)) < 0 ||
        DC_buffer_pool_init (&serv->net_peer_pool,
                             config->max_peers,
                             sizeof (NetPeer_t)) < 0) {
        fprintf (stderr, "DC_buffer_pool_init failed.\n");
        return -1;
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

    if (CreateSocketIO (serv, 1) < 0) {
        fprintf (stderr, "CreateSocketIO failed\n");
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

    SetEventIO (serv);

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

DC_INLINE void RunServer (Server_t *serv)
{
    ev_timer  timer;
    ev_signal sig_handler;

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
}

DC_INLINE void ReleaseServer (Server_t *serv)
{
    if (serv->delegate && serv->delegate->willStopServer) {
        serv->delegate->willStopServer (serv);
    }

    ev_loop_destroy (serv->ev_loop);
    CreateSocketIO (serv, 0);
    DC_signal_free (serv->sig_handle);
    DC_queue_destroy (&serv->request_queue);
    DC_queue_destroy (&serv->reply_queue);
    DC_buffer_pool_destroy (&serv->net_buffer_pool);
    DC_buffer_pool_destroy (&serv->net_io_pool);
    pthread_rwlock_destroy (&serv->serv_lock);
}

int ServerRun (Server_t *serv, NetConfig_t *config, NetDelegate_t *delegate)
{
    int ret;
    
    memset (serv, '\0', sizeof (Server_t));
    serv->config   = config;
    serv->delegate = delegate;

    if (!(ret = InitServer (serv))) {
        RunServer (serv);
    }

    ReleaseServer (serv);

    return ret;
}

void ServerStop (Server_t *serv)
{
    serv->exit_flag = 1;
    kill (0, SIGINT);
}
