#ifndef _PNET_H
#define _PNET_H

#include "libdc.h"

#include <ev.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

enum {
    NET_TCP = 0,
    NET_UDP = 1,
};

struct _NetIO;
struct _NetConfig;
struct _NetAddr;
struct _NetBuffer;
struct _NetSockAddr;

typedef struct _NetAddr {

#define NET_F_BIND  (1<<0)
#define NET_F_IPv6  (1<<1)
#define NET_F_SSL   (1<<2)

    int net_flag;
    int net_type;
    /*
     * if net_port is zero, a unix type socket will be created.
     * the net_addr field must be the path.
     */
    unsigned short net_port;
#define MAX_NET_ADDR_LEN   108
    char           net_addr[MAX_NET_ADDR_LEN];

    struct {
        char *CA;
        char *cert;
        char *key;
    } net_ssl;

} NetAddr_t;

struct _NetBuffer;

typedef struct _NetIOHandler {
    int (*netCreateIO) (struct _NetIO*);
    int (*netCtrlIO) (struct _NetIO*, int type, void *arg, int szarg);
    int (*willAcceptRemoteIO) (struct _NetIO*);
    int (*netCheckIO) (struct _NetIO*);
    int (*netAcceptRemoteIO) (struct _NetIO*,  const struct _NetIO*);
    long (*netReadFromIO) (struct _NetIO*, struct _NetBuffer *buf);
    long (*netWriteToIO) (struct _NetIO*, const struct _NetBuffer *buf);
    void (*netCloseIO) (struct _NetIO*);
}NetIOHandler_t;

typedef struct _NetSocketAddr {
    union {
        struct sockaddr_un      su;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
        struct sockaddr_storage ss;
    };

    socklen_t sock_length;
}NetSocketAddr_t;


struct _NetBuffer;
typedef struct _NetIO {
    int          fd;
    struct ev_io ev;
    struct _NetIO *to;
    NetAddr_t    *addr_info;
    int          connected;
    unsigned int timer;
    NetSocketAddr_t local_addr;
    struct {
        SSL_CTX *ctx;
        SSL     *ssl;
    } ssl;
    void        *private_data;
    DC_mutex_t      io_lock;
    DC_link_t       PRI (conn_link);
    DC_link_t       PRI (buffer_link);
    NetIOHandler_t* PRI (handler);
} NetIO_t;

extern int NetIOInit (NetIO_t *io, 
                      const NetAddr_t *addr);
extern void NetIODestroy (NetIO_t *io);

extern struct _NetBuffer *NetIOBufferFetch (NetIO_t *io);

#define NetIOSetUserData(_io, _data) do {_io->private_data = _data;}while (0)
#define NetIOGetUserData(_io)        (_io->private_data)
/*
 * NetIOCtrl sets socket options.
 */
enum {
    NET_IO_CTRL_BIND        =   1,
    NET_IO_CTRL_CONNECT,  
    NET_IO_CTRL_REUSEADDR,
    NET_IO_CTRL_SET_RECV_TIMEOUT,
    NET_IO_CTRL_SET_SEND_TIMEOUT,
    NET_IO_CTRL_GET_RECV_TIMEOUT,
    NET_IO_CTRL_GET_SEND_TIMEOUT,
    NET_IO_CTRL_SET_SNDBUF,
    NET_IO_CTRL_SET_RCVBUF,
    NET_IO_CTRL_GET_SNDBUF,
    NET_IO_CTRL_GET_RCVBUF,
    NET_IO_CTRL_SET_NONBLOCK,
};

#define NetIOCreate(_io)                  (_io->__handler->netCreateIO (_io))
#define NetIOCtrl(_io, _type, _arg, _szarg) (_io->__handler->netCtrlIO (_io, _type, _arg, _szarg))
#define NetIOConnectRemote(_io, _addr, _addrlen) (NetIOCtrl (_io, NET_IO_CTRL_CONNECT, _addr, _addrlen))
#define NetIOConnect(_io) NetIOConnectRemote(_io, NULL, 0)
#define NetIOBind(_io, _addr, _addrlen)    (NetIOCtrl (_io, NET_IO_CTRL_BIND, _addr, _addrlen))

#define NetIOAcceptRemote(_io, _to)       (_io->__handler->netAcceptRemoteIO (_to, _io))
#define NetIOWillAcceptRemote(_io)        (_io->__handler->willAcceptRemoteIO (_io))
#define NetIOReadFrom(_io, _buf)          (_io->__handler->netReadFromIO (_io, _buf))
#define NetIOWriteTo(_io, _buf)           (_io->__handler->netWriteToIO (_io, _buf))
#define NetIOClose(_io)                   (_io->__handler->netCloseIO(_io))
#define NetIOLock(_io)                    do {DC_mutex_lock (&_io->io_lock, 0, 1);} while(0)
#define NetIOUnlock(_io)                  do {DC_mutex_unlock (&_io->io_lock);} while (0)

typedef struct _NetBuffer {
    NetIO_t      *io;
    unsigned int id;
    unsigned int size;
    unsigned int data_length;
    unsigned int offset;
    DC_link_t  PRI(io_link);
    NetSocketAddr_t buffer_addr;
    union {
        int   int_value;
        void *pointer;
        unsigned char buffer[0];
    };
} NetBuffer_t;

#define NetBufferAlloc(_buf, _size) \
do {\
    _buf = (NetBuffer_t*)calloc (1, sizeof (NetBuffer_t)+_size);\
    if (_buf) {\
        _buf->size = _size;\
    }\
} while (0)

#define NetBufferFree(_buf) \
do {\
    if (_buf) free (_buf);\
} while (0)

