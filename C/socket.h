#ifndef _DC_SOCKET_H
#define _DC_SOCKET_H

#include "libdc.h"
#include "object.h"
#include "io.h"

enum {
    PROTO_UDP = 1,
    PROTO_TCP = 2,
    PROTO_UDPv6,
    PROTO_TCPv6,
};

enum {
    SOCK_CTL_BIND = 1,
    SOCK_CTL_LISTEN = 2,
    SOCK_CTL_ACCEPT = 3,
    SOCK_CTL_SET_OPT,
    SOCK_CTL_GET_OPT,
    SOCK_CTL_CONNECT,
    SOCK_CTL_RECV,
    SOCK_CTL_SEND,
};

typedef struct _DC_sockaddr {
    union {
        struct sockaddr_in  __s4;
        struct sockaddr_in6 __s6;
        struct sockaddr_un  __su;
        struct sockaddr     __sa;
    } addr;
    socklen_t socklen;
} DC_sockaddr_t;

#define s4 addr.__s4
#define s6 addr.__s6
#define sa addr.__sa
#define su addr.__su

struct _DC_socket;
typedef struct _DC_sockbuf {
    DC_OBJ_EXTENDS (DC_iobuf_t);
    struct _DC_socket *socket;
    DC_sockaddr_t sockaddr;
} DC_sockbuf_t;

struct _DC_socket;
typedef struct _DC_sockproto {
    int proto;
    int (*open) (struct _DC_socket*, const char *addr, int port);
    int (*contrl) (struct _DC_socket*, int op, void *arg, int szarg);
    void (*close) (struct _DC_socket*);
}DC_sockproto_t;

typedef struct _DC_socket {
    DC_OBJ_EXTENDS (DC_io_t);
    unsigned int total_recv_bytes;
    unsigned int total_send_bytes;
    int __bind_flag;
    DC_sockaddr_t sockaddr;
    DC_sockproto_t* __handler_ptr;
} DC_socket_t;

#define DC_socket_get_proto(sk)   (sk->__handler_ptr?sk->__handler_ptr->proto:0)
#define DC_socket_is_bound(sk)   (sk->__bind_flag)

typedef struct _DC_sockopt {
    int level;
    int name;
    void *optval;
    socklen_t optlen;    
} DC_sockopt_t;

extern int DC_socket_init (DC_socket_t *socket, int proto, const char *addr, int port, DC_sockproto_t*);
extern int DC_socket_ctl (DC_socket_t *socket, int op, void *arg, int argsz);
#define DC_socket_bind(sk)                  DC_socket_ctl(sk, SOCK_CTL_BIND, NULL, 0)
#define DC_socket_listen(sk, backlog)       DC_socket_ctl(sk, SOCK_CTL_LISTEN, &backlog, sizeof(int))
#define DC_socket_set_option(sk, opts, num) DC_socket_ctl(sk, SOCK_CTL_SET_OPT, opts, num*sizeof (DC_sockopt_t))
#define DC_socket_get_option(sk, opts, num) DC_socket_ctl(sk, SOCK_CTL_GET_OPT, opts, num*sizeof (DC_sockopt_t))
#define DC_socket_connect(sk)               DC_socket_ctl(sk, SOCK_CTL_CONNECT, NULL, 0)
#define DC_socket_accept(sk, newsk)         DC_socket_ctl(sk, SOCK_CTL_ACCEPT, newsk, sizeof (DC_socket_t))
#define DC_socket_send(sk, iobufs)     DC_socket_ctl(sk, SOCK_CTL_SEND, iobufs, sizeof (DC_sockbuf_t))
#define DC_socket_recv(sk, iobufs)     DC_socket_ctl(sk, SOCK_CTL_RECV, iobufs, sizeof (DC_sockbuf_t))
extern void DC_socket_destroy (DC_socket_t *socket);

#endif //_DC_SOCKET_H
