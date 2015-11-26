#include "N_netkit.h"

static int recved_count = 0;
static int processed_count = 0;

void timer_func (NetKit *nk)
{
    printf ("received: %d, processed: %d\n", recved_count, processed_count);
}

int recv_ok_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
    if (buf->length <= 0) {
        NK_remove_peer (nk, peer);
        printf ("Remove client.\n");
    }
    recved_count++;
    return 1;
}

void recv_error_func (NetKit *nk, NKPeer *peer)
{
    NK_remove_peer (nk, peer);
    printf ("Remove client.\n");
}

void accept_func (NetKit *nk, NKPeer *p1, NKPeer *p2)
{
    NK_add_peer (nk, p2, NK_EV_READ);
    printf ("Accpted a new client.\n");
}


void proc_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
/*
    buf->buffer[buf->length] = '\0';
    NKBuffer *wbuf = NULL;
    processed_count++;
    wbuf = NK_alloc_buffer_with_init (nk);
    NK_buffer_set_peer (wbuf, peer);
    sprintf ((char*)wbuf->buffer, "%s", buf->buffer);
    wbuf->length = strlen (wbuf->buffer);
    NK_commit_buffer (nk, wbuf);
*/
}


static NKDelegate delegate = {
    .didReceiveTimer = timer_func,
    .didSuccessToReceiveData = recv_ok_func,
    .didFailureToReceiveData = recv_error_func,
    .didAcceptRemoteClient = accept_func,
    .processData          = proc_func,
};

void main (int argc, char *argv[])
{
    NetKit nk;
    INetAddress_t sockaddr;
    int blog = 100;
    NKPeer *peer= NULL;

    if (argc < 3) {
        fprintf (stderr, "USAGE: %s HOST PORT\n", argv[0], argv[1], argv[2]);
        exit (1);
    }


    int flag = 1;
     NetSockOption_t opt_reuseaddr = {
        .level = SOL_SOCKET,
        .name  = SO_REUSEADDR,
        .optval = &flag,
        .optlen = sizeof (int),
    };

    NK_init (&nk, NULL);
    peer = NK_alloc_peer_with_init (&nk);
    peer->addr.flag = 0;
    peer->addr.type = NET_TCP;
    peer->addr.address = argv[1];
    peer->addr.port = atoi (argv[2]);

    if (NetIOInit (&peer->io, &peer->addr, 1) < 0 ||
        NetIOSetOpts (&peer->io, &opt_reuseaddr, 1) < 0 ||
        NetIOBind (&peer->io) < 0 ||
        NetIOListen (&peer->io, &blog) < 0) {
        fprintf (stderr, "NetIOInit failed\n");
        exit (1);
    }

    NK_set_delegate (&nk, &delegate);
    NK_add_peer (&nk, peer, NK_EV_ACCEPT);
    NK_run (&nk);
    NK_destroy (&nk);
}
