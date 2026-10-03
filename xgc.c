#include "xgc.h"
#include "functions.h"
#include "xlang_main.h"
#include "xcollection.h"
#include "xvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global GC state */
gc_state_t g_gc_state = {
	.allocated_bytes = 0,
	.threshold_bytes = 512 * 1024, /* 512 KB default threshold */
	.total_objects = 0,
	.collections_count = 0,
	.bytes_collected_total = 0,
	.enabled = true,
	.blocks_head = NULL,
	.roots_count = 0
};

extern var start_var;
extern var tinfo[];

static gc_block_t** g_gc_hash_buckets = NULL;
static size_t g_gc_hash_cap = 0;

static inline size_t gc_ptr_hash(const void* ptr)
{
	uintptr_t x = (uintptr_t)ptr;
	x ^= x >> 16;
	x *= 0x45d9f3b;
	x ^= x >> 16;
	return (size_t)x;
}

static void gc_hash_resize(size_t new_cap)
{
	if (new_cap < 1024) new_cap = 1024;
	gc_block_t** new_buckets = (gc_block_t**)calloc(new_cap, sizeof(gc_block_t*));
	if (new_buckets == NULL) return;

	size_t mask = new_cap - 1;
	for (gc_block_t* b = g_gc_state.blocks_head; b != NULL; b = b->next)
	{
		void* ptr = (void*)(b + 1);
		size_t idx = gc_ptr_hash(ptr) & mask;
		b->hash_next = new_buckets[idx];
		new_buckets[idx] = b;
	}

	free(g_gc_hash_buckets);
	g_gc_hash_buckets = new_buckets;
	g_gc_hash_cap = new_cap;
}

static void gc_hash_insert(gc_block_t* b)
{
	if (b == NULL) return;
	if (g_gc_hash_cap == 0 || g_gc_state.total_objects >= g_gc_hash_cap)
	{
		gc_hash_resize(g_gc_hash_cap == 0 ? 1024 : g_gc_hash_cap * 2);
		return;
	}
	void* ptr = (void*)(b + 1);
	size_t idx = gc_ptr_hash(ptr) & (g_gc_hash_cap - 1);
	b->hash_next = g_gc_hash_buckets[idx];
	g_gc_hash_buckets[idx] = b;
}

static void gc_hash_remove(gc_block_t* b)
{
	if (g_gc_hash_buckets == NULL || g_gc_hash_cap == 0 || b == NULL) return;
	void* ptr = (void*)(b + 1);
	size_t idx = gc_ptr_hash(ptr) & (g_gc_hash_cap - 1);
	gc_block_t* prev = NULL;
	for (gc_block_t* cur = g_gc_hash_buckets[idx]; cur != NULL; prev = cur, cur = cur->hash_next)
	{
		if (cur == b)
		{
			if (prev != NULL)
				prev->hash_next = cur->hash_next;
			else
				g_gc_hash_buckets[idx] = cur->hash_next;
			cur->hash_next = NULL;
			return;
		}
	}
}

void gc_init(void)
{
	/* Ensure clean state */
	g_gc_state.allocated_bytes = 0;
	g_gc_state.threshold_bytes = 512 * 1024;
	g_gc_state.total_objects = 0;
	g_gc_state.collections_count = 0;
	g_gc_state.bytes_collected_total = 0;
	g_gc_state.enabled = true;
	g_gc_state.blocks_head = NULL;
	g_gc_state.roots_count = 0;

	if (g_gc_hash_buckets != NULL)
	{
		free(g_gc_hash_buckets);
		g_gc_hash_buckets = NULL;
	}
	g_gc_hash_cap = 0;
	gc_hash_resize(1024);
}

void gc_cleanup(void)
{
	gc_block_t* cur = g_gc_state.blocks_head;
	while (cur != NULL)
	{
		gc_block_t* next = cur->next;
		cur->magic = 0;
		free(cur);
		cur = next;
	}
	g_gc_state.blocks_head = NULL;
	g_gc_state.allocated_bytes = 0;
	g_gc_state.total_objects = 0;
	g_gc_state.roots_count = 0;

	if (g_gc_hash_buckets != NULL)
	{
		free(g_gc_hash_buckets);
		g_gc_hash_buckets = NULL;
	}
	g_gc_hash_cap = 0;
}

