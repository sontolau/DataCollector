#ifndef _LIBDC_H
#define _LIBDC_H

#if defined (WIN32) || defined (WINDOWS)
#include <windows.h>
#define OS_WINDOWS
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SZ_CLASS_NAME
#define SZ_CLASS_NAME 50
#endif

#ifndef SZSHORTSTR
#define SZSHORTSTR 50
#endif


#ifdef __cplusplus
#define DC_CPP(x)   x
#else
#define DC_CPP(x)
#endif

#if __STDC_VERSION__ >= 199901L || __GNUC__ >= 3
#define DC_INLINE static inline
#else
#define DC_INLINE static
#endif

#endif
