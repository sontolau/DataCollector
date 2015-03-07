#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include "netio.h"

#define SERV_CERT "/root/Workspace/DataCollector/certs/server.pem"
#define SERV_KEY  "/root/Workspace/DataCollector/certs/server.key"

#define CLIENT_CERT "/root/Workspace/DataCollector/certs/client.pem"
#define CLIENT_KEY  "/root/Workspace/DataCollector/certs/client.key"

static NetConfig_t netConfig = {
    .chdir = "/",
    .daemon= 0,
    .num_listeners = 1,
    .max_requests = 2,
    .max_buffers  = 0,
    .check_conn_timeout = 1,
    .num_process_threads = 1,
    .buffer_size = 1000,
    .queue_size = 100,
    .timer_interval = 1,
};

void fun1 (Net_t *net, NetAddr_t *addr, int index)
{
    addr->net_type = NET_TCP;
    addr->net_flag |= NET_F_BIND;
    //addr->net_flag |= NET_F_SSL;
    addr->net_port = 8888;
    strncpy (addr->net_addr, "0.0.0.0", MAX_NET_ADDR_LEN);
    addr->net_ssl.cert = SERV_CERT;
    addr->net_ssl.key  = SERV_KEY;
}

int fun2 (Net_t *net, NetBuffer_t *buf)
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

static NetDelegate_t netDelegate = {
    .getNetAddressWithIndex = fun1,
    .processBuffer                 = fun2,
};

int main (int argc, const char *argv[])
{
    Net_t net;
    NetIO_t cio;
    NetAddr_t remote;
    int len = 0;

    if (argc <= 1) {
        memset (&net, '\0', sizeof (net));
        return NetRun (&net, &netConfig, &netDelegate);
    } else {
        memset (&remote, '\0', sizeof (remote));
        remote.net_type = NET_TCP;
        //remote.net_flag |= NET_F_SSL;
        remote.net_port = 8888;
        strcpy (remote.net_addr, "0.0.0.0");
        remote.net_ssl.cert = CLIENT_CERT;
        remote.net_ssl.key  = CLIENT_KEY;

        memset (&cio, '\0', sizeof (cio));
        NetIOInit (((NetIO_t*)&cio), &remote, NULL, NULL);
        if (NetIOCreate (((NetIO_t*)&cio)) < 0) {
            return -1;
        }
       // NetIOClose ((((NetIO_t*)&cio)));
       // NetIORelease (&cio);

        //sleep (1000);
        char message[50] = "How are you";
        strcpy (message, argv[1]);
        while (1) {
            if (NetIOWriteTo (((NetIO_t*)&cio), message, strlen (message)) <= 0 ||
                (len = NetIOReadFrom (((NetIO_t*)&cio), message, 50)) <= 0) {
                break;
            }
            message[len] = '\0';
            printf ("Read: %s\n", message);
        };
        NetIOClose ((((NetIO_t*)&cio)));
    }
}
