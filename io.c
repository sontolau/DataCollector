/*
 * io.c
 *
 *  Created on: 2016-2-26
 *      Author: sonto
 */

#include "io.h"

struct io_task_descr {
	DC_io_t *io;
	void    *data;
};


struct io_ctl_arg {
	int op;
	DC_io_t *io;
	void *arg;

} IO_CTL_MAP[]= {
    [0] = {
    	0,
        NULL,
        NULL
    },
	[IO_CTL_ADD] = {
		IO_CTL_ADD,
		NULL,
		NULL
	},
	[IO_CTL_DEL] = {
		IO_CTL_DEL,
		NULL,
		NULL
	},
	[IO_CTL_MOD] = {
		IO_CTL_MOD,
		NULL,
		NULL,
	}
};
#define SZIOARG sizeof(struct io_ctl_arg)

int DC_io_init (DC_io_t* io,
			    int fd,
			    void *user,
			    void (*ev_cb)(DC_io_t*, int,struct _DC_iobuf**, void*))

{
	if (DC_object_init ((DC_object_t*)io)) {
		return -1;
	}

	io->fd  = fd;
//	io->__last_rd = 0;
//	io->__last_wr = 0;
//	io->__rd_timeout = rdtimeo;
//	io->__wr_timeout = wrtimeo;
	io->priv_data = user;
	io->__ev_cb = ev_cb;

#if defined (ENABLE_LIBEV)
	//TODO

#elif defined (ENABLE_EPOLL)
	//TODO
#else
#endif
	return 0;
}

void DC_io_destroy (DC_io_t *io)
{
	DC_object_destroy ((DC_object_t*)io);
}

int DC_iobuf_init (DC_iobuf_t* iobuf, DC_io_t *io, void *buf, unsigned int bufsz)
{
	if (DC_object_init ((DC_object_t*)iobuf)) {
		return -1;
	}

	if (iobuf->__io) {
		DC_object_release ((DC_object_t*)iobuf->__io);
		iobuf->__io = (DC_io_t*)DC_object_get ((DC_object_t*)io);
	}
	iobuf->__bufptr = buf;
	iobuf->__bufsize= bufsz;

	return 0;
}

void DC_iobuf_destroy (DC_iobuf_t *iobuf)
{
	DC_object_destroy ((DC_object_t*)iobuf);

	if (iobuf->__io) {
		DC_object_release ((DC_object_t*)iobuf->__io);
	}
}

DC_INLINE void __do_timeout_check ();
DC_INLINE void __do_read_event ();

#ifdef ENABLE_LIBEV
static void __libev_timer_cb (struct ev_loop *ev, ev_timer *w, int revents)
{
	DC_io_task_t *io = (DC_io_task_t*)ev_userdata (ev);

//	io->__timer_in_sec++;
//	if (!(io->__timer_in_sec & io->__check_interval)) {
		__do_timeout_check (io);
//	}
}

static void __libev_read_cb (struct ev_loop *ev, ev_io *w, int revents)
{
	DC_io_task_t *io = (DC_io_task_t*)ev_userdata (ev);
	DC_io_t      *ioptr = w->data;

	__do_read_event (io, ioptr);
}

#elif defined (ENABLE_EPOLL)
#else
#endif

DC_INLINE int __io_task_add (DC_io_task_t *io,
		            DC_io_t *ionode)
{
#ifdef ENABLE_LIBEV
	ionode->ev_io.data = ionode;
	ev_io_init (&ionode->ev_io, __libev_read_cb, ionode->fd, EV_READ);
	ev_io_start (io->ev_loop, &ionode->ev_io);
#elif defined (ENABLE_EPOLL)
	ionode->ev_epoll.data.ptr = ionode;
	epoll_ctl (io->epollfd, EPOLL_CTL_ADD, ionode->fd, &ionode->ev_epoll);
#else
	io->__max_fds = MAX (io->__max_fds, ionode->fd);
	FD_SET (ionode->fd, &io->selectfd);
#endif

	DC_link_add (&io->__link, &ionode->__link_ptr);


	return 0;
}

