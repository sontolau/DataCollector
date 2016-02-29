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
	DC_link_t __link_ptr;
    void (*__ev_cb) (struct _DC_io*, int, struct _DC_iobuf**, void*);
} DC_io_t;

extern int DC_io_init (DC_io_t*, int fd, void *user, void (*ev_cb)(DC_io_t*, int,struct _DC_iobuf**, void*));
#define DC_io_set_callback(_io, _evcb) (_io->__ev_cb = _evcb)
#define DC_io_userdata(io) (io->priv_data)
extern void DC_io_destroy (DC_io_t*);


typedef struct _DC_iobuf {
	DC_OBJ_EXTENDS (DC_object_t);
	DC_io_t   *__io;
	void *__bufptr;
	unsigned int __bufsize;
} DC_iobuf_t;

extern int DC_iobuf_init (DC_iobuf_t*, DC_io_t *io, void *buf, unsigned int bufsz);
extern void DC_iobuf_destroy (DC_iobuf_t*);

#define DC_iobuf_get_buffer(_iobuf) ((_iobuf)->__bufptr)
#define DC_iobuf_get_size(_iobuf)   ((_iobuf)->__bufsize)
#define DC_iobuf_get_io(_iobuf)     ((_iobuf)->__io)

typedef struct _DC_io_task {
	DC_OBJ_EXTENDS (DC_object_t);
#ifdef ENABLE_LIBEV
	struct ev_loop *ev_loop;
//	int __timer_in_sec;
//	int __check_interval;
#elif defined (ENABLE_EPOLL)
	int epollfd;
#else
	fd_set selectfd;
	int __max_fds;
#endif
	int __pipes[2];
	volatile int __flag;
	DC_link_t __link;

	DC_task_queue_t __reader_tasks;
	DC_task_queue_t __process_tasks;
	DC_task_queue_t __writer_tasks;
} DC_io_task_t;

enum {
	IO_CTL_ADD = 1,
	IO_CTL_DEL = 2,
	IO_CTL_MOD = 3,
};

extern int DC_io_task_init (DC_io_task_t *io,
		             int szreader,
		             int numreaders,
		             int szprocs,
		             int numprocs,
		             int szwriter,
		             int numwriters);

extern int DC_io_task_start (DC_io_task_t *io,
		              int check_interval);

extern int DC_io_task_ctl (DC_io_task_t *io,
        int op,
        DC_io_t *ionode, void *);

extern void DC_io_task_stop (DC_io_task_t *io);



extern void DC_io_task_destroy (DC_io_task_t*);

#endif //_DC_IO_H
