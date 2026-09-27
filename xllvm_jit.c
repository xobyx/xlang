#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "xllvm_jit.h"
#include "xllvm.h"
#include "xllvm_rt.h"
#include "xdiag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if XLANG_HAS_LLVM_JIT

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Orc.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Error.h>

struct XLLVMJit {
	LLVMOrcLLJITRef jit;
	LLVMOrcJITDylibRef main_jd;
	LLVMOrcThreadSafeContextRef ts_ctx;
	LLVMContextRef ctx;
};

static void log_llvm_error(LLVMErrorRef err, const char* context_msg)
{
	if (err == LLVMErrorSuccess) return;
	char* msg = LLVMGetErrorMessage(err);
	fprintf(stderr, "LLVM JIT Error [%s]: %s\n", context_msg ? context_msg : "general", msg ? msg : "unknown");
	LLVMDisposeErrorMessage(msg);
}

bool xllvm_jit_is_supported(void)
{
	return true;
}

XLLVMJit* xllvm_jit_create(void)
{
	static bool s_targets_inited = false;
	if (!s_targets_inited)
	{
		LLVMInitializeNativeTarget();
		LLVMInitializeNativeAsmPrinter();
		s_targets_inited = true;
	}

	XLLVMJit* jit = (XLLVMJit*)calloc(1, sizeof(XLLVMJit));
	if (!jit) return NULL;

	LLVMOrcLLJITBuilderRef builder = LLVMOrcCreateLLJITBuilder();
	LLVMErrorRef err = LLVMOrcCreateLLJIT(&jit->jit, builder);
	if (err != LLVMErrorSuccess)
	{
		log_llvm_error(err, "LLVMOrcCreateLLJIT");
		free(jit);
		return NULL;
	}

	jit->main_jd = LLVMOrcLLJITGetMainJITDylib(jit->jit);
	char prefix = LLVMOrcLLJITGetGlobalPrefix(jit->jit);

	LLVMOrcDefinitionGeneratorRef proc_gen = NULL;
	err = LLVMOrcCreateDynamicLibrarySearchGeneratorForProcess(&proc_gen, prefix, NULL, NULL);
	if (err == LLVMErrorSuccess && proc_gen != NULL)
	{
		LLVMOrcJITDylibAddGenerator(jit->main_jd, proc_gen);
	}
	else if (err != LLVMErrorSuccess)
	{
		log_llvm_error(err, "CreateDynamicLibrarySearchGeneratorForProcess");
	}

	jit->ts_ctx = LLVMOrcCreateNewThreadSafeContext();
	jit->ctx = LLVMOrcThreadSafeContextGetContext(jit->ts_ctx);

	return jit;
}

void xllvm_jit_free(XLLVMJit* jit)
{
	if (!jit) return;
	if (jit->ts_ctx)
	{
		LLVMOrcDisposeThreadSafeContext(jit->ts_ctx);
		jit->ts_ctx = NULL;
	}
	if (jit->jit)
	{
		LLVMErrorRef err = LLVMOrcDisposeLLJIT(jit->jit);
		if (err != LLVMErrorSuccess)
		{
			log_llvm_error(err, "LLVMOrcDisposeLLJIT");
		}
		jit->jit = NULL;
	}
	free(jit);
}

bool xllvm_jit_add_ir(XLLVMJit* jit, const char* ir_code, size_t ir_len, const char* module_name)
{
	if (!jit || !ir_code || ir_len == 0) return false;

	LLVMMemoryBufferRef mem_buf = LLVMCreateMemoryBufferWithMemoryRange(
		ir_code, ir_len, module_name ? module_name : "jit_module", 0);
	if (!mem_buf)
	{
		fprintf(stderr, "LLVM JIT Error: Failed to allocate memory buffer for IR.\n");
		return false;
	}

	LLVMModuleRef mod = NULL;
	char* parse_err = NULL;
	if (LLVMParseIRInContext(jit->ctx, mem_buf, &mod, &parse_err))
	{
		fprintf(stderr, "LLVM JIT IR Parse Error: %s\n", parse_err ? parse_err : "syntax error in IR");
		if (parse_err) LLVMDisposeMessage(parse_err);
		return false;
	}

	LLVMOrcThreadSafeModuleRef ts_mod = LLVMOrcCreateNewThreadSafeModule(mod, jit->ts_ctx);
	LLVMErrorRef err = LLVMOrcLLJITAddLLVMIRModule(jit->jit, jit->main_jd, ts_mod);
	if (err != LLVMErrorSuccess)
	{
		log_llvm_error(err, "LLVMOrcLLJITAddLLVMIRModule");
		return false;
	}

	return true;
}

bool xllvm_jit_add_program(XLLVMJit* jit, const AstProgram* prog, const char* source_file)
{
	if (!jit || !prog) return false;

	char* buf = NULL;
	size_t buf_len = 0;
	FILE* memfile = open_memstream(&buf, &buf_len);
	if (!memfile)
	{
		fprintf(stderr, "LLVM JIT Error: open_memstream failed.\n");
		return false;
	}

	XLLVMConfig cfg = xllvm_default_config();
	bool ok = xllvm_emit_program(prog, source_file, memfile, &cfg);
	fclose(memfile);

	if (!ok || buf == NULL || buf_len == 0)
	{
		fprintf(stderr, "LLVM JIT Error: Failed to generate LLVM IR for program.\n");
		if (buf) free(buf);
		return false;
	}

	bool res = xllvm_jit_add_ir(jit, buf, buf_len, source_file);
	free(buf);
	return res;
}