void* gc_malloc(size_t size, gc_kind_t kind)
{
	gc_block_t* b = (gc_block_t*)malloc(sizeof(gc_block_t) + size);
	if (b == NULL)
		return NULL;

	b->magic = GC_MAGIC;
	b->size = size;
	b->kind = (uint8_t)kind;
	b->mark = 0;
	b->pinned = 0;
	b->reserved = 0;
	b->hash_next = NULL;

	b->prev = NULL;
	b->next = g_gc_state.blocks_head;
	if (g_gc_state.blocks_head != NULL)
	{
		g_gc_state.blocks_head->prev = b;
	}
	g_gc_state.blocks_head = b;

	g_gc_state.allocated_bytes += size;
	g_gc_state.total_objects++;

	gc_hash_insert(b);

	return (void*)(b + 1);
}

void* gc_calloc(size_t count, size_t size, gc_kind_t kind)
{
	size_t total = count * size;
	void* ptr = gc_malloc(total, kind);
	if (ptr != NULL)
	{
		memset(ptr, 0, total);
	}
	return ptr;
}

bool gc_is_managed(const void* ptr)
{
	if (ptr == NULL || g_gc_hash_buckets == NULL || g_gc_hash_cap == 0) return false;
	if (((uintptr_t)ptr) % sizeof(void*) != 0) return false;

	size_t idx = gc_ptr_hash(ptr) & (g_gc_hash_cap - 1);
	for (gc_block_t* b = g_gc_hash_buckets[idx]; b != NULL; b = b->hash_next)
	{
		if ((void*)(b + 1) == ptr)
		{
			return (b->magic == GC_MAGIC);
		}
	}

	return false;
}

void gc_free(void* ptr)
{
	if (ptr == NULL) return;
	if (!gc_is_managed(ptr))
	{
		free(ptr);
		return;
	}

	gc_block_t* b = ((gc_block_t*)ptr) - 1;
	gc_hash_remove(b);

	/* Unlink from doubly linked list */
	if (b->prev != NULL)
	{
		b->prev->next = b->next;
	}
	else
	{
		g_gc_state.blocks_head = b->next;
	}
	if (b->next != NULL)
	{
		b->next->prev = b->prev;
	}

	if (g_gc_state.allocated_bytes >= b->size)
		g_gc_state.allocated_bytes -= b->size;
	else
		g_gc_state.allocated_bytes = 0;

	if (g_gc_state.total_objects > 0)
		g_gc_state.total_objects--;

	b->magic = 0;
	free(b);
}

void gc_free_any(void* ptr)
{
	if (ptr == NULL) return;
	if (gc_is_managed(ptr))
	{
		gc_free(ptr);
	}
	else
	{
		free(ptr);
	}
}

void* gc_realloc(void* ptr, size_t new_size)
{
	if (ptr == NULL)
		return gc_malloc(new_size, GC_KIND_RAW);

	if (!gc_is_managed(ptr))
	{
		void* new_ptr = gc_malloc(new_size, GC_KIND_RAW);
		if (new_ptr != NULL)
		{
			memcpy(new_ptr, ptr, new_size);
			free(ptr);
		}
		return new_ptr;
	}

	gc_block_t* b = ((gc_block_t*)ptr) - 1;
	gc_block_t* prev = b->prev;
	gc_block_t* next = b->next;
	uint8_t kind = b->kind;
	uint8_t pinned = b->pinned;
	uint8_t mark = b->mark;
	size_t old_size = b->size;

	gc_hash_remove(b);

	if (g_gc_state.allocated_bytes >= old_size)
		g_gc_state.allocated_bytes -= old_size;

	gc_block_t* nb = (gc_block_t*)realloc(b, sizeof(gc_block_t) + new_size);
	if (nb == NULL)
	{
		g_gc_state.allocated_bytes += old_size;
		gc_hash_insert(b);
		return NULL;
	}

	nb->magic = GC_MAGIC;
	nb->size = new_size;
	nb->kind = kind;
	nb->pinned = pinned;
	nb->mark = mark;
	nb->prev = prev;
	nb->next = next;
	nb->hash_next = NULL;

	if (prev != NULL)
	{
		prev->next = nb;
	}
	else
	{
		g_gc_state.blocks_head = nb;
	}
	if (next != NULL)
	{
		next->prev = nb;
	}

	g_gc_state.allocated_bytes += new_size;
	gc_hash_insert(nb);

	return (void*)(nb + 1);
}

