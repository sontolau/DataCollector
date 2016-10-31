#ifndef _DC_HASH_H
#define _DC_HASH_H

#include "libdc.h"
#include "list.h"
#include "thread.h"

CPP (extern "C" {)


/*
 * The hash_id_func_t will return an ID for getting 
 * a hash path with the KEY argument.
 */
typedef uint32_t (*hash_func_t)(KEY_t key, void *data);

/*
 * The hash comparing function,which will be used for 
 * searching an object you want.
 */
typedef bool_t (*hash_compare_func_t)(OBJ_t, KEY_t key, void *data);


/*
 * This will be called when you remove 
 * an object or release a DC_hash_t.
 */
typedef void (*hash_destroy_func_t)(OBJ_t obj, void *data);

typedef struct _DC_hash {
    uint32_t size;
    uint32_t count;
    void *data;
    OBJ_t zero;
    DC_list_t *hash_map;
    DC_thread_rwlock_t rwlock;
    hash_func_t hash_func;
    hash_compare_func_t compare_cb;
    hash_destroy_func_t destroy_cb;
} DC_hash_t;

extern err_t DC_hash_init(__in__ DC_hash_t *hash,
                          uint32_t size,
                          OBJ_t zero,
                          hash_func_t hash_func,
                          hash_compare_func_t comp_func,
                          hash_destroy_func_t dest_func,
                          void *data);

extern err_t DC_hash_add_object(__in__ DC_hash_t *hash, KEY_t key, OBJ_t obj);

extern OBJ_t DC_hash_get_object(__in__ DC_hash_t *hash, KEY_t key);

extern OBJ_t *DC_hash_get_all_objects(__in__ DC_hash_t *hash, uint32_t *num);

extern OBJ_t DC_hash_remove_object(__in__ DC_hash_t *hash, KEY_t key);

extern void DC_hash_clear(__in__ DC_hash_t *hash);

extern void DC_hash_destroy(__in__ DC_hash_t *hash);

CPP (})

#endif
