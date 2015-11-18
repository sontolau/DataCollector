#include <libdc/log.h>
#include "N_netkit.h"
#include "N_helper.c"

extern int NK_wrlock ();
extern int NK_rdlock ();
extern void NK_unlock ();

static NKConfig DefaultConfig = {
    .chdir = NULL,
    .log   = NULL,
    .pidfile = NULL,
    .daemon = 0,
    .max_sockbufs = 0,
    .max_sockbuf_size = 0xFFFF,
    .max_peers = 0,
    .outgoing_queue_size = 500,
    .incoming_queue_size = 500,
    .num_processors = 2,
    .debug = 1,
    .watch_dog = WATCH_DOG (1, 0),
};

/*
#define COM_ACCEPT			1
#define	COM_PROCESS	        2
#define	COM_SEND			3
#define COM_RECEIVE         4

enum {
    ADD_PEER = 1,
    START_PEER = 2,
    STOP_PEER,
    REMOVE_PEER,
};
*/

typedef struct peer_ev {
    int ev;
} peer_ev_t;

int NK_sync (NetKit *nk)
{
    peer_ev_t pev = {
        .ev = 0
    };
    return sizeof (pev);
    //printf ("Write ....\n");
    //return (write (nk->pfds[1], &pev, sizeof (pev)) == sizeof (pev))?0:1;
}


static void __do_accept (NetKit *nk, NKPeer *peer)
{
    NKPeer *newpeer;

    if (!(newpeer = NK_alloc_peer_with_init (nk))) {
	return;
    }

    if (!NetIOAccept (&peer->io, &newpeer->io)) {
        if (nk->delegate && nk->delegate->didAcceptRemoteClient) {
            nk->delegate->didAcceptRemoteClient (nk, peer, newpeer);
        }
    }

    NK_release_peer (newpeer);
}

static void __do_read(NetKit *nk, NKPeer *peer)
{
    NKBuffer *nkbuf = NULL;

    if (!(nkbuf = NK_alloc_buffer_with_init(nk))) {
	return;
    }
    nkbuf->iobuf.data = nkbuf->buffer;
    nkbuf->iobuf.size = nkbuf->size;
    
    nkbuf->length = NetIORead(&peer->io, &nkbuf->iobuf);
    if (nkbuf->length < 0) {
        if (nk->delegate && nk->delegate->didFailureToReceiveData) {
            nk->delegate->didFailureToReceiveData (nk, peer);
        }
    } else {
	nk->__rd_bytes += nkbuf->length;
        if (nk->delegate && nk->delegate->didSuccessToReceiveData) {
            if (nk->delegate->didSuccessToReceiveData (nk, peer, nkbuf)) {
                 NK_buffer_set_peer (nkbuf, peer);
		 if (DC_task_queue_run_task (&nk->incoming_tasks, nkbuf, 1) != ERR_OK) {
       	             Wlog ("[NetKit] the incoming queue is full.");
                 } else {
	             NK_buffer_get (nkbuf);
	         }
            }
        }
    }

    NK_release_buffer (nkbuf);
}

static void __do_process(NetKit *nk, NKPeer *peer, NKBuffer *nkbuf) 
{
    nk->__proc_bytes += nkbuf->length;
/*
    if (nk->delegate && nk->delegate->processData) {
	nk->delegate->processData(nk, peer, nkbuf);
    }
*/
    //printf ("%s\n", __func__);
    NK_release_buffer(nkbuf);
}

static void __do_write (NetKit *nk, NKPeer *peer, NKBuffer *nkbuf) 
{
    nkbuf->iobuf.data = nkbuf->buffer; 
    nkbuf->iobuf.size = nkbuf->length;
    nkbuf->length = NetIOWrite(&peer->io, &nkbuf->iobuf);

    if (nkbuf->length < 0) {
        if (nk->delegate && nk->delegate->didFailureToSendData) {
            nk->delegate->didFailureToSendData (nk, peer, nkbuf);
        }
    } else {
	nk->__wr_bytes += nkbuf->length;
        if (nk->delegate && nk->delegate->didSuccessToSendData) {
            nk->delegate->didSuccessToSendData (nk, peer, nkbuf);
        }
    }
/*
    ok = (nkbuf->length == nkbuf->iobuf.size?1:0);
    if (nk->delegate && nk->delegate->didReceiveEvent) {
        nk->delegate->didReceiveEvent (nk, peer, NK_EV_WRITE, ok, nkbuf);
    }
*/
}