#define NetBufferSetID(_buf, _id) do {_buf->id = _id;}while (0)
#define NetBufferGetIO(_buf)  (_buf->io)
#define NetBuffetGetID(_buf)  (_buf->id)
#define NetBufferSetIO(_buf, _io) \
do {\
    NetIOLock (_io);\
    _buf->io = _io;\
    DC_link_add_front (&_io->PRI(buffer_link), &_buf->PRI(io_link));\
    NetIOUnlock (_io);\
} while (0)

#define NetBufferRemoveIO(_buf) \
do {\
    if (_buf && _buf->io) {\
        NetIOLock (_buf->io);\
        DC_link_remove (&_buf->PRI(io_link));\
        NetIOUnlock (_buf->io);\
        _buf->io = NULL;\
    }\
} while (0)

#define NetBufferSetData(_buf, _off, _data, _size) \
do {\
    memcpy (_buf->buffer+_off, _data, MIN(_buf->size-_off, _size));\
    _buf->data_length = MIN (_buf->size-_off, _size);\
} while (0)


typedef struct _NetConfig {
    char *chdir;
    char *log;
    char *pidfile;
    int  daemon;
    int  num_sockets;
    int  num_sockbufs;
    unsigned int max_sockbuf_size;
    int num_sock_conns;
    int  num_threads_each_queue;
    int  process_queue_size;
    int  num_process_queues;
    unsigned int  watch_dog;
} NetConfig_t;

#define WATCH_DOG(_delay, _timeout) (((_delay&0xFF)<<8)|(_timeout&0xFF))

enum {
    NET_STAT_IDLE   = 1,
    NET_STAT_NORMAL = 2,
    NET_STAT_BUSY   = 3,
};

struct _Net;

typedef struct _NetQueueManager {
    DC_queue_t  queue;
    DC_mutex_t  lock;
    DC_thread_t manager_thread;
    DC_thread_pool_manager_t task_pool;
    void (*handler) (struct _NetQueueManager*, qobject_t, void*);
    void *data;
}NetQueueManager_t;


extern int InitQueueManager (NetQueueManager_t *mgr,
                             int queue_size,
                             int num_threads,
                             void (*handler)(NetQueueManager_t*, qobject_t, void*),
                             void *data);

extern int RunTaskInQueue (NetQueueManager_t *mgr,
                           qobject_t task);

extern void DestroyQueueManager (NetQueueManager_t *mgr);

typedef struct _Net {
    NetIO_t  *net_io;
    
    struct ev_loop *ev_loop;
    int            status; //read only

    struct {
        DC_buffer_pool_t net_buffer_pool;
        DC_buffer_pool_t net_io_pool;
        DC_mutex_t       buf_lock;
    };

    NetAddr_t      *net_addr_array;
    NetQueueManager_t     *proc_queue_map;
    NetQueueManager_t     reply_queue;

    DC_thread_t    conn_checker;
    unsigned int   timer;
    NetConfig_t    *config;
    struct _NetDelegate  *delegate;
    void           *private_data;
    DC_mutex_t     PRI (serv_lock);
} Net_t;

typedef Net_t NetServer_t;
/*
 *  do not access members of Net_t instance directly, please
 *  use the following macro definitions or functions to 
 *  access.
 */
#define NetLockContext(srv)      do { DC_mutex_lock (&srv->PRI (serv_lock), 0, 1);}while (0)
#define NetUnlockContext(srv)    do { DC_mutex_unlock (&srv->PRI (serv_lock));} while (0)
#define NetGetIO(srv, index)    (&srv->net_io[index])
#define NetSetUserData(srv, data)        do{srv->private_data = data;}while(0)
#define NetGetUserData(srv)              (srv->private_data)
#define NetGetStatus(srv)                (srv->status)

extern NetIO_t *NetAllocIO (Net_t *net);

extern void NetCloseIO (Net_t *net, NetIO_t *io);

extern void NetFreeIO (Net_t *net, NetIO_t *io);


extern NetBuffer_t *NetAllocBuffer (Net_t *srv);

extern void NetFreeBuffer (Net_t *srv, NetBuffer_t *buf);

extern void NetCommitIO (Net_t *net, const NetIO_t *io);

enum {
    RES_IO    = 1,
    RES_BUFFER = 2,
};

typedef struct _NetDelegate {
    void (*getNetAddressWithIndex) (Net_t *srv, NetAddr_t *net, int index);
    int  (*willInitNet) (Net_t*);
    void (*didConnectToHost) (Net_t *net, NetIO_t *remote);
    void (*didBindToLocal) (Net_t *net, NetIO_t *local);
    unsigned int (*getNetQueueWithHashID) (Net_t *net, NetBuffer_t *buf);

    int  (*willAcceptRemote) (Net_t *net, const NetIO_t *local, NetIO_t *remote);
    long  (*readSocketData) (Net_t *net, NetIO_t *io, NetBuffer_t *buf);
    void (*didReadData) (Net_t *srv, NetIO_t *from, NetBuffer_t *buf);
    long (*writeSocketData) (Net_t *srv, NetIO_t *to, NetBuffer_t *buf);

    int (*didWriteData) (Net_t *srv, NetIO_t*, NetBuffer_t *buf,int ok, int errcode);
    int  (*processData) (Net_t *srv,NetIO_t *io, NetBuffer_t *buf);
    void (*willDisconnectWithRemote) (Net_t *srv, NetIO_t *remote);
    void (*willCloseNet) (Net_t *net);
    int  (*ping) (Net_t *net, NetBuffer_t *buf);
    void (*didReceiveTimer) (Net_t *net, unsigned int count);
    void (*resourceUsage) (Net_t *net, int type, float percent);
} NetDelegate_t;

extern int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void NetStop (Net_t *serv);

#endif
