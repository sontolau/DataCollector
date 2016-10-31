#include "hash.h"
#include "list.h"
#include "mm.h"


FASTCALL DC_list_t *__list_entry(DC_list_t *map,
                                 uint32_t size,
                                 KEY_t key,
                                 hash_func_t hash_func,
                                 void *data) {
    uint32_t id = 0;

    if (hash_func) id = hash_func(key, data);
    else id = (uint32_t) key;


    return &map[id % size];
}

OBJ_t __find_hash_elem(DC_hash_t *hash, KEY_t key) {
    DC_list_t *list;
    OBJ_t obj;
    void *saveptr = NULL;

    list = __list_entry(hash->hash_map,
                        hash->size,
                        key,
                        hash->hash_func,
                        hash->data);

    while ((obj = DC_list_next(list, &saveptr))) {
        if (hash->compare_cb(obj, key, hash->data)) {
            return obj;
        }
    }

    return hash->zero;
}

err_t DC_hash_init(DC_hash_t *hash,
                   uint32_t size,
                   OBJ_t zero,
                   hash_func_t hash_func,
                   hash_compare_func_t comp_func,
                   hash_destroy_func_t dest_func,
                   void *data) {

    DC_list_t *list_map;
    int i;

    memset(hash, '\0', sizeof(DC_hash_t));
    hash->zero = zero;
    if (ISNULL(list_map = (DC_list_t *) DC_calloc(size, sizeof(DC_list_t)))) {
        return E_ERROR;
    }

    if (ISERR (DC_thread_rwlock_init(hash->rwlock))) {
        DC_free (list_map);
        return E_ERROR;
    }

    for (i = 0; i < size; i++) {
        DC_list_init(&list_map[i], zero);
    }

    hash->size = size;
    hash->data = data;

    hash->hash_map = list_map;
    hash->hash_func = hash_func;
    hash->compare_cb = comp_func;
    hash->destroy_cb = dest_func;

    return E_OK;
}

err_t DC_hash_add_object(DC_hash_t *hash, KEY_t key, OBJ_t obj) {
    DC_list_t *list;

    DC_thread_rwlock_wrlock (hash->rwlock);

    list = __list_entry(hash->hash_map,
                        hash->size,
                        key,
                        hash->hash_func,
                        hash->data);
    hash->count++;
    DC_list_insert_at_index(list, obj, 0);
    DC_thread_rwlock_unlock (hash->rwlock);

    return E_OK;
}


OBJ_t DC_hash_get_object(DC_hash_t *hash, KEY_t key) {
    OBJ_t obj = hash->zero;

    DC_thread_rwlock_rdlock (hash->rwlock);
    obj = __find_hash_elem(hash, key);
    DC_thread_rwlock_unlock (hash->rwlock);
    return obj;
}

OBJ_t *DC_hash_get_all_objects(DC_hash_t *hash, uint32_t *num) {
    OBJ_t *objects = NULL;
    OBJ_t obj;
    void *saveptr = NULL;
    int i = 0, j = 0;

    *num = 0;

    DC_thread_rwlock_rdlock (hash->rwlock);

    if (hash->count > 0) {
        objects = (OBJ_t *) DC_calloc (hash->count + 1, sizeof(OBJ_t));
    }

    for (i = 0; objects && i < hash->size && j < hash->count; i++) {
        saveptr = NULL;
        while ((obj = DC_list_next(&hash->hash_map[i], &saveptr)) != hash->zero) {
            objects[j++] = obj;
        }
    }

    *num = j;

    DC_thread_rwlock_unlock (hash->rwlock);

    return objects;
}

OBJ_t DC_hash_remove_object(DC_hash_t *hash, KEY_t key) {
    DC_list_t *list;
    OBJ_t elem = hash->zero;

    DC_thread_rwlock_wrlock (hash->rwlock);
    list = __list_entry(hash->hash_map,
                        hash->size,
                        key,
                        hash->hash_func,
                        hash->data);

    if (list && (elem = __find_hash_elem(hash, key))) {
        DC_list_remove(list, elem);
        hash->count--;
    }

    DC_thread_rwlock_unlock (hash->rwlock);

    return elem;
}

void DC_hash_clear(DC_hash_t *hash) {
    int i;

    DC_thread_rwlock_wrlock (hash->rwlock);
    for (i = 0; i < hash->size; i++) {
        DC_list_clean(&hash->hash_map[i]);
    }
    DC_thread_rwlock_unlock (hash->rwlock);
}

void DC_hash_destroy(DC_hash_t *hash) {
    DC_hash_clear(hash);
    DC_free (hash->hash_map);
    DC_thread_rwlock_destroy (hash->rwlock);
}

#ifdef HASH_DEBUG

#endif