DC_INLINE void __NK_write_cb (void *user, void *data)
{
    NKBuffer *buf = data;
    NKPeer *peer = buf->peer;

    __do_write (user, peer, buf);
    NK_buffer_remove_peer (buf);
    NK_release_buffer(buf);
    NK_sync (user);
}

DC_INLINE void __NK_read_cb (void *userdata, void *data)
{
    NKPeer *peer = (NKPeer*)data;
    NetKit *nk = userdata;

    if (peer->server) {
        __do_accept (nk, peer);
    } else {
        __do_read (nk, peer);
    }
    NK_sync (nk);
}

DC_INLINE void __NK_process_cb (void *userdata, void *data)
{
	NetKit *nk = userdata;
	NKPeer *peer = NULL;
	NKBuffer *nkbuf = NULL;

    nkbuf = (NKBuffer*)data;
    peer    = nkbuf->peer;
    __do_process (nk, peer, nkbuf);
    NK_sync (nk);
}

DC_INLINE void __NK_pipe_destroy (NetKit *nk)
{
    close (nk->pfds[0]);
    close (nk->pfds[1]);
}

DC_INLINE void __NK_write_callback (struct ev_loop *ev, ev_io *w, int revents)
{

}

DC_INLINE void __NK_read_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    NKPeer *peer = w->data;
    NetKit *nk  = ev_userdata (ev);

    //NK_stop_peer (nk, peer);
    //DC_task_queue_run_task (&nk->read_queue, (void*)peer, 1);
    __do_read (nk, peer);
}

DC_INLINE void __NK_accept_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    NKPeer *peer = w->data;
    NetKit *nk   = ev_userdata (ev);
    
    //NK_remove_peer (nk, peer);
    //ev_io_stop (nk->ev_loop, &peer->ev_io);
    //NK_stop_peer (nk, peer);
    //DC_task_queue_run_task (&nk->read_queue, (void*)peer, 1);
    __do_accept (nk, peer);
}

DC_INLINE void __NK_pipe_read_callback (struct ev_loop *ev, ev_io *w, int revents)
{
    peer_ev_t event;
    NetKit *nk = ev_userdata (ev);
    //NKPeer *peer;

   // do {
        if (read (nk->pfds[0], &event, sizeof (event)) < sizeof (event)) {
   //         break;
        } else {
           //printf ("Read ...\n");
        }
   // } while (1);
	//TODO: do nothing just wake up blocking event.
}

DC_INLINE int __NK_pipe_init (NetKit *nk)
{
/*
    static struct ev_io pipe_ev;
    int flag = 0;

    if (pipe (nk->pfds) < 0) {
        return -1;
    }
    flag = fcntl (nk->pfds[0], F_GETFL);
    flag |= O_NONBLOCK;
    fcntl (nk->pfds[0], F_SETFL, &flag);
    ev_io_init (&pipe_ev, __NK_pipe_read_callback, nk->pfds[0], EV_READ);
    pipe_ev.data = nk;

    ev_io_start (nk->ev_loop, &pipe_ev);
*/
    return 0;
}

DC_INLINE void __NK_check_callback (void *data)
{
/*
    NetKit *nk = (NetKit*)data;
    DC_list_elem_t *ele = NULL;
    void *saveptr = NULL;
    NKPeer *peer = NULL, *subpeer = NULL;
    DC_list_t   tmplist;

    DC_list_init (&tmplist, NULL, NULL, NULL);
    while (1) {
        NK_wrlock (nk, 1);
        ele = DC_list_next_object (&nk->infaces, &saveptr);
        if (!ele) { NK_unlock (nk); break;}

        peer = CONTAINER_OF (ele, NKPeer, handle);
        while (peer->sub_peers.count > 0 && 
               (ele = DC_list_get_object_at_index (&peer->sub_peers, 
                                               peer->sub_peers.count - 1))) {
            subpeer = CONTAINER_OF (ele, NKPeer, handle);
            if (nk->counter - subpeer->last_counter 
                > WATCH_DOG_TIMEOUT (nk->config->watch_dog)) {
                subpeer->to = NULL;
                DC_list_remove_object_at_index (&peer->sub_peers, peer->sub_peers.count-1);
                DC_list_add_object (&tmplist, ele);
            } else {
                break;
            }
        }
        NK_unlock (nk);
    }
    
    while (tmplist.count > 0 && (ele = DC_list_get_object_at_index (&tmplist, 0))) {
        peer = CONTAINER_OF (ele, NKPeer, handle);
        DC_list_remove_object_at_index (&tmplist, 0);
        NK_close_peer (nk, peer);
        NK_release_peer (nk, peer);
    }
*/
}

