#ifndef XLLVM_H
#define XLLVM_H

#include "xast.h"
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration options for LLVM IR emission */
typedef struct XLLVMConfig {
	bool optimize_tail_calls;
	bool emit_comments;
	bool is_release;        /* True for release mode */
	bool enable_asserts;    /* True to emit assert() checks, false to elide them */
	const char* target_triple;
	const char* data_layout;
} XLLVMConfig;

/* Returns default LLVM config for current host architecture */
XLLVMConfig xllvm_default_config(void);

/* Emit LLVM IR for an AstProgram to a stdio FILE stream */
bool xllvm_emit_program(const AstProgram* prog, const char* source_file, FILE* out, const XLLVMConfig* config);

/* Emit LLVM IR for an AstProgram directly to a target file path (.ll) */
bool xllvm_emit_file(const AstProgram* prog, const char* source_file, const char* out_path, const XLLVMConfig* config);

#ifdef __cplusplus
}
#endif

#endif /* XLLVM_H */