void gc_pin(void* ptr)
{
	if (ptr == NULL) return;
	if (gc_is_managed(ptr))
	{
		gc_block_t* b = ((gc_block_t*)ptr) - 1;
		b->pinned = 1;
	}
	else
	{
		gc_add_root(ptr);
	}
}

void gc_unpin(void* ptr)
{
	if (ptr == NULL) return;
	if (gc_is_managed(ptr))
	{
		gc_block_t* b = ((gc_block_t*)ptr) - 1;
		b->pinned = 0;
	}
	else
	{
		gc_remove_root(ptr);
	}
}

void gc_add_root(void* ptr)
{
	if (ptr == NULL) return;
	for (int i = 0; i < g_gc_state.roots_count; i++)
	{
		if (g_gc_state.roots[i] == ptr) return;
	}
	if (g_gc_state.roots_count < 256)
	{
		g_gc_state.roots[g_gc_state.roots_count++] = ptr;
	}
}

void gc_remove_root(void* ptr)
{
	for (int i = 0; i < g_gc_state.roots_count; i++)
	{
		if (g_gc_state.roots[i] == ptr)
		{
			g_gc_state.roots[i] = g_gc_state.roots[g_gc_state.roots_count - 1];
			g_gc_state.roots_count--;
			return;
		}
	}
}

void gc_mark_ptr(void* ptr)
{
	if (ptr == NULL) return;
	if (!gc_is_managed(ptr)) return;

	gc_block_t* b = ((gc_block_t*)ptr) - 1;
	if (b->mark >= 1) return;
	b->mark = 1;
}

void gc_mark_instance(type_instance* inst)
{
	if (inst == NULL) return;

	if (gc_is_managed(inst))
	{
		gc_block_t* b = ((gc_block_t*)inst) - 1;
		if (b->mark == 2) return; /* Already fully scanned (Black) */
		b->mark = 2;              /* Mark as scanned */
	}

	for (uint32_t f = 0; f < inst->field_count; f++)
	{
		var* p = &inst->fields[f];
		if (p->name != NULL && strcmp(p->name, "this") == 0)
			continue;
		gc_mark_var(p);
	}
	if (inst->base != NULL)
	{
		gc_mark_instance(inst->base);
	}
}

void gc_mark_var(var* v)
{
	if (v == NULL) return;

	if (gc_is_managed(v))
	{
		gc_block_t* b = ((gc_block_t*)v) - 1;
		if (b->kind == GC_KIND_VAR)
		{
			if (b->mark == 2) return; /* Already fully scanned */
			b->mark = 2;              /* Mark as scanned */
		}
	}

	if (v->values != NULL && v->values != &v->inline_val && gc_is_managed(v->values))
	{
		gc_mark_ptr(v->values);
	}

	if (v->type_define == NULL) return;

	if (v->type_define == T_STRING)
	{
		if (v->value_str_ptr != NULL)
		{
			for (int i = 0; i < v->size; i++)
			{
				char* s = *(v->value_str_ptr + i);
				if (s != NULL && gc_is_managed(s))
				{
					gc_mark_ptr(s);
				}
			}
		}
	}
	else if (!is_base_type(v->type_define))
	{
		type_instance* inst = v->value_type_instsance;
		if (inst == NULL && v->values != NULL)
		{
			inst = (type_instance*)v->values;
			v->value_type_instsance = inst;
		}

		/* Collections */
		if (v->type_define->type_name != NULL)
		{
			if (strcmp(v->type_define->type_name, "List") == 0)
			{
				int list_id = type_instance_get_id(inst);
				if (list_id > 0)
				{
					x_collection_gc_mark_list(list_id);
				}
			}
			else if (strcmp(v->type_define->type_name, "Map") == 0 ||
			         strcmp(v->type_define->type_name, "HashMap") == 0)
			{
				int map_id = type_instance_get_id(inst);
				if (map_id > 0)
				{
					x_collection_gc_mark_map(map_id);
				}
			}
		}

		/* User class instance */
		if (inst != NULL)
		{
			gc_mark_instance(inst);
		}
	}
}

