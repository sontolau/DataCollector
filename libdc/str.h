#ifndef _DC_STRING_H
#define _DC_STRING_H

#include "libdc.h"

DC_CPP (extern "C" {)

extern const char *DC_struncate  (char *str);

extern const char *DC_strtok (char **str, const char *delim);

extern int DC_substr (const char *str, const char *substr, int start);
DC_CPP(})

#endif //
