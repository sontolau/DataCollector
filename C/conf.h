#ifndef _DC_CONF_H
#define _DC_CONF_H

#include "libdc.h"

CPP(extern "C" {)

extern err_t DC_read_ini (__in__ const char *path, 
                          __in__ bool_t (*cb) (const char *sec,
                                            const char *key,
                                            char *val));

CPP (})

#endif
