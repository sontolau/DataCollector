#include "hash.h"
#include "list.h"


static DC_list_t *__list_entry (DC_list_t *map, 
                                unsigned int size,
                                DC_key_t key, 
                                hash_id_func_t id_func,
                                void *data) {
    unsigned int id = 0;

    if (id_func) id = id_func (key, data);
    else         id = (unsigned int)key;


    return &map[id % size];
}

DC_hash_elem_t  *__find_hash_elem (DC_hash_t *hash, DC_key_t key) {
    DC_list_t *list;
    void *saveptr = NULL;
    DC_list_elem_t *objptr = NULL;
    DC_hash_elem_t *elem = NULL;

    list = __list_entry (hash->PRI (hash_map),
                         hash->size,
                         key,
                         hash->PRI (id_cb),
                         hash->data);
    if (list) {
        while ((objptr = DC_list_next_object (list, &saveptr))) {
            elem = CONTAINER_OF (objptr, DC_hash_elem_t, PRI (hash_list));

            if (hash->PRI (compare_cb) (elem, key, hash->data)) {
                return elem;
            }
        }
    }

    return NULL;
}

static void __list_destroy_cb (DC_list_elem_t *elem,
                               void *data)
{
    DC_hash_t *hash = (DC_hash_t*)data;

    if (hash->PRI (destroy_cb)) {
        hash->PRI (destroy_cb) (CONTAINER_OF (elem, DC_hash_elem_t, PRI (array_list)),
                                hash->data);
    }
}
 
int DC_hash_init (DC_hash_t *hash, 
                  int size,
                  hash_id_func_t id_func,
                  hash_compare_func_t comp_func,
                  hash_destroy_func_t dest_func,
                  void *data) {

    DC_list_t *list_map;
    int i;

    memset (hash, '\0', sizeof (DC_hash_t));

    list_map = (DC_list_t*)calloc (size, sizeof (DC_list_t));
    if (list_map == NULL)  {
        return ERR_FAILURE;
    }

    for (i=0; i<size; i++) {
        DC_list_init (&list_map[i], NULL, NULL, NULL);
    }

    DC_list_init (&hash->PRI (array_nodes), __list_destroy_cb, hash, NULL);
    hash->size = size;
    hash->data = data;

    hash->PRI (hash_map)  = list_map;
    hash->PRI (id_cb)   = id_func;
    hash->PRI (compare_cb) = comp_func;
    hash->PRI (destroy_cb) = dest_func;
    
    return ERR_OK;
}

int DC_hash_add_object (DC_hash_t *hash, DC_key_t key, DC_hash_elem_t *obj) {
    DC_list_t *list;

    
    list = __list_entry (hash->PRI (hash_map), 
                         hash->size, 
                         key,
                         hash->PRI (id_cb),
                         hash->data);
    hash->num_objects++;
    DC_list_add_object (list, &obj->PRI (hash_list));
    DC_list_add_object (&hash->PRI (array_nodes), &obj->PRI (array_list));

    return ERR_OK;
}


DC_hash_elem_t  *DC_hash_get_object (DC_hash_t *hash, DC_key_t key) {
    return __find_hash_elem (hash, key);
}

DC_hash_elem_t **DC_hash_get_all_objects (const DC_hash_t *hash)
{
    DC_hash_elem_t **objects = NULL;
    DC_list_elem_t *elem = NULL;
    void *saveptr = NULL;
    int i = 0;

    objects = (DC_hash_elem_t**)calloc (hash->PRI (array_nodes).count+1, sizeof (DC_hash_elem_t*));
    while ((elem = DC_list_next_object (&hash->PRI (array_nodes), &saveptr))) {
        objects[i++] = CONTAINER_OF (elem, DC_hash_elem_t, PRI (array_list));
    }

    return objects;
}

void DC_hash_remove_object (DC_hash_t *hash, DC_key_t key) {
    DC_list_t *list;
    DC_hash_elem_t *elem = NULL;

    list = __list_entry (hash->PRI (hash_map),             
                         hash->size,
                         key,
                         hash->PRI (id_cb),
                         hash->data);
    elem = __find_hash_elem (hash, key);

    if (elem) {
        hash->num_objects--;
        DC_list_remove_object (list, &elem->PRI (hash_list));
        DC_list_remove_object (&hash->PRI (array_nodes), &elem->PRI (array_list));
    }
}

void DC_hash_clear (DC_hash_t *hash)
{
    int i;

    for (i=0; i<hash->size; i++) {
        DC_list_remove_all_objects (&hash->PRI (hash_map)[i]);
    }

    DC_list_remove_all_objects (&hash->PRI (array_nodes));
}

void DC_hash_destroy (DC_hash_t *hash) {
    DC_hash_clear (hash);
    free (hash->PRI (hash_map));
}

