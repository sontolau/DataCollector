#include "netsrv.h"


static unsigned int peer_udp_hash (DC_key_t key)
{
    NetInfo_t *netinfo = (NetInfo_t*)key;

    return netinfo->net_port;
}

static int peer_compare_func (void *obj, DC_key_t key)
{
    NetInfo_t *netinfo = (NetInfo_t*)key;
    NetPeer_t *peer    = (NetPeer_t*)obj;

    if (netinfo->net_port == peer->net_info->net_port &&
        netinfo->net_addr == peer->net_info->net_addr) {
        return 1;
    }

    return 0;
}

NetPeer_t *FindPeerWithNetInfo (const Server_t *srv, const NetInfo_t *info)
{
    return (NetPeer_t*)DC_hash_get_object (&srv->peer_hash_table, (DC_key_t)info);
}
