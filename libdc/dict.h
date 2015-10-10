#ifndef _DC_DICT_T
#define _DC_DICT_T

#include "libdc.h"
#include "hash.h"
#include "error.h"

DC_CPP (extern "C" {)

typedef DC_hash_t DC_dict_t;

/*
 * initialize a dictionary type variable, it must be end with 
 * a null value.
 * example:
 *       DC_dict_t  dict;
 *       char *name = "sontolau";
 *       DC_dict_init (&dict, "name", (void*)name, NULL);
 */

extern int DC_dict_init (DC_dict_t *dic, ...);


// add a object.
extern int DC_dict_add_object_with_key (DC_dict_t *dic, 
                                        const char *key, 
                                        const void *obj);

// remove a object from dictionary collector.
extern void DC_dict_remove_object_with_key (DC_dict_t *dic,
                                            const char *key);


// get the object relative to the key.
extern void *DC_dict_get_object_with_key (DC_dict_t *dic,
                                          const char *key);


// destroy a dictionary.
extern void DC_dict_destroy (DC_dict_t *dic);

DC_CPP (})
#endif 
