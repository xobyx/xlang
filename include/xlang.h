#ifndef XLANG_H
#define XLANG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32) || defined(__CYGWIN__)
  #define XLANG_EXPORT __declspec(dllexport)
#else
  #define XLANG_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct XVm;
typedef struct XVm XVm;

#ifdef XLANG_INTERNAL
/* When compiling inside xlang core, reuse canonical xir.h definitions */
#include "xir.h"
#else

/* Value Types matching xlang runtime */
typedef enum {
	XLANG_VAL_NULL = 0,
	XLANG_VAL_BOOL,
	XLANG_VAL_INT,
	XLANG_VAL_FLOAT,
	XLANG_VAL_STRING,
	XLANG_VAL_OBJECT,
	XLANG_VAL_CLOSURE,
	XLANG_VAL_FUNCTION
} XValueType;

typedef struct XValue {
	XValueType type;
	union {
		bool bval;
		int64_t ival;
		double fval;
		char* sval;
		void* oval;
	} as;
} XValue;

/* Value Constructors */
static inline XValue xval_null(void)
{
	XValue v;
	v.type = XLANG_VAL_NULL;
	v.as.ival = 0;
	return v;
}

static inline XValue xval_bool(bool b)
{
	XValue v;
	v.type = XLANG_VAL_BOOL;
	v.as.bval = b;
	return v;
}

static inline XValue xval_int(int64_t i)
{
	XValue v;
	v.type = XLANG_VAL_INT;
	v.as.ival = i;
	return v;
}

static inline XValue xval_float(double f)
{
	XValue v;
	v.type = XLANG_VAL_FLOAT;
	v.as.fval = f;
	return v;
}

static inline XValue xval_str(const char* s)
{
	XValue v;
	v.type = XLANG_VAL_STRING;
	v.as.sval = (char*)s;
	return v;
}

static inline XValue xval_obj(void* o)
{
	XValue v;
	v.type = XLANG_VAL_OBJECT;
	v.as.oval = o;
	return v;
}

static inline XValue xval_pointer(void* ptr)
{
	XValue v;
	v.type = XLANG_VAL_INT;
	v.as.ival = (int64_t)(uintptr_t)ptr;
	return v;
}

static inline void* xval_as_pointer(XValue v)
{
	if (v.type == XLANG_VAL_INT) return (void*)(uintptr_t)v.as.ival;
	if (v.type == XLANG_VAL_OBJECT) return v.as.oval;
	return NULL;
}

/* Vectorcall Calling Convention */
typedef XValue (*XVectorFn)(XVm* vm, XValue receiver, int argc, const XValue* args);

#endif /* !XLANG_INTERNAL */

/* Type Identifiers for Extension Signatures */
typedef enum {
	XLANG_TYPE_VOID = 0,
	XLANG_TYPE_INT,
	XLANG_TYPE_FLOAT,
	XLANG_TYPE_STRING,
	XLANG_TYPE_BOOL,
	XLANG_TYPE_OBJECT,
	XLANG_TYPE_ANY
} XTypeKind;

/* Function Registration Descriptor */
typedef struct {
	const char* name;
	XVectorFn fn;
	int arity;
	XTypeKind return_type;
	XTypeKind param_types[8];
} XExtensionFunc;

/* Extension Context API (Passed to xlang_module_init) */
typedef struct XExtensionContext {
	int api_version;

	/* Registration services */
	int (*register_func)(struct XExtensionContext* ctx, const char* name, XVectorFn fn, int arity, XTypeKind ret, const XTypeKind* params);
	int (*register_module)(struct XExtensionContext* ctx, const char* mod_name, const XExtensionFunc* funcs);

	/* Runtime collection services */
	int (*list_alloc)(void);
	int (*list_append_str)(int list_id, const char* str);
	int (*list_append_int)(int list_id, int val);
	int (*list_append_float)(int list_id, float val);
	int (*list_count)(int list_id);
	const char* (*list_item_str)(int list_id, int idx);

	int (*map_alloc)(void);
	int (*map_insert_str)(int map_id, const char* key, const char* val);
	int (*map_insert_int)(int map_id, const char* key, int val);
	const char* (*map_fetch_str)(int map_id, const char* key);

	/* String creation and manipulation services */
	char* (*str_new)(const char* s);
	char* (*str_concat)(const char* a, const char* b);
	char* (*str_format)(const char* fmt, ...);
	int (*str_len)(const char* s);

	/* Diagnostic & Error services */
	void (*raise_error)(struct XExtensionContext* ctx, const char* msg);
	void (*raise_errorf)(struct XExtensionContext* ctx, const char* fmt, ...);

	/* Object & Instance reflection services */
	bool (*instance_is)(XValue val, const char* class_name);
	const char* (*instance_class_name)(XValue val);
	bool (*instance_has_field)(XValue val, const char* field_name);
	XValue (*instance_get_field)(XValue val, const char* field_name);
	bool (*instance_set_field)(XValue val, const char* field_name, XValue field_val);
	XValue (*instance_new)(struct XExtensionContext* ctx, const char* class_name);

	/* Opaque Pointer & Handle wrapping */
	XValue (*pointer_wrap)(void* ptr);
	void* (*pointer_unwrap)(XValue val);

	/* Global variables access */
	XValue (*get_global)(struct XExtensionContext* ctx, const char* name);
	bool (*set_global)(struct XExtensionContext* ctx, const char* name, XValue val);

	/* VM reference */
	XVm* vm;
} XExtensionContext;

#define XLANG_API_VERSION 2

/* Standard Entry Point Signature */
#define XLANG_EXTENSION_ENTRY int xlang_module_init(XExtensionContext* ctx)

#ifdef __cplusplus
}
#endif

#endif /* XLANG_H */
