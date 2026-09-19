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

static float* new_float_val(float val)
{
	float* f = (float*)malloc(sizeof(float));
	*f = val;
	return f;
}

static const char* get_str_arg(fcall* fc, int index, const char* default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return *p->value_str_ptr;
	}
	else if (p->type_define == T_CHAR || (p->type_define != NULL && p->type_define->type_id == 2))
	{
		if (p->value_char_ptr != NULL)
			return p->value_char_ptr;
	}
	return default_val;
}

static int get_int_arg(fcall* fc, int index, int default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_INT || (p->type_define != NULL && p->type_define->type_id == 3))
	{
		if (p->value_int != NULL) return *p->value_int;
	}
	else if (p->type_define == T_LONG || (p->type_define != NULL && p->type_define->type_id == 0))
	{
		if (p->value_long != NULL) return (int)*p->value_long;
	}
	else if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return atoi(*p->value_str_ptr);
	}
	return default_val;
}

static float get_float_arg(fcall* fc, int index, float default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_FLOAT || (p->type_define != NULL && p->type_define->type_id == 5))
	{
		if (p->value_float != NULL) return *p->value_float;
	}
	else if (p->type_define == T_INT || (p->type_define != NULL && p->type_define->type_id == 3))
	{
		if (p->value_int != NULL) return (float)*p->value_int;
	}
	else if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return (float)atof(*p->value_str_ptr);
	}
	return default_val;
}

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
/* List Functions for xlang                                                  */
/* ------------------------------------------------------------------------- */

void x_list_create(fcall* fc)
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
	if (slot == -1)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	if (s_lists[slot] == NULL)
	{
		s_lists[slot] = (x_list_t*)calloc(1, sizeof(x_list_t));
	}
	x_list_t* l = s_lists[slot];
	l->id = slot;
	l->active = true;
	l->size = 0;
	l->capacity = 0;
	l->items = NULL;
	list_ensure_capacity(l, INITIAL_LIST_CAP);

	fc->_return.value_int = new_int(1, slot);
	fc->_return.type_define = T_INT;
}

void x_list_free(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		free_list_items(s_lists[id]);
		s_lists[id]->active = false;
	}
	fc->_return.value_int = new_int(1, 0);
	fc->_return.type_define = T_INT;
}

void x_list_size(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int sz = 0;
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		sz = s_lists[id]->size;
	}
	fc->_return.value_int = new_int(1, sz);
	fc->_return.type_define = T_INT;
}

void x_list_add(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* val = get_str_arg(fc, 1, "");
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		list_ensure_capacity(l, l->size + 1);
		x_list_item_t* it = &l->items[l->size++];
		it->type = X_ELEM_STR;
		it->str_val = strdup(val != NULL ? val : "");
		sync_item_var(it);
		fc->_return.value_int = new_int(1, l->size);
	}
	else
	{
		fc->_return.value_int = new_int(1, -1);
	}
	fc->_return.type_define = T_INT;
}

void x_list_add_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int val = get_int_arg(fc, 1, 0);
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		list_ensure_capacity(l, l->size + 1);
		x_list_item_t* it = &l->items[l->size++];
		it->type = X_ELEM_INT;
		it->int_val = val;
		sync_item_var(it);
		fc->_return.value_int = new_int(1, l->size);
	}
	else
	{
		fc->_return.value_int = new_int(1, -1);
	}
	fc->_return.type_define = T_INT;
}

void x_list_add_float(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	float val = get_float_arg(fc, 1, 0.0f);
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		list_ensure_capacity(l, l->size + 1);
		x_list_item_t* it = &l->items[l->size++];
		it->type = X_ELEM_FLOAT;
		it->float_val = val;
		sync_item_var(it);
		fc->_return.value_int = new_int(1, l->size);
	}
	else
	{
		fc->_return.value_int = new_int(1, -1);
	}
	fc->_return.type_define = T_INT;
}

