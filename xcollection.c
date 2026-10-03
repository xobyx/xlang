#include "xcollection.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xgc.h"

#define MAX_LISTS 256
#define MAX_MAPS 256
#define INITIAL_LIST_CAP 8
#define INITIAL_MAP_BUCKETS 16

typedef enum {
	X_ELEM_INT = 1,
	X_ELEM_FLOAT = 2,
	X_ELEM_STR = 3
} x_elem_type_t;

typedef struct {
	x_elem_type_t type;
	int int_val;
	float float_val;
	char* str_val;
	var item_var; /* For subscript indexing */
} x_list_item_t;

typedef struct {
	int id;
	bool active;
	uint8_t mark;
	x_list_item_t* items;
	int size;
	int capacity;
} x_list_t;

typedef struct x_map_entry_s {
	char* key;
	x_elem_type_t type;
	int int_val;
	float float_val;
	char* str_val;
	struct x_map_entry_s* next;
} x_map_entry_t;

typedef struct {
	int id;
	bool active;
	uint8_t mark;
	x_map_entry_t** buckets;
	int num_buckets;
	int size;
} x_map_t;

static x_list_t* s_lists[MAX_LISTS] = {0};
static x_map_t* s_maps[MAX_MAPS] = {0};

/* ------------------------------------------------------------------------- */
/* Argument Extraction Helpers                                               */
/* ------------------------------------------------------------------------- */
/* Internal List Management                                                  */
/* ------------------------------------------------------------------------- */

static void sync_item_var(x_list_item_t* it)
{
	memset(&it->item_var, 0, sizeof(var));
	it->item_var.size = 1;
	if (it->type == X_ELEM_INT)
	{
		it->item_var.type_define = T_INT;
		it->item_var.values = &it->int_val;
	}
	else if (it->type == X_ELEM_FLOAT)
	{
		it->item_var.type_define = T_FLOAT;
		it->item_var.values = &it->float_val;
	}
	else if (it->type == X_ELEM_STR)
	{
		it->item_var.type_define = T_STRING;
		it->item_var.values = &it->str_val;
	}
}

static void list_ensure_capacity(x_list_t* l, int needed)
{
	if (needed > l->capacity)
	{
		int new_cap = l->capacity == 0 ? INITIAL_LIST_CAP : l->capacity * 2;
		while (new_cap < needed) new_cap *= 2;
		l->items = (x_list_item_t*)realloc(l->items, new_cap * sizeof(x_list_item_t));
		for (int i = l->capacity; i < new_cap; i++)
		{
			memset(&l->items[i], 0, sizeof(x_list_item_t));
		}
		l->capacity = new_cap;
		/* Re-sync addresses of item_var values */
		for (int i = 0; i < l->size; i++)
		{
			sync_item_var(&l->items[i]);
		}
	}
}

static void free_list_items(x_list_t* l)
{
	if (l->items != NULL)
	{
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_STR && l->items[i].str_val != NULL)
			{
				free(l->items[i].str_val);
				l->items[i].str_val = NULL;
			}
		}
		free(l->items);
		l->items = NULL;
	}
	l->size = 0;
	l->capacity = 0;
}

var* x_list_get_var(int list_id, int index)
{
	if (list_id <= 0 || list_id >= MAX_LISTS) return NULL;
	x_list_t* l = s_lists[list_id];
	if (l == NULL || !l->active) return NULL;
	if (index < 0 || index >= l->size) return NULL;
	sync_item_var(&l->items[index]);
	return &l->items[index].item_var;
}

/* ------------------------------------------------------------------------- */
/* Internal Map Management                                                   */
/* ------------------------------------------------------------------------- */

static unsigned long djb2_hash(const char* str)
{
	unsigned long hash = 5381;
	int c;
	while ((c = (unsigned char)*str++))
		hash = ((hash << 5) + hash) + c;
	return hash;
}

static void map_rehash(x_map_t* m)
{
	int old_num = m->num_buckets;
	x_map_entry_t** old_buckets = m->buckets;

	int new_num = old_num * 2;
	x_map_entry_t** new_buckets = (x_map_entry_t**)calloc(new_num, sizeof(x_map_entry_t*));

	for (int i = 0; i < old_num; i++)
	{
		x_map_entry_t* curr = old_buckets[i];
		while (curr != NULL)
		{
			x_map_entry_t* next = curr->next;
			unsigned long h = djb2_hash(curr->key) % new_num;
			curr->next = new_buckets[h];
			new_buckets[h] = curr;
			curr = next;
		}
	}
	free(old_buckets);
	m->buckets = new_buckets;
	m->num_buckets = new_num;
}

