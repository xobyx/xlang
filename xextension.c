#include "xextension.h"
#include "functions.h"
#include "type_stack.h"
#include "func_stack.h"
#include "xcollection.h"
#include "xvec_builtins.h"
#include "xvm.h"
#include "xdiag.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_WIN32) || defined(_MSC_VER)
#include <windows.h>
typedef HMODULE dl_handle_t;
#define DL_OPEN(path) LoadLibraryA(path)
#define DL_SYM(h, sym) GetProcAddress((h), (sym))
#define DL_CLOSE(h) FreeLibrary(h)
#define DL_ERROR() "LoadLibrary failed"
#else
#include <dlfcn.h>
typedef void* dl_handle_t;
#define DL_OPEN(path) dlopen((path), RTLD_NOW | RTLD_GLOBAL)
#define DL_SYM(h, sym) dlsym((h), (sym))
#define DL_CLOSE(h) dlclose(h)
#define DL_ERROR() dlerror()
#endif

#define MAX_EXT_MODULES 64
#define MAX_EXT_HANDLES 64

typedef struct {
	char* name;
	XExtensionFunc* funcs;
	int func_count;
} RegisteredModule;

static RegisteredModule s_modules[MAX_EXT_MODULES];
static int s_module_count = 0;

static dl_handle_t s_handles[MAX_EXT_HANDLES];
static char* s_lib_paths[MAX_EXT_HANDLES];
static int s_handle_count = 0;

static type_def* map_type_kind(XTypeKind k)
{
	switch (k)
	{
	case XLANG_TYPE_INT:    return T_INT;
	case XLANG_TYPE_FLOAT:  return T_FLOAT;
	case XLANG_TYPE_STRING: return T_STRING;
	case XLANG_TYPE_BOOL:   return T_BOOL;
	case XLANG_TYPE_OBJECT: return T_OBJECT;
	case XLANG_TYPE_VOID:   return T_INT;
	case XLANG_TYPE_ANY:
	default:
		return T_ANY;
	}
}

static int ext_register_func(XExtensionContext* ctx, const char* name, XVectorFn fn, int arity, XTypeKind ret, const XTypeKind* params)
{
	(void)ctx;
	if (!name || !fn) return -1;

	func_deftion* fd = new_func_on_stack(funcs);
	if (!fd) return -1;

	fd->func_name = strdup(name);
	fd->function_type = f_main;
	fd->start_parm_count = arity;
	fd->native_kind = NATIVE_KIND_VECTORCALL;
	fd->vector_func = (void*)fn;
	fd->return_type = map_type_kind(ret);
	for (int p = 0; p < arity && p < 8; p++)
	{
		fd->start_func_parmeters[p] = params ? map_type_kind(params[p]) : T_ANY;
	}

	xvm_register_vector_func(name, fn, arity);
	return 0;
}

static int ext_register_module(XExtensionContext* ctx, const char* mod_name, const XExtensionFunc* funcs_list)
{
	if (!mod_name || !funcs_list) return -1;
	if (s_module_count >= MAX_EXT_MODULES) return -1;

	/* Register module name in the compiler type system as a namespace type */
	type_def* td = new_type_stack(types);
	if (td)
	{
		td->type_name = strdup(mod_name);
		td->type_id = T_OBJECT->type_id;
	}

	/* Count functions in the null-terminated table */
	int count = 0;
	while (funcs_list[count].name != NULL)
	{
		count++;
	}

	RegisteredModule* rm = &s_modules[s_module_count++];
	rm->name = strdup(mod_name);
	rm->func_count = count;
	rm->funcs = (XExtensionFunc*)calloc(count + 1, sizeof(XExtensionFunc));

	for (int i = 0; i < count; i++)
	{
		rm->funcs[i] = funcs_list[i];
		rm->funcs[i].name = strdup(funcs_list[i].name);

		/* Namespaced symbol: mod_name.func_name (e.g. fastmath.hypot) */
		char qname[256];
		snprintf(qname, sizeof(qname), "%s.%s", mod_name, funcs_list[i].name);

		ext_register_func(ctx, qname, funcs_list[i].fn, funcs_list[i].arity,
		                  funcs_list[i].return_type, funcs_list[i].param_types);
	}

	return 0;
}

