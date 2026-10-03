#include "xffi.h"
#include "functions.h"
#include "xvec_builtins.h"
#include "xdiag.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FFI_FUNCS 512
#define MAX_FFI_LIBS 64

static XFFIFunc s_ffi_funcs[MAX_FFI_FUNCS];
static int s_ffi_func_count = 0;

static void* s_lib_handles[MAX_FFI_LIBS];
static char* s_lib_paths[MAX_FFI_LIBS];
static char* s_lib_modules[MAX_FFI_LIBS];
static int s_lib_count = 0;

static bool s_initialized = false;

void xffi_init(void)
{
	if (s_initialized) return;
	s_initialized = true;
}

static const char* extract_module_name(const char* lib_name, char* out_buf, size_t out_size)
{
	if (!lib_name || !out_buf || out_size == 0) return "";
	const char* base = strrchr(lib_name, '/');
	base = base ? base + 1 : lib_name;
	snprintf(out_buf, out_size, "%s", base);

	/* Remove extensions: .so, .dylib, .dll */
	char* dot = strstr(out_buf, ".so");
	if (dot) *dot = '\0';
	dot = strstr(out_buf, ".dylib");
	if (dot) *dot = '\0';
	dot = strstr(out_buf, ".dll");
	if (dot) *dot = '\0';

	return out_buf;
}

#define MAX_SEARCH_PATHS 64
static char* s_search_paths[MAX_SEARCH_PATHS];
static int s_search_path_count = 0;

void xffi_add_search_path(const char* path)
{
	if (!path || !*path || s_search_path_count >= MAX_SEARCH_PATHS) return;
	for (int i = 0; i < s_search_path_count; i++)
	{
		if (strcmp(s_search_paths[i], path) == 0) return;
	}
	s_search_paths[s_search_path_count++] = strdup(path);
}

