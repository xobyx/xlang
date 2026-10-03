#ifndef XFFI_H
#define XFFI_H

#include <ffi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "types.h"
#include "xast.h"
#include "xir.h"
#include "xvm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct XFFIFunc {
	char* lib_name;
	char* name;
	void* fn_ptr;
	ffi_cif cif;
	ffi_type** arg_types;
	ffi_type* ret_type;
	int arity;
	char* ret_type_name;
	char** param_type_names;
} XFFIFunc;

void xffi_init(void);
void xffi_add_search_path(const char* path);
void* xffi_load_library(const char* lib_name);
XFFIFunc* xffi_register_func(const char* lib_name, const char* name, const char* ret_type, const char** param_types, int param_count);
bool xffi_process_extern_block(const AstStmt* stmt);
XValue xffi_call(XFFIFunc* fn, XVm* vm, int argc, const XValue* args);
XFFIFunc* xffi_find_func(const char* name);
bool xffi_is_module(const char* name);
int xffi_get_loaded_lib_count(void);
const char* xffi_get_loaded_lib_path(int index);
void xffi_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* XFFI_H */
