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
    int (*netAcceptRemoteIO) (struct _NetIO*,  const struct _NetIO*);
    double (*netReadFromIO) (struct _NetIO*, unsigned char*, unsigned int szbuf);
    double (*netWriteToIO) (const struct _NetIO*, const unsigned char*, unsigned int length);
    void (*netDestroyIO) (struct _NetIO*);
}NetIOHandler_t;

typedef struct _NetIO {
    int          fd;
    //int          flag;
    struct ev_io ev;
    struct _NetIO *local;
    NetAddr_t    *addr_info;
    int          refcount;

    union {
        struct sockaddr_un      su;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
        struct sockaddr_storage ss;
    };
    socklen_t sock_len;

    struct {
        SSL_CTX *ctx;
        SSL     *ssl;
    } ssl;

    DC_mutex_t   PRI(io_lock);
    DC_link_t    PRI (buf_link);
    NetIOHandler_t *__handler;
} NetIO_t;

extern int NetIOInit (NetIO_t *io, const NetAddr_t *info);
extern NetIO_t *NetIOGet (NetIO_t *io);

#define NetIOCreate(_io) (_io->__handler->netCreateIO (_io))
#define NetIOAcceptRemote(_io, _to)       (_io->__handler->netAcceptRemoteIO (_to, _io))
#define NetIOWillAcceptRemote(_io)    (_io->__handler->willAcceptRemoteIO (_io))
#define NetIOReadFrom(_io, _buf, _szbuf)   (_io->__handler->netReadFromIO (_io, _buf, _szbuf))
#define NetIOWriteTo(_io, _buf, _len)      (_io->__handler->netWriteToIO (_io, _buf, _len))
#define NetIODestroy(_io)                (_io->__handler->netDestroyIO(_io))
#define NetIOLock(_io)    do {DC_mutex_lock (&_io->PRI (io_lock), 0, 0);}while (0)
#define NetIOUnlock(_io)  do {DC_mutex_unlock (&_io->PRI (io_lock));} while (0)

typedef struct _NetBuffer {
    NetIO_t      *io;
    unsigned int buffer_size;
    unsigned int buffer_length;
    int          refcount;
    DC_link_t    PRI (link);
    union {
        unsigned char buffer[0];
    };
} NetBuffer_t;

typedef struct _NetConfig {
    char *chdir;
    int  daemon;
    int  num_listeners;

    int  max_requests;
    int  max_buffers;
    int  num_process_threads;
    unsigned int buffer_size;
    int  queue_size;
    unsigned int timer_interval;
    //int  max_idle_time;
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
    
    NetAddr_t      *net_addr_array;
    DC_queue_t     request_queue;
    DC_queue_t     reply_queue;

    DC_thread_t    reply_thread;
    DC_thread_t    manager_thread;
    DC_thread_pool_manager_t  core_proc_pool;

    unsigned int   timer;
    NetConfig_t    *config;
    struct _NetDelegate  *delegate;
    void           *private_data;
    DC_mutex_t     serv_lock;
} Net_t;

typedef Net_t NetServer_t;
/*
 *  do not access members of Net_t instance directly, please
 *  use the following macro definitions or functions to 
 *  access.
 */

#define NetGetIOWithIndex(srv, index)    (&srv->net_io[index])
#define NetSetUserData(srv, data)        do{srv->private_data = data;}while(0);
#define NetGetUserData(srv)              (srv->private_data)
#define NetGetStatus(srv)                (srv->status)

extern NetBuffer_t *NetAllocIOBuffer (Net_t *srv, NetIO_t *io);
extern void NetFreeIOBuffer (Net_t *srv, NetBuffer_t *buf);
extern void NetCommitIOBuffer (Net_t *srv, NetIO_t *io);

enum {
    RES_IO    = 1,
    RES_BUFFER = 2,
};

typedef struct _NetDelegate {
    void (*getNetListenerAddressWithIndex) (Net_t *srv, NetAddr_t *net, int index);
    void (*timerout) (Net_t *srv, unsigned int timer);
    int  (*willInitNet) (Net_t *srv);
    void (*willRunNet) (Net_t *srv);

    int (*willAcceptRemoteNetIO) (Net_t *srv, NetIO_t *io);
    void (*willCloseNetIO) (Net_t *srv, NetIO_t *io);

    void (*willChangeStatus) (Net_t *srv, int status);
    int  (*processRequest) (Net_t *srv, const NetBuffer_t *buf);
    void (*willStopNet) (Net_t *srv);
    void (*resourceUsage) (Net_t *srv, int type, float percent);
} NetDelegate_t;

extern int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void NetStop (Net_t *serv);

#endif
