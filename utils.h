#ifndef _DC_UTILS_H
#define _DC_UTILS_H

#include "libdc.h"

extern int DC_set_nonblock (int fd);
extern int DC_set_daemonized (const char *chdir, int closeallfds);


#endif //