static void free_map_entries(x_map_t* m)
{
	if (m->buckets != NULL)
	{
		for (int i = 0; i < m->num_buckets; i++)
		{
			x_map_entry_t* curr = m->buckets[i];
			while (curr != NULL)
			{
				x_map_entry_t* next = curr->next;
				if (curr->key != NULL) free(curr->key);
				if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
				free(curr);
				curr = next;
			}
			m->buckets[i] = NULL;
		}
		free(m->buckets);
		m->buckets = NULL;
	}
	m->size = 0;
	m->num_buckets = 0;
}

/* ------------------------------------------------------------------------- */
/* Cleanup                                                                   */
/* ------------------------------------------------------------------------- */

void x_collections_init(void)
{
	memset(s_lists, 0, sizeof(s_lists));
	memset(s_maps, 0, sizeof(s_maps));
}

void x_collections_cleanup(void)
{
	for (int i = 1; i < MAX_LISTS; i++)
	{
		if (s_lists[i] != NULL)
		{
			free_list_items(s_lists[i]);
			free(s_lists[i]);
			s_lists[i] = NULL;
		}
	}
	for (int i = 1; i < MAX_MAPS; i++)
	{
		if (s_maps[i] != NULL)
		{
			free_map_entries(s_maps[i]);
			free(s_maps[i]);
			s_maps[i] = NULL;
		}
	}
}

void x_collection_gc_mark_list(int id)
{
	if (id <= 0 || id >= MAX_LISTS) return;
	x_list_t* l = s_lists[id];
	if (l == NULL || !l->active) return;
	if (l->mark) return;
	l->mark = 1;

	for (int i = 0; i < l->size; i++)
	{
		if (l->items[i].type == X_ELEM_STR && l->items[i].str_val != NULL)
		{
			gc_mark_ptr(l->items[i].str_val);
		}
	}
}

void x_collection_gc_mark_map(int id)
{
	if (id <= 0 || id >= MAX_MAPS) return;
	x_map_t* m = s_maps[id];
	if (m == NULL || !m->active) return;
	if (m->mark) return;
	m->mark = 1;

	for (int i = 0; i < m->num_buckets; i++)
	{
		for (x_map_entry_t* e = m->buckets[i]; e != NULL; e = e->next)
		{
			if (e->key != NULL)
				gc_mark_ptr(e->key);
			if (e->type == X_ELEM_STR && e->str_val != NULL)
				gc_mark_ptr(e->str_val);
		}
	}
}

void x_collection_gc_sweep(void)
{
	for (int i = 1; i < MAX_LISTS; i++)
	{
		if (s_lists[i] != NULL && s_lists[i]->active)
		{
			if (s_lists[i]->mark == 0)
			{
				free_list_items(s_lists[i]);
				free(s_lists[i]);
				s_lists[i] = NULL;
			}
			else
			{
				s_lists[i]->mark = 0;
			}
		}
	}

	for (int i = 1; i < MAX_MAPS; i++)
	{
		if (s_maps[i] != NULL && s_maps[i]->active)
		{
			if (s_maps[i]->mark == 0)
			{
				free_map_entries(s_maps[i]);
				free(s_maps[i]);
				s_maps[i] = NULL;
			}
			else
			{
				s_maps[i]->mark = 0;
			}
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Direct C API for collections                                              */
/* ------------------------------------------------------------------------- */

int x_list_alloc(void)
{
	int slot = -1;
	for (int i = 1; i < MAX_LISTS; i++)
	{
		if (s_lists[i] == NULL || !s_lists[i]->active)
		{
			slot = i;
			break;
		}
	}
	if (slot == -1) return -1;

	if (s_lists[slot] == NULL)
		s_lists[slot] = (x_list_t*)calloc(1, sizeof(x_list_t));

	x_list_t* l = s_lists[slot];
	l->id = slot;
	l->active = true;
	l->size = 0;
	l->capacity = 0;
	l->items = NULL;
	list_ensure_capacity(l, INITIAL_LIST_CAP);
	return slot;
}

int x_list_append_str(int id, const char* val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return -1;
	x_list_t* l = s_lists[id];
	list_ensure_capacity(l, l->size + 1);
	x_list_item_t* it = &l->items[l->size++];
	it->type = X_ELEM_STR;
	it->str_val = strdup(val != NULL ? val : "");
	sync_item_var(it);
	return l->size;
}

int x_list_append_int(int id, int val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return -1;
	x_list_t* l = s_lists[id];
	list_ensure_capacity(l, l->size + 1);
	x_list_item_t* it = &l->items[l->size++];
	it->type = X_ELEM_INT;
	it->int_val = val;
	sync_item_var(it);
	return l->size;
}

int x_list_append_float(int id, float val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return -1;
	x_list_t* l = s_lists[id];
	list_ensure_capacity(l, l->size + 1);
	x_list_item_t* it = &l->items[l->size++];
	it->type = X_ELEM_FLOAT;
	it->float_val = val;
	sync_item_var(it);
	return l->size;
}

int x_list_count(int id)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	return s_lists[id]->size;
}

const char* x_list_item_str(int id, int index)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return "";
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return "";
	if (l->items[index].type == X_ELEM_STR)
		return l->items[index].str_val ? l->items[index].str_val : "";
	return "";
}

int x_list_item_int(int id, int index)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0;
	if (l->items[index].type == X_ELEM_INT) return l->items[index].int_val;
	if (l->items[index].type == X_ELEM_FLOAT) return (int)l->items[index].float_val;
	if (l->items[index].type == X_ELEM_STR && l->items[index].str_val)
		return atoi(l->items[index].str_val);
	return 0;
}

