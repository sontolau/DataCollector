#ifndef _DC_DICT_T
#define _DC_DICT_T

#include "hash.h"

typedef DC_hash_t DC_dict_t;

extern int DC_dict_init (DC_dict_t *dic, ...);

extern int DC_dict_add_object_with_key (DC_dict_t *dic, 
                                        const char *key, 
                                        const void *obj);

extern void DC_dict_remove_object_with_key (DC_dict_t *dic,
                                            const char *key);


extern void *DC_dict_get_object_with_key (DC_dict_t *dic,
                                          const char *key);



extern void DC_dict_destroy (DC_dict_t *dic);

#endif 