void x_list_get(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	char buf[64] = {0};
	const char* res = "";

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			x_list_item_t* it = &l->items[idx];
			if (it->type == X_ELEM_STR)
			{
				res = it->str_val != NULL ? it->str_val : "";
			}
			else if (it->type == X_ELEM_INT)
			{
				snprintf(buf, sizeof(buf), "%d", it->int_val);
				res = buf;
			}
			else if (it->type == X_ELEM_FLOAT)
			{
				snprintf(buf, sizeof(buf), "%f", it->float_val);
				res = buf;
			}
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(strdup(res));
	fc->_return.type_define = T_STRING;
}

void x_list_get_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	int res = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			x_list_item_t* it = &l->items[idx];
			if (it->type == X_ELEM_INT)
				res = it->int_val;
			else if (it->type == X_ELEM_STR && it->str_val != NULL)
				res = atoi(it->str_val);
			else if (it->type == X_ELEM_FLOAT)
				res = (int)it->float_val;
		}
	}
	fc->_return.value_int = new_int(1, res);
	fc->_return.type_define = T_INT;
}

void x_list_get_float(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	float res = 0.0f;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			x_list_item_t* it = &l->items[idx];
			if (it->type == X_ELEM_FLOAT)
				res = it->float_val;
			else if (it->type == X_ELEM_INT)
				res = (float)it->int_val;
			else if (it->type == X_ELEM_STR && it->str_val != NULL)
				res = (float)atof(it->str_val);
		}
	}
	fc->_return.value_float = new_float_val(res);
	fc->_return.type_define = T_FLOAT;
}

void x_list_set(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	const char* val = get_str_arg(fc, 2, "");
	int rc = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			x_list_item_t* it = &l->items[idx];
			if (it->type == X_ELEM_STR && it->str_val != NULL)
				free(it->str_val);
			it->type = X_ELEM_STR;
			it->str_val = strdup(val != NULL ? val : "");
			sync_item_var(it);
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_list_set_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	int val = get_int_arg(fc, 2, 0);
	int rc = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			x_list_item_t* it = &l->items[idx];
			if (it->type == X_ELEM_STR && it->str_val != NULL)
				free(it->str_val);
			it->type = X_ELEM_INT;
			it->int_val = val;
			sync_item_var(it);
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_list_remove_at(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int idx = get_int_arg(fc, 1, -1);
	int rc = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (idx >= 0 && idx < l->size)
		{
			if (l->items[idx].type == X_ELEM_STR && l->items[idx].str_val != NULL)
			{
				free(l->items[idx].str_val);
			}
			for (int i = idx; i < l->size - 1; i++)
			{
				l->items[i] = l->items[i + 1];
				sync_item_var(&l->items[i]);
			}
			l->size--;
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_list_clear(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_STR && l->items[i].str_val != NULL)
			{
				free(l->items[i].str_val);
				l->items[i].str_val = NULL;
			}
		}
		l->size = 0;
	}
	fc->_return.value_int = new_int(1, 0);
	fc->_return.type_define = T_INT;
}

void x_list_contains(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* val = get_str_arg(fc, 1, "");
	int found = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active && val != NULL)
	{
		x_list_t* l = s_lists[id];
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_STR && l->items[i].str_val != NULL && strcmp(l->items[i].str_val, val) == 0)
			{
				found = 1;
				break;
			}
		}
	}
	fc->_return.value_int = new_int(1, found);
	fc->_return.type_define = T_INT;
}

void x_list_contains_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int val = get_int_arg(fc, 1, 0);
	int found = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_INT && l->items[i].int_val == val)
			{
				found = 1;
				break;
			}
		}
	}
	fc->_return.value_int = new_int(1, found);
	fc->_return.type_define = T_INT;
}

void x_list_index_of(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* val = get_str_arg(fc, 1, "");
	int idx = -1;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active && val != NULL)
	{
		x_list_t* l = s_lists[id];
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_STR && l->items[i].str_val != NULL && strcmp(l->items[i].str_val, val) == 0)
			{
				idx = i;
				break;
			}
		}
	}
	fc->_return.value_int = new_int(1, idx);
	fc->_return.type_define = T_INT;
}