#define MS(bytes, secs) (double)(((double)bytes)/secs/1024)
void NK_print_usage (const NetKit *nk)
{
    unsigned int secs = (nk->counter * (WATCH_DOG_DELAY (nk->config->watch_dog)));
    double read_rate  = MS(nk->__rd_bytes, secs);
    double write_rate = MS(nk->__wr_bytes, secs);
    double proc_rate  = MS(nk->__proc_bytes, secs);

    if (nk->config->debug) {
        Dlog ("**************** NetKit Account Information *****************");
	Dlog ("Seconds: %u", secs);
        Dlog ("Read: %u(bytes), %f(ms)", nk->__rd_bytes, read_rate);
        Dlog ("Write: %u(bytes), %f(ms)", nk->__wr_bytes, write_rate);
        Dlog ("Process: %u(bytes), %f(ms)", nk->__proc_bytes, proc_rate);
	Dlog ("\n");
	Dlog ("Packets In Writing: %u", DC_queue_get_length (&nk->outgoing_tasks.queue));
	Dlog ("Packets In Processing: %u", DC_queue_get_length (&nk->incoming_tasks.queue));
	Dlog ("\n");
    }
}

DC_INLINE void __NK_timer_callback (struct ev_loop *ev, ev_timer *w, int revents)
{
    NetKit *nk = ev_userdata (ev);
    unsigned int timeout = WATCH_DOG_TIMEOUT (nk->config->watch_dog);
    
    if (nk->config->debug) {
        NK_print_usage (nk);
    }

    nk->counter++;
    if (nk->delegate && nk->delegate->didReceiveTimer) {
        nk->delegate->didReceiveTimer (nk);
    }
    if (timeout && !(nk->counter % timeout)) {
        DC_thread_run (&nk->checker_thread, __NK_check_callback, nk);
    }
}

DC_INLINE void __NK_signal_callback (struct ev_loop *ev, ev_signal *w, int revents)
{
    NetKit *nk = ev_userdata (ev);

    if (nk->delegate && nk->delegate->didReceiveSignal) {
        nk->delegate->didReceiveSignal (nk, w->signum);
    }
}


int NK_init (NetKit *nk, const NKConfig *cfg)
{
    static ev_signal sigs[_NSIG];

    memset (nk, '\0', sizeof (NetKit));

    if (!cfg) cfg = &DefaultConfig;

    nk->config = (NKConfig*)cfg;
    srandom (time (NULL));

    if (!(nk->ev_loop = ev_loop_new (0))) {
        return -1;
    }

    if (__NK_pipe_init (nk) < 0) {
        return -1;
    }

    if (DC_thread_init (&nk->checker_thread) != ERR_OK) {
        return -1;
    }

    //DC_task_queue_init (&nk->read_queue, cfg->read_queue_size, cfg->num_readers, __NK_read_cb, nk);

    DC_task_queue_init (&nk->incoming_tasks, cfg->incoming_queue_size, cfg->num_processors, __NK_process_cb, nk);

    DC_task_queue_init (&nk->outgoing_tasks, cfg->outgoing_queue_size, 1, __NK_write_cb, nk);

/*
    if (DC_task_queue_init (&nk->task_queue, cfg->queue_size, cfg->num_process_threads, __NK_process_cb, nk) < 0) {
        return -1;
    }
*/
    if (DC_list_init (&nk->peer_set, NULL, NULL, NULL) != ERR_OK) {
        return -1;
    }

    if (cfg->max_peers > 0 &&
        DC_buffer_pool_init (&nk->peer_pool,
                             cfg->max_peers,
                             sizeof (NKPeer)) != ERR_OK) {
        return -1;
    }

    if (cfg->max_sockbufs > 0 &&
        DC_buffer_pool_init (&nk->buffer_pool,
                             cfg->max_sockbufs,
                             sizeof (NKBuffer)+cfg->max_sockbuf_size) != ERR_OK) {
        return -1;
    }


    if (DC_locker_init (&nk->locker, 1, NULL) != ERR_OK) {
        return -1;
    }

    nk->sig_map = sigs;
    return 0;
}

