#ifndef _PNET_H
#define _PNET_H

#include "libdc.h"

#include <ev.h>


typedef struct _NetKitConfig {
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
} NetKitConfig_t;

#define WATCH_DOG(_delay, _timeout) (((_delay&0xFF)<<8)|(_timeout&0xFF))


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

struct _QueueManager {

} QueueManager_t;

typedef struct _NetKit {
    NetIO_t  *net_io;
    struct ev_loop *ev_loop;
    int            status; //read only
    struct {
        DC_buffer_pool_t net_buffer_pool;
        DC_buffer_pool_t net_io_pool;
        DC_mutex_t       buf_lock;
    };

    NetAddrInfo_t      *net_addr_array;
    NetQueueManager_t     *proc_queue_map;
    NetQueueManager_t     reply_queue;

    DC_thread_t    conn_checker;
    unsigned int   timer;
    NetConfig_t    *config;
    struct _NetDelegate  *delegate;
    void           *private_data;
    DC_mutex_t     PRI (serv_lock);
} NetKit_t;

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

//extern void NetCommitIO (Net_t *net, const NetIO_t *io);

enum {
    RES_IO    = 1,
    RES_BUFFER = 2,
};

typedef struct _NetDelegate {
    void (*getNetAddressInfo)(Net_t *self, NetAddrInfo_t *net, int index);
    int  (*willInitNet) (Net_t *self);
    void (*didConnectToHost) (Net_t *self, NetIO_t *remote);
    void (*didBindToLocal) (Net_t *self, NetIO_t *local);
    unsigned int (*getHashQueue) (Net_t *self, NetBuffer_t *buf);

    int  (*willAcceptRemote) (Net_t *self, const NetIO_t *local, NetIO_t *remote);
    void (*didReadData) (Net_t *self, NetIO_t *from, NetBuffer_t *buf);
    int (*didWriteData) (Net_t *self, NetIO_t *to, NetBuffer_t *buf,int errcode);
    int  (*processData) (Net_t *self, const NetIO_t *io, NetBuffer_t *buf);
    void (*didReceiveTimer) (Net_t *self, unsigned int count);
    void (*willCloseRemote) (Net_t *self, NetIO_t *remote);

    void (*willCloseNet) (Net_t *self);
} NetDelegate_t;

extern int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void NetStop (Net_t *serv);
extern void NetWrite (Net_t *net, const NetBuffer_t *buf);

#endif
