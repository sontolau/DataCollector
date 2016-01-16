#include "hash.h"
#include "list.h"


DC_INLINE DC_list_t *__list_entry (DC_list_t *map,
                                unsigned int size,
                                DC_key_t key, 
                                hash_func_t hash_func,
                                void *data) {
    unsigned int id = 0;

    if (hash_func) id = hash_func (key, data);
    else         id = (unsigned int)key;


    return &map[id % size];
}

LLVOID_t __find_hash_elem (DC_hash_t *hash, DC_key_t key) {
    DC_list_t *list;
    LLVOID_t obj;
    void *saveptr = NULL;

    list = __list_entry (hash->hash_map,
                         hash->size,
                         key,
                         hash->PRI (hash_func),
			hash->data);

	while ((obj = DC_list_next (list, &saveptr))) {
		if (hash->PRI (compare_cb)(obj, key, hash->data)) {
			return obj;
		}
	}

    return hash->defvalue;
}

static void __list_destroy_cb (LLVOID_t elem,
                               void *data)
{
    DC_hash_t *hash = (DC_hash_t*)data;

    if (hash->PRI (destroy_cb)) {
        hash->PRI (destroy_cb) (elem,
                                hash->data);
    }
    hash->count--;
}
 
int DC_hash_init (DC_hash_t *hash, 
                  int size,
                  LLVOID_t defval,
                  hash_func_t hash_func,
                  hash_compare_func_t comp_func,
                  hash_destroy_func_t dest_func,
                  void *data) {

    DC_list_t *list_map;
    int i;

    memset (hash, '\0', sizeof (DC_hash_t));
    hash->defvalue = defval;
    if (!(list_map = (DC_list_t*)DC_malloc (size * sizeof (DC_list_t)))) {
        return ERR_FAILURE;
    }

    for (i=0; i<size; i++) {
        DC_list_init (&list_map[i], defval, __list_destroy_cb, hash,  NULL);
    }

    hash->size = size;
    hash->data = data;

    hash->hash_map  = list_map;
    hash->PRI (hash_func)   = hash_func;
    hash->PRI (compare_cb) = comp_func;
    hash->PRI (destroy_cb) = dest_func;
    
    return ERR_OK;
}

int DC_hash_add_object (DC_hash_t *hash, DC_key_t key, LLVOID_t obj) {
    DC_list_t *list;

    
    list = __list_entry (hash->hash_map,
                         hash->size, 
                         key,
                         hash->PRI (hash_func),
                         hash->data);
    hash->count++;
    DC_list_add (list, obj);

    return ERR_OK;
}


LLVOID_t DC_hash_get_object (DC_hash_t *hash, DC_key_t key) {
    return __find_hash_elem (hash, key);
}

LLVOID_t *DC_hash_get_all_objects (const DC_hash_t *hash)
{
    LLVOID_t *objects = NULL;
    LLVOID_t obj;
    void *saveptr = NULL;
    int i = 0, j=0;

    objects = (LLVOID_t*)calloc (hash->count+1, sizeof (LLVOID_t));
    for (i=0; i < hash->size; i++) {
    	saveptr = NULL;
    	while ((obj = DC_list_next(&hash->hash_map[i], &saveptr)) && j < hash->count) {
    		objects[j++] = obj;
    	}
    }

    return objects;
}

LLVOID_t DC_hash_remove_object (DC_hash_t *hash, DC_key_t key) {
    DC_list_t *list;
    LLVOID_t elem = 0;

    list = __list_entry (hash->hash_map,
                         hash->size,
                         key,
                         hash->PRI (hash_func),
                         hash->data);

    if (list && (elem = __find_hash_elem (hash, key))) {
    	DC_list_remove (list, elem);
    	hash->count--;
    	return elem;
    }

    return hash->defvalue;
}

void DC_hash_clear (DC_hash_t *hash)
{
    int i;

    for (i=0; i<hash->size; i++) {
    	DC_list_clean (&hash->hash_map[i]);
    }
}

void DC_hash_destroy (DC_hash_t *hash) {
    DC_hash_clear (hash);
    DC_free (hash->hash_map);
}

#ifdef HASH_DEBUG

#endif