void NK_set_signal (NetKit *nk, int signum)
{
    ev_signal_init (&nk->sig_map[signum], __NK_signal_callback, signum);
    nk->sig_map[signum].data =nk;
    ev_signal_start (nk->ev_loop, &nk->sig_map[signum]);
}

void NK_remove_signal (NetKit *nk, int signum)
{
    ev_signal_stop (nk->ev_loop, &nk->sig_map[signum]);
}

int NK_run (NetKit *nk)
{
    ev_timer timer;
    NKConfig       *cfg  = nk->config;

    if (cfg->daemon) {
        daemon (0, 0);
    }

    if (cfg->chdir) chdir (cfg->chdir);
    if (cfg->pidfile) __NK_write_pid (cfg->pidfile, getpid ());
    if (cfg->log) __NK_config_log (cfg->log);

    timer.data = nk;
    ev_timer_init (&timer, __NK_timer_callback, 1, WATCH_DOG_DELAY(nk->config->watch_dog));
    ev_timer_start (nk->ev_loop, &timer);

    ev_set_userdata (nk->ev_loop, nk);
    nk->running = 1;
    ev_loop (nk->ev_loop, 0);
    ev_timer_stop (nk->ev_loop, &timer);

    return 0;
}

void NK_add_peer (NetKit *nk, NKPeer *peer, int ev)
{
	NK_wrlock (nk, 1);

	if (ev == NK_EV_ACCEPT) {
		ev_io_init(&peer->ev_io, __NK_accept_callback, peer->io.fd, EV_READ);
	} else if (ev == NK_EV_READ) {
		ev_io_init(&peer->ev_io, __NK_read_callback, peer->io.fd, EV_READ);
	} else if (ev == NK_EV_WRITE) {
		ev_io_init(&peer->ev_io, __NK_write_callback, peer->io.fd, EV_WRITE);
	} else if (ev == NK_EV_RDWR) {
	}

/*
    //peer->ev = ev;
    if (peer->server) {
        ev_io_init (&peer->ev_io, __NK_accept_callback, peer->io.fd, EV_READ);
    } else {
        ev_io_init (&peer->ev_io, __NK_read_callback, peer->io.fd, EV_READ);
    }
*/
    DC_list_add_object (&nk->peer_set, &peer->peer_list);
    peer->ev_io.data = peer;
    ev_io_start (nk->ev_loop, &peer->ev_io);
    DC_object_get ((DC_object_t*)peer);
    //NK_sync (nk);
    NK_unlock (nk);
}

void NK_remove_peer (NetKit *nk, NKPeer *peer)
{
	NK_wrlock (nk, 1);
    DC_list_remove_object (&nk->peer_set, &peer->peer_list);
    ev_io_stop (nk->ev_loop, &peer->ev_io);
    //NK_sync (nk);
    NK_unlock (nk);
    NK_release_peer (peer);
}

void NK_stop_peer (NetKit *nk, NKPeer *peer)
{
	NK_wrlock (nk, 1);
	ev_io_stop (nk->ev_loop, &peer->ev_io);
    NK_unlock (nk);
}

void NK_start_peer (NetKit *nk, NKPeer *peer)
{
	NK_wrlock (nk, 1);
    ev_io_start (nk->ev_loop, &peer->ev_io);
    //NK_sync (nk);
    NK_unlock (nk);

}

void NK_destroy (NetKit *nk)
{
    __NK_pipe_destroy (nk);
    DC_task_queue_destroy (&nk->incoming_tasks);
    //DC_task_queue_destroy (&nk->read_queue);
    DC_task_queue_destroy (&nk->outgoing_tasks);
    ev_loop_destroy (nk->ev_loop);
    DC_list_destroy (&nk->peer_set);
    DC_buffer_pool_destroy (&nk->buffer_pool);
    DC_buffer_pool_destroy (&nk->peer_pool);
    DC_locker_destroy (&nk->locker);
}

void NK_stop (NetKit *nk)
{
    if (nk->running) {
    	ev_break (nk->ev_loop, EVBREAK_ALL);
        NK_sync (nk);
        nk->running = 0;
    }
}
