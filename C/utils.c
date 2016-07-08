#include "utils.h"

int DC_set_nonblock (int fd)
{
    int flags = 0;

    if (fcntl (fd,F_GETFL, &flags) < 0) {
        return -1;
    } 

    flags |= O_NONBLOCK;
   
    return fcntl (fd, F_SETFL, &flags);
}


int DC_set_daemonized (const char *dir, int closeallfds)
{
    struct rlimit rlimit;
    int i;
    pid_t pid;

    if (getrlimit (RLIMIT_NOFILE, &rlimit) < 0) {
        return -1;
    }

    pid = fork ();
    if (pid < 0) {
        return -1;
    } else if (pid > 0) {
        exit (0);
    }

    if (dir) {
        chdir (dir);
    }

    if (closeallfds) {
        if (!getrlimit (RLIMIT_NOFILE, &rlimit)) {
            for (i=0; i<MAX (rlimit.rlim_cur, rlimit.rlim_max); i++) {
                close (i);
            }
        }
    }


    pid = fork ();
    if (pid > 0) {
        exit (0);
    }

    return 0;
}
