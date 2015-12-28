

/*
static int NK_lock (NetKit *kit, int wait)
{
    return DC_locker_lock (&kit->locker, LOCK_IN_READ, wait);
}


static int DC_locker_lock (NetKit *kit, int wait)
{
    return DC_locker_lock (&kit->locker, LOCK_IN_WRITE, wait);
}
void DC_locker_unlock (NetKit *kit)
{
    DC_locker_unlock (&kit->locker);
}
DC_INLINE DC_object_t *__NK_alloc_buffer_cb (void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKBuffer    *nkbuf = NULL;

    if (nk->config->num_sockbufs > 0) {
    	DC_locker_lock (nk, 1);
        buf = DC_buffer_pool_alloc (&nk->buffer_pool, nk->config->max_sockbuf_size);
        if (buf) {
            nkbuf = (NKBuffer*)buf->buffer;
        }
        DC_locker_unlock (nk);
    } else {
        nkbuf = (NKBuffer*)calloc (1, sizeof (NKBuffer)+nk->config->max_sockbuf_size);
    }

    if (nkbuf) {
        nkbuf->peer = NULL;
        nkbuf->size = nk->config->max_sockbuf_size;
        nkbuf->length = 0;
        memset (nkbuf->buffer, '\0', nkbuf->size);
    }

    return (DC_object_t*)nkbuf;
}

DC_INLINE void __NK_release_buffer_cb (DC_object_t *obj, void *data)
{
    NetKit *nk = (NetKit*)data;
    DC_buffer_t *buf = NULL;
    NKBuffer    *nbuf = (NKBuffer*)obj;

    if (nbuf->peer) {
        NK_release_peer (nbuf->peer);
        nbuf->peer = NULL;
    }

    if (nk->config->num_sockbufs > 0) {
    	DC_locker_lock (nk, 1);
        buf = DC_buffer_from (obj);
        DC_buffer_pool_free (&nk->buffer_pool, DC_buffer_from ((DC_buffer_t*)nbuf));
        DC_locker_unlock (nk);
    } else {
        free (obj);
    }
}
*/


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


void NK_release_peer (NKPeer *peer)
{
    //Dlog ("release peer: %d", peer->super.refcount);
    DC_object_release ((DC_object_t*)peer);
}


