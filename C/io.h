/*
 * io.h
 *
 *  Created on: 2016-2-26
 *      Author: sonto
 */


#ifndef _DC_IO_H
#define _DC_IO_H

#include "libdc.h"
#include "object.h"
#include "link.h"
#include "thread.h"

#define ENABLE_LIBEV

#ifdef ENABLE_LIBEV
#include <ev.h>
#elif defined (ENABLE_EPOLL)
#include <sys/epoll.h>
#else
#include <sys/select.h>
#endif

enum {
	EV_IO_READ = 1,
	EV_IO_PROCESS = 2,
	EV_IO_WRITE = 3,
	EV_IO_TIMEDOUT,
};

struct _DC_iobuf;
typedef struct _DC_io {
	DC_OBJ_EXTENDS (DC_object_t);
	int fd;
#ifdef ENABLE_LIBEV
	struct ev_io ev_io;
#elif defined (ENABLE_EPOLL)
	struct epoll_event ev_epoll;
#endif
	void *priv_data;
	DC_link_t __static_link;
	DC_link_t __dynamic_link;
    int (*__ev_cb) (struct _DC_io*, int, struct _DC_iobuf*, void*);
} DC_io_t;

extern int DC_io_init (DC_io_t*);
#define DC_io_set_fd(_io, _fd)         (_io->fd = _fd)
#define DC_io_get_fd(_io)              (_io->fd)
#define DC_io_set_callback(_io, _evcb) (_io->__ev_cb = _evcb)
#define DC_io_get_callback(_io)        (_io->__ev_cb)
#define DC_io_set_userdata(_io, _data) (_io->priv_data = _data)
#define DC_io_userdata(io)             (io->priv_data)
extern void DC_io_destroy (DC_io_t*);


typedef struct _DC_iobuf {
	DC_OBJ_EXTENDS (DC_object_t);
	DC_io_t   *__io;
	void *__bufptr;
	unsigned int __bufsize;
} DC_iobuf_t;

extern int DC_iobuf_init (DC_iobuf_t*);
#define DC_iobuf_set_io(_iobuf, _io) \
do {\
    _iobuf->__io = DC_object_get (_io);\
} while (0)
#define DC_iobuf_get_io(_iobuf) (_iobuf->__io)

#define DC_iobuf_set_buffer(_iobuf, _buf) (_iobuf->__bufptr = _buf)
#define DC_iobuf_set_size(_iobuf, _size) (_iobuf->__bufsize = _size)
#define DC_iobuf_get_buffer(_iobuf)       (_iobuf->__bufptr)
#define DC_iobuf_get_size(_iobuf)  (_iobuf->__bufsize)
extern void DC_iobuf_destroy (DC_iobuf_t*);

typedef struct _DC_io_task {
	DC_OBJ_EXTENDS (DC_object_t);
#ifdef ENABLE_LIBEV
	struct ev_loop *ev_loop;
#elif defined (ENABLE_EPOLL)
	int epollfd;
#else
	fd_set selectfd;
	int __max_fds;
#endif
	int __pipes[2];
	volatile int __flag;
	DC_link_t __static_link;
	DC_link_t __active_link;
	DC_link_t __timedout_link;
//	DC_task_queue_t __reader_tasks;
	DC_task_queue_t __io_incoming_queue;
	DC_task_queue_t __io_outgoing_queue;
} DC_io_task_t;

enum {
	IO_CTL_ADD = 1,
	IO_CTL_DEL = 2,
	IO_CTL_MOD = 3,
};

extern int DC_io_task_init (DC_io_task_t *io,
		                                              int in_queue_size,
		                                              int num_procs,
		                                              int out_queue_size);
//		             int szreader,
//		             int numreaders,
//		             int szprocs,
//		             int numprocs,
//		             int szwriter,
//		             int numwriters);

extern int DC_io_task_start (DC_io_task_t *io,
		              int check_interval);

extern int DC_io_task_ctl (DC_io_task_t *io,
        int op,
        DC_io_t *ionode, void *);

extern void DC_io_task_stop (DC_io_task_t *io);

extern int DC_io_task_commit (DC_io_task_t *io, int ev, DC_iobuf_t *buf);

extern void DC_io_task_destroy (DC_io_task_t*);

#endif //_DC_IO_H