DC_INLINE int __io_task_del (DC_io_task_t* io, DC_io_t *ionode)
{
#ifdef ENABLE_LIBEV
	ev_io_stop (io->ev_loop, &ionode->ev_io);
#elif defined (ENABLE_EPOLL)
	epoll_ctl (io->epollfd, EPOLL_CTL_DEL, ionode->fd, &ionode->ev_epoll);
#else
	FD_CLR (ionode->fd, &io->selectfd);
#endif

	DC_link_remove (&ionode->__link_ptr);

	return 0;
}

static void __wr_process_cb (void *userdata, void *data)
{
//	DC_io_task_t *io = (DC_io_task_t*)userdata;
	DC_iobuf_t      *iobuf  = (DC_iobuf_t*)data;
	DC_io_t   *ioptr  = NULL;

	if ((ioptr = DC_iobuf_get_io (iobuf)) && ioptr->__ev_cb) {
		ioptr->__ev_cb (ioptr, EV_IO_WRITE, &iobuf, DC_io_userdata (ioptr));
	}
}

DC_INLINE void __proc_process_cb (void *userdata, void *data)
{
//	DC_io_task_t *io = (DC_io_task_t*)userdata;
	DC_iobuf_t      *iobuf  = (DC_iobuf_t*)data;
	DC_io_t   *ioptr  = NULL;

	if ((ioptr = DC_iobuf_get_io (iobuf)) && ioptr->__ev_cb) {
		ioptr->__ev_cb (ioptr, EV_IO_PROCESS, &iobuf, DC_io_userdata (ioptr));
	}
}

DC_INLINE void __rd_process_cb (void *userdata, void *data)
{
	DC_io_task_t *io = (DC_io_task_t*)userdata;
	DC_io_t      *ioptr  = (DC_io_t*)data;
	DC_iobuf_t   *iobuf  = NULL;

	if (ioptr->__ev_cb) {
		ioptr->__ev_cb (ioptr, EV_IO_READ, &iobuf, DC_io_userdata (ioptr));
		if (iobuf) {
			if (DC_task_queue_run_task (&io->__process_tasks,
					                    iobuf,
					                    1) != ERR_OK) {

			}
		}
	}

	DC_object_sync_run (io, {
		__io_task_add (io, ioptr);
	});
}

DC_INLINE void __do_read_event (DC_io_task_t *io, DC_io_t *ioptr)
{
	DC_object_sync_run (io, {
	    __io_task_del (io, ioptr);
	});

	if (DC_task_queue_run_task (&io->__reader_tasks,
			                    ioptr,
			                    1) != ERR_OK) {
	}
}

DC_INLINE void __do_timeout_check (DC_io_task_t *io)
{

}

DC_INLINE void __pipe_ev_cb (DC_io_t *io, int ev, DC_iobuf_t **iobuf, void *data)
{
	struct io_ctl_arg ioarg = {
		0, NULL, NULL
	};
    int ret;


	if ((ret = read(io->fd, &ioarg, SZIOARG)) < 0) {

	}

}

int DC_io_task_start (DC_io_task_t *io,
		              int check_interval)
{
	DC_io_t pipe_io;

	if (DC_io_init (&pipe_io, io->__pipes[0], __pipe_ev_cb, NULL) < 0) {
		return -1;
	}

#if defined (ENABLE_LIBEV)
	ev_timer timer;

	timer.data = io;
//	io->__check_interval = check_interval;
	ev_timer_init (&timer, __libev_timer_cb,(int)(check_interval/1000), 1);
	ev_timer_start (io->ev_loop, &timer);

	__io_task_add (io, &pipe_io);
	ev_loop (io->ev_loop, 0);
	__io_task_del (io, &pipe_io);

#elif defined (ENABLE_EPOLL)
#define MAX_EPOLL_EVENT 100
	struct epoll_event epollevs[MAX_EPOLL_EVENT];
	int numfdsready = 0;
	int i;
	int run_counter = 0;

	__io_task_add (io, &pipe_io);

	do {
		numfdsready = epoll_wait (io->epollfd, epollevs, MAX_EPOLL_EVENT, check_interval);
		if (numfdsready < 0) {
			return numfdsready;
		} else if (numfdsready == 0) {
			run_counter++;
			if (!(run_counter % check_interval)) {
				__do_timeout_check (io);
			}
		} else {
			for (i=0; i<numfdsready; i++) {
				if (epollevs[i].data.ptr) {
					__do_read_event (io, epollevs[i].data.ptr);
				} else {

				}
			}
		}
	} while (!io->__flag);

	__io_task_del (io, &pipe_io);
#else
	int numfdsready = 0;
	int run_counter = 0;
	DC_link_t *linkptr = NULL;
	DC_io_t *ioptr = NULL;
	struct timeval timeout = {1, 0};

	__io_task_add (io, &pipe_io);

	do {
		numfdsready = select (io->__max_fds+1, &io->selectfd, NULL, NULL, &timeout);
		if (numfdsready < 0) {
			return numfdsready;
		} else if (numfdsready == 0) {
			run_counter++;
			if (!(run_counter & check_interval)) {
				__do_timeout_check (io);
			}
		} else {
			DC_object_sync_run (io, {
				DC_link_foreach (linkptr, io->__link, 1) {
					if (numfdsready <= 0) {
						break;
					}
					ioptr = DC_link_container_of (linkptr, DC_io_t, __link_ptr);
					if (FD_ISSET (ioptr->fd, &io->selectfd)) {
						numfdsready--;
						__do_read_event (io, ioptr);
					}
				}
			});
		}
	} while (!io->__flag);

	__io_task_del (io, &pipe_io);
#endif

	return 0;
}

