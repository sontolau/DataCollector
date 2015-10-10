#ifndef _DC_STRING_H
#define _DC_STRING_H

#include "libdc.h"

DC_CPP (extern "C" {)

extern const char *DC_struncate  (char *str);

extern const char *DC_strtok (char **str, const char *delim);
DC_CPP(})

#endif //
