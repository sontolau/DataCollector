#ifndef _NETIO_KIT_CORE_H
#define _NETIO_KIT_CORE_H

#include "N_io.h"
#include <libdc/list.h>
#include <libdc/thread.h>
#include <libdc/queue.h>
#include <libdc/object.h>
#include <libdc/buffer.h>

#include <ev.h>

struct _NKPeer;
typedef struct _NKBuffer {
    DC_OBJ_EXTENDS (DC_object_t);
    struct _NKPeer *peer;
    NetBuf_t iobuf;
    int  tag;
    long size;
    long length;
    unsigned char buffer[0];
} NKBuffer;

extern int NK_buffer_init (NKBuffer *buf, int szbuf);
extern void NK_buffer_destroy (NKBuffer *buf);
extern void NK_buffer_remove_peer (NKBuffer *buf);
extern void NK_buffer_set_peer (NKBuffer *buf, struct _NKPeer *peer);

enum {
    NK_EV_READ    = 1 << 0,
    NK_EV_WRITE   = 1 << 1,
    NK_EV_ACCEPT  = 1 << 2,
    NK_EV_RDWR    = 3,
};

enum {
    NK_CLIENT_PEER = 1,
    NK_SERVER_PEER = 2,
};

typedef struct _NKPeer {
    DC_OBJ_EXTENDS (DC_object_t);
    struct ev_io ev_io;
    int server; //server or client.
    NetAddrInfo_t addr;
    NetIO_t io;
    unsigned int last_update;
    unsigned long total_bytes;
    double        byte_rate;
    DC_object_t   *object;
    union {
        int int_value;
        void *pointer_value;
    };
} NKPeer;

extern int NK_peer_init (NKPeer*, const NetAddrInfo_t*, int type);
extern void NK_peer_destroy (NKPeer*);
#define NK_peer_get_object(peer) (peer->object)
//#define NK_peer_set_object(peer, obj) (peer->object = DC_object_get((DC_object_t*)obj))
#define NK_peer_set_int(peer, intval) (peer->int_value = intval)
#define NK_peer_set_pointer(peer, pt) (peer->pointer_value = pt)
#define NK_peer_get_int(peer) (peer->int_value)
#define NK_peer_get_pointer(peer) (peer->pointer_value)


typedef struct _NKConfig {
    char *chdir;
    char *log;
    char *pidfile;
    int  daemon;
    int debug;
    int  max_sockbufs;
    unsigned int max_sockbuf_size;
    int max_peers;
    int read_queue_size;
    int outgoing_queue_size;
    int incoming_queue_size;
//    int num_readers;
//    int num_writers;
    int num_processors;
    unsigned int  watch_dog;
//    double memory_warn_percent;
//    double conn_warn_percent;
//    double request_busy_percent;
//    double reply_busy_percent;
} NKConfig;

#define WATCH_DOG(_delay, _timeout) (((_delay&0xFFFF)<<16)|(_timeout&0xFFFF))
#define WATCH_DOG_DELAY(_dt)  ((_dt&0xFFFF0000)>>16)
#define WATCH_DOG_TIMEOUT(_dt) (_dt&0xFFFF)

struct _NetKit;
typedef struct _NKDelegate {
    void (*didReceiveTimer) (struct _NetKit*);
    void (*didAcceptRemoteClient) (struct _NetKit*, NKPeer*, NKPeer *new);
    void (*didFailureToReceiveData) (struct _NetKit*, NKPeer *peer);
    int  (*didSuccessToReceiveData)(struct _NetKit*, NKPeer *peer, NKBuffer *buf);
    void (*didSuccessToSendData) (struct _NetKit*, NKPeer *peer, NKBuffer *buf);
    void (*didReceiveDataTimedout) (struct _NetKit*, NKPeer*);
    void (*didFailureToSendData) (struct _NetKit*, NKPeer *peer, NKBuffer *buf);
    void (*processData) (struct _NetKit*, NKPeer*, NKBuffer*);
    void (*didReceiveSignal) (struct _NetKit*, int);
//
//    NKPeer* (*allocPeerWithInit) (struct _NetKit*);
//    NKBuffer* (*allocBufferWithInit) (struct _NetKit*);

} NKDelegate;

typedef struct _NetKit {
    DC_list_t      peer_set;
    //int            pfds[2];
    struct ev_loop *ev_loop;
    int running;
    ev_signal *sig_map;
    DC_thread_t watch_dog;
    DC_task_queue_t incoming_tasks;
    DC_task_queue_t outgoing_tasks;
//    struct {
//    	DC_buffer_pool_t peer_pool;
//    	DC_buffer_pool_t buffer_pool;
//    };
    unsigned int counter;
    NKConfig   *config;
    void        *private_data;
    DC_locker_t  locker;
    NKDelegate  *delegate;

    unsigned int __rd_bytes;
    unsigned int __proc_bytes;
    unsigned int __wr_bytes;
} NetKit;


extern int NK_init (NetKit *nk, const NKConfig *config);
extern void NK_set_delegate (NetKit *nk, NKDelegate *delegate);
extern void NK_set_userdata (NetKit *nk, const void *data);
extern void NK_set_signal (NetKit *nk, int);
extern void NK_remove_signal (NetKit *nk ,int);
extern void *NK_get_userdata (NetKit *nk);
extern void NK_print_usage (const NetKit *nk);
extern void NK_add_peer (NetKit *nk, NKPeer *peer, int ev);
extern void NK_remove_peer (NetKit *nk, NKPeer *peer);
extern void NK_start_peer (NetKit *nk, NKPeer *peer);
extern void NK_stop_peer (NetKit *nk, NKPeer *peer);
extern int NK_commit_buffer (NetKit *kit, NKBuffer *buf);
extern int NK_commit_bulk_buffers (NetKit *kit, NKBuffer **buf, int num);
extern int NK_run (NetKit *nk);
extern void NK_stop (NetKit *nk);
extern void NK_destroy (NetKit *nk);
extern NKPeer *NK_alloc_peer_with_init (NetKit *nk, NetAddrInfo_t *addr, int type);
extern NKBuffer *NK_alloc_buffer_with_init (NetKit *nk);
/*
extern void *NK_malloc (NetKit *nk, int *sz);
extern void NK_memcpy (NetKit *nk, void *dest, void *src, int ncpy);
extern void NK_free (NetKit *nk, void *buf);
*/
#endif
