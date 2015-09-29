#ifndef _DC_HASH_H
#define _DC_HASH_H

#include "libdc.h"
#include "list.h"

DC_CPP (extern "C" {)

typedef DC_id_t DC_key_t;

typedef struct _DC_hash_elem {
    DC_list_elem_t   PRI (hash_list);
    DC_list_elem_t   PRI (array_list);

} DC_hash_elem_t;

/*
 * The hash_id_func_t will return an ID for getting 
 * a hash path with the KEY argument.
 */
typedef unsigned int (*hash_id_func_t) (DC_key_t key, void *data);

/*
 * The hash comparing function,which will be used for 
 * searching an object you want.
 */
typedef int (*hash_compare_func_t) (DC_hash_elem_t*, DC_key_t key, void *data);


/*
 * This will be called when you remove 
 * an object or release a DC_hash_t.
 */
typedef void (*hash_destroy_func_t) (DC_hash_elem_t *obj, void *data);

typedef struct DC_hash {
    unsigned int size;
    unsigned int num_objects;
    void         *data;

    DC_list_t *PRI (hash_map); 
    DC_list_t PRI (array_nodes);
    hash_id_func_t  PRI (id_cb);
    hash_compare_func_t PRI (compare_cb);
    hash_destroy_func_t PRI (destroy_cb); 
} DC_hash_t;

extern int DC_hash_init (DC_hash_t *hash, 
                         int size, 
                         hash_id_func_t id_func,
                         hash_compare_func_t comp_func,
                         hash_destroy_func_t dest_func,
                         void *data);

extern int DC_hash_add_object (DC_hash_t *hash, DC_key_t key, DC_hash_elem_t *elem);

extern DC_hash_elem_t *DC_hash_get_object (DC_hash_t *hash, DC_key_t key);

extern DC_hash_elem_t **DC_hash_get_all_objects (const DC_hash_t *hash);

extern void DC_hash_remove_object (DC_hash_t *hash, DC_key_t key);

extern void DC_hash_clear (DC_hash_t *hash);

extern void DC_hash_destroy (DC_hash_t *hash);

DC_CPP (})

#endif