void x_list_index_of_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int val = get_int_arg(fc, 1, 0);
	int idx = -1;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		for (int i = 0; i < l->size; i++)
		{
			if (l->items[i].type == X_ELEM_INT && l->items[i].int_val == val)
			{
				idx = i;
				break;
			}
		}
	}
	fc->_return.value_int = new_int(1, idx);
	fc->_return.type_define = T_INT;
}

void x_list_pop(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	char buf[64] = {0};
	char* res = strdup("");

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (l->size > 0)
		{
			x_list_item_t* it = &l->items[--l->size];
			free(res);
			if (it->type == X_ELEM_STR)
			{
				res = it->str_val != NULL ? strdup(it->str_val) : strdup("");
				free(it->str_val);
				it->str_val = NULL;
			}
			else if (it->type == X_ELEM_INT)
			{
				snprintf(buf, sizeof(buf), "%d", it->int_val);
				res = strdup(buf);
			}
			else if (it->type == X_ELEM_FLOAT)
			{
				snprintf(buf, sizeof(buf), "%f", it->float_val);
				res = strdup(buf);
			}
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(res);
	fc->_return.type_define = T_STRING;
}

void x_list_pop_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int res = 0;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		if (l->size > 0)
		{
			x_list_item_t* it = &l->items[--l->size];
			if (it->type == X_ELEM_INT)
				res = it->int_val;
			else if (it->type == X_ELEM_STR && it->str_val != NULL)
			{
				res = atoi(it->str_val);
				free(it->str_val);
				it->str_val = NULL;
			}
			else if (it->type == X_ELEM_FLOAT)
				res = (int)it->float_val;
		}
	}
	fc->_return.value_int = new_int(1, res);
	fc->_return.type_define = T_INT;
}

void x_list_join(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* sep = get_str_arg(fc, 1, ", ");
	char* result = NULL;
	size_t cap = 256;
	size_t len = 0;

	result = (char*)malloc(cap);
	result[0] = '\0';

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		char buf[64];
		for (int i = 0; i < l->size; i++)
		{
			const char* s = "";
			if (l->items[i].type == X_ELEM_STR)
				s = l->items[i].str_val != NULL ? l->items[i].str_val : "";
			else if (l->items[i].type == X_ELEM_INT)
			{
				snprintf(buf, sizeof(buf), "%d", l->items[i].int_val);
				s = buf;
			}
			else if (l->items[i].type == X_ELEM_FLOAT)
			{
				snprintf(buf, sizeof(buf), "%f", l->items[i].float_val);
				s = buf;
			}

			size_t slen = strlen(s);
			size_t seplen = (i > 0 && sep != NULL) ? strlen(sep) : 0;
			while (len + slen + seplen + 1 >= cap)
			{
				cap *= 2;
				result = (char*)realloc(result, cap);
			}
			if (i > 0 && sep != NULL)
			{
				memcpy(result + len, sep, seplen);
				len += seplen;
			}
			memcpy(result + len, s, slen);
			len += slen;
			result[len] = '\0';
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(result);
	fc->_return.type_define = T_STRING;
}

void x_list_to_string(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	char* result = (char*)malloc(256);
	result[0] = '[';
	result[1] = '\0';
	size_t cap = 256;
	size_t len = 1;

	if (id > 0 && id < MAX_LISTS && s_lists[id] != NULL && s_lists[id]->active)
	{
		x_list_t* l = s_lists[id];
		char buf[64];
		for (int i = 0; i < l->size; i++)
		{
			const char* s = "";
			if (l->items[i].type == X_ELEM_STR)
				s = l->items[i].str_val != NULL ? l->items[i].str_val : "";
			else if (l->items[i].type == X_ELEM_INT)
			{
				snprintf(buf, sizeof(buf), "%d", l->items[i].int_val);
				s = buf;
			}
			else if (l->items[i].type == X_ELEM_FLOAT)
			{
				snprintf(buf, sizeof(buf), "%f", l->items[i].float_val);
				s = buf;
			}

			size_t slen = strlen(s);
			size_t seplen = i > 0 ? 2 : 0;
			while (len + slen + seplen + 2 >= cap)
			{
				cap *= 2;
				result = (char*)realloc(result, cap);
			}
			if (i > 0)
			{
				memcpy(result + len, ", ", 2);
				len += 2;
			}
			memcpy(result + len, s, slen);
			len += slen;
			result[len] = '\0';
		}
	}
	result[len++] = ']';
	result[len] = '\0';

	fc->_return.value_str_ptr = get_pptr_string(result);
	fc->_return.type_define = T_STRING;
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
/* Map Functions for xlang                                                   */
/* ------------------------------------------------------------------------- */

void x_map_create(fcall* fc)
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
	if (slot == -1)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	if (s_maps[slot] == NULL)
	{
		s_maps[slot] = (x_map_t*)calloc(1, sizeof(x_map_t));
	}
	x_map_t* m = s_maps[slot];
	m->id = slot;
	m->active = true;
	m->size = 0;
	m->num_buckets = INITIAL_MAP_BUCKETS;
	m->buckets = (x_map_entry_t**)calloc(INITIAL_MAP_BUCKETS, sizeof(x_map_entry_t*));

	fc->_return.value_int = new_int(1, slot);
	fc->_return.type_define = T_INT;
}

void x_map_free(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		free_map_entries(s_maps[id]);
		s_maps[id]->active = false;
	}
	fc->_return.value_int = new_int(1, 0);
	fc->_return.type_define = T_INT;
}

