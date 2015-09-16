#ifndef _N_BUFFER_H
#define _N_BUFFER_H

#include "N_io.h"
#include <libdc/libdc.h>

typedef struct _NetBuffer {
    NetIO_t      *io;
    unsigned int tag;
    unsigned int size;
    unsigned int data_length;

    DC_link_t       PRI(io_link);
    NetSockAddr_t   PRI(sock_addr);

    union {
        int   int_value;
        void *pointer;
        unsigned char buffer[0];
    };
} NetBuffer_t;

extern NetBuffer_t *NetBufferAlloc (long size);

extern void NetBufferFree (NetBuffer_t *buf);

extern void NetBufferClear (NetBuffer_t *buf);

extern NetBuffer_t *NetBufferInitWithTag (NetBuffer_t *buf, 
                                          NetSockAddr_t *addr,
                                          void *data,
                                          long size,
                                          int tag);

extern NetSockAddr_t *NetBufferGetAddrInfo(const NetBuffer_t *buf);

extern void NetBufferAttachIO (NetBuffer_t *buf,
                               NetIO_t *io);

extern void NetBufferDettachIO (NetBuffer_t *buf);

#define NetBufferInit(_buf, _io, _addr, _data, _size) \
        NetBufferInitWithTag(_buf, _io, _addr, _data, _size, 0)
#endif 
