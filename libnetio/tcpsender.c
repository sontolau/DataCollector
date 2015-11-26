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
        .type = NET_TCP,
        .port = atoi (argv[2]),
        .address = argv[1],
    };

    NetIOInit (&io, &addrinfo, 0);
    if (NetIOConnect (&io) < 0) {
        fprintf (stderr, "Connected failed.\n");
        exit (1);
    }
    getchar ();
    while (1) {
        char *str = "How are you";
        nbuf.data = str;
        nbuf.size = strlen (str);

        if (NetIOWrite (&io, &nbuf) < 0) {
           printf ("Send data failed.\n");
        }
/*
        nbuf.data = buf;
        nbuf.size = 1000;
        long szread = NetIORead (&io, &nbuf);
        nbuf.data[szread] = '\0';
        printf ("Received: %s\n", nbuf.data);
*/
	usleep (atoi (argv[3]));
    } while (0);
    NetIODestroy (&io);
    return 0;
}
