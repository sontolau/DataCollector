#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>

#if 0

#define ROOT_CA "/root/Workspace/DataCollector/certs/root.pem"
#define SERV_CERT "/root/Workspace/DataCollector/certs/server.pem"
#define SERV_KEY  "/root/Workspace/DataCollector/certs/server.key"

#define CLIENT_CERT "/root/Workspace/DataCollector/certs/client.pem"
#define CLIENT_KEY  "/root/Workspace/DataCollector/certs/client.key"

static NetConfig_t netConfig = {
    .chdir = "/",
    .daemon= 0,
    .num_sockets = 1,
    .max_requests = 0,
    .max_buffers  = 0,
    .conn_timeout = 3,
    .num_process_threads = 1,
    .buffer_size = 1000,
    .queue_size = 100,
    .timer_interval = 2,
    .rw_timeout = 2000,
};

void server_fun (Net_t *net, NetAddr_t *addr, int index)
{
    addr->net_type = NET_UDP;
    addr->net_flag |= NET_F_BIND;
    //addr->net_flag |= NET_F_SSL;
    addr->net_port = 8888;
    strncpy (addr->net_addr, "0.0.0.0", MAX_NET_ADDR_LEN);
    addr->net_ssl.cert = SERV_CERT;
    addr->net_ssl.key  = SERV_KEY;
    addr->net_ssl.CA   = ROOT_CA;
}

int server_proc (Net_t *net, NetBuffer_t *buf)
{
    NetBuffer_t *tmpbuf = NetAllocBuffer (net);

    if (tmpbuf) {
        strncpy (tmpbuf->buffer, buf->buffer, buf->buffer_length);
        tmpbuf->buffer_length = buf->buffer_length;
        NetBufferSetIO (net, tmpbuf, buf->io);
        NetCommitIO (net, buf->io);
    }

    return 0;
}


void client_fun (Net_t *net, NetAddr_t *addr, int index)
{
    addr->net_type = NET_UDP;
    //addr->net_flag = NET_F_SSL;
    addr->net_port = 8888;
    strncpy (addr->net_addr, "0.0.0.0", MAX_NET_ADDR_LEN);
    addr->net_ssl.cert = CLIENT_CERT;
    addr->net_ssl.key  = CLIENT_KEY;
    addr->net_ssl.CA   = ROOT_CA;
}

int client_proc (Net_t *net, NetBuffer_t *buf)
{
    printf ("Received from server: %s\n", buf->buffer);
    return 0;
}

static NetDelegate_t serverDelegate = {
    .getNetAddressWithIndex        = server_fun,
    .processBuffer                 = server_proc,
};

static Net_t net;
void *send_proc (void *data)
{
     NetIO_t *io = NetGetIO (((Net_t*)&net), 0);
    while (1) {
        NetIOWriteTo (io, "hello", 5);
        break;
    }
}

void run_net (Net_t *net)
{
    pthread_t pt;
    pthread_create (&pt, NULL, send_proc, NULL);
}

static NetDelegate_t clientDelegate = {
    .getNetAddressWithIndex = client_fun,
    .willRunNet             = run_net,
    .processBuffer          = client_proc,
};

#endif


#define IF_WHEN(_cond, _e1, _e2) ((_cond)?(_e1):(_e2))

#define CB(_net, _func, n, ...) \
       IF_WHEN(n, IF_WHEN(_net->delegate && _net->delegate->_func, _net->delegate->_func(_net, __VA_ARGS__),0), IF_WHEN(_net->delegate && _net->delegate->_func, _net->delegate->_func(_net),0))
void func() {

};

int main (int argc, const char *argv[])
{
/*
    NetAddr_t remote;
    int len = 0;
    memset (&net, '\0', sizeof (net));
    if (argc <= 1) {
        return NetRun (&net, &netConfig, &serverDelegate);
    } else {
        return NetRun (&net, &netConfig, &clientDelegate);
    }
    NetBuffer_t *buf = NULL;
    NetBufferAlloc (buf, 10);
    NetBufferSetData (buf, 0, "hello", strlen ("hello"));
    printf ("%s\n", buf->buffer);

    NetBufferFree (buf);

*/ 
   // CB(_net, willInitNet, 0);
    printf ("%d, %d", (int)2.5, (int)2.6);
    return 0;
}
