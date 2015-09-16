#ifndef _NETIO_KIT_CORE_H
#define _NETIO_KIT_CORE_H

#include "N_io.h"
#include "../list.h"
#include "../thread.h"
#include "../queue.h"
#include "../object.h"

#include <ev.h>

struct _NKPeer;

typedef struct _NKBuffer {
    OBJ_EXTENDS (DC_object_t);

    struct _NKPeer *peer;
    unsigned int tag;
    unsigned int size;
    unsigned int length;
    NetSockAddr_t sockaddr;
    unsigned char buffer[0];
} NKBuffer;

typedef struct _NKPeer {
    OBJ_EXTENDS (DC_object_t);

    NetIO_t io;
    NetIO_t *to;
    struct ev_io ev_io;
    unsigned long last_counter;
    DC_list_t peer_list;
    DC_list_elem_t list_handle;
} NKPeer;

typedef struct _NKConfig {
    char *chdir;
    char *log;
    char *pidfile;
    int  daemon;
    int  num_sockets;
    int  num_sockbufs;
    unsigned int max_sockbuf_size;
    int num_sock_conns;
    int  num_threads_each_queue;
    int  queue_size;
    int  rw_timeout;
    unsigned int  watch_dog;
} NKConfig;

#define WATCH_DOG(_delay, _timeout) (((_delay&0xFF)<<8)|(_timeout&0xFF))

struct _NetKit;
typedef struct _NKDelegate {
    void (*getAddrInfo) (struct _NetKit*, NetAddrInfo_t *info, int index, int *bind);
    void (*timerExpired) (struct _NetKit*);
    int  (*willInit) (struct _NetKit*);
    void (*didConnectToHost) (struct _NetKit*, NKPeer *remote);
    void (*didBindToLocal) (struct _NetKit*, NKPeer *local);
    void (*didReceiveSignal) (struct _NetKit*, int sig);
    int  (*willAcceptRemote) (struct _NetKit*, NKPeer *local, NKPeer *remote);
    void (*didDisconnectWithRemote) (struct _NetKit*, NKPeer *local, NKPeer *remote);
    void (*didReceiveData) (struct _NetKit*, NKPeer *remote, NKBuffer *buf);
    void (*didFailToReceiveData) (struct _NetKit*, NKPeer *remote);
    void (*processData) (struct _NetKit*, NKPeer *remote, NKBuffer *buf)
    void (*didSendData) (struct _NetKit*, NKPeer *remote, NKBuffer *buf);
    void (*didFailToSendData) (struct _NetKit*, NKPeer *remote, NKBuffer *buf);
    void (*willDestroy) (struct _NetKit*);
} NKDelegate;

typedef struct _NetKit {
    struct ev_loop *ev_core;
    struct {
        DC_buffer_pool_t peers;
        DC_buffer_pool_t buffs;
        DC_locker_t      lock;
    } buffer_pool;

    DC_queue_t  request_queue;
    DC_queue_t  reply_queue;
    DC_task_manager_t  task_manager;
    unsigned int counter;
    NKConfig   *config;
    NKDelegate *delegate;
    void        *private_data;
    DC_locker_t  locker;
} NetKit;

#endif
