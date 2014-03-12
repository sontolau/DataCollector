#ifndef _DC_HASH_H
#define _DC_HASH_H

/*
 * The hash_id_func_t will return an ID for getting 
 * a hash path with the KEY argument.
 */
typedef unsigned int (*hash_id_func_t) (void *key);

/*
 * The hash comparing function,which will be used for 
 * searching an object you want.
 */
typedef int (*hash_compare_func_t) (void *obj, void *key);


/*
 * This will be called when you remove 
 * an object or release a DC_hash_t.
 */
typedef void (*hash_destroy_func_t) (void *obj);

typedef struct DC_hash {
    unsigned int size;
    unsigned int num_objects;
    void **__hash_map;
    hash_id_func_t  __hash_id;
    hash_compare_func_t __hash_compare;
    hash_destroy_func_t __hash_destroy;
} DC_hash_t;

extern int DC_hash_init (DC_hash_t *hash, 
                         int size, 
                         hash_id_func_t id_func,
                         hash_compare_func_t comp_func,
                         hash_destroy_func_t dest_func);

extern int DC_hash_add_object (DC_hash_t *hash, void *key, void *obj);

extern void *DC_hash_get_object (DC_hash_t *hash, void *key);

extern void DC_hash_remove_object (DC_hash_t *hash, void *key);

extern void DC_hash_destroy (DC_hash_t *hash);

#endif