void gc_sweep_temp_vars(void)
{
	if (t_varss == NULL) return;

	var* prev = NULL;
	var* cur = t_varss->root;
	while (cur != NULL)
	{
		var* next = cur->stack_next;
		bool keep = false;

		/* Check if pinned in roots */
		for (int i = 0; i < g_gc_state.roots_count; i++)
		{
			if (g_gc_state.roots[i] == cur)
			{
				keep = true;
				break;
			}
		}

		/* Check if managed and marked or pinned */
		if (!keep && gc_is_managed(cur))
		{
			gc_block_t* b = ((gc_block_t*)cur) - 1;
			if (b->mark >= 1 || b->pinned)
			{
				keep = true;
			}
		}

		if (keep)
		{
			prev = cur;
		}
		else
		{
			/* Unlink cur from t_varss */
			if (prev != NULL)
			{
				prev->stack_next = next;
			}
			else
			{
				t_varss->root = next;
			}
			if (t_varss->top == cur)
			{
				t_varss->top = prev;
			}
			if (t_varss->size > 0)
			{
				t_varss->size--;
			}

			/* Free cur itself; unreachable values and strings are swept by mark-and-sweep */
			gc_free_any(cur);
		}
		cur = next;
	}
}

static void gc_mark_vm_value(XValue val)
{
	if (val.type == VAL_STRING && val.as.sval != NULL)
	{
		gc_mark_ptr((void*)val.as.sval);
	}
	else if (val.type == VAL_OBJECT && val.as.oval != NULL)
	{
		XInstance* inst = (XInstance*)val.as.oval;
		const char* cname = (inst->klass && inst->klass->name) ? inst->klass->name : "";
		if (strcmp(cname, "List") == 0) x_collection_gc_mark_list(inst->id);
		else if (strcmp(cname, "Map") == 0 || strcmp(cname, "HashMap") == 0) x_collection_gc_mark_map(inst->id);
	}
}

static void gc_mark_vm(XVm* vm)
{
	if (!vm) return;

	/* Mark all values on the VM evaluation stack */
	for (XValue* slot = vm->stack; slot < vm->stack_top; slot++)
	{
		gc_mark_vm_value(*slot);
	}

	/* Mark all VM global variables */
	for (int i = 0; i < vm->global_count; i++)
	{
		gc_mark_vm_value(vm->globals[i].value);
	}

	/* Mark all VM instances and their fields */
	for (XInstance* inst = vm->all_instances; inst != NULL; inst = inst->next)
	{
		const char* cname = (inst->klass && inst->klass->name) ? inst->klass->name : "";
		if (strcmp(cname, "List") == 0) x_collection_gc_mark_list(inst->id);
		else if (strcmp(cname, "Map") == 0 || strcmp(cname, "HashMap") == 0) x_collection_gc_mark_map(inst->id);

		for (uint32_t f = 0; f < inst->field_count; f++)
		{
			gc_mark_vm_value(inst->fields[f]);
		}
	}
}

