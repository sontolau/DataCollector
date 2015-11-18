#ifndef _DC_OBJECT_H
#define _DC_OBJECT_H

#include "libdc.h"


typedef struct _DC_object {
    int refcount;
    char class_name[SZ_CLASS_NAME+1];
    void *data;
   
    struct _DC_object* (*PRI (alloc)) (const char*, void*);
    void (*PRI (release)) (struct _DC_object*, void*);
} DC_object_t;

extern DC_object_t *DC_object_alloc (long size,
                                     const char *cls,
                                     void *data,
                                     DC_object_t* (*__alloc) (const char *cls, void*),
                                     void (*__release)(DC_object_t*, void*));
#define DC_object_new(__class, __data) (__class*)DC_object_alloc(sizeof (__class), #__class, __data, NULL, __class##_release)

extern int DC_object_init (DC_object_t *obj);

extern DC_object_t *DC_object_get (DC_object_t *obj);

extern int DC_object_get_refcount (DC_object_t *obj);

extern int DC_object_is_kind_of (DC_object_t *obj, const char *cls);

//extern void DC_object_destroy (DC_object_t *obj);

extern void DC_object_release (DC_object_t *);

#define DC_OBJ_EXTENDS(__class)   __class super

/*
#define DC_OBJECT_NEW(__class, __data) \
    ((__class*)DC_object_alloc(sizeof(__class), #__class, __data, NULL, __class##_release))
*/
#ifdef _OBJ
#undef _OBJ
#endif

#define _OBJ(x) ((DC_object_t*)x)

/*
#define DC_OBJECT_RELEASE(__obj) \
do { \
    if (__obj) { \
        _OBJ(__obj)->refcount--; \
        if (_OBJ(__obj)->refcount <= 0) { \
            _OBJ(__obj)->__release (_OBJ(__obj), _OBJ(__obj)->data); \
        } \
    } \
} while (0)
*/

#endif
