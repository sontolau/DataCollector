#ifndef _NETIO_IO_H
#define _NETIO_IO_H

#include "../libdc.h"
#include <gnutls/gnutls.h>

enum {
    NET_TCP = 0,
    NET_UDP = 1,
};

typedef struct _NetAddrInfo {
    
#define NET_F_IPv6  (1<<1)
#define NET_F_SSL   (1<<2)

    int flag;
    int type;
    /*
     * if net_port is zero, a unix type socket will be created.
     * the net_addr field must be the path.
     */
    unsigned short port;
#define MAX_NET_ADDR_LEN   108
    char  *address;
    struct {
        char *ca;
        char *cert;
        char *key;
        int  verify_peer;
        char *crl;
        int (*verify_func)();
    } ssl;

} NetAddrInfo_t;

typedef struct _INetAddress {
    union {
        struct sockaddr_un      su;
        struct sockaddr_in      s4;
        struct sockaddr_in6     s6;
        struct sockaddr_storage ss; 
    } addr;
    
    socklen_t addrlen;
    NetAddrInfo_t *addr_info;
} INetAddress_t;

extern void INetAddressFrom (INetAddress_t *addr, const NetAddrInfo_t *info);

typedef struct _NetIO {
    int          fd;
    struct {
        gnutls_certificate_credentials_t x509_cred;
        gnutls_priority_t  priority_cache;
        gnutls_session_t   session;
    } ssl;
    
    INetAddress_t inet_addr;
    void        *private_data;
} NetIO_t;

enum {
    NET_IO_CTRL_BIND        =   1,
    NET_IO_CTRL_LISTEN      =   2,
    NET_IO_CTRL_CONNECT     =   3,
    NET_IO_CTRL_ACCEPT,
    NET_IO_CTRL_SET_OPTS,
    NET_IO_CTRL_GET_OPTS,
    NET_IO_CTRL_READ,
    NET_IO_CTRL_WRITE,
};

typedef struct _NetBuf {
    unsigned int  size;
    unsigned char *data;
    INetAddress_t inet_address;
} NetBuf_t;

#define NetBufSetData(_nbuf, _buf, _szbuf)\
do {\
    _nbuf.size   = _szbuf;\
    _nbuf.data   = _buf;\
} while (0)

#define NetBufSetInetAddress(_nbuf, _inet) (_nbuf.inet_address = _inet)

typedef struct _NetSockOptions {
    int level;
    int name;
    void *optval;
    socklen_t optlen;
} NetSockOption_t;

extern int      NetIOInit (NetIO_t *io,
                           const NetAddrInfo_t *addr,
                           int servflag);

extern long NetIOCtrl (NetIO_t *io, int cmd, void *arg, long szarg);

#define NetIOConnect(_io) NetIOCtrl(_io, NET_IO_CTRL_CONNECT, NULL, 0)

#define NetIOBind(_io) NetIOCtrl(_io, NET_IO_CTRL_BIND, NULL, 0)

#define NetIOListen(_io,_backlog) NetIOCtrl(_io, NET_IO_CTRL_LISTEN, _backlog, sizeof (int)) 

#define NetIOAccept(_io, _newio) NetIOCtrl(_io, NET_IO_CTRL_ACCEPT, _newio, sizeof (NetIO_t))

#define NetIORead(_io, _buf) NetIOCtrl(_io, NET_IO_CTRL_READ, _buf, sizeof (NetBuf_t))

#define NetIOWrite(_io, _buf) NetIOCtrl(_io, NET_IO_CTRL_WRITE, _buf, sizeof (NetBuf_t))

#define NetIOSetOpts(_io, _opts, _num) NetIOCtrl(_io, NET_IO_CTRL_SET_OPTS, _opts, _num)

#define NetIOGetOpts(_io, _opts, _num) NetIOCtrl(_io, NET_IO_CTRL_GET_OPTS, _opts, _num)

#define NetIOSetUserData(_io, _data) do {_io->private_data = _data;}while (0)

#define NetIOGetUserData(_io)        (_io->private_data)

extern void NetIODestroy (NetIO_t *io);


#endif