void NK_release_buffer (NKBuffer *buf)
{
    DC_object_release ((DC_object_t*)buf);
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

NKBuffer *NK_buffer_get (NKBuffer *buf)
{
    return (NKBuffer*)DC_object_get ((DC_object_t*)buf);
}

NKPeer *NK_peer_get (NKPeer *peer)
{
    DC_object_get ((DC_object_t*)peer);
    //Dlog ("inc peer: %d", peer->super.refcount);
    return peer;
}

void NK_buffer_set_peer (NKBuffer *buf, NKPeer *peer)
{
    if (buf->peer && buf->peer != peer) {
        NK_buffer_remove_peer (buf);
    }

    buf->peer = NK_peer_get (peer);
}

void NK_buffer_remove_peer (NKBuffer *buf)
{
    if (buf->peer) {
        NK_release_peer (buf->peer);
        buf->peer = NULL;
    }
}

int NK_commit_buffer (NetKit *nk, NKBuffer *buf)
{
    int ret = ERR_OK;
    
    NK_buffer_get (buf);
    if ((ret = DC_task_queue_run_task (&nk->outgoing_tasks, 
                                         (void*)buf, 1))) {
        NK_release_buffer (buf);
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

static DC_object_t *NK_malloc (const char *cls,
                               unsigned int szcls,
                               void *data)
{
    NetKit *nk = data;
    NKPeer *peer = NULL;
    NKBuffer *nkbuf = NULL;
    DC_buffer_t *buf = NULL;

    if (!strcmp (cls, "NKPeer")) {
        DC_locker_lock (&nk->locker, 0, 1);
        if (nk->config->max_peers) {
            buf = DC_buffer_pool_alloc (&nk->peer_pool, sizeof (NKPeer));
            if (buf) {
                peer = (NKPeer*)buf->buffer;
            }
        } else {
            peer = calloc (1, sizeof (NKPeer));
        }
        DC_locker_unlock (&nk->locker);
        return (DC_object_t*)peer;

    } else if (!strcmp (cls, "NKBuffer")) {
        DC_locker_lock (&nk->locker, 0, 1);
        if (nk->config->max_sockbufs > 0) {
            DC_locker_lock (&nk->locker ,0, 1);
            buf = DC_buffer_pool_alloc (&nk->buffer_pool, 
                                        nk->config->max_sockbuf_size);
            if (buf) {
                nkbuf = (NKBuffer*)buf->buffer;
            }
            DC_locker_unlock (&nk->locker);
        } else {
            nkbuf = (NKBuffer*)calloc (1, 
                    sizeof (NKBuffer)+nk->config->max_sockbuf_size);
        }
        if (nkbuf) {
            nkbuf->peer = NULL;
            nkbuf->size = nk->config->max_sockbuf_size;
            nkbuf->length = 0;
            memset (nkbuf->buffer, '\0', nkbuf->size);
        }
        //Dlog ("Alloc: %p", nkbuf);
        DC_locker_unlock (&nk->locker);
        return (DC_object_t*)nkbuf;
    }

    return NULL;
}

int NK_peer_init (NKPeer *peer)
{
    DC_locker_init (&peer->lock, 0, NULL);
    //DC_list_init (&peer->buf_list, NULL, NULL);
    return 0;
}

void NK_peer_release (NKPeer *peer)
{
    DC_locker_destroy (&peer->lock);
}

int NK_buffer_init (NKBuffer *buf)
{
    buf->iobuf.data = buf->buffer;
    buf->iobuf.size = buf->size;
    return 0;
}

void NK_buffer_release (NKBuffer *buf)
{
    if (buf->peer) {
        NK_release_peer (buf->peer);
        buf->peer = NULL;
    }
}

static void NK_free (const char *cls, DC_object_t *obj, void *data)
{
    NetKit *nk = data;

    if (DC_object_is_kind_of (obj, "NKPeer")) {
        NK_peer_release ((NKPeer*)obj);
        if (nk->config->max_peers) {
            DC_locker_lock (&nk->locker, 0, 1);
            DC_buffer_pool_free (&nk->peer_pool, DC_buffer_from ((DC_buffer_t*)obj));
            DC_locker_unlock (&nk->locker);
        } else {
            free (obj);
        }
    } else if (DC_object_is_kind_of (obj, "NKBuffer")) {
        NK_buffer_release ((NKBuffer*)obj);
        if (nk->config->max_sockbufs) {
            DC_locker_lock (&nk->locker, 0, 1);
            DC_buffer_pool_free (&nk->buffer_pool, DC_buffer_from ((DC_buffer_t*)obj));
            DC_locker_unlock (&nk->locker);
        } else {
            free (obj);
        }
    }
}

void *NK_get_userdata (NetKit *nk)
{
    void *data = NULL;

    data = nk->private_data;

    return data;
}

NKPeer *NK_alloc_peer_with_init (NetKit *nk)
{
    NKPeer *peer = (NKPeer*)DC_object_alloc (sizeof (NKPeer),
                                    "NKPeer",
                                    nk,
                                    NK_malloc,
                                    NK_free,
                                    NULL);
    if (peer && NK_peer_init (peer)) {
        DC_object_release ((DC_object_t*)peer);
        return NULL;
    }

    return peer;
}

NKBuffer *NK_alloc_buffer_with_init (NetKit *nk)
{
    NKBuffer *buf = (NKBuffer*)DC_object_alloc (sizeof (NKBuffer),
                                    "NKBuffer",
                                    nk,
                                    NK_malloc,
                                    NK_free,
                                    NULL);
    if (buf && NK_buffer_init (buf)) {
        DC_object_release ((DC_object_t*)buf);
        return NULL;
    }

    return buf;
}