size_t gc_collect(void)
{
	if (!g_gc_state.enabled) return 0;

	/* 1. Mark phase: clear all marks */
	for (gc_block_t* b = g_gc_state.blocks_head; b != NULL; b = b->next)
	{
		b->mark = 0;
	}

	/* 2. Mark roots */
	/* 2a. Global variables (varss) */
	if (varss != NULL)
	{
		for (var* v = varss->root; v != NULL; v = v->stack_next)
		{
			gc_mark_var(v);
		}
	}

	/* 2c. Static class properties */
	if (types != NULL)
	{
		for (type_def* td = types->root; td != NULL; td = td->stack_next)
		{
			for (int p = 0; p < td->d_propertys_size; p++)
			{
				gc_mark_var(&td->d_propertys[p]);
			}
		}
	}

	/* 2d. Explicit roots */
	for (int i = 0; i < g_gc_state.roots_count; i++)
	{
		void* root_ptr = g_gc_state.roots[i];
		if (root_ptr != NULL)
		{
			if (gc_is_managed(root_ptr))
			{
				gc_block_t* b = ((gc_block_t*)root_ptr) - 1;
				if (b->kind == GC_KIND_VAR)
				{
					gc_mark_var((var*)root_ptr);
				}
				else if (b->kind == GC_KIND_INSTANCE)
				{
					gc_mark_instance((type_instance*)root_ptr);
				}
				else
				{
					gc_mark_ptr(root_ptr);
				}
			}
			else
			{
				gc_mark_var((var*)root_ptr);
			}
		}
	}

	/* 2e. VM Roots (when executing in bytecode VM) */
	if (g_current_vm != NULL)
	{
		gc_mark_vm(g_current_vm);
	}

	/* 3. Sweep unreferenced collections */
	x_collection_gc_sweep();

	/* 4. Sweep temporary variables */
	gc_sweep_temp_vars();

	/* 5. Sweep managed blocks */
	gc_block_t* curr = g_gc_state.blocks_head;
	size_t collected_bytes = 0;

	while (curr != NULL)
	{
		gc_block_t* next = curr->next;
		if (curr->mark >= 1)
		{
			curr->mark = 0;
		}
		else if (!curr->pinned)
		{
			collected_bytes += curr->size;
			gc_hash_remove(curr);

			if (curr->prev != NULL)
			{
				curr->prev->next = curr->next;
			}
			else
			{
				g_gc_state.blocks_head = curr->next;
			}
			if (curr->next != NULL)
			{
				curr->next->prev = curr->prev;
			}

			if (g_gc_state.allocated_bytes >= curr->size)
				g_gc_state.allocated_bytes -= curr->size;
			else
				g_gc_state.allocated_bytes = 0;

			if (g_gc_state.total_objects > 0)
				g_gc_state.total_objects--;

			curr->magic = 0;
			free(curr);
		}
		curr = next;
	}

	g_gc_state.collections_count++;
	g_gc_state.bytes_collected_total += collected_bytes;

	return collected_bytes;
}

void gc_check_auto(void)
{
	if (!g_gc_state.enabled) return;

	if (g_gc_state.allocated_bytes >= g_gc_state.threshold_bytes)
	{
		gc_collect();
	}
}

void gc_enable(void)
{
	g_gc_state.enabled = true;
}

void gc_disable(void)
{
	g_gc_state.enabled = false;
}

bool gc_is_enabled(void)
{
	return g_gc_state.enabled;
}

void gc_set_threshold(size_t bytes)
{
	if (bytes > 0)
		g_gc_state.threshold_bytes = bytes;
}

size_t gc_get_threshold(void)
{
	return g_gc_state.threshold_bytes;
}

size_t gc_allocated_bytes(void)
{
	return g_gc_state.allocated_bytes;
}

size_t gc_total_objects(void)
{
	return g_gc_state.total_objects;
}

void gc_dump(void)
{
	printf("\n=== XLANG Garbage Collector Diagnostics ===\n");
	printf("  Status:               %s\n", g_gc_state.enabled ? "ENABLED" : "DISABLED");
	printf("  Allocated Memory:     %lu bytes (%.2f KB)\n",
	       (unsigned long)g_gc_state.allocated_bytes, (double)g_gc_state.allocated_bytes / 1024.0);
	printf("  Collection Threshold: %lu bytes (%.2f KB)\n",
	       (unsigned long)g_gc_state.threshold_bytes, (double)g_gc_state.threshold_bytes / 1024.0);
	printf("  Active Heap Objects:  %lu\n", (unsigned long)g_gc_state.total_objects);
	printf("  Collections Triggered:%lu\n", (unsigned long)g_gc_state.collections_count);
	printf("  Total Bytes Swept:    %lu bytes (%.2f KB)\n",
	       (unsigned long)g_gc_state.bytes_collected_total, (double)g_gc_state.bytes_collected_total / 1024.0);
	printf("  Temp Stack Count:     %d vars\n", t_varss != NULL ? t_varss->size : 0);
	printf("============================================\n");
}