void x_map_size(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int sz = 0;
	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		sz = s_maps[id]->size;
	}
	fc->_return.value_int = new_int(1, sz);
	fc->_return.type_define = T_INT;
}

void x_map_put(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	const char* val = get_str_arg(fc, 2, "");
	int rc = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		if (m->size >= m->num_buckets * 3 / 4)
			map_rehash(m);

		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
				curr->type = X_ELEM_STR;
				curr->str_val = strdup(val != NULL ? val : "");
				rc = 1;
				break;
			}
			curr = curr->next;
		}
		if (curr == NULL)
		{
			x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
			n->key = strdup(key);
			n->type = X_ELEM_STR;
			n->str_val = strdup(val != NULL ? val : "");
			n->next = m->buckets[h];
			m->buckets[h] = n;
			m->size++;
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_map_put_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	int val = get_int_arg(fc, 2, 0);
	int rc = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		if (m->size >= m->num_buckets * 3 / 4)
			map_rehash(m);

		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
				curr->type = X_ELEM_INT;
				curr->int_val = val;
				rc = 1;
				break;
			}
			curr = curr->next;
		}
		if (curr == NULL)
		{
			x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
			n->key = strdup(key);
			n->type = X_ELEM_INT;
			n->int_val = val;
			n->next = m->buckets[h];
			m->buckets[h] = n;
			m->size++;
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_map_put_float(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	float val = get_float_arg(fc, 2, 0.0f);
	int rc = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		if (m->size >= m->num_buckets * 3 / 4)
			map_rehash(m);

		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
				curr->type = X_ELEM_FLOAT;
				curr->float_val = val;
				rc = 1;
				break;
			}
			curr = curr->next;
		}
		if (curr == NULL)
		{
			x_map_entry_t* n = (x_map_entry_t*)malloc(sizeof(x_map_entry_t));
			n->key = strdup(key);
			n->type = X_ELEM_FLOAT;
			n->float_val = val;
			n->next = m->buckets[h];
			m->buckets[h] = n;
			m->size++;
			rc = 1;
		}
	}
	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_map_get(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	char buf[64] = {0};
	const char* res = "";

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_STR)
					res = curr->str_val != NULL ? curr->str_val : "";
				else if (curr->type == X_ELEM_INT)
				{
					snprintf(buf, sizeof(buf), "%d", curr->int_val);
					res = buf;
				}
				else if (curr->type == X_ELEM_FLOAT)
				{
					snprintf(buf, sizeof(buf), "%f", curr->float_val);
					res = buf;
				}
				break;
			}
			curr = curr->next;
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(strdup(res));
	fc->_return.type_define = T_STRING;
}

