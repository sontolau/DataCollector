#ifndef _DC_STRING_H
#define _DC_STRING_H

#include "libdc.h"

CPP (extern "C" {)

extern char *DC_strip(__in__ char *str);

extern char *DC_strtok(__inout__ char **str, __in__ const char *delim);

extern int DC_substr(__in__ const char *str, __in__ const char *substr, int start);

extern char **DC_split_str(__in__ char *str, __in__ const char *delim, __out__ int *num);

CPP(})

#endif //