/* String services */
static char* ext_str_new(const char* s)
{
	return s ? strdup(s) : strdup("");
}

static char* ext_str_concat(const char* a, const char* b)
{
	if (!a && !b) return strdup("");
	if (!a) return strdup(b);
	if (!b) return strdup(a);
	size_t la = strlen(a);
	size_t lb = strlen(b);
	char* res = (char*)malloc(la + lb + 1);
	if (!res) return NULL;
	memcpy(res, a, la);
	memcpy(res + la, b, lb);
	res[la + lb] = '\0';
	return res;
}

static char* ext_str_format(const char* fmt, ...)
{
	if (!fmt) return strdup("");
	va_list args;
	va_start(args, fmt);
	char buf[2048];
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return strdup(buf);
}

static int ext_str_len(const char* s)
{
	return s ? (int)strlen(s) : 0;
}

/* Diagnostic & Error services */
static void ext_raise_error(XExtensionContext* ctx, const char* msg)
{
	(void)ctx;
	fprintf(stderr, "Runtime Error in native extension: %s\n", msg ? msg : "unknown");
	exit(1);
}

static void ext_raise_errorf(XExtensionContext* ctx, const char* fmt, ...)
{
	(void)ctx;
	if (!fmt)
	{
		fprintf(stderr, "Runtime Error in native extension\n");
	}
	else
	{
		va_list args;
		va_start(args, fmt);
		char buf[2048];
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		fprintf(stderr, "Runtime Error in native extension: %s\n", buf);
	}
	exit(1);
}

/* Object & Instance reflection services */
static bool ext_instance_is(XValue val, const char* class_name)
{
	if (val.type != VAL_OBJECT || val.as.oval == NULL || !class_name) return false;
	XInstance* inst = (XInstance*)val.as.oval;
	return (inst->klass && inst->klass->name && strcmp(inst->klass->name, class_name) == 0);
}

static const char* ext_instance_class_name(XValue val)
{
	if (val.type != VAL_OBJECT || val.as.oval == NULL) return NULL;
	XInstance* inst = (XInstance*)val.as.oval;
	return inst->klass ? inst->klass->name : NULL;
}

static bool ext_instance_has_field(XValue val, const char* field_name)
{
	if (val.type != VAL_OBJECT || val.as.oval == NULL || !field_name) return false;
	XInstance* inst = (XInstance*)val.as.oval;
	if (!inst->klass) return false;
	return xclass_find_field_slot(inst->klass, field_name) >= 0;
}

static XValue ext_instance_get_field(XValue val, const char* field_name)
{
	if (val.type != VAL_OBJECT || val.as.oval == NULL || !field_name) return xval_null();
	XInstance* inst = (XInstance*)val.as.oval;
	if (!inst->klass) return xval_null();
	int slot = xclass_find_field_slot(inst->klass, field_name);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
		return inst->fields[slot];
	return xval_null();
}

static bool ext_instance_set_field(XValue val, const char* field_name, XValue field_val)
{
	if (val.type != VAL_OBJECT || val.as.oval == NULL || !field_name) return false;
	XInstance* inst = (XInstance*)val.as.oval;
	if (!inst->klass) return false;
	int slot = xclass_find_field_slot(inst->klass, field_name);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		inst->fields[slot] = field_val;
		return true;
	}
	return false;
}

static XValue ext_instance_new(XExtensionContext* ctx, const char* class_name)
{
	XVm* vm = ctx && ctx->vm ? ctx->vm : g_current_vm;
	if (!vm || !class_name) return xval_null();
	XInstance* inst = xinstance_create(vm, class_name);
	return inst ? xval_obj(inst) : xval_null();
}

/* Opaque Pointer & Handle wrapping */
static XValue ext_pointer_wrap(void* ptr)
{
	return xval_pointer(ptr);
}

static void* ext_pointer_unwrap(XValue val)
{
	return xval_as_pointer(val);
}

/* Global variables access */
static XValue ext_get_global(XExtensionContext* ctx, const char* name)
{
	XVm* vm = ctx && ctx->vm ? ctx->vm : g_current_vm;
	if (!vm || !name) return xval_null();
	XValue out = xval_null();
	if (xvm_get_global(vm, name, &out)) return out;
	return xval_null();
}

