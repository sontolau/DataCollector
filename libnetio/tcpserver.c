#include "N_netkit.h"

static NKConfig config = {
    .chdir = NULL,
    .log   = NULL,
    .pidfile = "./netkit.pid",
    .daemon  = 0,
    .num_sockbufs = 10000,
    .max_sockbuf_size = 1024,
    .num_sock_conns = 100,
    .queue_size = 10000,
    .watch_dog = WATCH_DOG (1, 5),
    .memory_warn_percent = 0.80,
    .conn_warn_percent = 0.80,
    .request_busy_percent = 0.80,
    .reply_busy_percent = 0.80,
};

static unsigned long recv_counter = 0, send_counter = 0, proc_counter=0;

void timer_func (NetKit *nk)
{
    fprintf (stderr, "Read: %u\tProcess: %u\tWrite: %u\n", recv_counter, proc_counter, send_counter);
    fprintf (stderr, "Available Buffers: %d\n", nk->buffer_pool.num_left);
    recv_counter = 0;
    send_counter = 0;
    proc_counter = 0;
}

void recv_ok_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
    recv_counter++;
    buf->buffer[NK_buffer_get_length(buf)] = '\0';
    printf ("Received: %s (%d)\n", (char*)NK_buffer_get_data (buf), NK_buffer_get_length(buf));
}

void recv_error_func (NetKit *nk, NKPeer *peer)
{
    fprintf (stderr, "recv failed.\n");
}

void send_ok_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
    send_counter++;
    
}

void send_error_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
    fprintf (stderr, "send failed.\n");
}

void request_busy_func (NetKit *nk, double percent)
{
    fprintf (stderr, "request busy: %F\n", percent);
}

void reply_busy_func (NetKit *nk, double percent)
{
    fprintf (stderr, "reply busy: %F\n", percent);
}

void memory_warn_func (NetKit *nk, double percent)
{
   fprintf (stderr, "memory warnning: %F %u. \n", percent, nk->buffer_pool.num_left);
}

void conn_warn_func (NetKit *nk, double percent)
{
   fprintf (stderr, "connection warning: %F.\n", percent);
}

void proc_func (NetKit *nk, NKPeer *peer, NKBuffer *buf)
{
    char *str = "Replying ....\n";
    proc_counter++;
    NK_buffer_set_data (buf, (unsigned char*)str, strlen (str));
    NK_commit_buffer (nk, NK_buffer_get (buf));
}

void accept_func (NetKit *nk, NKPeer *peer, NKPeer *newpeer)
{
    char buf[255] = {0};
    static int index = 0;
    
    snprintf (buf, sizeof (buf)-1, "PEER: %d", index++);
    fprintf (stderr, "ACCEPT PEER: %s\n", buf);
    NK_peer_set_name (newpeer, buf);
}

void close_func (NetKit *nk, NKPeer *peer)
{
    fprintf (stderr, "WILL CLOSE PEER: %s\n", NK_peer_get_name (peer));
}

static NKDelegate delegate = {
    .didReceiveTimer = timer_func,
    .didSuccessToReceiveData = recv_ok_func,
    .didFailToReceiveData = recv_error_func,
    .didSuccessToSendData = send_ok_func,
    .willAcceptRemotePeer = accept_func,
    .willClosePeer        = close_func,
    .processData          = proc_func,
    .didFailToSendData    = send_error_func,
    .didReceiveRequestBusyWarning = request_busy_func,
    .didReceiveReplyBusyWarning   = reply_busy_func,
    .didReceiveMemoryWarning = memory_warn_func,
    .didReceiveConnectionWarning = conn_warn_func,
};

void main (int argc, char *argv[])
{
    NetIO_t serv_io;
    NetKit nk;
    INetAddress_t sockaddr;
    int blog = 100;

    if (argc < 3) {
        fprintf (stderr, "USAGE: %s HOST PORT\n", argv[0], argv[1], argv[2]);
        exit (1);
    }

    NetAddrInfo_t addrinfo = {
    .flag = 0,
    .type = NET_TCP,
    .port = atoi (argv[2]),
    .address = argv[1],
    };

    int flag = 1;
     NetSockOption_t opt_reuseaddr = {
        .level = SOL_SOCKET,
        .name  = SO_REUSEADDR,
        .optval = &flag,
        .optlen = sizeof (int),
    };

    //NetSockAddrFrom (&sockaddr, &addrinfo);
    if (NetIOInit (&serv_io, &addrinfo, 1) < 0 ||
        NetIOSetOpts (&serv_io, &opt_reuseaddr, 1) < 0 ||
        NetIOBind (&serv_io) < 0 ||
        NetIOListen (&serv_io, &blog) < 0 ||
        NK_init (&nk, &config) < 0) {
        fprintf (stderr, "NetIOInit failed\n");
        exit (1);
    }


    NK_add_netio (&nk, &serv_io, NK_EV_ACCEPT);
    NK_set_delegate (&nk, &delegate);
    NK_run (&nk);
    NK_destroy (&nk);
}