float x_list_item_float(int id, int index)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0.0f;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0.0f;
	if (l->items[index].type == X_ELEM_FLOAT) return l->items[index].float_val;
	if (l->items[index].type == X_ELEM_INT) return (float)l->items[index].int_val;
	if (l->items[index].type == X_ELEM_STR && l->items[index].str_val)
		return (float)atof(l->items[index].str_val);
	return 0.0f;
}

int x_list_item_type(int id, int index)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0;
	return (int)l->items[index].type;
}

int x_list_set_item_int(int id, int index, int val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0;
	if (l->items[index].type == X_ELEM_STR && l->items[index].str_val) free(l->items[index].str_val);
	l->items[index].type = X_ELEM_INT;
	l->items[index].int_val = val;
	sync_item_var(&l->items[index]);
	return 1;
}

int x_list_set_item_float(int id, int index, float val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0;
	if (l->items[index].type == X_ELEM_STR && l->items[index].str_val) free(l->items[index].str_val);
	l->items[index].type = X_ELEM_FLOAT;
	l->items[index].float_val = val;
	sync_item_var(&l->items[index]);
	return 1;
}

int x_list_set_item_str(int id, int index, const char* val)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (index < 0 || index >= l->size) return 0;
	if (l->items[index].type == X_ELEM_STR && l->items[index].str_val) free(l->items[index].str_val);
	l->items[index].type = X_ELEM_STR;
	l->items[index].str_val = strdup(val ? val : "");
	sync_item_var(&l->items[index]);
	return 1;
}

int x_map_alloc(void)
{
	int slot = -1;
	for (int i = 1; i < MAX_MAPS; i++)
	{
		if (s_maps[i] == NULL || !s_maps[i]->active)
		{
			slot = i;
			break;
		}
	}
	if (slot == -1) return -1;

	if (s_maps[slot] == NULL)
		s_maps[slot] = (x_map_t*)calloc(1, sizeof(x_map_t));

	x_map_t* m = s_maps[slot];
	m->id = slot;
	m->active = true;
	m->size = 0;
	m->num_buckets = INITIAL_MAP_BUCKETS;
	m->buckets = (x_map_entry_t**)calloc(INITIAL_MAP_BUCKETS, sizeof(x_map_entry_t*));
	return slot;
}

int x_map_insert_str(int id, const char* key, const char* val)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->size >= m->num_buckets * 3 / 4) map_rehash(m);

	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
			curr->type = X_ELEM_STR;
			curr->str_val = strdup(val != NULL ? val : "");
			return 1;
		}
		curr = curr->next;
	}
	x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
	n->key = strdup(key);
	n->type = X_ELEM_STR;
	n->str_val = strdup(val != NULL ? val : "");
	n->next = m->buckets[h];
	m->buckets[h] = n;
	m->size++;
	return 1;
}

int x_map_insert_int(int id, const char* key, int val)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->size >= m->num_buckets * 3 / 4) map_rehash(m);

	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
			curr->type = X_ELEM_INT;
			curr->int_val = val;
			return 1;
		}
		curr = curr->next;
	}
	x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
	n->key = strdup(key);
	n->type = X_ELEM_INT;
	n->int_val = val;
	n->next = m->buckets[h];
	m->buckets[h] = n;
	m->size++;
	return 1;
}

