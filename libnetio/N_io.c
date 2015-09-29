#include "N_io.h"
#include "N_util.c"
#include "N_tcp.c"
#include "N_udp.c"

struct _NetIOHandler {
    int (*ioOpen) (struct _NetIO*, int server);
    long (*ioCtrl) (struct _NetIO*, int type, void *arg, long szarg);
    void (*ioClose) (struct _NetIO*);
} __proto_handler_map[] = {
    [NET_TCP] = {
        .ioOpen            = tcpOpenIO,
        .ioCtrl            = tcpCtrlIO,
        .ioClose           = tcpCloseIO,
    },

    [NET_UDP] = {
        .ioOpen            = udpOpenIO,
        .ioCtrl            = udpCtrlIO,
        .ioClose           = udpCloseIO,
    }
};

#define __N_OpenIO(proto, io, type)            __proto_handler_map[proto].ioOpen (io, type)
#define __N_CtrlIO(proto, io, cmd, arg, szarg) __proto_handler_map[proto].ioCtrl (io, cmd, arg, szarg)
#define __N_CloseIO(proto, io)                 __proto_handler_map[proto].ioClose (io)


int      NetIOInit (NetIO_t *io,
                    const NetAddrInfo_t *addr,
                    int server)
{

    assert (addr!=NULL);
    assert (addr->type == NET_UDP || addr->type == NET_TCP);

    INetAddressFrom (&io->inet_addr, addr);
    if (__N_OpenIO (io->inet_addr.addr_info->type, io, server) < 0) {
        return -1;
    }

    return 0;
}

long NetIOCtrl (NetIO_t *io, int cmd, void *arg, long szarg)
{
    return __N_CtrlIO (io->inet_addr.addr_info->type, io, cmd, arg, szarg);
}

void NetIODestroy (NetIO_t *io)
{
    if (io) {
        __N_CloseIO (io->inet_addr.addr_info->type, io);
    }
}
/*
void NetBufInit (NetBuf_t *buf, void *data, long szdata, const NetAddrInfo_t *info)
{
    buf->bufptr = data;
    buf->szbuf  = szdata;
    INetAddressFrom (&buf->sockaddr, info);
}
*/
void INetAddressFrom (INetAddress_t *addr, const NetAddrInfo_t *info)
{
    memset (addr, '\0', sizeof (INetAddress_t));

    addr->addr_info = (NetAddrInfo_t*)info;
    if (info->port == 0) {
        addr->addr.su.sun_family = AF_UNIX;
        strncpy (addr->addr.su.sun_path, info->address, UNIX_PATH_MAX);
        addr->addrlen = sizeof (addr->addr.su);
    } else {
        if (info->flag & NET_F_IPv6) {
            addr->addr.s6.sin6_family = AF_INET6;
            addr->addr.s6.sin6_port   = htons (info->port);
            //addr->addr.s6.sin6_addr.s6_addr =
            addr->addrlen = sizeof (addr->addr.s6);
        } else {
            addr->addr.s4.sin_family = AF_INET;
            addr->addr.s4.sin_port   = htons (info->port);
            addr->addr.s4.sin_addr.s_addr = inet_addr (info->address);
            addr->addrlen = sizeof (addr->addr.s4);
        }
    }
}
/*
NetIO_t *NetIOAccept (NetIO_t *io)
{
    NetIO_t *newio = NetIOAlloc ();

    if (newio) {
        if (!NetIOInit (newio, io->inet_addr.addr_info)) {
__err_quit:
            NetIORelease (newio);
            return NULL;
        }
        if (__N_AcceptIO (io->inet_addr.addr_info->type, io, newio) < 0) {
            goto __err_quit;
        }

        DC_link_add (&io->io_link, &newio->io_link);
        newio->to = io;
        newio->last_update_time = time (NULL);
    }

    return newio;
}

long NetIORead (NetIO_t *io, void *buf, long szbuf)
{
    szread = __N_ReadIO (io->inet_addr.addr_info->type, io, buf, szbuf);
    if (szread >= 0 && io->to) {
        DC_link_remove (&io->io_link);
        DC_link_add (&io->io_link, &_io->to->io_link);
    }

    return szread;
}

long NetIOWrite (NetIO_t *io, void *buf, long szdata)
{
    szwrite = __N_WriteIO (io->inet_addr.addr_info->type, io, buf, szdata);
    if (szwrite >= 0 && io->to) {
	DC_link_remove (&io->io_link);
        DC_link_add (&io->io_link, &_io->to->io_link);
    }

    return szwrite;
}
*/

