#ifndef _NETIO_KIT_CORE_H
#define _NETIO_KIT_CORE_H

#include "N_io.h"
#include <libdc/list.h>
#include <libdc/thread.h>
#include <libdc/queue.h>
#include <libdc/object.h>
#include <libdc/buffer.h>

#include <ev.h>

enum {
    NK_NoError = 0,
};

typedef int NKError_t;

struct _NKPeer;
typedef struct _NKBuffer {
    DC_OBJ_EXTENDS (DC_object_t);
    struct _NKPeer *peer;
    int  tag;
    long skbuf_size;
    NetBuf_t skbuf;
    long length;
    unsigned char buffer[0];
} NKBuffer;

enum {
    NK_EV_READ    = 1,
    NK_EV_WRITE,
    NK_EV_ACCEPT,
};

#define MAX_PEER_NAME 255

typedef struct _NKPeer {
    DC_OBJ_EXTENDS (DC_object_t);
    NetIO_t *io;
    struct _NKPeer *to;
    struct ev_io ev_io;
    time_t last_counter;
    char   name[MAX_PEER_NAME+1];
    DC_list_t sub_peers;
    DC_list_elem_t handle;
} NKPeer;

typedef struct _NKConfig {
    char *chdir;
    char *log;
    char *pidfile;
    int  daemon;
    int  num_sockbufs;
    unsigned int max_sockbuf_size;
    int num_sock_conns;
    int  queue_size;
    unsigned int  watch_dog;
    double memory_warn_percent;
    double conn_warn_percent;
    double request_busy_percent;
    double reply_busy_percent;
} NKConfig;

#define WATCH_DOG(_delay, _timeout) (((_delay&0xFFFF)<<16)|(_timeout&0xFFFF))
#define WATCH_DOG_DELAY(_dt)  ((_dt&0xFFFF0000)>>16)
#define WATCH_DOG_TIMEOUT(_dt) (_dt&0xFFFF)

#ifndef FAST_CALL
#define FAST_CALL
#endif

struct _NetKit;
typedef struct _NKDelegate {
    void (*didReceiveTimer) (struct _NetKit*);
    void (*willAcceptRemotePeer) (struct _NetKit*, NKPeer *local, NKPeer *remote);
    void (*willClosePeer) (struct _NetKit*, NKPeer *peer);
    void (*didSuccessToReceiveData) (struct _NetKit*, NKPeer*, NKBuffer*);
    void (*didFailToReceiveData) (struct _NetKit*, NKPeer*);
    FAST_CALL void (*processData) (struct _NetKit*, NKPeer*, NKBuffer*);
    void (*didSuccessToSendData) (struct _NetKit*, NKPeer*, NKBuffer*);
    void (*didFailToSendData) (struct _NetKit*, NKPeer*, NKBuffer*);
    void (*didReceiveSignal) (struct _NetKit*, int);
    void (*didReceiveRequestBusyWarning) (struct _NetKit*, double);
    void (*didReceiveReplyBusyWarning) (struct _NetKit*, double);
    void (*didReceiveMemoryWarning) (struct _NetKit*, double);
    void (*didReceiveConnectionWarning) (struct _NetKit*, double);
} NKDelegate;

typedef struct _NetKit {
    DC_list_t      infaces;
    struct ev_loop *ev_loop;
    DC_buffer_pool_t peer_pool;
    DC_buffer_pool_t buffer_pool;
    DC_buffer_pool_t io_pool;
    DC_queue_t  request_queue;
    DC_queue_t  reply_queue;
    DC_thread_t checker_thread;
    DC_thread_t proc_thread;
    DC_thread_t reply_thread;
    unsigned int counter;
    NKConfig   *config;
    void        *private_data;
    DC_locker_t  locker;
    NKDelegate  *delegate;
} NetKit;

extern int NK_init (NetKit *nk, const NKConfig *config);
extern void NK_set_delegate (NetKit *nk, NKDelegate *delegate);
extern void NK_set_userdata (NetKit *nk, const void *data);
extern void *NK_get_userdata (NetKit *nk);
extern int  NK_add_netio (NetKit *nk, NetIO_t *io, int ev);
extern void NK_remove_netio (NetKit *nk, NetIO_t *io);
extern int NK_commit_buffer (NetKit *kit, NKBuffer *buf);
extern int NK_commit_bulk_buffers (NetKit *kit, NKBuffer **buf, int num);
extern int NK_run (NetKit *nk);
extern void NK_stop (NetKit *nk);
extern void NK_destroy (NetKit *nk);
extern void NK_close_peer (NetKit *nk, NKPeer *peer);
extern NKBuffer *NK_alloc_buffer (NetKit *kit);
extern NKBuffer *NK_buffer_get (NKBuffer *buf);

extern INetAddress_t *NK_buffer_get_inet_addr (NKBuffer *buf);
extern void NK_buffer_set_inet_addr (NKBuffer *buf, INetAddress_t *addr);

#define NK_buffer_get_inet_addr(_buf) (&_buf->skbuf.inet_address)
#define NK_buffer_get_data(_buf)        (_buf->skbuf.data)
#define NK_buffer_get_length(_buf)      (_buf->length)
#define NK_buffer_get_size(_buf)        (_buf->skbuf.size)
#define NK_buffer_set_data(_buf, _data, _szdata) \
do {\
    _buf->skbuf.size = (_szdata>_buf->skbuf_size?_buf->skbuf_size:_szdata);\
    memcpy (_buf->buffer, _data, _buf->skbuf.size);\
} while (0)

#define NK_buffer_set_inet_addr(_buf, _inet) NetBufSetInetAddress(((NetBuf_t*)&_buf->skbuf), _inet)

extern void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer);

extern void NK_free_buffer (NetKit *kit, NKBuffer *buf);

extern NKPeer *NK_alloc_peer (NetKit *kit);
extern NKPeer *NK_peer_get (NKPeer *peer);
#define NK_peer_set_name(_peer, _name) \
do {\
    if (_name) {\
        strncpy (_peer->name, (char*)_name, MAX_PEER_NAME);\
    }\
} while (0)

#define NK_peer_get_name(_peer) (_peer->name)

extern void NK_free_peer (NetKit *kit, NKPeer *peer);

#endif