void x_map_get_int(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	int res = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_INT)
					res = curr->int_val;
				else if (curr->type == X_ELEM_STR && curr->str_val != NULL)
					res = atoi(curr->str_val);
				else if (curr->type == X_ELEM_FLOAT)
					res = (int)curr->float_val;
				break;
			}
			curr = curr->next;
		}
	}
	fc->_return.value_int = new_int(1, res);
	fc->_return.type_define = T_INT;
}

void x_map_get_float(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	float res = 0.0f;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (curr->type == X_ELEM_FLOAT)
					res = curr->float_val;
				else if (curr->type == X_ELEM_INT)
					res = (float)curr->int_val;
				else if (curr->type == X_ELEM_STR && curr->str_val != NULL)
					res = (float)atof(curr->str_val);
				break;
			}
			curr = curr->next;
		}
	}
	fc->_return.value_float = new_float_val(res);
	fc->_return.type_define = T_FLOAT;
}

void x_map_has(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	int found = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				found = 1;
				break;
			}
			curr = curr->next;
		}
	}
	fc->_return.value_int = new_int(1, found);
	fc->_return.type_define = T_INT;
}

void x_map_remove(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	const char* key = get_str_arg(fc, 1, "");
	int removed = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active && key != NULL)
	{
		x_map_t* m = s_maps[id];
		unsigned long h = djb2_hash(key) % m->num_buckets;
		x_map_entry_t* curr = m->buckets[h];
		x_map_entry_t* prev = NULL;
		while (curr != NULL)
		{
			if (strcmp(curr->key, key) == 0)
			{
				if (prev == NULL)
					m->buckets[h] = curr->next;
				else
					prev->next = curr->next;

				if (curr->key != NULL) free(curr->key);
				if (curr->type == X_ELEM_STR && curr->str_val != NULL) free(curr->str_val);
				free(curr);
				m->size--;
				removed = 1;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
	}
	fc->_return.value_int = new_int(1, removed);
	fc->_return.type_define = T_INT;
}

void x_map_clear(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		free_map_entries(s_maps[id]);
		s_maps[id]->num_buckets = INITIAL_MAP_BUCKETS;
		s_maps[id]->buckets = (x_map_entry_t**)calloc(INITIAL_MAP_BUCKETS, sizeof(x_map_entry_t*));
		s_maps[id]->size = 0;
	}
	fc->_return.value_int = new_int(1, 0);
	fc->_return.type_define = T_INT;
}

void x_map_keys(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	size_t cap = 256;
	size_t len = 0;
	char* res = (char*)malloc(cap);
	res[0] = '\0';
	int count = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		x_map_t* m = s_maps[id];
		for (int i = 0; i < m->num_buckets; i++)
		{
			for (x_map_entry_t* curr = m->buckets[i]; curr != NULL; curr = curr->next)
			{
				size_t klen = strlen(curr->key);
				size_t seplen = count > 0 ? 2 : 0;
				while (len + klen + seplen + 1 >= cap)
				{
					cap *= 2;
					res = (char*)realloc(res, cap);
				}
				if (count > 0)
				{
					memcpy(res + len, ", ", 2);
					len += 2;
				}
				memcpy(res + len, curr->key, klen);
				len += klen;
				res[len] = '\0';
				count++;
			}
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(res);
	fc->_return.type_define = T_STRING;
}

void x_map_values(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	size_t cap = 256;
	size_t len = 0;
	char* res = (char*)malloc(cap);
	res[0] = '\0';
	int count = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		x_map_t* m = s_maps[id];
		char buf[64];
		for (int i = 0; i < m->num_buckets; i++)
		{
			for (x_map_entry_t* curr = m->buckets[i]; curr != NULL; curr = curr->next)
			{
				const char* s = "";
				if (curr->type == X_ELEM_STR)
					s = curr->str_val != NULL ? curr->str_val : "";
				else if (curr->type == X_ELEM_INT)
				{
					snprintf(buf, sizeof(buf), "%d", curr->int_val);
					s = buf;
				}
				else if (curr->type == X_ELEM_FLOAT)
				{
					snprintf(buf, sizeof(buf), "%f", curr->float_val);
					s = buf;
				}

				size_t vlen = strlen(s);
				size_t seplen = count > 0 ? 2 : 0;
				while (len + vlen + seplen + 1 >= cap)
				{
					cap *= 2;
					res = (char*)realloc(res, cap);
				}
				if (count > 0)
				{
					memcpy(res + len, ", ", 2);
					len += 2;
				}
				memcpy(res + len, s, vlen);
				len += vlen;
				res[len] = '\0';
				count++;
			}
		}
	}
	fc->_return.value_str_ptr = get_pptr_string(res);
	fc->_return.type_define = T_STRING;
}

