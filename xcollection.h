#ifndef XCOLLECTION_H
#define XCOLLECTION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void x_collections_init(void);
void x_collections_cleanup(void);

/* Internal subscript indexing support */
var* x_list_get_var(int list_id, int index);

/* GC support for collections */
void x_collection_gc_mark_list(int id);
void x_collection_gc_mark_map(int id);
void x_collection_gc_sweep(void);

/* Direct C API for collections */
int x_list_alloc(void);
int x_list_append_str(int id, const char* val);
int x_list_append_int(int id, int val);
int x_list_append_float(int id, float val);
int x_list_count(int id);
const char* x_list_item_str(int id, int index);
int x_list_item_int(int id, int index);
float x_list_item_float(int id, int index);
int x_list_item_type(int id, int index);
int x_list_set_item_int(int id, int index, int val);
int x_list_set_item_float(int id, int index, float val);
int x_list_set_item_str(int id, int index, const char* val);
int x_list_remove_item(int id, int idx);
int x_list_clear_items(int id);

int x_map_alloc(void);
int x_map_insert_str(int id, const char* key, const char* val);
int x_map_insert_int(int id, const char* key, int val);
int x_map_insert_float(int id, const char* key, float val);
int x_map_count(int id);
bool x_map_contains_key(int id, const char* key);
const char* x_map_fetch_str(int id, const char* key);
int x_map_fetch_int(int id, const char* key);
float x_map_fetch_float(int id, const char* key);
int x_map_fetch_type(int id, const char* key);
int x_list_free_id(int id);
int x_map_free_id(int id);
int x_map_remove_key(int id, const char* key);
int x_map_get_all_keys(int id, char*** out_keys);

#ifdef __cplusplus
}
#endif

#endif /* XCOLLECTION_H */