void* xffi_load_library(const char* lib_name)
{
	if (!lib_name || !*lib_name) return NULL;

	/* Check if already loaded */
	for (int i = 0; i < s_lib_count; i++)
	{
		if (strcmp(s_lib_paths[i], lib_name) == 0)
		{
			return s_lib_handles[i];
		}
	}

	if (s_lib_count >= MAX_FFI_LIBS) return NULL;

	char candidate[1024];
	void* handle = NULL;

	/* 1. Direct path / system name */
	handle = dlopen(lib_name, RTLD_NOW | RTLD_GLOBAL);

	/* 2. Common Linux versions if libm.so or libc.so failed */
	if (!handle && strcmp(lib_name, "libm.so") == 0)
	{
		handle = dlopen("libm.so.6", RTLD_NOW | RTLD_GLOBAL);
	}
	if (!handle && strcmp(lib_name, "libc.so") == 0)
	{
		handle = dlopen("libc.so.6", RTLD_NOW | RTLD_GLOBAL);
	}

	/* 3. Try user-specified search paths (-L dirs) */
	for (int sp = 0; !handle && sp < s_search_path_count; sp++)
	{
		const char* dir = s_search_paths[sp];
		snprintf(candidate, sizeof(candidate), "%s/%s", dir, lib_name);
		handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		if (!handle && strchr(lib_name, '.') == NULL)
		{
			snprintf(candidate, sizeof(candidate), "%s/lib%s.so", dir, lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
		if (!handle && strstr(lib_name, ".so") != NULL && strncmp(lib_name, "lib", 3) != 0)
		{
			snprintf(candidate, sizeof(candidate), "%s/lib%s", dir, lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
	}

	/* 4. Try standard relative library directories: ./lib, ../lib, ../../lib, ., ./bin */
	static const char* default_lib_dirs[] = { "./lib", "../lib", "../../lib", ".", "./bin", "../bin" };
	for (size_t d = 0; !handle && d < sizeof(default_lib_dirs) / sizeof(default_lib_dirs[0]); d++)
	{
		snprintf(candidate, sizeof(candidate), "%s/%s", default_lib_dirs[d], lib_name);
		handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		if (!handle && strchr(lib_name, '.') == NULL)
		{
			snprintf(candidate, sizeof(candidate), "%s/lib%s.so", default_lib_dirs[d], lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
	}

	/* 5. If short name e.g. "m" or "sqlite3", try lib<name>.so */
	if (!handle && strchr(lib_name, '.') == NULL)
	{
		snprintf(candidate, sizeof(candidate), "lib%s.so", lib_name);
		handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		if (!handle)
		{
			snprintf(candidate, sizeof(candidate), "lib%s.so.6", lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
		if (!handle)
		{
			snprintf(candidate, sizeof(candidate), "./lib/lib%s.so", lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
		if (!handle)
		{
			snprintf(candidate, sizeof(candidate), "./lib%s.so", lib_name);
			handle = dlopen(candidate, RTLD_NOW | RTLD_GLOBAL);
		}
	}

	/* Always track the requested library for compiler and linker flags */
	s_lib_handles[s_lib_count] = handle;
	s_lib_paths[s_lib_count] = strdup(lib_name);

	char mod_buf[256];
	extract_module_name(lib_name, mod_buf, sizeof(mod_buf));
	s_lib_modules[s_lib_count] = strdup(mod_buf);

	s_lib_count++;
	return handle;
}

static ffi_type* map_ffi_type(const char* type_name)
{
	if (!type_name) return &ffi_type_sint32;
	if (strcmp(type_name, "int") == 0) return &ffi_type_sint32;
	if (strcmp(type_name, "long") == 0) return &ffi_type_sint64;
	if (strcmp(type_name, "float32") == 0) return &ffi_type_float;
	if (strcmp(type_name, "float") == 0 || strcmp(type_name, "double") == 0 || strcmp(type_name, "float64") == 0) return &ffi_type_double;
	if (strcmp(type_name, "char") == 0) return &ffi_type_sint8;
	if (strcmp(type_name, "bool") == 0) return &ffi_type_uint8;
	if (strcmp(type_name, "string") == 0 || strcmp(type_name, "char*") == 0) return &ffi_type_pointer;
	if (strcmp(type_name, "object") == 0 || strcmp(type_name, "pointer") == 0 || strcmp(type_name, "void*") == 0) return &ffi_type_pointer;
	if (strcmp(type_name, "void") == 0) return &ffi_type_void;
	return &ffi_type_sint32;
}

static type_def* map_type_to_internal(const char* type_name)
{
	if (!type_name) return T_INT;
	if (strcmp(type_name, "int") == 0) return T_INT;
	if (strcmp(type_name, "long") == 0) return T_LONG;
	if (strcmp(type_name, "float") == 0 || strcmp(type_name, "double") == 0) return T_FLOAT;
	if (strcmp(type_name, "char") == 0) return T_CHAR;
	if (strcmp(type_name, "bool") == 0) return T_BOOL;
	if (strcmp(type_name, "string") == 0 || strcmp(type_name, "char*") == 0) return T_STRING;
	if (strcmp(type_name, "object") == 0 || strcmp(type_name, "pointer") == 0) return T_OBJECT;
	if (strcmp(type_name, "void") == 0) return T_INT;
	type_def* td = get_type_by_name((char*)type_name);
	return td ? td : T_ANY;
}

XFFIFunc* xffi_find_func(const char* name)
{
	if (!name) return NULL;
	for (int i = 0; i < s_ffi_func_count; i++)
	{
		if (strcmp(s_ffi_funcs[i].name, name) == 0)
		{
			return &s_ffi_funcs[i];
		}
	}
	return NULL;
}

bool xffi_is_module(const char* name)
{
	if (!name) return false;
	for (int i = 0; i < s_lib_count; i++)
	{
		if (s_lib_modules[i] && strcmp(s_lib_modules[i], name) == 0) return true;
		if (s_lib_paths[i] && strcmp(s_lib_paths[i], name) == 0) return true;
		if (s_lib_modules[i] && strncmp(s_lib_modules[i], "lib", 3) == 0 &&
		    strcmp(s_lib_modules[i] + 3, name) == 0) return true;
	}
	return false;
}

XFFIFunc* xffi_register_func(const char* lib_name, const char* name, const char* ret_type, const char** param_types, int param_count)
{
	if (!lib_name || !name) return NULL;
	void* handle = xffi_load_library(lib_name);
	void* fn_ptr = handle ? dlsym(handle, name) : NULL;

	if (s_ffi_func_count >= MAX_FFI_FUNCS) return NULL;

	XFFIFunc* fn = &s_ffi_funcs[s_ffi_func_count++];
	fn->lib_name = strdup(lib_name);
	fn->name = strdup(name);
	fn->fn_ptr = fn_ptr;
	fn->arity = param_count;
	fn->ret_type_name = strdup(ret_type ? ret_type : "void");
	fn->ret_type = map_ffi_type(fn->ret_type_name);

	fn->arg_types = (ffi_type**)malloc(sizeof(ffi_type*) * (param_count > 0 ? param_count : 1));
	fn->param_type_names = (char**)malloc(sizeof(char*) * (param_count > 0 ? param_count : 1));

	for (int i = 0; i < param_count; i++)
	{
		fn->param_type_names[i] = strdup(param_types[i] ? param_types[i] : "int");
		fn->arg_types[i] = map_ffi_type(fn->param_type_names[i]);
	}

	if (fn_ptr)
	{
		if (ffi_prep_cif(&fn->cif, FFI_DEFAULT_ABI, param_count, fn->ret_type, fn->arg_types) != FFI_OK)
		{
			xdiag_error("E0042", NULL, 0, 0, 0, NULL, "Failed to prepare FFI call interface for '%s'", name);
			return NULL;
		}
	}

	/* Register function in global function stack (funcs) for parser and VM */
	func_deftion* fd = get_func_by_name((char*)name);
	if (!fd)
	{
		fd = new_func_on_stack(funcs);
		if (fd)
		{
			fd->func_name = strdup(name);
		}
	}
	if (fd)
	{
		fd->function_type = f_main;
		fd->start_parm_count = param_count;
		fd->native_kind = NATIVE_KIND_FFI;
		fd->ffi_func = fn;
		fd->return_type = map_type_to_internal(fn->ret_type_name);
		for (int p = 0; p < param_count && p < 100; p++)
		{
			fd->start_func_parmeters[p] = map_type_to_internal(fn->param_type_names[p]);
		}
	}

	/* Also register namespaced alias: <lib>.<func> */
	char mod_buf[256];
	extract_module_name(lib_name, mod_buf, sizeof(mod_buf));
	if (mod_buf[0])
	{
		char qname[512];
		snprintf(qname, sizeof(qname), "%s.%s", mod_buf, name);
		func_deftion* fd_q = get_func_by_name(qname);
		if (!fd_q)
		{
			fd_q = new_func_on_stack(funcs);
			if (fd_q)
			{
				fd_q->func_name = strdup(qname);
			}
		}
		if (fd_q)
		{
			fd_q->function_type = f_main;
			fd_q->start_parm_count = param_count;
			fd_q->native_kind = NATIVE_KIND_FFI;
			fd_q->ffi_func = fn;
			fd_q->return_type = map_type_to_internal(fn->ret_type_name);
			for (int p = 0; p < param_count && p < 100; p++)
			{
				fd_q->start_func_parmeters[p] = map_type_to_internal(fn->param_type_names[p]);
			}
		}

		/* If module starts with "lib", also register stripped alias e.g. m.sin */
		if (strncmp(mod_buf, "lib", 3) == 0 && mod_buf[3] != '\0')
		{
			snprintf(qname, sizeof(qname), "%s.%s", mod_buf + 3, name);
			func_deftion* fd_s = get_func_by_name(qname);
			if (!fd_s)
			{
				fd_s = new_func_on_stack(funcs);
				if (fd_s)
				{
					fd_s->func_name = strdup(qname);
				}
			}
			if (fd_s)
			{
				fd_s->function_type = f_main;
				fd_s->start_parm_count = param_count;
				fd_s->native_kind = NATIVE_KIND_FFI;
				fd_s->ffi_func = fn;
				fd_s->return_type = map_type_to_internal(fn->ret_type_name);
				for (int p = 0; p < param_count && p < 100; p++)
				{
					fd_s->start_func_parmeters[p] = map_type_to_internal(fn->param_type_names[p]);
				}
			}
		}

		/* Register module name as a type so static member access compiles */
		if (!get_type_by_name((char*)mod_buf))
		{
			type_def* td = new_type_stack(types);
			if (td)
			{
				td->type_name = strdup(mod_buf);
				td->type_id = T_OBJECT->type_id;
			}
		}
		if (strncmp(mod_buf, "lib", 3) == 0 && mod_buf[3] != '\0')
		{
			if (!get_type_by_name((char*)(mod_buf + 3)))
			{
				type_def* td2 = new_type_stack(types);
				if (td2)
				{
					td2->type_name = strdup(mod_buf + 3);
					td2->type_id = T_OBJECT->type_id;
				}
			}
		}
	}

	return fn;
}

bool xffi_process_extern_block(const AstStmt* stmt)
{
	if (!stmt || stmt->type != AST_STMT_EXTERN_BLOCK) return false;
	const char* lib_name = stmt->as.extern_block.lib_name;

	/* Attempt to load library (or track it for linking if not loadable at compile time) */
	xffi_load_library(lib_name);

	for (int i = 0; i < stmt->as.extern_block.func_count; i++)
	{
		AstStmt* fs = stmt->as.extern_block.func_decls[i];
		if (!fs || fs->type != AST_STMT_FUNC_DECL) continue;

		const char* fn_name = fs->as.func_decl.name;
		const char* ret_type = fs->as.func_decl.return_type ? fs->as.func_decl.return_type : "void";
		int pcount = fs->as.func_decl.param_count;

		const char* ptypes[32];
		for (int p = 0; p < pcount && p < 32; p++)
		{
			ptypes[p] = fs->as.func_decl.params[p].type_name ? fs->as.func_decl.params[p].type_name : "int";
		}

		xffi_register_func(lib_name, fn_name, ret_type, ptypes, pcount);
	}

	return true;
}

XValue xffi_call(XFFIFunc* fn, XVm* vm, int argc, const XValue* args)
{
	(void)vm;
	if (!fn || !fn->fn_ptr)
	{
		fprintf(stderr, "VM Runtime Error: External function '%s' could not be resolved from '%s'\n",
		        fn ? fn->name : "unknown", (fn && fn->lib_name) ? fn->lib_name : "unknown");
		return xval_null();
	}

	void* arg_ptrs[32];
	union {
		int32_t i32;
		int64_t i64;
		double f64;
		float f32;
		uint8_t u8;
		int8_t i8;
		void* ptr;
	} storage[32];

	int call_arity = fn->arity;
	for (int i = 0; i < call_arity && i < 32; i++)
	{
		XValue a = (i < argc) ? args[i] : xval_null();
		const char* pty = fn->param_type_names[i];

		if (strcmp(pty, "int") == 0)
		{
			storage[i].i32 = (a.type == VAL_INT) ? (int32_t)a.as.ival :
			                 (a.type == VAL_FLOAT) ? (int32_t)a.as.fval : 0;
			arg_ptrs[i] = &storage[i].i32;
		}
		else if (strcmp(pty, "long") == 0)
		{
			storage[i].i64 = (a.type == VAL_INT) ? (int64_t)a.as.ival :
			                 (a.type == VAL_FLOAT) ? (int64_t)a.as.fval : 0;
			arg_ptrs[i] = &storage[i].i64;
		}
		else if (strcmp(pty, "float32") == 0)
		{
			storage[i].f32 = (a.type == VAL_FLOAT) ? (float)a.as.fval :
			                 (a.type == VAL_INT) ? (float)a.as.ival : 0.0f;
			arg_ptrs[i] = &storage[i].f32;
		}
		else if (strcmp(pty, "float") == 0 || strcmp(pty, "double") == 0 || strcmp(pty, "float64") == 0)
		{
			storage[i].f64 = (a.type == VAL_FLOAT) ? a.as.fval :
			                 (a.type == VAL_INT) ? (double)a.as.ival : 0.0;
			arg_ptrs[i] = &storage[i].f64;
		}
		else if (strcmp(pty, "bool") == 0)
		{
			storage[i].u8 = (a.type == VAL_BOOL) ? (uint8_t)a.as.bval :
			                (a.type == VAL_INT) ? (uint8_t)(a.as.ival != 0) : 0;
			arg_ptrs[i] = &storage[i].u8;
		}
		else if (strcmp(pty, "char") == 0)
		{
			storage[i].i8 = (a.type == VAL_INT) ? (int8_t)a.as.ival : 0;
			arg_ptrs[i] = &storage[i].i8;
		}
		else if (strcmp(pty, "string") == 0 || strcmp(pty, "char*") == 0)
		{
			storage[i].ptr = (a.type == VAL_STRING && a.as.sval) ? (void*)a.as.sval : (void*)"";
			arg_ptrs[i] = &storage[i].ptr;
		}
		else
		{
			/* object / pointer */
			storage[i].ptr = (a.type == VAL_OBJECT) ? a.as.oval :
			                 (a.type == VAL_INT) ? (void*)(uintptr_t)a.as.ival : NULL;
			arg_ptrs[i] = &storage[i].ptr;
		}
	}

	union {
		ffi_arg arg;
		int32_t i32;
		int64_t i64;
		double f64;
		float f32;
		uint8_t u8;
		void* ptr;
	} rval;
	memset(&rval, 0, sizeof(rval));

	ffi_call(&fn->cif, FFI_FN(fn->fn_ptr), &rval, arg_ptrs);

	const char* rty = fn->ret_type_name;
	if (strcmp(rty, "void") == 0)
	{
		return xval_null();
	}
	if (strcmp(rty, "int") == 0)
	{
		return xval_int((int64_t)(int32_t)rval.arg);
	}
	if (strcmp(rty, "long") == 0)
	{
		return xval_int((int64_t)rval.i64);
	}
	if (strcmp(rty, "float32") == 0)
	{
		return xval_float((double)rval.f32);
	}
	if (strcmp(rty, "float") == 0 || strcmp(rty, "double") == 0 || strcmp(rty, "float64") == 0)
	{
		return xval_float(rval.f64);
	}
	if (strcmp(rty, "bool") == 0)
	{
		return xval_bool(rval.arg != 0);
	}
	if (strcmp(rty, "char") == 0)
	{
		return xval_int((int64_t)(int8_t)rval.arg);
	}
	if (strcmp(rty, "string") == 0 || strcmp(rty, "char*") == 0)
	{
		return xval_str(rval.ptr ? (const char*)rval.ptr : "");
	}
	if (strcmp(rty, "object") == 0 || strcmp(rty, "pointer") == 0 || strcmp(rty, "void*") == 0)
	{
		return xval_pointer(rval.ptr);
	}

	return xval_int((int64_t)(int32_t)rval.arg);
}

int xffi_get_loaded_lib_count(void)
{
	return s_lib_count;
}

const char* xffi_get_loaded_lib_path(int index)
{
	if (index >= 0 && index < s_lib_count)
		return s_lib_paths[index];
	return NULL;
}

void xffi_cleanup(void)
{
	for (int i = 0; i < s_ffi_func_count; i++)
	{
		free(s_ffi_funcs[i].lib_name);
		free(s_ffi_funcs[i].name);
		free(s_ffi_funcs[i].ret_type_name);
		for (int p = 0; p < s_ffi_funcs[i].arity; p++)
		{
			free(s_ffi_funcs[i].param_type_names[p]);
		}
		free(s_ffi_funcs[i].param_type_names);
		free(s_ffi_funcs[i].arg_types);
	}
	s_ffi_func_count = 0;

	for (int i = 0; i < s_lib_count; i++)
	{
		if (s_lib_handles[i])
		{
			dlclose(s_lib_handles[i]);
			s_lib_handles[i] = NULL;
		}
		free(s_lib_paths[i]);
		s_lib_paths[i] = NULL;
		free(s_lib_modules[i]);
		s_lib_modules[i] = NULL;
	}
	s_lib_count = 0;

	for (int i = 0; i < s_search_path_count; i++)
	{
		free(s_search_paths[i]);
		s_search_paths[i] = NULL;
	}
	s_search_path_count = 0;
	s_initialized = false;
}