int x_map_insert_float(int id, const char* key, float val)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->size >= m->num_buckets * 3 / 4) map_rehash(m);

	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
			curr->type = X_ELEM_FLOAT;
			curr->float_val = val;
			return 1;
		}
		curr = curr->next;
	}
	x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
	n->key = strdup(key);
	n->type = X_ELEM_FLOAT;
	n->float_val = val;
	n->next = m->buckets[h];
	m->buckets[h] = n;
	m->size++;
	return 1;
}

int x_map_count(int id)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active)
		return 0;
	return s_maps[id]->size;
}

bool x_map_contains_key(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return false;
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return false;
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0) return true;
		curr = curr->next;
	}
	return false;
}

const char* x_map_fetch_str(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return "";
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return "";
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_STR) return curr->str_val ? curr->str_val : "";
			return "";
		}
		curr = curr->next;
	}
	return "";
}

int x_map_fetch_int(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return 0;
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_INT) return curr->int_val;
			if (curr->type == X_ELEM_FLOAT) return (int)curr->float_val;
			if (curr->type == X_ELEM_STR && curr->str_val) return atoi(curr->str_val);
			return 0;
		}
		curr = curr->next;
	}
	return 0;
}

float x_map_fetch_float(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0.0f;
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return 0.0f;
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (curr->type == X_ELEM_FLOAT) return curr->float_val;
			if (curr->type == X_ELEM_INT) return (float)curr->int_val;
			if (curr->type == X_ELEM_STR && curr->str_val) return (float)atof(curr->str_val);
			return 0.0f;
		}
		curr = curr->next;
	}
	return 0.0f;
}

int x_map_fetch_type(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return 0;
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0) return (int)curr->type;
		curr = curr->next;
	}
	return 0;
}

int x_map_get_all_keys(int id, char*** out_keys)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || out_keys == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->size == 0) { *out_keys = NULL; return 0; }
	char** keys = (char**)malloc(m->size * sizeof(char*));
	int count = 0;
	for (int i = 0; i < m->num_buckets; i++)
	{
		x_map_entry_t* curr = m->buckets[i];
		while (curr != NULL)
		{
			if (count < m->size)
			{
				keys[count++] = curr->key;
			}
			curr = curr->next;
		}
	}
	*out_keys = keys;
	return count;
}

int x_list_free_id(int id)
{
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		free_list_items(s_lists[id]);
		s_lists[id]->active = false;
		return 1;
	}
	return 0;
}

int x_map_free_id(int id)
{
	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		free_map_entries(s_maps[id]);
		s_maps[id]->active = false;
		return 1;
	}
	return 0;
}

int x_map_remove_key(int id, const char* key)
{
	if (id <= 0 || id >= MAX_MAPS || s_maps[id] == NULL || !s_maps[id]->active || key == NULL)
		return 0;
	x_map_t* m = s_maps[id];
	if (m->num_buckets == 0 || m->buckets == NULL) return 0;
	unsigned long h = djb2_hash(key) % m->num_buckets;
	x_map_entry_t* prev = NULL;
	x_map_entry_t* curr = m->buckets[h];
	while (curr != NULL)
	{
		if (strcmp(curr->key, key) == 0)
		{
			if (prev != NULL)
				prev->next = curr->next;
			else
				m->buckets[h] = curr->next;
			if (curr->key != NULL) free(curr->key);
			if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
			free(curr);
			m->size--;
			return 1;
		}
		prev = curr;
		curr = curr->next;
	}
	return 0;
}

int x_list_remove_item(int id, int idx)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	x_list_t* l = s_lists[id];
	if (idx < 0 || idx >= l->size)
		return 0;
	if (l->items[idx].type == X_ELEM_STR && l->items[idx].str_val != NULL)
		free(l->items[idx].str_val);
	for (int i = idx; i < l->size - 1; i++)
	{
		l->items[i] = l->items[i + 1];
		sync_item_var(&l->items[i]);
	}
	l->size--;
	return 1;
}

int x_list_clear_items(int id)
{
	if (id <= 0 || id >= MAX_LISTS || s_lists[id] == NULL || !s_lists[id]->active)
		return 0;
	free_list_items(s_lists[id]);
	s_lists[id]->size = 0;
	return 1;
}

