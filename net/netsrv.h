#ifndef _PMS_SERVMASTER_H
#define _PMS_SERVMASTER_H
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include <ev.h>
#include <libdc/libdc.h>
#include <libdc/buffer.h>
#include <libdc/queue.h>
#include <libdc/signal.h>

enum {
    NET_TYPE_TCP = 1,
    NET_TYPE_UDP = 2,
};


#ifndef S_ERROR
#define S_ERROR   (0)
#endif

#ifndef S_OK
#define S_OK  (!S_ERROR)
#endif

typedef enum {
    NET_COM_PING = 1,
    NET_COM_CONN = 2,
    NET_COM_TRANSACTION,
    NET_COM_DISCONN,
}NetCom_t;


typedef struct _NetHeader {
    unsigned short magic;

#define NET_FLAG_REPLY   (1<<31)
#define NET_FLAG_CONN (1<<1)

    unsigned short flag;
    NetCom_t       command;
    unsigned int   cid;
    unsigned int   status;
    unsigned int   data_len;
    unsigned char  data[0];
} NetHeader_t;
#define SZNETHEADER (sizeof (NetHeader_t))

#define NET_REPLY(hdr)     (hdr.flag & NET_FLAG_REPLY)

typedef struct _NetInfo {
    int net_type;
    unsigned short net_port;
    unsigned int   net_addr;
} NetInfo_t;

struct _NetPeer;
typedef struct _NetIO {
    int       sock_fd;
    struct    ev_io io;
    unsigned int expire_time;
    unsigned int trans_time;
#define PEER_FLAG_WAITING       (1<<0)
#define PEER_FLAG_CONN          (1<<1)
    int          flag;
    NetInfo_t net_info;
    void     *userdata;

    DC_link_t    __status_link;
    DC_link_t    __peer_link;
}NetIO_t;
#define NetIOType(io)   (io->net_info.net_type)
#define NetIOSetStatus(_io, _st)   do { _io->flag &= 0xFFF0; _io->flag |= _st;} while (0)
#define NetIOGetStatus(io)        (io->flag & 0xFFF0)
#define NetIOSetUserData(io, data) (io->userdata = data)
#define NetIOGetUserData(io)       (io->userdata)

typedef struct _NetPeer {
    long long cid;
    NetIO_t   *net_io;
    DC_link_t  io_link;
} NetPeer_t;

typedef struct _NetBuffer {
    NetIO_t      *net_io;
    unsigned int buffer_size;
    union {
        unsigned char buffer[0];
        NetHeader_t   net_buf;
    };
} NetBuffer_t;

#define NetBufferSetData(_buf, _com, _data, _size) \
    do {\
        memcpy (_buf->net_buf.data, _data, _size);\
        _buf->net_buf.command  = _com;\
        _buf->net_buf.data_len = _size;\
    } while (0);

#define NetBufferGetDataLength(_buf) (_buf->net_buf.data_len)
#define NetBufferGetDataAddr(_buf)   (_buf->net_buf.data)

typedef enum {
    NEV_DONOTHING = 1,
    NEV_REPLY = 2,
    NEV_CLOSE,
} NEv_t;

typedef struct _NetConfig {
    char *chdir;
    int  daemon;
    int  max_peers;
    int  num_sock_io;
    int  max_buffers;
    unsigned int buffer_size;
    int  queue_size;
    unsigned short ping_keepalive;
    int  max_idle_time;
} NetConfig_t;

struct _NetDelegate;
typedef struct _Server {
    NetIO_t  *net_io;
    struct ev_loop *ev_loop;
    
    DC_buffer_pool_t net_buffer_pool;
    DC_buffer_pool_t net_io_pool;
    DC_buffer_pool_t net_peer_pool;

    DC_queue_t     request_queue;
    DC_queue_t     reply_queue;
    HDC            sig_handle;
    unsigned int   timer;
    NetConfig_t    *config;
    struct _NetDelegate  *delegate;
    void           *private_data;
    DC_link_t      peer_link[2];
    volatile int   exit_flag;
    pthread_rwlock_t serv_lock;
} Server_t;

#define NetGetActiveLink(srv)    ((DC_link_t*)&srv->peer_link[0])
#define NetGetInactiveLink(srv)   ((DC_link_t*)&srv->peer_link[1])

#define ServerSetUserData(srv, data) do{srv->private_data = data;}while(0);
#define ServerGetUserData(srv)          (srv->private_data)

extern NetIO_t *NetIOAlloc (Server_t *srv);
extern void NetIOFree (Server_t *srv, NetIO_t *io);

extern NetPeer_t *NetPeerAlloc (Server_t *srv);
extern void NetPeerFree (Server_t *srv, NetPeer_t *peer);

extern NetBuffer_t *NetBufferAlloc (Server_t *srv);
extern void NetBufferFree (Server_t *srv, NetBuffer_t *buf);

//extern int AllocNetBuffer (Server_t *srv);
//extern void FreeNetBuffer (Server_t *srv);

typedef struct _NetDelegate {
    void (*getNetInfoWithIndex) (Server_t *srv, NetInfo_t *net, int index);
    void (*timerIsUp) (Server_t *srv, unsigned int timer);
    int  (*willInitServer) (Server_t *srv);
    void (*willChangePeerStatus) (Server_t*srv, Remote_t *peer, int from, int to);
    void (*willClosePeer) (Server_t *srv, Remote_t *peer);

    int (*isPeerConnected) (Server_t *srv, Remote_t *remote);

    NEv_t  (*processRequest) (Server_t *srv, NetBuffer_t *buf, int *status);
    void (*didSendReply) (Server_t *srv, NetBuffer_t *buf, int status);
    void (*willRunServer) (Server_t *srv);
    void (*willStopServer) (Server_t *srv);
} NetDelegate_t;

extern int ServerRun (Server_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void ServerStop (Server_t *serv);

#endif