void x_map_to_string(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	size_t cap = 256;
	size_t len = 1;
	char* res = (char*)malloc(cap);
	res[0] = '{';
	res[1] = '\0';
	int count = 0;

	if (id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		x_map_t* m = s_maps[id];
		char buf[128];
		for (int i = 0; i < m->num_buckets; i++)
		{
			for (x_map_entry_t* curr = m->buckets[i]; curr != NULL; curr = curr->next)
			{
				const char* v = "";
				if (curr->type == X_ELEM_STR)
					v = curr->str_val != NULL ? curr->str_val : "";
				else if (curr->type == X_ELEM_INT)
				{
					snprintf(buf, sizeof(buf), "%d", curr->int_val);
					v = buf;
				}
				else if (curr->type == X_ELEM_FLOAT)
				{
					snprintf(buf, sizeof(buf), "%f", curr->float_val);
					v = buf;
				}

				char entry_buf[512];
				snprintf(entry_buf, sizeof(entry_buf), "\"%s\": \"%s\"", curr->key, v);
				size_t elen = strlen(entry_buf);
				size_t seplen = count > 0 ? 2 : 0;
				while (len + elen + seplen + 2 >= cap)
				{
					cap *= 2;
					res = (char*)realloc(res, cap);
				}
				if (count > 0)
				{
					memcpy(res + len, ", ", 2);
					len += 2;
				}
				memcpy(res + len, entry_buf, elen);
				len += elen;
				res[len] = '\0';
				count++;
			}
		}
	}
	res[len++] = '}';
	res[len] = '\0';

	fc->_return.value_str_ptr = get_pptr_string(res);
	fc->_return.type_define = T_STRING;
}

void x_map_keys_list(fcall* fc)
{
	int id = get_int_arg(fc, 0, -1);
	int list_slot = -1;

	for (int i = 1; i < MAX_LISTS; i++)
	{
		if (s_lists[i] == NULL || !s_lists[i]->active)
		{
			list_slot = i;
			break;
		}
	}

	if (list_slot != -1 && id > 0 && id < MAX_MAPS && s_maps[id] != NULL && s_maps[id]->active)
	{
		if (s_lists[list_slot] == NULL)
			s_lists[list_slot] = (x_list_t*)calloc(1, sizeof(x_list_t));

		x_list_t* l = s_lists[list_slot];
		l->id = list_slot;
		l->active = true;
		l->size = 0;
		l->capacity = 0;
		l->items = NULL;
		list_ensure_capacity(l, INITIAL_LIST_CAP);

		x_map_t* m = s_maps[id];
		for (int i = 0; i < m->num_buckets; i++)
		{
			for (x_map_entry_t* curr = m->buckets[i]; curr != NULL; curr = curr->next)
			{
				list_ensure_capacity(l, l->size + 1);
				x_list_item_t* it = &l->items[l->size++];
				it->type = X_ELEM_STR;
				it->str_val = strdup(curr->key);
				sync_item_var(it);
			}
		}
	}

	fc->_return.value_int = new_int(1, list_slot);
	fc->_return.type_define = T_INT;
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