void* xllvm_jit_lookup(XLLVMJit* jit, const char* symbol_name)
{
	if (!jit || !symbol_name) return NULL;
	LLVMOrcExecutorAddress addr = 0;
	LLVMErrorRef err = LLVMOrcLLJITLookup(jit->jit, &addr, symbol_name);
	if (err != LLVMErrorSuccess)
	{
		LLVMConsumeError(err);
		return NULL;
	}
	return (void*)(uintptr_t)addr;
}

int xllvm_jit_run_main(XLLVMJit* jit, int argc, char** argv)
{
	if (!jit) return -1;
	void* main_ptr = xllvm_jit_lookup(jit, "main");
	if (!main_ptr)
	{
		fprintf(stderr, "LLVM JIT Error: Symbol 'main' not found in JIT session.\n");
		return -1;
	}
	int (*native_main)(int, char**) = (int (*)(int, char**))main_ptr;
	return native_main(argc, argv);
}

int xllvm_jit_run_program(const AstProgram* prog, const char* source_file, int argc, char** argv)
{
	if (!prog) return 1;
	xllvm_rt_init(argc, argv);

	XLLVMJit* jit = xllvm_jit_create();
	if (!jit)
	{
		fprintf(stderr, "Error: Failed to initialize in-process LLVM JIT engine.\n");
		return 1;
	}

	if (!xllvm_jit_add_program(jit, prog, source_file))
	{
		fprintf(stderr, "Error: Failed to compile program into LLVM JIT.\n");
		xllvm_jit_free(jit);
		return 1;
	}

	int ret = xllvm_jit_run_main(jit, argc, argv);
	xllvm_jit_free(jit);
	return ret;
}

int xllvm_jit_bind_vm(XLLVMJit* jit, XVm* vm, const AstProgram* prog)
{
	if (!jit || !vm || !prog) return 0;
	int bound = 0;

	for (int i = 0; i < prog->statement_count; i++)
	{
		AstStmt* stmt = prog->statements[i];
		if (!stmt) continue;

		if (stmt->type == AST_STMT_FUNC_DECL)
		{
			const char* fname = stmt->as.func_decl.name;
			if (!fname) continue;
			void* native_fn = xllvm_jit_lookup(jit, fname);
			if (native_fn)
			{
				XValue val = xval_null();
				char arity_name[256];
				snprintf(arity_name, sizeof(arity_name), "%s#%d", fname, stmt->as.func_decl.param_count);
				if (xvm_get_global(vm, arity_name, &val) || xvm_get_global(vm, fname, &val))
				{
					if (val.type == VAL_CLOSURE && val.as.closureval && val.as.closureval->function)
					{
						val.as.closureval->function->jit_native_entry = native_fn;
						bound++;
					}
					else if (val.type == VAL_FUNCTION && val.as.fnval)
					{
						val.as.fnval->jit_native_entry = native_fn;
						bound++;
					}
				}
			}
		}
		else if (stmt->type == AST_STMT_CLASS_DECL)
		{
			const char* cname = stmt->as.class_decl.name;
			if (!cname) continue;
			XClass* klass = xvm_find_class(vm, cname);
			if (!klass) continue;

			for (int m = 0; m < stmt->as.class_decl.member_count; m++)
			{
				AstStmt* mstmt = stmt->as.class_decl.members[m];
				if (!mstmt || mstmt->type != AST_STMT_FUNC_DECL) continue;
				const char* mname = mstmt->as.func_decl.name;
				if (!mname) continue;

				char mangled[256];
				snprintf(mangled, sizeof(mangled), "%s_%s", cname, mname);
				void* native_m = xllvm_jit_lookup(jit, mangled);
				if (native_m)
				{
					XClosure* clo = xclass_find_method(klass, mname, mstmt->as.func_decl.param_count);
					if (clo && clo->function)
					{
						clo->function->jit_native_entry = native_m;
						bound++;
					}
				}
			}
		}
	}

	return bound;
}

#else /* !XLANG_HAS_LLVM_JIT */

bool xllvm_jit_is_supported(void)
{
	return false;
}

XLLVMJit* xllvm_jit_create(void)
{
	fprintf(stderr, "Error: LLVM JIT support is not compiled into this binary.\n");
	return NULL;
}

void xllvm_jit_free(XLLVMJit* jit)
{
	(void)jit;
}

bool xllvm_jit_add_ir(XLLVMJit* jit, const char* ir_code, size_t ir_len, const char* module_name)
{
	(void)jit; (void)ir_code; (void)ir_len; (void)module_name;
	return false;
}

bool xllvm_jit_add_program(XLLVMJit* jit, const AstProgram* prog, const char* source_file)
{
	(void)jit; (void)prog; (void)source_file;
	return false;
}

void* xllvm_jit_lookup(XLLVMJit* jit, const char* symbol_name)
{
	(void)jit; (void)symbol_name;
	return NULL;
}

int xllvm_jit_run_main(XLLVMJit* jit, int argc, char** argv)
{
	(void)jit; (void)argc; (void)argv;
	return -1;
}

int xllvm_jit_run_program(const AstProgram* prog, const char* source_file, int argc, char** argv)
{
	(void)prog; (void)source_file; (void)argc; (void)argv;
	fprintf(stderr, "Error: LLVM JIT support is not compiled into this binary.\n");
	return 1;
}

int xllvm_jit_bind_vm(XLLVMJit* jit, XVm* vm, const AstProgram* prog)
{
	(void)jit; (void)vm; (void)prog;
	return 0;
}

#endif /* XLANG_HAS_LLVM_JIT */
