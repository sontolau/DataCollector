#include "socket.h"

DC_INLINE int __v4_set_options (DC_socket_t *sk, int op, void *arg, int szarg)
{
	DC_sockopt_t *optptr = NULL;
	int i;

	switch (op) {
	case SOCK_CTL_BIND:
		return bind (sk->super.fd,
				     &sk->sockaddr.sa,
				     sizeof (sk->sockaddr.socklen));
	case SOCK_CTL_SET_OPT: {
		optptr = (DC_sockopt_t*)arg;
		for (i=0; i<(int)(szarg/sizeof (DC_sockopt_t)); i++) {
			setsockopt (sk->super.fd,
					    optptr[i].level,
					    optptr[i].name,
					    optptr[i].optval,
					    optptr[i].optlen);
		}
	} break;
	case SOCK_CTL_GET_OPT: {
		optptr = (DC_sockopt_t*) arg;
		for (i = 0; i < (int) (szarg / sizeof(DC_sockopt_t)); i++) {
			getsockopt(sk->super.fd,
					   optptr[i].level,
					   optptr[i].name,
					   optptr[i].optval,
					   &optptr[i].optlen);
		}
	} break;
	case SOCK_CTL_CONNECT:
		return connect (sk->super.fd,
				        &sk->sockaddr.sa,
				        sizeof (sk->sockaddr.socklen));
		break;
	}
	return 0;
}

DC_INLINE int udpv4_open (DC_socket_t *sk, const char *addr, int port)
{
	int skfd;

    if (port == 0) {
        sk->sockaddr.su.sun_family = AF_UNIX;
        strncpy (sk->sockaddr.su.sun_path,
        		 addr,
        		 sizeof (sk->sockaddr.su.sun_path));
        sk->sockaddr.socklen = sizeof (sk->sockaddr.su);
        skfd = socket (AF_UNIX, SOCK_DGRAM, 0);
    } else {
    	sk->sockaddr.s4.sin_family = AF_INET;
    	sk->sockaddr.s4.sin_addr.s_addr   = inet_addr (addr);
    	sk->sockaddr.s4.sin_port   = htons (port);
    	sk->sockaddr.socklen = sizeof (sk->sockaddr.s4);
    	skfd = socket (AF_INET, SOCK_DGRAM, 0);
    }

    if (skfd < 0) return -1;
    return DC_io_init ((DC_io_t*)sk, skfd, NULL, NULL);
}

DC_INLINE int udpv4_ctl (DC_socket_t *sk, int op, void *arg, int szarg)
{
	DC_sockbuf_t *skbuf = NULL;

	switch (op) {
	case SOCK_CTL_LISTEN:
		return 0;
	case SOCK_CTL_ACCEPT:
		return -1;
	case SOCK_CTL_RECV: {
		skbuf = (DC_sockbuf_t*)arg;
		skbuf->sockaddr.socklen = sizeof (skbuf->sockaddr.addr);
		return recvfrom (sk->super.fd,
				         DC_iobuf_get_buffer((DC_iobuf_t*)skbuf),
				         DC_iobuf_get_size ((DC_iobuf_t*)skbuf),
				         0,
				         &skbuf->sockaddr.sa,
				         &skbuf->sockaddr.socklen);
	} break;
	case SOCK_CTL_SEND:
		skbuf = (DC_sockbuf_t*)arg;
		skbuf->sockaddr.socklen = sizeof (skbuf->sockaddr.addr);
		return sendto (sk->super.fd,
				       DC_iobuf_get_buffer((DC_iobuf_t*)skbuf),
				       DC_iobuf_get_size ((DC_iobuf_t*)skbuf),
				       0,
				       &skbuf->sockaddr.sa,
				       skbuf->sockaddr.socklen);
		break;
	default:
		return __v4_set_options (sk, op, arg, szarg);
		break;
	}
	return 0;
}

DC_INLINE void udpv4_close (DC_socket_t *sk)
{
	close (sk->super.fd);
}

