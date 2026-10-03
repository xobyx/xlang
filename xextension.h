#ifndef XEXTENSION_H
#define XEXTENSION_H

#ifndef XLANG_INTERNAL
#define XLANG_INTERNAL 1
#endif

#include "include/xlang.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void xextension_init(void);
void xextension_cleanup(void);

/* Load a native shared library (.so / .dll / .dylib) and run its xlang_module_init */
bool xextension_load(const char* lib_path, struct XVm* vm);

/* Query whether an identifier is a registered native extension module */
bool xextension_is_module(const char* name);

/* Get an extension function descriptor by module name and function name */
const XExtensionFunc* xextension_find_func(const char* mod_name, const char* func_name);

/* Emit LLVM IR declarations for registered extension functions */
struct _IO_FILE;
void xextension_emit_llvm_declarations(struct _IO_FILE* out);

/* Query loaded native library paths for AOT linker integration */
int xextension_get_loaded_lib_count(void);
const char* xextension_get_loaded_lib_path(int index);

#ifdef __cplusplus
}
#endif

#endif /* XEXTENSION_H */
