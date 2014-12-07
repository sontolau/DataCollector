#include "netsrv.h"

DC_INLINE void TCPReadEventCallBack (struct ev_loop *ev, ev_io *io, int revents)
{
    NetBuffer_t *buf = NULL;
    Server_t *
    buf = Net
}

void TCPEventCallBack (Server_t *srv, NetIO_t *io)
{
    NetPeer_t *peer = NULL;
    struct sockaddr_in addr;
    socklen_t          sklen = sizeof (addr);

    peer = NetPeerAlloc (srv);
    if (peer) {
        //TODO: process exception.
        close (accept (io->io_fd, NULL, NULL));
        return;
    } else {
        memset (peer, '\0', sizeof (NetPeer_t));

        peer->netio.io_fd = accept (io->io_fd, (struct sockaddr*)&addr, &sklen);
        if (peer->netio.io_fd <= 0) {
            io->io_handler->closeNetIO (srv, io);
            NetPeerFree (srv, peer);
            return;
        }
        peer->trans_time  = srv->timer;
        peer->netio.peer  = peer;
        peer->netio.io_net.net_type = NET_TYPE_TCP;
        peer->netio.io_net.net_port = ntohs (addr.sin_port);
        peer->netio.io_net.net_addr = addr.sin_addr.s_addr;
        peer->flag        = PEER_FLAG_WAITING;
       
        pthread_rwlock_wrlock (&srv->serv_lock);
        ev_io_init (&peer->netio.io_ev, TCPReadEventCallBack, peer->netio.io_fd, EV_READ);
        ev_io_start (srv->ev_loop, &peer->netio.io_ev);
        DC_link_add (&srv->peer_link[0], &peer->__link);
        ptread_rwlock_unlock (&srv->serv_lock);
    }
}
