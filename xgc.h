#ifndef XGC_H
#define XGC_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC_MAGIC 0x58474331  /* "XGC1" */

typedef enum {
	GC_KIND_RAW = 0,        /* Generic raw buffer (e.g. primitive values) */
	GC_KIND_VAR,            /* struct var */
	GC_KIND_FCALL,          /* struct fcall */
	GC_KIND_STRING,         /* Dynamic string buffer */
	GC_KIND_INSTANCE,       /* struct type_instance */
	GC_KIND_ARRAY,          /* Array buffer */
	GC_KIND_COLLECTION,     /* Collection internal buffer */
	GC_KIND_MISC
} gc_kind_t;

typedef struct gc_block_s {
	uint32_t magic;
	size_t size;
	uint8_t kind;
	uint8_t mark;
	uint8_t pinned;
	uint8_t reserved;
	struct gc_block_s* next;
	struct gc_block_s* prev;
	struct gc_block_s* hash_next;
} gc_block_t;

typedef struct {
	size_t allocated_bytes;
	size_t threshold_bytes;
	size_t total_objects;
	size_t collections_count;
	size_t bytes_collected_total;
	bool enabled;
	gc_block_t* blocks_head;

	/* Active call frames stack */
	fcall* call_stack[256];
	int call_depth;

	/* Explicitly pinned root pointers */
	void* roots[256];
	int roots_count;
} gc_state_t;

extern gc_state_t g_gc_state;

/* Lifecycle */
void gc_init(void);
void gc_cleanup(void);

/* Allocator */
void* gc_malloc(size_t size, gc_kind_t kind);
void* gc_calloc(size_t count, size_t size, gc_kind_t kind);
void* gc_realloc(void* ptr, size_t new_size);
void  gc_free(void* ptr);
bool  gc_is_managed(const void* ptr);
void  gc_free_any(void* ptr);

/* Roots and Pinning */
void  gc_pin(void* ptr);
void  gc_unpin(void* ptr);
void  gc_add_root(void* ptr);
void  gc_remove_root(void* ptr);

/* Call frame stack tracking */
void  gc_push_frame(fcall* fc);
void  gc_pop_frame(void);
fcall* gc_peek_frame(void);

/* Mark and Sweep */
void  gc_mark_ptr(void* ptr);
void  gc_mark_var(var* v);
void  gc_mark_instance(type_instance* inst);
size_t gc_collect(void);
void  gc_check_auto(void);
void  gc_sweep_temp_vars(void);

/* Configuration and Metrics */
void   gc_enable(void);
void   gc_disable(void);
bool   gc_is_enabled(void);
void   gc_set_threshold(size_t bytes);
size_t gc_get_threshold(void);
size_t gc_allocated_bytes(void);
size_t gc_total_objects(void);
void   gc_dump(void);

/* Native simple function bindings (callable from xlang scripts) */
void x_gc_collect(fcall* fc);
void x_gc_allocated_bytes(fcall* fc);
void x_gc_total_objects(fcall* fc);
void x_gc_enable(fcall* fc);
void x_gc_disable(fcall* fc);
void x_gc_set_threshold(fcall* fc);
void x_gc_dump(fcall* fc);

#ifdef __cplusplus
}
#endif

#endif /* XGC_H */
