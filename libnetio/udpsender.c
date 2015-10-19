#include "N_netkit.h"

int main (int argc, char *argv[]) {
    NetIO_t    io;
    NetBuf_t   nbuf;
    unsigned char buf[1000] = {0};

    if (argc < 4) {
        fprintf (stderr, "USAGE: HOST PORT DELAY\n");
        exit (1);
    }

    NetAddrInfo_t addrinfo = {
        .flag = 0,
        .type = NET_UDP,
        .port = atoi (argv[2]),
        .address = argv[1],
    };

    NetIOInit (&io, &addrinfo, 0);

    while (1) {
        char *str = "How are you";
        NetBufSetBuffer (nbuf, (unsigned char*)str, strlen (str));
        NetBufSetInetAddress(nbuf, io.inet_addr);
        NetIOWrite (&io, &nbuf);
        nbuf.data = buf;
        nbuf.size = 1000;
        long szread = NetIORead (&io, &nbuf);
        nbuf.data[szread] = '\0';
        printf ("Received: %s\n", nbuf.data);
    } while (0);

    NetIODestroy (&io);
    return 0;
}