static bool ext_set_global(XExtensionContext* ctx, const char* name, XValue val)
{
	XVm* vm = ctx && ctx->vm ? ctx->vm : g_current_vm;
	if (!vm || !name) return false;
	xvm_set_global(vm, name, val);
	return true;
}

static void init_extension_context(XExtensionContext* ctx, XVm* vm)
{
	memset(ctx, 0, sizeof(XExtensionContext));
	ctx->api_version = XLANG_API_VERSION;
	ctx->vm = vm ? vm : g_current_vm;

	ctx->register_func = ext_register_func;
	ctx->register_module = ext_register_module;

	/* Collection bridge functions */
	ctx->list_alloc = x_list_alloc;
	ctx->list_append_str = x_list_append_str;
	ctx->list_append_int = x_list_append_int;
	ctx->list_append_float = x_list_append_float;
	ctx->list_count = x_list_count;
	ctx->list_item_str = x_list_item_str;

	ctx->map_alloc = x_map_alloc;
	ctx->map_insert_str = x_map_insert_str;
	ctx->map_insert_int = x_map_insert_int;
	ctx->map_fetch_str = x_map_fetch_str;

	/* String creation and manipulation services */
	ctx->str_new = ext_str_new;
	ctx->str_concat = ext_str_concat;
	ctx->str_format = ext_str_format;
	ctx->str_len = ext_str_len;

	/* Diagnostic & Error services */
	ctx->raise_error = ext_raise_error;
	ctx->raise_errorf = ext_raise_errorf;

	/* Object & Instance reflection services */
	ctx->instance_is = ext_instance_is;
	ctx->instance_class_name = ext_instance_class_name;
	ctx->instance_has_field = ext_instance_has_field;
	ctx->instance_get_field = ext_instance_get_field;
	ctx->instance_set_field = ext_instance_set_field;
	ctx->instance_new = ext_instance_new;

	/* Opaque Pointer & Handle wrapping */
	ctx->pointer_wrap = ext_pointer_wrap;
	ctx->pointer_unwrap = ext_pointer_unwrap;

	/* Global variables access */
	ctx->get_global = ext_get_global;
	ctx->set_global = ext_set_global;
}

void xextension_init(void)
{
	s_module_count = 0;
	s_handle_count = 0;
}

void xextension_cleanup(void)
{
	for (int i = 0; i < s_module_count; i++)
	{
		if (s_modules[i].name) free(s_modules[i].name);
		if (s_modules[i].funcs)
		{
			for (int j = 0; j < s_modules[i].func_count; j++)
			{
				if (s_modules[i].funcs[j].name)
					free((void*)s_modules[i].funcs[j].name);
			}
			free(s_modules[i].funcs);
		}
	}
	s_module_count = 0;

	for (int i = 0; i < s_handle_count; i++)
	{
		if (s_lib_paths[i])
		{
			free(s_lib_paths[i]);
			s_lib_paths[i] = NULL;
		}
		if (s_handles[i])
		{
			DL_CLOSE(s_handles[i]);
			s_handles[i] = NULL;
		}
	}
	s_handle_count = 0;
}

bool xextension_is_module(const char* name)
{
	if (!name) return false;
	for (int i = 0; i < s_module_count; i++)
	{
		if (s_modules[i].name && strcmp(s_modules[i].name, name) == 0)
			return true;
	}
	return false;
}

const XExtensionFunc* xextension_find_func(const char* mod_name, const char* func_name)
{
	if (!mod_name || !func_name) return NULL;
	for (int i = 0; i < s_module_count; i++)
	{
		if (s_modules[i].name && strcmp(s_modules[i].name, mod_name) == 0)
		{
			for (int j = 0; j < s_modules[i].func_count; j++)
			{
				if (s_modules[i].funcs[j].name && strcmp(s_modules[i].funcs[j].name, func_name) == 0)
					return &s_modules[i].funcs[j];
			}
		}
	}
	return NULL;
}