int DC_io_task_init (DC_io_task_t *io,
		             int szreader,
		             int numreaders,
		             int szprocs,
		             int numprocs,
		             int szwriter,
		             int numwriters)
{
	if (DC_object_init ((DC_object_t*)io)) {
		return -1;
	}

	if (pipe (io->__pipes)) {
		__goto (err_quit);
	}

#if defined (ENABLE_LIBEV)
	io->ev_loop = ev_default_loop (0);
	ev_set_userdata (io->ev_loop, io);
#elif defined (ENABLE_EPOLL)
	if ((io->epollfd = epoll_create (0)) < 0) {
		__goto (err_quit);
	}
#else
	FD_ZERO (&io->selectfd);
	io->__max_fds = io->__pipes[0];
#endif

	DC_link_init (io->__link);

	DC_task_queue_init (&io->__reader_tasks, szreader, numreaders, __rd_process_cb, io);
	DC_task_queue_init (&io->__writer_tasks, szwriter, numwriters, __wr_process_cb, io);
	DC_task_queue_init (&io->__process_tasks, szprocs, numprocs,   __proc_process_cb, io);

	return 0;

	__block (err_quit, {
		DC_object_destroy ((DC_object_t*)io);
		if (io->__pipes[0] > 0) {
			close (io->__pipes[0]);
		}
		if (io->__pipes[1]) {
			close (io->__pipes[1]);
		}
		return -1;
	});
}

int DC_io_task_ctl (DC_io_task_t *io,
		            int op,
		            DC_io_t *ionode,
		            void *arg)
{
	switch (op) {
	case IO_CTL_ADD: {
		DC_object_sync_run (io, {
			__io_task_add (io, ionode);
		});
	} break;
	case IO_CTL_DEL: {
		DC_object_sync_run (io, {
		    __io_task_del (io, ionode);
		});
	} break;
	case IO_CTL_MOD: {
		DC_object_sync_run (io, {
		    __io_task_del (io, ionode);
		    __io_task_add (io, ionode);
		});
	} break;
	default:
		return -1;
		break;
	}

	write (io->__pipes[1], &IO_CTL_MAP[op], SZIOARG);

	return 0;
}

void DC_io_task_stop (DC_io_task_t *io)
{
	DC_object_sync_run (io, {
	    io->__flag = 1;
	    write (io->__pipes[1], &IO_CTL_MAP[0], SZIOARG);
	});
}

void DC_io_task_destroy (DC_io_task_t *io)
{
	DC_task_queue_destroy (&io->__reader_tasks);
	DC_task_queue_destroy (&io->__writer_tasks);
	DC_task_queue_destroy (&io->__process_tasks);

	close (io->__pipes[0]);
	close (io->__pipes[1]);

#if defined (ENABLE_LIBEV)
	ev_break (io->ev_loop, EVBREAK_ALL);
	ev_loop_destroy (io->ev_loop);
#elif defined (ENABLE_EPOLL)
	close (io->epollfd);
#else
	FD_ZERO (&io->selectfd);
#endif
}
