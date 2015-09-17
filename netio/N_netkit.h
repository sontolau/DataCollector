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
    void *pointer;
    unsigned char buffer[0];
} NKBuffer;

enum {
    NK_EV_RD = 1<<0,
    NK_EV_WR = 1<<1,
    NK_EV_ACCEPT = 1<<2,
};

typedef struct _NKPeer {
    OBJ_EXTENDS (DC_object_t);
    NetIO_t *io;
    struct _NKPeer *to;
    struct ev_io ev_io;
    unsigned long last_counter;
    DC_list_t sub_peers;
    DC_list_elem_t handle;
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
    DC_buffer_pool_t peer_pool;
    DC_buffer_pool_t buffer_pool;
    DC_queue_t  request_queue;
    DC_queue_t  reply_queue;
    DC_thread_t proc_thread;
    DC_thread_t reply_thread;
    DC_task_manager_t  task_manager;
    unsigned int counter;
    NKConfig   *config;
    NKDelegate *delegate;
    void        *private_data;
    DC_locker_t  locker;
} NetKit;

extern int NK_init (NetKit *nk, const NKConfig *config, const NKDelegate *delegate);
extern void NK_set_userdata (NetKit *nk, const void *data);
extern void *NK_get_userdata (NetKit *nk);
extern void NK_set_netio (NetKit *nk, NetIO_t *io, int ev);
extern int NK_run (NetKit *nk);
extern void NK_stop (NetKit *nk);
extern void NK_close_peer (NetKit *nk, NKPeer *peer);
extern NKBuffer *NK_buffer_alloc (NetKit *kit);
extern NKBuffer *NK_buffer_get (NKBuffer *buf);

extern void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer);
extern void NK_buffer_commit (NetKit *kit, NKPeer *peer, NKBuffer *buf);
extern void NK_buufer_commit_bulk (NetKit *kit, NKPeer *peer, NKBuffer **buf, int num);
extern void NK_buffer_free (NetKit *kit, NKBuffer *buf);

extern NKPeer *NK_peer_get (NKBuffer *buf);
#endif
