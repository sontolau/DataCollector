#ifndef _PNET_H
#define _PNET_H

#include "libdc.h"

#include <ev.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "buffer.h"
#include "queue.h"
#include "mutex.h"
#include "thread.h"

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
        char *cert;
        char *key;
    } net_ssl;

} NetAddr_t;

typedef struct _NetIOHandler {
    int (*netCreateIO) (struct _NetIO*);
    int (*willAcceptRemoteIO) (struct _NetIO*);
    int (*netCheckIO) (struct _NetIO*);
    int (*netAcceptRemoteIO) (struct _NetIO*,  const struct _NetIO*);
    long (*netReadFromIO) (struct _NetIO*, unsigned char *buf, unsigned int szbuf);
    long (*netWriteToIO) (const struct _NetIO*, const unsigned char *bytes, unsigned int len);
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

typedef struct _NetIO {
    int          fd;
    struct ev_io ev;
    struct _NetIO *to;
    NetAddr_t    *addr_info;
    int          connected;
    unsigned int timer;
    int          refcount;
    NetSocketAddr_t local_addr;

    struct {
        SSL_CTX *ctx;
        SSL     *ssl;
    } ssl;

    void (*release_cb)(struct _NetIO *io, void*);
    void            *cb_data;
    DC_mutex_t      io_lock;
    DC_link_t       PRI (buffer_link);
    DC_link_t       PRI (conn_link);
    NetIOHandler_t* PRI (handler);
} NetIO_t;

extern int NetIOInit (NetIO_t *io, 
                      const NetAddr_t *addr,
                      void (*release)(NetIO_t*, void*),
                      void*); 

extern void NetIORelease (NetIO_t *io);

#define NetIOCreate(_io) (_io->__handler->netCreateIO (_io))
#define NetIOAcceptRemote(_io, _to)       (_io->__handler->netAcceptRemoteIO (_to, _io))
#define NetIOWillAcceptRemote(_io)    (_io->__handler->willAcceptRemoteIO (_io))
#define NetIOReadFrom(_io, _buf, _szbuf)   (_io->__handler->netReadFromIO (_io, _buf, _szbuf))
#define NetIOWriteTo(_io, _bytes, _blen)      (_io->__handler->netWriteToIO (_io, _bytes, _blen))
#define NetIOClose(_io)                    (_io->__handler->netCloseIO(_io))
#define NetIOLock(_io)                   do {DC_mutex_lock (&_io->io_lock, 0, 1);} while(0)
#define NetIOUnlock(_io)                 do {DC_mutex_unlock (&_io->io_lock);} while (0)
#define NetIOConnect(_from, _to) \
    do {\
        _from->connected = 1;\
        DC_link_add (&_to->PRI (conn_link), &_from->PRI (conn_link));\
        _from->to = _to;\
    } while (0)

#define NetIOConnected(_io) (_io->connected)

#define NetIODisconnect(_io) \
    do {\
        if (NetIOConnected (_io)) {\
            DC_link_remove (&_io->PRI (conn_link));\
            _io->to = NULL;\
            _io->connected = 0;\
        }\
    } while (0)

#define NetIOUpdate(_io) \
    do {\
        DC_link_remove (&_io->PRI (conn_link));\
        DC_link_add (&_io->PRI (conn_link), &_io->to->PRI (conn_link));\
    } while (0)

typedef struct _NetBuffer {
    NetIO_t      *io;
    unsigned int buffer_size;
    unsigned int buffer_length;
    DC_link_t    PRI(buffer_link);
    union {
        unsigned char buffer[0];
    };
} NetBuffer_t;

typedef struct _NetConfig {
    char *chdir;
    char *log;
    int  daemon;
    int  num_listeners;

    int  max_requests;
    int  max_buffers;
    int  num_process_threads;
    unsigned int buffer_size;
    int  queue_size;
    unsigned int timer_interval;
    int  conn_timeout;
} NetConfig_t;

enum {
    NET_STAT_IDLE   = 1,
    NET_STAT_NORMAL = 2,
    NET_STAT_BUSY   = 3,
};

typedef struct _Net {
    NetIO_t  *net_io;
    
    struct ev_loop *ev_loop;
    int            status; //read only

    struct {
        DC_buffer_pool_t net_buffer_pool;
        DC_buffer_pool_t net_io_pool;
        DC_mutex_t       buf_lock;
    };

    FILE           *logfp;
    NetAddr_t      *net_addr_array;
    DC_queue_t     request_queue;
    DC_queue_t     reply_queue;

    DC_thread_t    reply_thread;
    DC_thread_t    manager_thread;
    DC_thread_t    conn_checker;
    DC_thread_pool_manager_t  core_proc_pool;

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
#define NetSetUserData(srv, data)        do{srv->private_data = data;}while(0);
#define NetGetUserData(srv)              (srv->private_data)
#define NetGetStatus(srv)                (srv->status)

extern NetIO_t *NetAllocIO (Net_t *net);

extern void NetFreeIO (Net_t *net, NetIO_t *io);


extern void NetCommitIO (Net_t *srv, NetIO_t *io);

extern NetBuffer_t *NetAllocBuffer (Net_t *srv);

extern void NetFreeBuffer (Net_t *srv, NetBuffer_t *buf);

extern void NetBufferSetRemote(NetBuffer_t *buf, NetSocketAddr_t *addr);

extern void NetBufferSetIO (Net_t *srv, NetBuffer_t *buf, NetIO_t *io);

extern void NetBufferRemoveIO (Net_t *srv, NetBuffer_t *buf);

enum {
    RES_IO    = 1,
    RES_BUFFER = 2,
};

typedef struct _NetDelegate {
    void (*getNetAddressWithIndex) (Net_t *srv, NetAddr_t *net, int index);
    void (*timerout) (Net_t *srv, unsigned int timer);
    int  (*willInitNet) (Net_t *srv);
    void (*willRunNet) (Net_t *srv);
    int (*willAcceptRemoteNetIO) (Net_t *srv, NetIO_t *io);
    void (*willCloseNetIO) (Net_t *srv, NetIO_t *io);
    void (*willChangeStatus) (Net_t *srv, int status);
    int  (*processBuffer) (Net_t *srv, NetBuffer_t *buf);
    void (*willStopNet) (Net_t *srv);
    void (*resourceUsage) (Net_t *srv, int type, float percent);
} NetDelegate_t;

extern int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void NetStop (Net_t *serv);

#endif
