#ifndef XCOLLECTION_H
#define XCOLLECTION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void x_collections_init(void);
void x_collections_cleanup(void);

/* List C functions for xlang */
void x_list_create(fcall* fc);
void x_list_free(fcall* fc);
void x_list_size(fcall* fc);
void x_list_add(fcall* fc);
void x_list_add_int(fcall* fc);
void x_list_add_float(fcall* fc);
void x_list_get(fcall* fc);
void x_list_get_int(fcall* fc);
void x_list_get_float(fcall* fc);
void x_list_set(fcall* fc);
void x_list_set_int(fcall* fc);
void x_list_remove_at(fcall* fc);
void x_list_clear(fcall* fc);
void x_list_contains(fcall* fc);
void x_list_contains_int(fcall* fc);
void x_list_index_of(fcall* fc);
void x_list_index_of_int(fcall* fc);
void x_list_pop(fcall* fc);
void x_list_pop_int(fcall* fc);
void x_list_join(fcall* fc);
void x_list_to_string(fcall* fc);

/* Map C functions for xlang */
void x_map_create(fcall* fc);
void x_map_free(fcall* fc);
void x_map_size(fcall* fc);
void x_map_put(fcall* fc);
void x_map_put_int(fcall* fc);
void x_map_put_float(fcall* fc);
void x_map_get(fcall* fc);
void x_map_get_int(fcall* fc);
void x_map_get_float(fcall* fc);
void x_map_has(fcall* fc);
void x_map_remove(fcall* fc);
void x_map_clear(fcall* fc);
void x_map_keys(fcall* fc);
void x_map_values(fcall* fc);
void x_map_to_string(fcall* fc);
void x_map_keys_list(fcall* fc);

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
int x_map_get_all_keys(int id, char*** out_keys);

#ifdef __cplusplus
}
#endif

#endif /* XCOLLECTION_H */