bool xextension_load(const char* lib_path, XVm* vm)
{
	if (!lib_path || *lib_path == '\0') return false;

	if (s_handle_count >= MAX_EXT_HANDLES)
	{
		fprintf(stderr, "Error: Maximum number of native extensions exceeded (%d)\n", MAX_EXT_HANDLES);
		return false;
	}

	dl_handle_t handle = DL_OPEN(lib_path);
	if (!handle)
	{
		fprintf(stderr, "Error loading extension library '%s': %s\n", lib_path, DL_ERROR());
		return false;
	}

	/* Try generic entry point xlang_module_init */
	int (*init_fn)(XExtensionContext*) = (int (*)(XExtensionContext*))DL_SYM(handle, "xlang_module_init");

	/* If not found, try module-specific entry point xlang_init_<modname> */
	if (!init_fn)
	{
		const char* bname = strrchr(lib_path, '/');
#if defined(_WIN32) || defined(_MSC_VER)
		if (!bname) bname = strrchr(lib_path, '\\');
#endif
		bname = bname ? (bname + 1) : lib_path;

		char mod_candidate[128] = {0};
		snprintf(mod_candidate, sizeof(mod_candidate), "%s", bname);
		char* dot = strrchr(mod_candidate, '.');
		if (dot) *dot = '\0';

		/* Strip leading "lib" prefix if present (e.g. libfastmath -> fastmath) */
		const char* stripped = mod_candidate;
		if (strncmp(stripped, "lib", 3) == 0 && strlen(stripped) > 3)
		{
			stripped += 3;
		}

		char init_sym[256];
		snprintf(init_sym, sizeof(init_sym), "xlang_init_%s", stripped);
		init_fn = (int (*)(XExtensionContext*))DL_SYM(handle, init_sym);
	}

	if (!init_fn)
	{
		fprintf(stderr, "Error: Extension '%s' does not export 'xlang_module_init' entrypoint\n", lib_path);
		DL_CLOSE(handle);
		return false;
	}

	XExtensionContext ctx;
	init_extension_context(&ctx, vm);

	int res = init_fn(&ctx);
	if (res != 0)
	{
		fprintf(stderr, "Error: Extension initialization function in '%s' returned %d\n", lib_path, res);
		DL_CLOSE(handle);
		return false;
	}

	s_lib_paths[s_handle_count] = strdup(lib_path);
	s_handles[s_handle_count++] = handle;
	return true;
}

void xextension_emit_llvm_declarations(FILE* out)
{
	if (!out || s_module_count == 0) return;
	fprintf(out, "\n; Native Extension Module Declarations\n");
	for (int m = 0; m < s_module_count; m++)
	{
		RegisteredModule* mod = &s_modules[m];
		for (int f = 0; f < mod->func_count; f++)
		{
			XExtensionFunc* fn = &mod->funcs[f];
			const char* ret_s = "void";
			switch (fn->return_type)
			{
			case XLANG_TYPE_INT:    ret_s = "i32"; break;
			case XLANG_TYPE_FLOAT:  ret_s = "double"; break;
			case XLANG_TYPE_STRING: ret_s = "i8*"; break;
			case XLANG_TYPE_BOOL:   ret_s = "i1"; break;
			case XLANG_TYPE_OBJECT: ret_s = "i8*"; break;
			default: ret_s = "void"; break;
			}

			fprintf(out, "declare %s @%s_%s(", ret_s, mod->name, fn->name);
			for (int p = 0; p < fn->arity; p++)
			{
				const char* parm_s = "i32";
				switch (fn->param_types[p])
				{
				case XLANG_TYPE_INT:    parm_s = "i32"; break;
				case XLANG_TYPE_FLOAT:  parm_s = "double"; break;
				case XLANG_TYPE_STRING: parm_s = "i8*"; break;
				case XLANG_TYPE_BOOL:   parm_s = "i1"; break;
				case XLANG_TYPE_OBJECT: parm_s = "i8*"; break;
				default: parm_s = "i32"; break;
				}
				if (p > 0) fprintf(out, ", ");
				fprintf(out, "%s", parm_s);
			}
			fprintf(out, ")\n");
		}
	}
}

int xextension_get_loaded_lib_count(void)
{
	return s_handle_count;
}

const char* xextension_get_loaded_lib_path(int index)
{
	if (index >= 0 && index < s_handle_count)
		return s_lib_paths[index];
	return NULL;
}