DC_INLINE int tcpv4_open (DC_socket_t *sk, const char *addr, int port)
{
	int skfd;
    if (port == 0) {
        sk->sockaddr.su.sun_family = AF_UNIX;
        strncpy (sk->sockaddr.su.sun_path,
        		 addr,
        		 sizeof (sk->sockaddr.su.sun_path));
        sk->sockaddr.socklen = sizeof (sk->sockaddr.su);
        skfd = socket (AF_UNIX, SOCK_STREAM, 0);
    } else {
    	sk->sockaddr.s4.sin_family = AF_INET;
    	sk->sockaddr.s4.sin_addr.s_addr   = inet_addr (addr);
    	sk->sockaddr.s4.sin_port   = htons (port);
    	sk->sockaddr.socklen = sizeof (sk->sockaddr.s4);
    	skfd = socket (AF_INET, SOCK_STREAM, 0);
    }

    if (skfd < 0) return -1;
    return DC_io_init ((DC_io_t*)sk, skfd, NULL, NULL);
}

DC_INLINE int tcpv4_ctl (DC_socket_t *sk, int op, void *arg, int szarg)
{
	DC_sockbuf_t *skbuf = NULL;
    DC_socket_t  *newsk = NULL;
	switch (op) {
	case SOCK_CTL_LISTEN:
		return listen (sk->super.fd, *(int*)arg);
	case SOCK_CTL_ACCEPT: {
		newsk = (DC_socket_t*)arg;
		newsk->sockaddr.socklen = sizeof (newsk->sockaddr.addr);
		return (newsk->super.fd = accept (sk->super.fd,
				                          &newsk->sockaddr.sa,
				                          &newsk->sockaddr.socklen));
	} break;
	case SOCK_CTL_RECV: {
		skbuf = (DC_sockbuf_t*)arg;
		return recv     (sk->super.fd,
				         DC_iobuf_get_buffer((DC_iobuf_t*)skbuf),
				         DC_iobuf_get_size ((DC_iobuf_t*)skbuf),
				         0);
	} break;
	case SOCK_CTL_SEND:
		skbuf = (DC_sockbuf_t*)arg;
		return send   (sk->super.fd,
				       DC_iobuf_get_buffer((DC_iobuf_t*)skbuf),
				       DC_iobuf_get_size ((DC_iobuf_t*)skbuf),
				       0);
		break;
	default:
		return __v4_set_options (sk, op, arg, szarg);
		break;
	}
	return 0;
}

DC_INLINE void tcpv4_close (DC_socket_t *sk)
{
	close (sk->super.fd);
}


DC_INLINE int udpv6_open (DC_socket_t* sk, const char *addr, int port)
{
	return -1;
}

DC_INLINE int udpv6_ctl (DC_socket_t *sk, int op, void *arg, int szarg)
{
	return -1;
}

DC_INLINE void udpv6_close (DC_socket_t *sk)
{
}

DC_INLINE int tcpv6_open (DC_socket_t* sk, const char *addr, int port)
{
	return -1;
}

DC_INLINE int tcpv6_ctl (DC_socket_t *sk, int op, void *arg, int szarg)
{
	return -1;
}

DC_INLINE void tcpv6_close (DC_socket_t *sk)
{
}

static DC_sockproto_t DEFAULT_PROTO_HANDLER[] = {
    [0] = {
        NULL,
        NULL,
        NULL,
    },
    [PROTO_UDP] = {
        udpv4_open,
        udpv4_ctl,
        udpv4_close,
    },
    [PROTO_TCP] = {
        tcpv4_open,
        tcpv4_ctl,
        tcpv4_close,
    },
    [PROTO_UDPv6] = {
        udpv6_open,
        udpv6_ctl,
        udpv6_close,
    },
    [PROTO_TCPv6] = {
        tcpv6_open,
        tcpv6_ctl,
        tcpv6_close,
    },
};


int DC_socket_init (DC_socket_t *sk, int proto, const char *addr, int port, DC_sockproto_t* handler)
{
    if (handler) {
        sk->__handler_ptr = handler;
    } else {
        sk->__handler_ptr = &DEFAULT_PROTO_HANDLER[proto];
    }

    return sk->__handler_ptr->open (sk, addr, port);
}

int DC_socket_ctl (DC_socket_t *sk, int op, void *arg, int sz)
{
    return sk->__handler_ptr->contrl(sk, op, arg, sz);
}

void DC_socket_destroy (DC_socket_t *sk)
{
    sk->__handler_ptr->close (sk);
}
