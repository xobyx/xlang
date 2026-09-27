#ifndef XLLVM_JIT_H
#define XLLVM_JIT_H

#include "xast.h"
#include "xvm.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct XLLVMJit XLLVMJit;

/* Returns true if LLVM JIT support is compiled into this binary */
bool xllvm_jit_is_supported(void);

/* Create an in-process LLVM ORC JIT engine instance. Returns NULL on failure or if unsupported. */
XLLVMJit* xllvm_jit_create(void);

/* Dispose of the JIT engine instance and release all associated resources */
void xllvm_jit_free(XLLVMJit* jit);

/* Add textual LLVM IR into the JIT engine */
bool xllvm_jit_add_ir(XLLVMJit* jit, const char* ir_code, size_t ir_len, const char* module_name);

/* Compile an AstProgram directly into the JIT engine */
bool xllvm_jit_add_program(XLLVMJit* jit, const AstProgram* prog, const char* source_file);

/* Resolve an address for a symbol name (e.g. "main", "my_func", "Class_method") */
void* xllvm_jit_lookup(XLLVMJit* jit, const char* symbol_name);

/* Run native main(argc, argv) in the JIT engine */
int xllvm_jit_run_main(XLLVMJit* jit, int argc, char** argv);

/* Compile and execute an AstProgram directly in-process via JIT */
int xllvm_jit_run_program(const AstProgram* prog, const char* source_file, int argc, char** argv);

/* Bind JIT-compiled native functions to matching XFunction instances in an XVm / chunk */
int xllvm_jit_bind_vm(XLLVMJit* jit, XVm* vm, const AstProgram* prog);

#ifdef __cplusplus
}
#endif

#endif /* XLLVM_JIT_H */
