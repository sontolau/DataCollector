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
    NET_TYPE_TCP = 0,
    NET_TYPE_UDP = 1,
};


#ifndef S_ERROR
#define S_ERROR   (0)
#endif

#ifndef S_OK
#define S_OK  (!S_ERROR)
#endif

typedef struct _NetInfo {
    int net_type;
    unsigned short net_port;
    unsigned int   net_addr;
} NetInfo_t;


typedef struct _NetIO {
    int io_fd;
    struct ev_io io_ev;
    NetInfo_t    io_net;
    DC_link_t    __buf_link;
} NetIO_t;


typedef struct _NetBuffer {
    NetIO_t      io;
    NetIO_t      *netio;
    unsigned int buffer_size;
    unsigned int buffer_length;
    DC_link_t    __link;
    union {
        unsigned char buffer[0];
    };
} NetBuffer_t;

typedef struct _NetConfig {
    char *chdir;
    int  daemon;
    int  max_peers;
    int  num_net_io;
    int  max_buffers;
    unsigned int buffer_size;
    int  queue_size;
    unsigned int timer_interval;
    int  max_idle_time;
} NetConfig_t;

typedef struct _Net {
    NetIO_t  *net_io;
    struct ev_loop *ev_loop;
    
    DC_buffer_pool_t net_buffer_pool;
    DC_buffer_pool_t net_io_pool;

    DC_queue_t     request_queue;
    DC_queue_t     reply_queue;

    HDC            sig_handle;
    unsigned int   timer;
    NetConfig_t    *config;
    struct _NetDelegate  *delegate;
    void           *private_data;
    pthread_rwlock_t serv_lock;
} Net_t;


#define NetSetUserData(srv, data) do{srv->private_data = data;}while(0);
#define NetGetUserData(srv)          (srv->private_data)

extern NetBuffer_t *NetAllocIOBuffer (Net_t *srv, NetIO_t *io);
extern void NetFreeIOBuffer (Net_t *srv, NetBuffer_t *buf);
extern void NetCommitIOBuffer (Net_t *srv, NetIO_t *io);

enum {
    RES_IO    = 1,
    RES_BUFFER = 2,
};

typedef struct _NetDelegate {
    void (*getNetInfoWithIndex) (Net_t *srv, NetInfo_t *net, int index);
    int (*timerout) (Net_t *srv, unsigned int timer);
    int  (*willInitNet) (Net_t *srv);
    void (*willRunNet) (Net_t *srv);
    void (*willCloseNetIO) (Net_t *srv, NetIO_t *io);
    void  (*processRequest) (Net_t *srv, NetBuffer_t *buf);
    void (*willStopNet) (Net_t *srv);
    void (*resourceUsage) (Net_t *srv, int type, float percent);
} NetDelegate_t;

extern int NetRun (Net_t *serv, NetConfig_t *config, NetDelegate_t *delegate);
extern void NetStop (Net_t *serv);

#endif
