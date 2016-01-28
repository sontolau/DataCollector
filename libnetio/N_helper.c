
DC_INLINE void __NK_write_pid (const char *path, pid_t pid)
{
    FILE *fp = NULL;

    fp = fopen (path, "w");
    if (fp == NULL) {
        return;
    }

    fprintf (fp, "%u", pid);
    fclose (fp);
}

DC_INLINE void __NK_config_log (const char *logpath)
{
    FILE *logfp = NULL;

    if (logpath) {
        logfp = fopen (logpath, "w+");
    }

    if (!logfp) logfp = stdout;
    dup2 (fileno (logfp), fileno (stderr));
    dup2 (fileno (logfp), fileno (stdout));
}

void NK_set_userdata (NetKit *nk, const void *data)
{
    DC_locker_lock (&nk->locker, 0, 1);
    nk->private_data = (void*)data;
    DC_locker_unlock (&nk->locker);
}

void NK_set_delegate (NetKit *nk, NKDelegate *delegate)
{
    DC_locker_lock (&nk->locker, 0, 1);
    nk->delegate = delegate;
    DC_locker_unlock (&nk->locker);
}

void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer)
{
    if (buf->peer && buf->peer != peer) {
        NK_buffer_remove_peer (buf);
    }

    buf->peer = (NKPeer*)DC_object_get ((DC_object_t*)peer);
}

void NK_buffer_remove_peer (NKBuffer *buf)
{
    if (buf->peer) {
        DC_object_release ((DC_object_t*)buf->peer);
        buf->peer = NULL;
    }
}

int NK_commit_buffer (NetKit *nk, NKBuffer *buf)
{
    int ret = ERR_OK;
    
    DC_object_get ((DC_object_t*)buf);
    if ((ret = DC_task_queue_run_task (&nk->outgoing_tasks, 
                                         (void*)buf, 1))) {
        DC_object_release ((DC_object_t*)buf);
    }

    return ret;
}

int NK_commit_bulk_buffers (NetKit *nk, NKBuffer **buf, int num)
{
    int i;

    for (i=0;  i<num; i++) {
        if (NK_commit_buffer (nk, buf[i]) == ERR_OK) {
            i++;
        } else {
            break;
        }
    }

    return i;
}

int NK_peer_init (NKPeer* peer, const NetAddrInfo_t* addr, int type)
{
	if (addr && NetIOInit (&peer->io, addr, type) < 0) {
		return -1;
	}
    return 0;
}

void NK_peer_destroy (NKPeer *peer)
{
	if (peer->object) {
		DC_object_release (peer->object);
	}

	NetIODestroy (&peer->io);
}
int NK_buffer_init (NKBuffer *buf, int szbuf)
{
	buf->size = szbuf;
    buf->iobuf.data = buf->buffer;
    buf->iobuf.size = buf->size;

    return 0;
}

void NK_buffer_destroy (NKBuffer *buf)
{
    if (buf->peer) {
        DC_object_release ((DC_object_t*)buf->peer);
        buf->peer = NULL;
    }
}


void *NK_get_userdata (NetKit *nk)
{
    void *data = NULL;

    data = nk->private_data;

    return data;
}

static void NKPeer_release (DC_object_t *o, void *data)
{
	NK_peer_destroy ((NKPeer*)o);
}

NKPeer *NK_alloc_peer_with_init (NetKit *nk, NetAddrInfo_t *addr, int type)
{

    NKPeer *peer = NULL;

    if ((peer = DC_object_new (NKPeer, NULL))) {
    	if (NK_peer_init (peer, addr, type) < 0) {
    		DC_object_release ((DC_object_t*)peer);
    		return NULL;
    	}
    }

    return peer;
}

static void NKBuffer_release (DC_object_t *o, void *data)
{
	NK_buffer_destroy ((NKBuffer*)o);
}

NKBuffer *NK_alloc_buffer_with_init (NetKit *nk)
{
    NKBuffer *buf = NULL;

    buf = (NKBuffer*)DC_object_alloc (sizeof(NKBuffer)+nk->config->max_sockbuf_size,
    		"NKBuffer",
    		NULL,
    		NULL,
    		NULL,
    		NKBuffer_release);

    if (buf) {
    	if (NK_buffer_init (buf, nk->config->max_sockbuf_size) < 0) {
    		DC_object_release ((DC_object_t*)buf);
    		return NULL;
    	}
    }

    return buf;
}

void *NK_malloc (NetKit *nk, int *szbuf)
{
	NKBuffer *buf = NK_alloc_buffer_with_init (nk);

	if (buf) *szbuf = buf->size;
    Dlog ("Alloc: %p", buf);
	return buf?buf->buffer:NULL;
}

void NK_memcpy (NetKit *nk, void *dest, void *src, int ncpy)
{
    NKBuffer *nkbuf = NULL;

    nkbuf = CONTAINER_OF (dest, NKBuffer, buffer);
    ncpy = (ncpy>nkbuf->size?nkbuf->size:ncpy);
    memcpy (dest, src, ncpy);
    nkbuf->length = ncpy;
}

void NK_free (NetKit *nk, void *buf)
{
	NKBuffer *nkbuf = NULL;

	nkbuf = CONTAINER_OF (buf, NKBuffer, buffer);
    Dlog ("Free: %p", nkbuf);
	DC_object_release ((DC_object_t*)nkbuf);
}
