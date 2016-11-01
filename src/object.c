#include "object.h"

#if 0

static DC_object_t *__alloc_zone (const char *cls,
                                  unsigned int szcls,
                                  void *data)
{
    return (DC_object_t*)calloc (1, szcls);
}

static void __free_zone (const char *cls,
                         DC_object_t *obj,
                         void *data)
{
    free (obj);
}

DC_object_t *DC_object_alloc (long size,
		                      const char *cls,
		                      void *data,
		                      DC_object_t* (*__alloc) (const char*, unsigned int, void*),
                              void (*__free) (const char*, DC_object_t*, void*),
		                      void (*__release)(DC_object_t*, void*)) {
	DC_object_t *obj = NULL;

    //assert (__release != NULL);

    if (!__alloc) __alloc = __alloc_zone;
    if (!__free) __free   = __free_zone;

    obj = __alloc (cls, size, data);

    if (obj) {
        obj->data     = data;
        obj->refcount = 1;
        strncpy(obj->class_name, cls, sizeof(obj->class_name) - 1);
        obj->PRI (release) = __release;
        obj->PRI (alloc)   = __alloc;
        obj->PRI (free)    = __free;
    }
	return obj;
}

int DC_object_init (DC_object_t *obj)
{
    DC_locker_init (&obj->lock, 0, NULL);

    return ERR_OK;
}

void DC_object_destroy (DC_object_t *obj)
{
    DC_locker_destroy (&obj->lock);
}

int DC_object_is_kind_of(DC_object_t *obj, const char *cls) {
	return strncmp(obj->class_name, cls, SZ_CLASS_NAME) ? 0 : 1;
}


void DC_object_decref(DC_object_t *obj) {
	if (obj) {
		obj->refcount--;
		if (obj->refcount <= 0) {
            if (obj->PRI (release)) obj->PRI (release) (obj, obj->data);
            obj->PRI (free) (obj->class_name, obj, obj->data);
		}
	}
}

DC_object_t *DC_object_incref(DC_object_t *obj) {
	if (obj) {
		obj->refcount++;
	}

	return obj;
}

int DC_object_get_refcount(DC_object_t *obj) {
	return obj ? obj->refcount : 0;
}
#endif
