#include "xllvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define MAX_LOCALS 512
#define MAX_STR_CONSTS 1024
#define MAX_CLASSES 128
#define MAX_FIELDS_PER_CLASS 64

typedef struct LLVMLocalVar {
	char name[64];
	char llvm_type[128];  /* e.g. "i32", "double", "i1", "i8*", "%struct.Point*" */
	char alloca_reg[32];  /* e.g. "%x.addr", "%y" */
	int depth;
} LLVMLocalVar;

typedef struct LLVMLoop {
	char cond_label[32];
	char step_label[32];
	char end_label[32];
	struct LLVMLoop* enclosing;
} LLVMLoop;

typedef struct LLVMStringConst {
	char* text;
	int len; /* byte length including null */
	int id;  /* 0 -> @.str.0 */
} LLVMStringConst;

typedef struct LLVMFieldDesc {
	char name[64];
	char type_name[32];
	char llvm_type[128];
	int slot;
} LLVMFieldDesc;

typedef struct LLVMClassDesc {
	char name[64];
	char base_name[64];
	LLVMFieldDesc fields[MAX_FIELDS_PER_CLASS];
	int field_count;
} LLVMClassDesc;

typedef struct LLVMValue {
	char repr[128];     /* SSA register "%t1" or literal constant "42" */
	char type[128];     /* "i32", "double", "i1", "i8*", "%struct.Point*" */
} LLVMValue;

typedef struct XLLVMEmitter {
	FILE* out;
	const AstProgram* prog;
	const char* source_file;
	XLLVMConfig config;

	int temp_counter;
	int label_counter;

	LLVMStringConst string_constants[MAX_STR_CONSTS];
	int string_constant_count;

	LLVMLocalVar locals[MAX_LOCALS];
	int local_count;
	int scope_depth;

	LLVMLoop* current_loop;

	LLVMClassDesc classes[MAX_CLASSES];
	int class_count;
	const LLVMClassDesc* current_class;

	char current_func_ret_type[128];
	bool current_block_terminated;

	/* Buffer for code within a function or main */
	char* code_buf;
	size_t code_buf_cap;
	size_t code_buf_len;
} XLLVMEmitter;

XLLVMConfig xllvm_default_config(void)
{
	XLLVMConfig cfg;
	cfg.optimize_tail_calls = true;
	cfg.emit_comments = true;
#if defined(__x86_64__) || defined(_M_X64)
	cfg.target_triple = "x86_64-unknown-linux-gnu";
	cfg.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";
#elif defined(__aarch64__) || defined(_M_ARM64)
	cfg.target_triple = "aarch64-unknown-linux-gnu";
	cfg.data_layout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128";
#else
	cfg.target_triple = "unknown-unknown-unknown";
	cfg.data_layout = "";
#endif
	return cfg;
}

/* -------------------------------------------------------------------------
 * Code Buffer Utilities
 * ------------------------------------------------------------------------- */
static void buf_emit(XLLVMEmitter* e, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	int needed = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (needed > 0)
	{
		if (e->code_buf_len + needed + 1 > e->code_buf_cap)
		{
			size_t new_cap = (e->code_buf_cap == 0) ? 16384 : e->code_buf_cap * 2;
			while (new_cap < e->code_buf_len + needed + 1)
				new_cap *= 2;
			e->code_buf = (char*)realloc(e->code_buf, new_cap);
			e->code_buf_cap = new_cap;
		}
		vsnprintf(e->code_buf + e->code_buf_len, needed + 1, fmt, args_copy);
		e->code_buf_len += needed;
	}
	va_end(args_copy);
}

static void buf_clear(XLLVMEmitter* e)
{
	e->code_buf_len = 0;
	if (e->code_buf != NULL)
		e->code_buf[0] = '\0';
}

/* -------------------------------------------------------------------------
 * SSA Identifiers & Label Generators
 * ------------------------------------------------------------------------- */
static int new_temp_id(XLLVMEmitter* e)
{
	return e->temp_counter++;
}

static int new_label_id(XLLVMEmitter* e)
{
	return e->label_counter++;
}

static void enter_scope(XLLVMEmitter* e)
{
	e->scope_depth++;
}

static void exit_scope(XLLVMEmitter* e)
{
	e->scope_depth--;
	while (e->local_count > 0 && e->locals[e->local_count - 1].depth > e->scope_depth)
	{
		e->local_count--;
	}
}

static LLVMLocalVar* find_local(XLLVMEmitter* e, const char* name)
{
	if (!name) return NULL;
	for (int i = e->local_count - 1; i >= 0; i--)
	{
		if (strcmp(e->locals[i].name, name) == 0)
		{
			return &e->locals[i];
		}
	}
	return NULL;
}

static void add_local(XLLVMEmitter* e, const char* name, const char* llvm_type, const char* alloca_reg)
{
	if (e->local_count < MAX_LOCALS)
	{
		LLVMLocalVar* v = &e->locals[e->local_count++];
		snprintf(v->name, sizeof(v->name), "%s", name);
		snprintf(v->llvm_type, sizeof(v->llvm_type), "%s", llvm_type);
		snprintf(v->alloca_reg, sizeof(v->alloca_reg), "%s", alloca_reg);
		v->depth = e->scope_depth;
	}
}

/* -------------------------------------------------------------------------
 * Type Mapping
 * ------------------------------------------------------------------------- */
static const char* xlang_type_to_llvm(XLLVMEmitter* e, const char* type_name)
{
	if (!type_name) return "i32";
	if (strcmp(type_name, "int") == 0) return "i32";
	if (strcmp(type_name, "long") == 0) return "i64";
	if (strcmp(type_name, "float") == 0 || strcmp(type_name, "double") == 0) return "double";
	if (strcmp(type_name, "bool") == 0) return "i1";
	if (strcmp(type_name, "char") == 0) return "i8";
	if (strcmp(type_name, "string") == 0) return "i8*";
	if (strcmp(type_name, "void") == 0) return "void";

	/* Check if known class */
	for (int i = 0; i < e->class_count; i++)
	{
		if (strcmp(e->classes[i].name, type_name) == 0)
		{
			static char class_ptr_buf[128];
			snprintf(class_ptr_buf, sizeof(class_ptr_buf), "%%struct.%s*", type_name);
			return class_ptr_buf;
		}
	}
	return "i8*"; /* Generic pointer fallback */
}

/* -------------------------------------------------------------------------
 * String Constant Pool
 * ------------------------------------------------------------------------- */
static int get_string_const_id(XLLVMEmitter* e, const char* str)
{
	if (!str) str = "";
	for (int i = 0; i < e->string_constant_count; i++)
	{
		if (strcmp(e->string_constants[i].text, str) == 0)
		{
			return e->string_constants[i].id;
		}
	}
	if (e->string_constant_count < MAX_STR_CONSTS)
	{
		int id = e->string_constant_count++;
		e->string_constants[id].text = strdup(str);
		e->string_constants[id].len = (int)strlen(str) + 1;
		e->string_constants[id].id = id;
		return id;
	}
	return 0;
}

static void escape_llvm_string(const char* in, char* out, size_t out_cap)
{
	size_t o = 0;
	for (size_t i = 0; in[i] != '\0' && o + 4 < out_cap; i++)
	{
		unsigned char c = (unsigned char)in[i];
		if (c >= 32 && c <= 126 && c != '\\' && c != '"')
		{
			out[o++] = c;
		}
		else
		{
			snprintf(out + o, out_cap - o, "\\%02X", c);
			o += 3;
		}
	}
	out[o] = '\0';
}

/* -------------------------------------------------------------------------
 * Forward Declarations for Recursive CodeGen
 * ------------------------------------------------------------------------- */
static LLVMValue emit_expr(XLLVMEmitter* e, const AstExpr* expr);
static void emit_stmt(XLLVMEmitter* e, const AstStmt* stmt);

static const AstStmt* find_function(XLLVMEmitter* e, const char* name)
{
	if (!e->prog || !name) return NULL;
	for (int i = 0; i < e->prog->statement_count; i++)
	{
		const AstStmt* stmt = e->prog->statements[i];
		if (stmt->type == AST_STMT_FUNC_DECL && strcmp(stmt->as.func_decl.name, name) == 0)
			return stmt;
	}
	return NULL;
}

static int llvm_type_align(const char* type)
{
	if (type && (strchr(type, '*') != NULL || strcmp(type, "i64") == 0 || strcmp(type, "double") == 0))
		return 8;
	return 4;
}

static const char* get_builtin_ret_type(const char* name)
{
	if (!name) return NULL;
	if (strcmp(name, "_lower") == 0 ||
	    strcmp(name, "_upper") == 0 ||
	    strcmp(name, "_trim") == 0 ||
	    strcmp(name, "substr") == 0 ||
	    strcmp(name, "chr") == 0 ||
	    strcmp(name, "_str_concat") == 0 ||
	    strcmp(name, "_str_from_int") == 0 ||
	    strcmp(name, "_str_from_float") == 0 ||
	    strcmp(name, "map_get") == 0 ||
	    strcmp(name, "map_keys") == 0 ||
	    strcmp(name, "map_values") == 0 ||
	    strcmp(name, "map_to_string") == 0 ||
	    strcmp(name, "list_get") == 0 ||
	    strcmp(name, "list_pop") == 0 ||
	    strcmp(name, "list_join") == 0 ||
	    strcmp(name, "list_to_string") == 0 ||
	    strcmp(name, "socket_recv") == 0 ||
	    strcmp(name, "socket_recvfrom") == 0 ||
	    strcmp(name, "system_getenv") == 0 ||
	    strcmp(name, "get_arg") == 0)
	{
		return "i8*";
	}

	if (strcmp(name, "map_get_float") == 0 ||
	    strcmp(name, "list_get_float") == 0)
	{
		return "double";
	}

	if (strcmp(name, "map_new") == 0 ||
	    strcmp(name, "map_put") == 0 ||
	    strcmp(name, "map_put_int") == 0 ||
	    strcmp(name, "map_put_float") == 0 ||
	    strcmp(name, "map_get_int") == 0 ||
	    strcmp(name, "map_has") == 0 ||
	    strcmp(name, "map_remove") == 0 ||
	    strcmp(name, "map_size") == 0 ||
	    strcmp(name, "map_clear") == 0 ||
	    strcmp(name, "map_free") == 0 ||
	    strcmp(name, "map_keys_list") == 0 ||
	    strcmp(name, "list_new") == 0 ||
	    strcmp(name, "list_add") == 0 ||
	    strcmp(name, "list_add_int") == 0 ||
	    strcmp(name, "list_add_float") == 0 ||
	    strcmp(name, "list_get_int") == 0 ||
	    strcmp(name, "list_set") == 0 ||
	    strcmp(name, "list_set_int") == 0 ||
	    strcmp(name, "list_remove_at") == 0 ||
	    strcmp(name, "list_size") == 0 ||
	    strcmp(name, "list_clear") == 0 ||
	    strcmp(name, "list_contains") == 0 ||
	    strcmp(name, "list_contains_int") == 0 ||
	    strcmp(name, "list_index_of") == 0 ||
	    strcmp(name, "list_index_of_int") == 0 ||
	    strcmp(name, "list_pop_int") == 0 ||
	    strcmp(name, "list_free") == 0 ||
	    strcmp(name, "socket_create") == 0 ||
	    strcmp(name, "socket_connect") == 0 ||
	    strcmp(name, "socket_bind") == 0 ||
	    strcmp(name, "socket_listen") == 0 ||
	    strcmp(name, "socket_accept") == 0 ||
	    strcmp(name, "socket_send") == 0 ||
	    strcmp(name, "socket_close") == 0 ||
	    strcmp(name, "socket_set_timeout") == 0 ||
	    strcmp(name, "socket_set_reuseaddr") == 0 ||
	    strcmp(name, "socket_sendto") == 0 ||
	    strcmp(name, "clock_ms") == 0 ||
	    strcmp(name, "get_argc") == 0 ||
	    strcmp(name, "system_exec") == 0 ||
	    strcmp(name, "system_setenv") == 0 ||
	    strcmp(name, "gc_collect") == 0 ||
	    strcmp(name, "gc_allocated_bytes") == 0 ||
	    strcmp(name, "gc_total_objects") == 0 ||
	    strcmp(name, "gc_enable") == 0 ||
	    strcmp(name, "gc_disable") == 0 ||
	    strcmp(name, "gc_set_threshold") == 0 ||
	    strcmp(name, "gc_dump") == 0 ||
	    strcmp(name, "_len") == 0 ||
	    strcmp(name, "index_of") == 0 ||
	    strcmp(name, "str_eq") == 0)
	{
		return "i32";
	}

	return NULL;
}

static const char* get_builtin_param_type(const char* name, int p)
{
	if (!name) return "i32";
	if (strcmp(name, "_len") == 0 ||
	    strcmp(name, "_trim") == 0 ||
	    strcmp(name, "_lower") == 0 ||
	    strcmp(name, "_upper") == 0 ||
	    strcmp(name, "socket_create") == 0 ||
	    strcmp(name, "system_exec") == 0 ||
	    strcmp(name, "system_getenv") == 0)
	{
		if (p == 0) return "i8*";
	}
	else if (strcmp(name, "substr") == 0)
	{
		if (p == 0) return "i8*";
		return "i32";
	}
	else if (strcmp(name, "index_of") == 0 || strcmp(name, "str_eq") == 0 || strcmp(name, "system_setenv") == 0)
	{
		return "i8*";
	}
	else if (strcmp(name, "chr") == 0 || strcmp(name, "get_arg") == 0)
	{
		return "i32";
	}
	else if (strcmp(name, "socket_send") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
	}
	else if (strcmp(name, "socket_bind") == 0 || strcmp(name, "socket_connect") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
		if (p == 2) return "i32";
	}
	else if (strcmp(name, "map_put") == 0 || strcmp(name, "map_set") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
		if (p == 2) return "i8*";
	}
	else if (strcmp(name, "map_get") == 0 || strcmp(name, "map_has") == 0 || strcmp(name, "map_remove") == 0 || strcmp(name, "map_contains") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
	}
	else if (strcmp(name, "map_put_int") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
		if (p == 2) return "i32";
	}
	else if (strcmp(name, "map_put_float") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
		if (p == 2) return "double";
	}
	else if (strcmp(name, "list_add") == 0 || strcmp(name, "list_contains") == 0 || strcmp(name, "list_index_of") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
	}
	else if (strcmp(name, "list_set") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i32";
		if (p == 2) return "i8*";
	}
	else if (strcmp(name, "list_join") == 0)
	{
		if (p == 0) return "i32";
		if (p == 1) return "i8*";
	}
	return "i32";
}

static bool is_global_builtin(const char* name)
{
	return get_builtin_ret_type(name) != NULL;
}

static const AstStmt* find_class_method_defining_class(XLLVMEmitter* e, const char* class_name, const char* method_name, int arity, char* out_defining_class, size_t out_sz)
{
	if (!e->prog || !class_name || !method_name) return NULL;
	for (int i = 0; i < e->prog->statement_count; i++)
	{
		const AstStmt* stmt = e->prog->statements[i];
		if (stmt->type == AST_STMT_CLASS_DECL && strcmp(stmt->as.class_decl.name, class_name) == 0)
		{
			for (int m = 0; m < stmt->as.class_decl.member_count; m++)
			{
				const AstStmt* mem = stmt->as.class_decl.members[m];
				if (mem->type == AST_STMT_FUNC_DECL && strcmp(mem->as.func_decl.name, method_name) == 0)
				{
					if (arity < 0 || mem->as.func_decl.param_count == arity)
					{
						if (out_defining_class && out_sz > 0)
							snprintf(out_defining_class, out_sz, "%s", class_name);
						return mem;
					}
				}
			}
			/* Also check base class */
			if (stmt->as.class_decl.base_name)
			{
				return find_class_method_defining_class(e, stmt->as.class_decl.base_name, method_name, arity, out_defining_class, out_sz);
			}
		}
	}
	return NULL;
}

static const AstStmt* find_class_method(XLLVMEmitter* e, const char* class_name, const char* method_name, int arity)
{
	return find_class_method_defining_class(e, class_name, method_name, arity, NULL, 0);
}

static bool is_class_method_overloaded(XLLVMEmitter* e, const char* class_name, const char* method_name)
{
	if (!e->prog || !class_name || !method_name) return false;
	int matches = 0;
	for (int i = 0; i < e->prog->statement_count; i++)
	{
		const AstStmt* stmt = e->prog->statements[i];
		if (stmt->type == AST_STMT_CLASS_DECL && strcmp(stmt->as.class_decl.name, class_name) == 0)
		{
			for (int m = 0; m < stmt->as.class_decl.member_count; m++)
			{
				const AstStmt* mem = stmt->as.class_decl.members[m];
				if (mem->type == AST_STMT_FUNC_DECL && strcmp(mem->as.func_decl.name, method_name) == 0)
				{
					matches++;
					if (matches > 1) return true;
				}
			}
		}
	}
	return false;
}

static void get_method_symbol_name(XLLVMEmitter* e, const char* class_name, const char* method_name, int arity, char* out_buf, size_t out_sz)
{
	if (is_class_method_overloaded(e, class_name, method_name) && arity > 0)
		snprintf(out_buf, out_sz, "%s_%s_%d", class_name, method_name, arity);
	else
		snprintf(out_buf, out_sz, "%s_%s", class_name, method_name);
}

/* -------------------------------------------------------------------------
 * Type Conversions / Coercions in LLVM
 * ------------------------------------------------------------------------- */
static LLVMValue coerce_value(XLLVMEmitter* e, LLVMValue val, const char* target_type)
{
	if (strcmp(val.type, target_type) == 0)
		return val;

	LLVMValue res;
	snprintf(res.type, sizeof(res.type), "%s", target_type);

	if (strcmp(val.type, "i32") == 0 && strcmp(target_type, "double") == 0)
	{
		int t = new_temp_id(e);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t);
		buf_emit(e, "  %s = sitofp i32 %s to double\n", res.repr, val.repr);
		return res;
	}
	if (strcmp(val.type, "double") == 0 && strcmp(target_type, "i32") == 0)
	{
		int t = new_temp_id(e);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t);
		buf_emit(e, "  %s = fptosi double %s to i32\n", res.repr, val.repr);
		return res;
	}
	if (strcmp(val.type, "i1") == 0 && strcmp(target_type, "i32") == 0)
	{
		int t = new_temp_id(e);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t);
		buf_emit(e, "  %s = zext i1 %s to i32\n", res.repr, val.repr);
		return res;
	}
	if (strcmp(val.type, "i32") == 0 && strcmp(target_type, "i1") == 0)
	{
		int t = new_temp_id(e);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t);
		buf_emit(e, "  %s = icmp ne i32 %s, 0\n", res.repr, val.repr);
		return res;
	}
	if (strchr(val.type, '*') != NULL && strchr(target_type, '*') != NULL)
	{
		int t = new_temp_id(e);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t);
		buf_emit(e, "  %s = bitcast %s %s to %s\n", res.repr, val.type, val.repr, target_type);
		return res;
	}
	if (strcmp(val.type, "i32") == 0 && strchr(target_type, '*') != NULL)
	{
		int t_ext = new_temp_id(e);
		int t_ptr = new_temp_id(e);
		buf_emit(e, "  %%t%d = zext i32 %s to i64\n", t_ext, val.repr);
		buf_emit(e, "  %%t%d = inttoptr i64 %%t%d to %s\n", t_ptr, t_ext, target_type);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t_ptr);
		return res;
	}
	if (strchr(val.type, '*') != NULL && strcmp(target_type, "i32") == 0)
	{
		int t_i64 = new_temp_id(e);
		int t_i32 = new_temp_id(e);
		buf_emit(e, "  %%t%d = ptrtoint %s %s to i64\n", t_i64, val.type, val.repr);
		buf_emit(e, "  %%t%d = trunc i64 %%t%d to i32\n", t_i32, t_i64);
		snprintf(res.repr, sizeof(res.repr), "%%t%d", t_i32);
		return res;
	}
	return val;
}

/* -------------------------------------------------------------------------
 * Expression Code Generation
 * ------------------------------------------------------------------------- */
static LLVMValue emit_expr(XLLVMEmitter* e, const AstExpr* expr)
{
	LLVMValue val;
	val.repr[0] = '\0';
	strcpy(val.type, "i32");
	if (!expr) return val;

	switch (expr->type)
	{
	case AST_EXPR_LITERAL_INT:
		snprintf(val.repr, sizeof(val.repr), "%lld", (long long)expr->as.int_val);
		strcpy(val.type, "i32");
		return val;

	case AST_EXPR_LITERAL_FLOAT:
		snprintf(val.repr, sizeof(val.repr), "%e", expr->as.float_val);
		strcpy(val.type, "double");
		return val;

	case AST_EXPR_LITERAL_BOOL:
		snprintf(val.repr, sizeof(val.repr), "%s", expr->as.bool_val ? "true" : "false");
		strcpy(val.type, "i1");
		return val;

	case AST_EXPR_LITERAL_NULL:
		strcpy(val.repr, "null");
		strcpy(val.type, "i8*");
		return val;

	case AST_EXPR_LITERAL_STRING:
		{
			int sid = get_string_const_id(e, expr->as.string_val);
			int len = e->string_constants[sid].len;
			int t = new_temp_id(e);
			snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
			strcpy(val.type, "i8*");
			buf_emit(e, "  %s = getelementptr inbounds [%d x i8], [%d x i8]* @.str.%d, i64 0, i64 0\n",
			         val.repr, len, len, sid);
			return val;
		}

	case AST_EXPR_IDENTIFIER:
		{
			const char* name = expr->as.identifier_name;
			LLVMLocalVar* local = find_local(e, name);
			if (local != NULL)
			{
				int t = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
				snprintf(val.type, sizeof(val.type), "%s", local->llvm_type);
				buf_emit(e, "  %s = load %s, %s* %s, align %d\n",
				         val.repr, local->llvm_type, local->llvm_type, local->alloca_reg, llvm_type_align(local->llvm_type));
				return val;
			}
			/* Check if it's a field of this in an instance method */
			if (e->current_class != NULL)
			{
				for (int i = 0; i < e->current_class->field_count; i++)
				{
					if (strcmp(e->current_class->fields[i].name, name) == 0)
					{
						int t_ptr = new_temp_id(e);
						int t_val = new_temp_id(e);
						const char* fty = e->current_class->fields[i].llvm_type;
						buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %%this, i32 0, i32 %d\n",
						         t_ptr, e->current_class->name, e->current_class->name, e->current_class->fields[i].slot);
						buf_emit(e, "  %%t%d = load %s, %s* %%t%d, align %d\n",
						         t_val, fty, fty, t_ptr, llvm_type_align(fty));
						snprintf(val.repr, sizeof(val.repr), "%%t%d", t_val);
						snprintf(val.type, sizeof(val.type), "%s", fty);
						return val;
					}
				}
			}
			/* Fallback to global symbol */
			int t = new_temp_id(e);
			snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
			strcpy(val.type, "i32");
			buf_emit(e, "  %s = load i32, i32* @%s, align 4\n", val.repr, name);
			return val;
		}

	case AST_EXPR_ASSIGN:
		{
			LLVMValue rhs = emit_expr(e, expr->as.assign.value);
			if (expr->as.assign.target->type == AST_EXPR_IDENTIFIER)
			{
				const char* name = expr->as.assign.target->as.identifier_name;
				LLVMLocalVar* local = find_local(e, name);
				if (local != NULL)
				{
					rhs = coerce_value(e, rhs, local->llvm_type);
					buf_emit(e, "  store %s %s, %s* %s, align %d\n",
					         local->llvm_type, rhs.repr, local->llvm_type, local->alloca_reg, llvm_type_align(local->llvm_type));
					return rhs;
				}
				if (e->current_class != NULL)
				{
					for (int i = 0; i < e->current_class->field_count; i++)
					{
						if (strcmp(e->current_class->fields[i].name, name) == 0)
						{
							int t_ptr = new_temp_id(e);
							const char* fty = e->current_class->fields[i].llvm_type;
							rhs = coerce_value(e, rhs, fty);
							buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %%this, i32 0, i32 %d\n",
							         t_ptr, e->current_class->name, e->current_class->name, e->current_class->fields[i].slot);
							buf_emit(e, "  store %s %s, %s* %%t%d, align %d\n",
							         fty, rhs.repr, fty, t_ptr, llvm_type_align(fty));
							return rhs;
						}
					}
				}
				buf_emit(e, "  store %s %s, %s* @%s, align %d\n",
				         rhs.type, rhs.repr, rhs.type, name, llvm_type_align(rhs.type));
				return rhs;
			}
			else if (expr->as.assign.target->type == AST_EXPR_MEMBER)
			{
				LLVMValue obj = emit_expr(e, expr->as.assign.target->as.member.object);
				const char* member_name = expr->as.assign.target->as.member.member_name;
				/* Extract struct class name from type e.g. %struct.Point* */
				char cname[64] = {0};
				if (strncmp(obj.type, "%struct.", 8) == 0)
				{
					const char* start = obj.type + 8;
					const char* end = strchr(start, '*');
					if (end) strncpy(cname, start, end - start);
					else strcpy(cname, start);
				}
				for (int c = 0; c < e->class_count; c++)
				{
					if (strcmp(e->classes[c].name, cname) == 0)
					{
						for (int f = 0; f < e->classes[c].field_count; f++)
						{
							if (strcmp(e->classes[c].fields[f].name, member_name) == 0)
							{
								int t_ptr = new_temp_id(e);
								const char* fty = e->classes[c].fields[f].llvm_type;
								rhs = coerce_value(e, rhs, fty);
								buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
								         t_ptr, cname, cname, obj.repr, e->classes[c].fields[f].slot);
								buf_emit(e, "  store %s %s, %s* %%t%d, align %d\n",
								         fty, rhs.repr, fty, t_ptr, llvm_type_align(fty));
								return rhs;
							}
						}
					}
				}
			}
			return rhs;
		}

	case AST_EXPR_BINARY:
		{
			LLVMValue left = emit_expr(e, expr->as.binary.left);
			LLVMValue right = emit_expr(e, expr->as.binary.right);

			/* 1. String Concatenation */
			if (expr->as.binary.op == BINOP_ADD && (strcmp(left.type, "i8*") == 0 || strcmp(right.type, "i8*") == 0))
			{
				if (strcmp(left.type, "i8*") != 0)
				{
					int t_s = new_temp_id(e);
					if (strcmp(left.type, "double") == 0)
						buf_emit(e, "  %%t%d = call i8* @_str_from_float(double %s)\n", t_s, left.repr);
					else
					{
						left = coerce_value(e, left, "i32");
						buf_emit(e, "  %%t%d = call i8* @_str_from_int(i32 %s)\n", t_s, left.repr);
					}
					snprintf(left.repr, sizeof(left.repr), "%%t%d", t_s);
					strcpy(left.type, "i8*");
				}
				if (strcmp(right.type, "i8*") != 0)
				{
					int t_s = new_temp_id(e);
					if (strcmp(right.type, "double") == 0)
						buf_emit(e, "  %%t%d = call i8* @_str_from_float(double %s)\n", t_s, right.repr);
					else
					{
						right = coerce_value(e, right, "i32");
						buf_emit(e, "  %%t%d = call i8* @_str_from_int(i32 %s)\n", t_s, right.repr);
					}
					snprintf(right.repr, sizeof(right.repr), "%%t%d", t_s);
					strcpy(right.type, "i8*");
				}

				int t_res = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t_res);
				strcpy(val.type, "i8*");
				buf_emit(e, "  %s = call i8* @_str_concat(i8* %s, i8* %s)\n", val.repr, left.repr, right.repr);
				return val;
			}

			/* 2. String Equality / Inequality */
			if ((expr->as.binary.op == BINOP_EQ || expr->as.binary.op == BINOP_NEQ) &&
			    (strcmp(left.type, "i8*") == 0 || strcmp(right.type, "i8*") == 0))
			{
				if (strcmp(left.repr, "null") == 0 || strcmp(right.repr, "null") == 0)
				{
					left = coerce_value(e, left, "i8*");
					right = coerce_value(e, right, "i8*");
					int t_res = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t_res);
					strcpy(val.type, "i1");
					buf_emit(e, "  %s = icmp %s i8* %s, %s\n", val.repr, expr->as.binary.op == BINOP_EQ ? "eq" : "ne", left.repr, right.repr);
					return val;
				}

				left = coerce_value(e, left, "i8*");
				right = coerce_value(e, right, "i8*");
				int t_cmp = new_temp_id(e);
				buf_emit(e, "  %%t%d = call i32 @str_eq(i8* %s, i8* %s)\n", t_cmp, left.repr, right.repr);
				int t_res = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t_res);
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = icmp %s i32 %%t%d, 1\n", val.repr, expr->as.binary.op == BINOP_EQ ? "eq" : "ne", t_cmp);
				return val;
			}

			/* 3. Pointer Equality / Inequality */
			if ((expr->as.binary.op == BINOP_EQ || expr->as.binary.op == BINOP_NEQ) &&
			    (strchr(left.type, '*') != NULL || strchr(right.type, '*') != NULL))
			{
				left = coerce_value(e, left, "i8*");
				right = coerce_value(e, right, "i8*");
				int t_res = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t_res);
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = icmp %s i8* %s, %s\n", val.repr, expr->as.binary.op == BINOP_EQ ? "eq" : "ne", left.repr, right.repr);
				return val;
			}

			bool is_float = (strcmp(left.type, "double") == 0 || strcmp(right.type, "double") == 0);
			if (is_float)
			{
				left = coerce_value(e, left, "double");
				right = coerce_value(e, right, "double");
			}
			else
			{
				left = coerce_value(e, left, "i32");
				right = coerce_value(e, right, "i32");
			}

			int t = new_temp_id(e);
			snprintf(val.repr, sizeof(val.repr), "%%t%d", t);

			switch (expr->as.binary.op)
			{
			case BINOP_ADD:
				strcpy(val.type, is_float ? "double" : "i32");
				buf_emit(e, "  %s = %s %s %s, %s\n", val.repr, is_float ? "fadd" : "add nsw", val.type, left.repr, right.repr);
				break;
			case BINOP_SUB:
				strcpy(val.type, is_float ? "double" : "i32");
				buf_emit(e, "  %s = %s %s %s, %s\n", val.repr, is_float ? "fsub" : "sub nsw", val.type, left.repr, right.repr);
				break;
			case BINOP_MUL:
				strcpy(val.type, is_float ? "double" : "i32");
				buf_emit(e, "  %s = %s %s %s, %s\n", val.repr, is_float ? "fmul" : "mul nsw", val.type, left.repr, right.repr);
				break;
			case BINOP_DIV:
				strcpy(val.type, is_float ? "double" : "i32");
				buf_emit(e, "  %s = %s %s %s, %s\n", val.repr, is_float ? "fdiv" : "sdiv", val.type, left.repr, right.repr);
				break;
			case BINOP_MOD:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = srem i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_EQ:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp oeq double" : "icmp eq i32", left.repr, right.repr);
				break;
			case BINOP_NEQ:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp one double" : "icmp ne i32", left.repr, right.repr);
				break;
			case BINOP_LT:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp olt double" : "icmp slt i32", left.repr, right.repr);
				break;
			case BINOP_LTE:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp ole double" : "icmp sle i32", left.repr, right.repr);
				break;
			case BINOP_GT:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp ogt double" : "icmp sgt i32", left.repr, right.repr);
				break;
			case BINOP_GTE:
				strcpy(val.type, "i1");
				buf_emit(e, "  %s = %s %s, %s\n", val.repr, is_float ? "fcmp oge double" : "icmp sge i32", left.repr, right.repr);
				break;
			case BINOP_AND:
				strcpy(val.type, "i1");
				left = coerce_value(e, left, "i1");
				right = coerce_value(e, right, "i1");
				buf_emit(e, "  %s = and i1 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_OR:
				strcpy(val.type, "i1");
				left = coerce_value(e, left, "i1");
				right = coerce_value(e, right, "i1");
				buf_emit(e, "  %s = or i1 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_BIT_AND:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = and i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_BIT_OR:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = or i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_BIT_XOR:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = xor i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_SHL:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = shl i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			case BINOP_SHR:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = ashr i32 %s, %s\n", val.repr, left.repr, right.repr);
				break;
			default:
				break;
			}
			return val;
		}

	case AST_EXPR_UNARY:
		{
			LLVMValue op_val = emit_expr(e, expr->as.unary.operand);
			int t = new_temp_id(e);
			snprintf(val.repr, sizeof(val.repr), "%%t%d", t);

			switch (expr->as.unary.op)
			{
			case UNOP_NEG:
				if (strcmp(op_val.type, "double") == 0)
				{
					strcpy(val.type, "double");
					buf_emit(e, "  %s = fneg double %s\n", val.repr, op_val.repr);
				}
				else
				{
					strcpy(val.type, "i32");
					buf_emit(e, "  %s = sub nsw i32 0, %s\n", val.repr, op_val.repr);
				}
				break;
			case UNOP_NOT:
				strcpy(val.type, "i1");
				op_val = coerce_value(e, op_val, "i1");
				buf_emit(e, "  %s = xor i1 %s, true\n", val.repr, op_val.repr);
				break;
			case UNOP_BIT_NOT:
				strcpy(val.type, "i32");
				buf_emit(e, "  %s = xor i32 %s, -1\n", val.repr, op_val.repr);
				break;
			default:
				break;
			}
			return val;
		}

	case AST_EXPR_CALL:
		{
			const char* callee = expr->as.call.name;
			int argc = expr->as.call.arg_count;

			/* Standard Output Builtins: print / println / echo */
			if (strcmp(callee, "print") == 0 || strcmp(callee, "println") == 0 || strcmp(callee, "echo") == 0)
			{
				for (int i = 0; i < argc; i++)
				{
					LLVMValue arg = emit_expr(e, expr->as.call.args[i]);
					if (strcmp(arg.type, "i32") == 0)
					{
						int fmt_id = get_string_const_id(e, "%d\n");
						buf_emit(e, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.%d, i64 0, i64 0), i32 %s)\n",
						         fmt_id, arg.repr);
					}
					else if (strcmp(arg.type, "double") == 0)
					{
						int fmt_id = get_string_const_id(e, "%g\n");
						buf_emit(e, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.%d, i64 0, i64 0), double %s)\n",
						         fmt_id, arg.repr);
					}
					else if (strcmp(arg.type, "i8*") == 0)
					{
						int fmt_id = get_string_const_id(e, "%s\n");
						buf_emit(e, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.%d, i64 0, i64 0), i8* %s)\n",
						         fmt_id, arg.repr);
					}
					else if (strcmp(arg.type, "i1") == 0)
					{
						int t_str = new_temp_id(e);
						int str_t = get_string_const_id(e, "True");
						int str_f = get_string_const_id(e, "False");
						buf_emit(e, "  %%t%d = select i1 %s, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.%d, i64 0, i64 0), i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.str.%d, i64 0, i64 0)\n",
						         t_str, arg.repr, str_t, str_f);
						int fmt_id = get_string_const_id(e, "%s\n");
						buf_emit(e, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.%d, i64 0, i64 0), i8* %%t%d)\n",
						         fmt_id, t_str);
					}
				}
				strcpy(val.repr, "0");
				strcpy(val.type, "i32");
				return val;
			}

			/* Builtin: assert(condition, message) */
			if (strcmp(callee, "assert") == 0 && argc >= 1)
			{
				LLVMValue cond = emit_expr(e, expr->as.call.args[0]);
				cond = coerce_value(e, cond, "i1");
				char msg_repr[256];
				if (argc > 1)
				{
					LLVMValue msg = emit_expr(e, expr->as.call.args[1]);
					snprintf(msg_repr, sizeof(msg_repr), "%s", msg.repr);
				}
				else
				{
					int def_msg = get_string_const_id(e, "Assertion failed");
					snprintf(msg_repr, sizeof(msg_repr), "getelementptr inbounds ([17 x i8], [17 x i8]* @.str.%d, i64 0, i64 0)", def_msg);
				}

				int pass_lbl_id = new_label_id(e);
				int fail_lbl_id = new_label_id(e);
				char pass_lbl[32], fail_lbl[32];
				snprintf(pass_lbl, sizeof(pass_lbl), "assert_pass.%d", pass_lbl_id);
				snprintf(fail_lbl, sizeof(fail_lbl), "assert_fail.%d", fail_lbl_id);

				buf_emit(e, "  br i1 %s, label %%%s, label %%%s\n\n", cond.repr, pass_lbl, fail_lbl);
				buf_emit(e, "%s:\n", fail_lbl);
				int err_fmt = get_string_const_id(e, "Assertion failure: %s\n");
				buf_emit(e, "  call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.%d, i64 0, i64 0), i8* %s)\n", err_fmt, msg_repr);
				buf_emit(e, "  call void @exit(i32 1)\n");
				buf_emit(e, "  unreachable\n\n");
				buf_emit(e, "%s:\n", pass_lbl);

				strcpy(val.repr, "0");
				strcpy(val.type, "i32");
				return val;
			}

			/* General User Function Call */
			const AstStmt* fn_decl = find_function(e, callee);
			const char* ret_llvm = "i32";
			if (fn_decl)
			{
				const char* ret_name = fn_decl->as.func_decl.return_type ? fn_decl->as.func_decl.return_type : "void";
				ret_llvm = xlang_type_to_llvm(e, ret_name);
			}
			else if (is_global_builtin(callee))
			{
				ret_llvm = get_builtin_ret_type(callee);
			}

			/* Implicit this method call within an instance method */
			if (!fn_decl && !is_global_builtin(callee) && e->current_class != NULL)
			{
				char def_class[64] = {0};
				const AstStmt* m_decl = find_class_method_defining_class(e, e->current_class->name, callee, argc, def_class, sizeof(def_class));
				if (m_decl != NULL)
				{
					const char* ret_name = m_decl->as.func_decl.return_type ? m_decl->as.func_decl.return_type : "void";
					ret_llvm = xlang_type_to_llvm(e, ret_name);

					LLVMValue evaluated_args[32];
					for (int i = 0; i < argc && i < 32; i++)
					{
						evaluated_args[i] = emit_expr(e, expr->as.call.args[i]);
						if (i < m_decl->as.func_decl.param_count)
						{
							const char* pty = xlang_type_to_llvm(e, m_decl->as.func_decl.params[i].type_name);
							evaluated_args[i] = coerce_value(e, evaluated_args[i], pty);
						}
					}

					const char* actual_class = def_class[0] ? def_class : e->current_class->name;
					char sym_name[128];
					get_method_symbol_name(e, actual_class, callee, argc, sym_name, sizeof(sym_name));

					char this_target_type[128];
					snprintf(this_target_type, sizeof(this_target_type), "%%struct.%s*", actual_class);

					LLVMValue this_val;
					snprintf(this_val.repr, sizeof(this_val.repr), "%%this");
					snprintf(this_val.type, sizeof(this_val.type), "%%struct.%s*", e->current_class->name);
					this_val = coerce_value(e, this_val, this_target_type);

					if (strcmp(ret_llvm, "void") == 0)
					{
						buf_emit(e, "  call void @%s(%s %s", sym_name, this_val.type, this_val.repr);
						for (int i = 0; i < argc && i < 32; i++)
						{
							buf_emit(e, ", %s %s", evaluated_args[i].type, evaluated_args[i].repr);
						}
						buf_emit(e, ")\n");
						strcpy(val.repr, "");
						strcpy(val.type, "void");
						return val;
					}
					else
					{
						int t = new_temp_id(e);
						snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
						strcpy(val.type, ret_llvm);

						buf_emit(e, "  %s = call %s @%s(%s %s", val.repr, ret_llvm, sym_name, this_val.type, this_val.repr);
						for (int i = 0; i < argc && i < 32; i++)
						{
							buf_emit(e, ", %s %s", evaluated_args[i].type, evaluated_args[i].repr);
						}
						buf_emit(e, ")\n");
						return val;
					}
				}
			}

			LLVMValue evaluated_args[32];
			for (int i = 0; i < argc && i < 32; i++)
			{
				evaluated_args[i] = emit_expr(e, expr->as.call.args[i]);
				if (fn_decl && i < fn_decl->as.func_decl.param_count)
				{
					const char* pty = xlang_type_to_llvm(e, fn_decl->as.func_decl.params[i].type_name);
					evaluated_args[i] = coerce_value(e, evaluated_args[i], pty);
				}
				else if (is_global_builtin(callee))
				{
					const char* pty = get_builtin_param_type(callee, i);
					evaluated_args[i] = coerce_value(e, evaluated_args[i], pty);
				}
			}

			const char* emit_callee = callee;
			if (strcmp(callee, "main") == 0 && fn_decl != NULL)
			{
				emit_callee = "_user_main";
			}

			if (strcmp(ret_llvm, "void") == 0)
			{
				buf_emit(e, "  call void @%s(", emit_callee);
				for (int i = 0; i < argc && i < 32; i++)
				{
					if (i > 0) buf_emit(e, ", ");
					buf_emit(e, "%s %s", evaluated_args[i].type, evaluated_args[i].repr);
				}
				buf_emit(e, ")\n");
				strcpy(val.repr, "");
				strcpy(val.type, "void");
				return val;
			}
			else
			{
				int t = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
				strcpy(val.type, ret_llvm);

				buf_emit(e, "  %s = call %s @%s(", val.repr, ret_llvm, emit_callee);
				for (int i = 0; i < argc && i < 32; i++)
				{
					if (i > 0) buf_emit(e, ", ");
					buf_emit(e, "%s %s", evaluated_args[i].type, evaluated_args[i].repr);
				}
				buf_emit(e, ")\n");
				return val;
			}
		}

	case AST_EXPR_NEW:
		{
			const char* cname = expr->as.new_expr.class_name;
			const LLVMClassDesc* cd = NULL;
			for (int i = 0; i < e->class_count; i++)
			{
				if (strcmp(e->classes[i].name, cname) == 0)
				{
					cd = &e->classes[i];
					break;
				}
			}
			size_t alloc_bytes = cd ? (cd->field_count > 0 ? cd->field_count * 8 : 8) : 64;

			int t_raw = new_temp_id(e);
			int t_obj = new_temp_id(e);
			buf_emit(e, "  %%t%d = call i8* @malloc(i64 %zu)\n", t_raw, alloc_bytes);
			buf_emit(e, "  %%t%d = bitcast i8* %%t%d to %%struct.%s*\n", t_obj, t_raw, cname);

			/* Find AST class declaration for default field inits & nested class inits */
			const AstStmt* cls_decl = NULL;
			for (int i = 0; i < e->prog->statement_count; i++)
			{
				const AstStmt* s = e->prog->statements[i];
				if (s->type == AST_STMT_CLASS_DECL && strcmp(s->as.class_decl.name, cname) == 0)
				{
					cls_decl = s;
					break;
				}
			}

			if (cls_decl != NULL && cd != NULL)
			{
				for (int m = 0; m < cls_decl->as.class_decl.member_count; m++)
				{
					const AstStmt* mem = cls_decl->as.class_decl.members[m];
					if (mem->type == AST_STMT_VAR_DECL)
					{
						const char* fname = mem->as.var_decl.var_name;
						int slot = -1;
						const char* fty = "i32";
						for (int f = 0; f < cd->field_count; f++)
						{
							if (strcmp(cd->fields[f].name, fname) == 0)
							{
								slot = cd->fields[f].slot;
								fty = cd->fields[f].llvm_type;
								break;
							}
						}
						if (slot >= 0)
						{
							if (mem->as.var_decl.init_expr != NULL)
							{
								LLVMValue fval = emit_expr(e, mem->as.var_decl.init_expr);
								fval = coerce_value(e, fval, fty);
								int t_slot = new_temp_id(e);
								buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %%t%d, i32 0, i32 %d\n",
								         t_slot, cname, cname, t_obj, slot);
								buf_emit(e, "  store %s %s, %s* %%t%d, align 8\n", fty, fval.repr, fty, t_slot);
							}
							else if (mem->as.var_decl.type_name && strncmp(fty, "%struct.", 8) == 0)
							{
								/* Nested class instance initialization */
								AstExpr dummy_new;
								memset(&dummy_new, 0, sizeof(dummy_new));
								dummy_new.type = AST_EXPR_NEW;
								dummy_new.as.new_expr.class_name = (char*)mem->as.var_decl.type_name;
								LLVMValue nested_obj = emit_expr(e, &dummy_new);
								int t_slot = new_temp_id(e);
								buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %%t%d, i32 0, i32 %d\n",
								         t_slot, cname, cname, t_obj, slot);
								buf_emit(e, "  store %s %s, %s* %%t%d, align 8\n", fty, nested_obj.repr, fty, t_slot);
							}
						}
					}
				}
			}

			/* Invoke constructor if present */
			const AstStmt* ctor = find_class_method(e, cname, cname, expr->as.new_expr.arg_count);
			if (ctor != NULL)
			{
				char ctor_sym[128];
				get_method_symbol_name(e, cname, cname, expr->as.new_expr.arg_count, ctor_sym, sizeof(ctor_sym));
				buf_emit(e, "  call void @%s(%%struct.%s* %%t%d", ctor_sym, cname, t_obj);
				for (int a = 0; a < expr->as.new_expr.arg_count; a++)
				{
					LLVMValue arg_val = emit_expr(e, expr->as.new_expr.args[a]);
					if (a < ctor->as.func_decl.param_count)
					{
						const char* pty = xlang_type_to_llvm(e, ctor->as.func_decl.params[a].type_name);
						arg_val = coerce_value(e, arg_val, pty);
					}
					buf_emit(e, ", %s %s", arg_val.type, arg_val.repr);
				}
				buf_emit(e, ")\n");
			}

			snprintf(val.repr, sizeof(val.repr), "%%t%d", t_obj);
			snprintf(val.type, sizeof(val.type), "%%struct.%s*", cname);
			return val;
		}

	case AST_EXPR_METHOD_CALL:
		{
			LLVMValue obj = emit_expr(e, expr->as.method_call.object);
			const char* mname = expr->as.method_call.method_name;
			int argc = expr->as.method_call.arg_count;

			/* String method dispatch */
			if (strcmp(obj.type, "i8*") == 0)
			{
				if (strcmp(mname, "len") == 0 || strcmp(mname, "length") == 0)
				{
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i32");
					buf_emit(e, "  %s = call i32 @_len(i8* %s)\n", val.repr, obj.repr);
					return val;
				}
				if (strcmp(mname, "trim") == 0)
				{
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i8*");
					buf_emit(e, "  %s = call i8* @_trim(i8* %s)\n", val.repr, obj.repr);
					return val;
				}
				if (strcmp(mname, "lower") == 0)
				{
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i8*");
					buf_emit(e, "  %s = call i8* @_lower(i8* %s)\n", val.repr, obj.repr);
					return val;
				}
				if (strcmp(mname, "upper") == 0)
				{
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i8*");
					buf_emit(e, "  %s = call i8* @_upper(i8* %s)\n", val.repr, obj.repr);
					return val;
				}
				if (strcmp(mname, "substr") == 0 && argc >= 2)
				{
					LLVMValue a0 = emit_expr(e, expr->as.method_call.args[0]);
					LLVMValue a1 = emit_expr(e, expr->as.method_call.args[1]);
					a0 = coerce_value(e, a0, "i32");
					a1 = coerce_value(e, a1, "i32");
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i8*");
					buf_emit(e, "  %s = call i8* @substr(i8* %s, i32 %s, i32 %s)\n", val.repr, obj.repr, a0.repr, a1.repr);
					return val;
				}
				if ((strcmp(mname, "index_of") == 0 || strcmp(mname, "indexOf") == 0) && argc >= 1)
				{
					LLVMValue a0 = emit_expr(e, expr->as.method_call.args[0]);
					a0 = coerce_value(e, a0, "i8*");
					int t = new_temp_id(e);
					snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
					strcpy(val.type, "i32");
					buf_emit(e, "  %s = call i32 @index_of(i8* %s, i8* %s)\n", val.repr, obj.repr, a0.repr);
					return val;
				}
			}

			char cname[64] = {0};
			if (strncmp(obj.type, "%struct.", 8) == 0)
			{
				const char* start = obj.type + 8;
				const char* end = strchr(start, '*');
				if (end) strncpy(cname, start, end - start);
				else strcpy(cname, start);
			}

			char def_class[64] = {0};
			const AstStmt* m_decl = find_class_method_defining_class(e, cname, mname, argc, def_class, sizeof(def_class));
			const char* ret_llvm = "i32";
			if (m_decl)
			{
				const char* ret_name = m_decl->as.func_decl.return_type ? m_decl->as.func_decl.return_type : "void";
				ret_llvm = xlang_type_to_llvm(e, ret_name);
			}

			const char* actual_class = def_class[0] ? def_class : cname;
			char sym_name[128];
			get_method_symbol_name(e, actual_class, mname, argc, sym_name, sizeof(sym_name));

			char target_this_type[128];
			snprintf(target_this_type, sizeof(target_this_type), "%%struct.%s*", actual_class);
			obj = coerce_value(e, obj, target_this_type);

			LLVMValue evaluated_args[32];
			for (int i = 0; i < argc && i < 32; i++)
			{
				evaluated_args[i] = emit_expr(e, expr->as.method_call.args[i]);
				if (m_decl && i < m_decl->as.func_decl.param_count)
				{
					const char* pty = xlang_type_to_llvm(e, m_decl->as.func_decl.params[i].type_name);
					evaluated_args[i] = coerce_value(e, evaluated_args[i], pty);
				}
			}

			if (strcmp(ret_llvm, "void") == 0)
			{
				buf_emit(e, "  call void @%s(%s %s", sym_name, obj.type, obj.repr);
				for (int i = 0; i < argc && i < 32; i++)
				{
					buf_emit(e, ", %s %s", evaluated_args[i].type, evaluated_args[i].repr);
				}
				buf_emit(e, ")\n");
				strcpy(val.repr, "");
				strcpy(val.type, "void");
				return val;
			}
			else
			{
				int t = new_temp_id(e);
				snprintf(val.repr, sizeof(val.repr), "%%t%d", t);
				strcpy(val.type, ret_llvm);

				buf_emit(e, "  %s = call %s @%s(%s %s", val.repr, ret_llvm, sym_name, obj.type, obj.repr);
				for (int i = 0; i < argc && i < 32; i++)
				{
					buf_emit(e, ", %s %s", evaluated_args[i].type, evaluated_args[i].repr);
				}
				buf_emit(e, ")\n");
				return val;
			}
		}

	case AST_EXPR_MEMBER:
		{
			LLVMValue obj = emit_expr(e, expr->as.member.object);
			const char* member_name = expr->as.member.member_name;
			char cname[64] = {0};
			if (strncmp(obj.type, "%struct.", 8) == 0)
			{
				const char* start = obj.type + 8;
				const char* end = strchr(start, '*');
				if (end) strncpy(cname, start, end - start);
				else strcpy(cname, start);
			}
			for (int c = 0; c < e->class_count; c++)
			{
				if (strcmp(e->classes[c].name, cname) == 0)
				{
					for (int f = 0; f < e->classes[c].field_count; f++)
					{
						if (strcmp(e->classes[c].fields[f].name, member_name) == 0)
						{
							int t_ptr = new_temp_id(e);
							int t_val = new_temp_id(e);
							const char* fty = e->classes[c].fields[f].llvm_type;
							buf_emit(e, "  %%t%d = getelementptr inbounds %%struct.%s, %%struct.%s* %s, i32 0, i32 %d\n",
							         t_ptr, cname, cname, obj.repr, e->classes[c].fields[f].slot);
							buf_emit(e, "  %%t%d = load %s, %s* %%t%d, align %d\n",
							         t_val, fty, fty, t_ptr, llvm_type_align(fty));
							snprintf(val.repr, sizeof(val.repr), "%%t%d", t_val);
							snprintf(val.type, sizeof(val.type), "%s", fty);
							return val;
						}
					}
				}
			}
			return val;
		}

	default:
		break;
	}

	return val;
}

/* -------------------------------------------------------------------------
 * Statement Code Generation
 * ------------------------------------------------------------------------- */
static void emit_stmt(XLLVMEmitter* e, const AstStmt* stmt)
{
	if (!stmt || e->current_block_terminated) return;

	switch (stmt->type)
	{
	case AST_STMT_VAR_DECL:
		{
			const char* var_name = stmt->as.var_decl.var_name;
			const char* ty_name = stmt->as.var_decl.type_name;
			const char* lty = xlang_type_to_llvm(e, ty_name);

			/* Unique alloca register */
			char alloca_name[64];
			snprintf(alloca_name, sizeof(alloca_name), "%%%s.%d", var_name, new_temp_id(e));
			buf_emit(e, "  %s = alloca %s, align 8\n", alloca_name, lty);
			add_local(e, var_name, lty, alloca_name);

			if (stmt->as.var_decl.init_expr != NULL)
			{
				LLVMValue init_val = emit_expr(e, stmt->as.var_decl.init_expr);
				init_val = coerce_value(e, init_val, lty);
				buf_emit(e, "  store %s %s, %s* %s, align 8\n",
				         lty, init_val.repr, lty, alloca_name);
			}
			else
			{
				/* Zero initialize default */
				if (strcmp(lty, "i32") == 0)
					buf_emit(e, "  store i32 0, i32* %s, align 4\n", alloca_name);
				else if (strcmp(lty, "double") == 0)
					buf_emit(e, "  store double 0.0, double* %s, align 8\n", alloca_name);
				else if (strcmp(lty, "i1") == 0)
					buf_emit(e, "  store i1 false, i1* %s, align 1\n", alloca_name);
				else
					buf_emit(e, "  store %s null, %s* %s, align 8\n", lty, lty, alloca_name);
			}
			break;
		}

	case AST_STMT_EXPR:
		emit_expr(e, stmt->as.expr);
		break;

	case AST_STMT_BLOCK:
		enter_scope(e);
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
		{
			emit_stmt(e, stmt->as.block.stmts[i]);
			if (e->current_block_terminated) break;
		}
		exit_scope(e);
		break;

	case AST_STMT_IF:
		{
			int lbl_id = new_label_id(e);
			char then_lbl[32], else_lbl[32], merge_lbl[32];
			snprintf(then_lbl, sizeof(then_lbl), "then.%d", lbl_id);
			snprintf(else_lbl, sizeof(else_lbl), "else.%d", lbl_id);
			snprintf(merge_lbl, sizeof(merge_lbl), "merge.%d", lbl_id);

			LLVMValue cond = emit_expr(e, stmt->as.if_stmt.condition);
			cond = coerce_value(e, cond, "i1");

			buf_emit(e, "  br i1 %s, label %%%s, label %%%s\n\n",
			         cond.repr, then_lbl, stmt->as.if_stmt.else_branch ? else_lbl : merge_lbl);

			/* Then Branch */
			buf_emit(e, "%s:\n", then_lbl);
			e->current_block_terminated = false;
			emit_stmt(e, stmt->as.if_stmt.then_branch);
			bool then_term = e->current_block_terminated;
			if (!then_term)
			{
				buf_emit(e, "  br label %%%s\n\n", merge_lbl);
			}

			/* Else Branch */
			bool else_term = false;
			if (stmt->as.if_stmt.else_branch != NULL)
			{
				buf_emit(e, "%s:\n", else_lbl);
				e->current_block_terminated = false;
				emit_stmt(e, stmt->as.if_stmt.else_branch);
				else_term = e->current_block_terminated;
				if (!else_term)
				{
					buf_emit(e, "  br label %%%s\n\n", merge_lbl);
				}
			}

			/* Merge Label */
			if (!then_term || !else_term)
			{
				buf_emit(e, "%s:\n", merge_lbl);
				e->current_block_terminated = false;
			}
			else
			{
				e->current_block_terminated = true;
			}
			break;
		}

	case AST_STMT_WHILE:
		{
			int lbl_id = new_label_id(e);
			char cond_lbl[32], body_lbl[32], end_lbl[32];
			snprintf(cond_lbl, sizeof(cond_lbl), "while.cond.%d", lbl_id);
			snprintf(body_lbl, sizeof(body_lbl), "while.body.%d", lbl_id);
			snprintf(end_lbl, sizeof(end_lbl), "while.end.%d", lbl_id);

			buf_emit(e, "  br label %%%s\n\n", cond_lbl);
			buf_emit(e, "%s:\n", cond_lbl);

			LLVMValue cond = emit_expr(e, stmt->as.while_stmt.condition);
			cond = coerce_value(e, cond, "i1");
			buf_emit(e, "  br i1 %s, label %%%s, label %%%s\n\n", cond.repr, body_lbl, end_lbl);

			/* Loop body */
			buf_emit(e, "%s:\n", body_lbl);
			LLVMLoop loop;
			snprintf(loop.cond_label, sizeof(loop.cond_label), "%s", cond_lbl);
			snprintf(loop.step_label, sizeof(loop.step_label), "%s", cond_lbl);
			snprintf(loop.end_label, sizeof(loop.end_label), "%s", end_lbl);
			loop.enclosing = e->current_loop;
			e->current_loop = &loop;

			e->current_block_terminated = false;
			emit_stmt(e, stmt->as.while_stmt.body);
			if (!e->current_block_terminated)
			{
				buf_emit(e, "  br label %%%s\n\n", cond_lbl);
			}
			e->current_loop = loop.enclosing;

			buf_emit(e, "%s:\n", end_lbl);
			e->current_block_terminated = false;
			break;
		}

	case AST_STMT_FOR_C:
		{
			int lbl_id = new_label_id(e);
			char cond_lbl[32], body_lbl[32], step_lbl[32], end_lbl[32];
			snprintf(cond_lbl, sizeof(cond_lbl), "for.cond.%d", lbl_id);
			snprintf(body_lbl, sizeof(body_lbl), "for.body.%d", lbl_id);
			snprintf(step_lbl, sizeof(step_lbl), "for.step.%d", lbl_id);
			snprintf(end_lbl, sizeof(end_lbl), "for.end.%d", lbl_id);

			enter_scope(e);
			if (stmt->as.for_c.init != NULL)
			{
				emit_stmt(e, stmt->as.for_c.init);
			}
			buf_emit(e, "  br label %%%s\n\n", cond_lbl);
			buf_emit(e, "%s:\n", cond_lbl);

			if (stmt->as.for_c.condition != NULL)
			{
				LLVMValue cond = emit_expr(e, stmt->as.for_c.condition);
				cond = coerce_value(e, cond, "i1");
				buf_emit(e, "  br i1 %s, label %%%s, label %%%s\n\n", cond.repr, body_lbl, end_lbl);
			}
			else
			{
				buf_emit(e, "  br label %%%s\n\n", body_lbl);
			}

			/* Body */
			buf_emit(e, "%s:\n", body_lbl);
			LLVMLoop loop;
			snprintf(loop.cond_label, sizeof(loop.cond_label), "%s", cond_lbl);
			snprintf(loop.step_label, sizeof(loop.step_label), "%s", step_lbl);
			snprintf(loop.end_label, sizeof(loop.end_label), "%s", end_lbl);
			loop.enclosing = e->current_loop;
			e->current_loop = &loop;

			e->current_block_terminated = false;
			emit_stmt(e, stmt->as.for_c.body);
			if (!e->current_block_terminated)
			{
				buf_emit(e, "  br label %%%s\n\n", step_lbl);
			}

			/* Step */
			buf_emit(e, "%s:\n", step_lbl);
			if (stmt->as.for_c.step != NULL)
			{
				emit_expr(e, stmt->as.for_c.step);
			}
			buf_emit(e, "  br label %%%s\n\n", cond_lbl);

			e->current_loop = loop.enclosing;
			exit_scope(e);

			buf_emit(e, "%s:\n", end_lbl);
			e->current_block_terminated = false;
			break;
		}

	case AST_STMT_RETURN:
		{
			if (stmt->as.return_expr != NULL)
			{
				LLVMValue ret_val = emit_expr(e, stmt->as.return_expr);
				ret_val = coerce_value(e, ret_val, e->current_func_ret_type);
				buf_emit(e, "  ret %s %s\n", ret_val.type, ret_val.repr);
			}
			else
			{
				if (strcmp(e->current_func_ret_type, "void") == 0)
					buf_emit(e, "  ret void\n");
				else
					buf_emit(e, "  ret %s 0\n", e->current_func_ret_type);
			}
			e->current_block_terminated = true;
			break;
		}

	case AST_STMT_BREAK:
		if (e->current_loop != NULL)
		{
			buf_emit(e, "  br label %%%s\n", e->current_loop->end_label);
			e->current_block_terminated = true;
		}
		break;

	case AST_STMT_CONTINUE:
		if (e->current_loop != NULL)
		{
			buf_emit(e, "  br label %%%s\n", e->current_loop->step_label);
			e->current_block_terminated = true;
		}
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------
 * Top-Level Discovery & Class Struct Collection
 * ------------------------------------------------------------------------- */
static void resolve_class_inheritance(XLLVMEmitter* e, LLVMClassDesc* cd)
{
	if (cd->base_name[0] == '\0') return;
	LLVMClassDesc* base = NULL;
	for (int i = 0; i < e->class_count; i++)
	{
		if (strcmp(e->classes[i].name, cd->base_name) == 0)
		{
			base = &e->classes[i];
			break;
		}
	}
	if (!base) return;
	resolve_class_inheritance(e, base);

	/* Check if base fields already copied */
	if (base->field_count > 0 && cd->field_count + base->field_count <= MAX_FIELDS_PER_CLASS)
	{
		bool already_has = false;
		for (int f = 0; f < cd->field_count; f++)
		{
			if (strcmp(cd->fields[f].name, base->fields[0].name) == 0)
			{
				already_has = true;
				break;
			}
		}
		if (!already_has)
		{
			int base_count = base->field_count;
			memmove(&cd->fields[base_count], &cd->fields[0], cd->field_count * sizeof(LLVMFieldDesc));
			memcpy(&cd->fields[0], &base->fields[0], base_count * sizeof(LLVMFieldDesc));
			cd->field_count += base_count;
			for (int f = 0; f < cd->field_count; f++)
			{
				cd->fields[f].slot = f;
			}
		}
	}
}

static void collect_classes(XLLVMEmitter* e)
{
	e->class_count = 0;
	for (int i = 0; i < e->prog->statement_count; i++)
	{
		const AstStmt* stmt = e->prog->statements[i];
		if (stmt->type == AST_STMT_CLASS_DECL)
		{
			if (e->class_count < MAX_CLASSES)
			{
				LLVMClassDesc* cd = &e->classes[e->class_count++];
				snprintf(cd->name, sizeof(cd->name), "%s", stmt->as.class_decl.name);
				if (stmt->as.class_decl.base_name)
					snprintf(cd->base_name, sizeof(cd->base_name), "%s", stmt->as.class_decl.base_name);
				else
					cd->base_name[0] = '\0';

				cd->field_count = 0;
				for (int m = 0; m < stmt->as.class_decl.member_count; m++)
				{
					const AstStmt* mem = stmt->as.class_decl.members[m];
					if (mem->type == AST_STMT_VAR_DECL)
					{
						if (cd->field_count < MAX_FIELDS_PER_CLASS)
						{
							LLVMFieldDesc* fd = &cd->fields[cd->field_count];
							snprintf(fd->name, sizeof(fd->name), "%s", mem->as.var_decl.var_name);
							snprintf(fd->type_name, sizeof(fd->type_name), "%s", mem->as.var_decl.type_name ? mem->as.var_decl.type_name : "int");
							snprintf(fd->llvm_type, sizeof(fd->llvm_type), "%s", xlang_type_to_llvm(e, fd->type_name));
							fd->slot = cd->field_count;
							cd->field_count++;
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < e->class_count; i++)
	{
		resolve_class_inheritance(e, &e->classes[i]);
	}
}

/* -------------------------------------------------------------------------
 * Function Definition CodeGen
 * ------------------------------------------------------------------------- */
static void emit_function(XLLVMEmitter* e, const AstStmt* fn_stmt, const char* class_prefix)
{
	const char* fn_name = fn_stmt->as.func_decl.name;
	const char* ret_type_name = fn_stmt->as.func_decl.return_type ? fn_stmt->as.func_decl.return_type : "void";
	const char* lret = xlang_type_to_llvm(e, ret_type_name);
	snprintf(e->current_func_ret_type, sizeof(e->current_func_ret_type), "%s", lret);

	char full_name[128];
	if (class_prefix != NULL)
		get_method_symbol_name(e, class_prefix, fn_name, fn_stmt->as.func_decl.param_count, full_name, sizeof(full_name));
	else if (strcmp(fn_name, "main") == 0)
		snprintf(full_name, sizeof(full_name), "_user_main");
	else
		snprintf(full_name, sizeof(full_name), "%s", fn_name);

	/* Reset locals for function body */
	e->local_count = 0;
	e->scope_depth = 0;
	e->current_loop = NULL;
	e->current_block_terminated = false;

	buf_emit(e, "define %s @%s(", lret, full_name);

	/* If method, prepend %this */
	int p_offset = 0;
	if (class_prefix != NULL)
	{
		buf_emit(e, "%%struct.%s* %%this", class_prefix);
		p_offset = 1;
	}

	for (int p = 0; p < fn_stmt->as.func_decl.param_count; p++)
	{
		if (p > 0 || p_offset > 0) buf_emit(e, ", ");
		const char* pty = xlang_type_to_llvm(e, fn_stmt->as.func_decl.params[p].type_name);
		buf_emit(e, "%s %%%s", pty, fn_stmt->as.func_decl.params[p].name);
	}
	buf_emit(e, ") {\nentry:\n");

	/* Store parameters in allocas */
	for (int p = 0; p < fn_stmt->as.func_decl.param_count; p++)
	{
		const char* pname = fn_stmt->as.func_decl.params[p].name;
		const char* pty = xlang_type_to_llvm(e, fn_stmt->as.func_decl.params[p].type_name);
		char addr_name[64];
		snprintf(addr_name, sizeof(addr_name), "%%%s.addr", pname);
		buf_emit(e, "  %s = alloca %s, align 8\n", addr_name, pty);
		buf_emit(e, "  store %s %%%s, %s* %s, align 8\n", pty, pname, pty, addr_name);
		add_local(e, pname, pty, addr_name);
	}

	/* Emit body */
	if (fn_stmt->as.func_decl.body != NULL)
	{
		emit_stmt(e, fn_stmt->as.func_decl.body);
	}

	/* Implicit return if missing */
	if (!e->current_block_terminated)
	{
		if (strcmp(lret, "void") == 0)
			buf_emit(e, "  ret void\n");
		else if (strcmp(lret, "double") == 0)
			buf_emit(e, "  ret double 0.0\n");
		else
			buf_emit(e, "  ret %s 0\n", lret);
	}

	buf_emit(e, "}\n\n");
}

/* -------------------------------------------------------------------------
 * Main Program Emission Entrypoint
 * ------------------------------------------------------------------------- */
bool xllvm_emit_program(const AstProgram* prog, const char* source_file, FILE* out, const XLLVMConfig* config)
{
	if (!prog || !out) return false;

	XLLVMEmitter emitter;
	memset(&emitter, 0, sizeof(XLLVMEmitter));
	emitter.out = out;
	emitter.prog = prog;
	emitter.source_file = source_file ? source_file : "input.xb";
	emitter.config = config ? *config : xllvm_default_config();

	collect_classes(&emitter);

	/* 1. Header Comments & Target Config */
	fprintf(out, "; ModuleID = '%s'\n", emitter.source_file);
	fprintf(out, "source_filename = \"%s\"\n", emitter.source_file);
	if (emitter.config.data_layout && emitter.config.data_layout[0])
		fprintf(out, "target datalayout = \"%s\"\n", emitter.config.data_layout);
	if (emitter.config.target_triple && emitter.config.target_triple[0])
		fprintf(out, "target triple = \"%s\"\n\n", emitter.config.target_triple);

	/* 2. Class Struct Declarations */
	for (int i = 0; i < emitter.class_count; i++)
	{
		LLVMClassDesc* cd = &emitter.classes[i];
		fprintf(out, "%%struct.%s = type { ", cd->name);
		for (int f = 0; f < cd->field_count; f++)
		{
			if (f > 0) fprintf(out, ", ");
			fprintf(out, "%s", cd->fields[f].llvm_type);
		}
		if (cd->field_count == 0)
			fprintf(out, "i8"); /* Empty struct gets 1-byte filler */
		fprintf(out, " }\n");
	}
	if (emitter.class_count > 0)
		fprintf(out, "\n");

	/* 3. Emit functions and collect top-level statements into code_buf */
	bool has_user_main = false;

	/* Buffer for top-level code inside @main */
	emitter.code_buf_cap = 32768;
	emitter.code_buf = (char*)malloc(emitter.code_buf_cap);
	emitter.code_buf_len = 0;
	emitter.code_buf[0] = '\0';

	/* Class methods first */
	for (int i = 0; i < prog->statement_count; i++)
	{
		const AstStmt* stmt = prog->statements[i];
		if (stmt->type == AST_STMT_CLASS_DECL)
		{
			emitter.current_class = NULL;
			for (int c = 0; c < emitter.class_count; c++)
			{
				if (strcmp(emitter.classes[c].name, stmt->as.class_decl.name) == 0)
				{
					emitter.current_class = &emitter.classes[c];
					break;
				}
			}
			for (int m = 0; m < stmt->as.class_decl.member_count; m++)
			{
				const AstStmt* mem = stmt->as.class_decl.members[m];
				if (mem->type == AST_STMT_FUNC_DECL)
				{
					emit_function(&emitter, mem, stmt->as.class_decl.name);
				}
			}
			emitter.current_class = NULL;
		}
		else if (stmt->type == AST_STMT_FUNC_DECL)
		{
			if (strcmp(stmt->as.func_decl.name, "main") == 0)
				has_user_main = true;
			emit_function(&emitter, stmt, NULL);
		}
	}

	char* functions_ir = strdup(emitter.code_buf ? emitter.code_buf : "");
	buf_clear(&emitter);

	/* Top level statements into @main */
	emitter.local_count = 0;
	emitter.scope_depth = 0;
	emitter.current_loop = NULL;
	emitter.current_block_terminated = false;
	strcpy(emitter.current_func_ret_type, "i32");

	for (int i = 0; i < prog->statement_count; i++)
	{
		const AstStmt* stmt = prog->statements[i];
		if (stmt->type != AST_STMT_FUNC_DECL && stmt->type != AST_STMT_CLASS_DECL)
		{
			emit_stmt(&emitter, stmt);
		}
	}

	if (has_user_main && !emitter.current_block_terminated)
	{
		int t_res = new_temp_id(&emitter);
		buf_emit(&emitter, "  %%t%d = call i32 @_user_main()\n", t_res);
		buf_emit(&emitter, "  ret i32 %%t%d\n", t_res);
		emitter.current_block_terminated = true;
	}
	else if (!emitter.current_block_terminated)
	{
		buf_emit(&emitter, "  ret i32 0\n");
	}

	char* main_ir = strdup(emitter.code_buf ? emitter.code_buf : "");

	/* 4. Global String Constant Declarations */
	for (int i = 0; i < emitter.string_constant_count; i++)
	{
		char escaped[2048] = {0};
		escape_llvm_string(emitter.string_constants[i].text, escaped, sizeof(escaped));
		fprintf(out, "@.str.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\", align 1\n",
		        emitter.string_constants[i].id,
		        emitter.string_constants[i].len,
		        escaped);
	}
	if (emitter.string_constant_count > 0)
		fprintf(out, "\n");

	/* 5. Runtime Intrinsics & C Library Declarations */
	fprintf(out, "declare i32 @printf(i8*, ...)\n");
	fprintf(out, "declare i32 @puts(i8*)\n");
	fprintf(out, "declare void @exit(i32)\n");
	fprintf(out, "declare i8* @malloc(i64)\n");
	fprintf(out, "declare void @free(i8*)\n");
	fprintf(out, "declare i8* @gc_malloc(i64, i32)\n");
	fprintf(out, "declare i8* @gc_calloc(i64, i64, i32)\n");
	fprintf(out, "declare i32 @strcmp(i8*, i8*)\n");
	fprintf(out, "declare void @xllvm_rt_init(i32, i8**)\n\n");

	fprintf(out, "; String Helpers\n");
	fprintf(out, "declare i8* @_str_concat(i8*, i8*)\n");
	fprintf(out, "declare i8* @_str_from_int(i32)\n");
	fprintf(out, "declare i8* @_str_from_float(double)\n");
	fprintf(out, "declare i32 @_len(i8*)\n");
	fprintf(out, "declare i8* @_trim(i8*)\n");
	fprintf(out, "declare i8* @_lower(i8*)\n");
	fprintf(out, "declare i8* @_upper(i8*)\n");
	fprintf(out, "declare i8* @substr(i8*, i32, i32)\n");
	fprintf(out, "declare i8* @chr(i32)\n");
	fprintf(out, "declare i32 @index_of(i8*, i8*)\n");
	fprintf(out, "declare i32 @str_eq(i8*, i8*)\n\n");

	fprintf(out, "; Map Operations\n");
	fprintf(out, "declare i32 @map_new()\n");
	fprintf(out, "declare i32 @map_put(i32, i8*, i8*)\n");
	fprintf(out, "declare i32 @map_put_int(i32, i8*, i32)\n");
	fprintf(out, "declare i32 @map_put_float(i32, i8*, double)\n");
	fprintf(out, "declare i8* @map_get(i32, i8*)\n");
	fprintf(out, "declare i32 @map_get_int(i32, i8*)\n");
	fprintf(out, "declare double @map_get_float(i32, i8*)\n");
	fprintf(out, "declare i32 @map_has(i32, i8*)\n");
	fprintf(out, "declare i32 @map_remove(i32, i8*)\n");
	fprintf(out, "declare i32 @map_size(i32)\n");
	fprintf(out, "declare i32 @map_clear(i32)\n");
	fprintf(out, "declare i8* @map_keys(i32)\n");
	fprintf(out, "declare i8* @map_values(i32)\n");
	fprintf(out, "declare i8* @map_to_string(i32)\n");
	fprintf(out, "declare i32 @map_free(i32)\n");
	fprintf(out, "declare i32 @map_keys_list(i32)\n\n");

	fprintf(out, "; List Operations\n");
	fprintf(out, "declare i32 @list_new()\n");
	fprintf(out, "declare i32 @list_add(i32, i8*)\n");
	fprintf(out, "declare i32 @list_add_int(i32, i32)\n");
	fprintf(out, "declare i32 @list_add_float(i32, double)\n");
	fprintf(out, "declare i8* @list_get(i32, i32)\n");
	fprintf(out, "declare i32 @list_get_int(i32, i32)\n");
	fprintf(out, "declare double @list_get_float(i32, i32)\n");
	fprintf(out, "declare i32 @list_set(i32, i32, i8*)\n");
	fprintf(out, "declare i32 @list_set_int(i32, i32, i32)\n");
	fprintf(out, "declare i32 @list_remove_at(i32, i32)\n");
	fprintf(out, "declare i32 @list_size(i32)\n");
	fprintf(out, "declare i32 @list_clear(i32)\n");
	fprintf(out, "declare i32 @list_contains(i32, i8*)\n");
	fprintf(out, "declare i32 @list_contains_int(i32, i32)\n");
	fprintf(out, "declare i32 @list_index_of(i32, i8*)\n");
	fprintf(out, "declare i32 @list_index_of_int(i32, i32)\n");
	fprintf(out, "declare i8* @list_pop(i32)\n");
	fprintf(out, "declare i32 @list_pop_int(i32)\n");
	fprintf(out, "declare i8* @list_join(i32, i8*)\n");
	fprintf(out, "declare i8* @list_to_string(i32)\n");
	fprintf(out, "declare i32 @list_free(i32)\n\n");

	fprintf(out, "; Socket Operations\n");
	fprintf(out, "declare i32 @socket_create(i8*)\n");
	fprintf(out, "declare i32 @socket_connect(i32, i8*, i32)\n");
	fprintf(out, "declare i32 @socket_bind(i32, i8*, i32)\n");
	fprintf(out, "declare i32 @socket_listen(i32, i32)\n");
	fprintf(out, "declare i32 @socket_accept(i32)\n");
	fprintf(out, "declare i32 @socket_send(i32, i8*)\n");
	fprintf(out, "declare i8* @socket_recv(i32, i32)\n");
	fprintf(out, "declare i32 @socket_close(i32)\n");
	fprintf(out, "declare i32 @socket_set_timeout(i32, i32)\n");
	fprintf(out, "declare i32 @socket_set_reuseaddr(i32, i32)\n");
	fprintf(out, "declare i32 @socket_sendto(i32, i8*, i8*, i32)\n");
	fprintf(out, "declare i8* @socket_recvfrom(i32, i32)\n\n");

	fprintf(out, "; System Operations\n");
	fprintf(out, "declare i32 @clock_ms()\n");
	fprintf(out, "declare i32 @get_argc()\n");
	fprintf(out, "declare i8* @get_arg(i32)\n");
	fprintf(out, "declare i32 @system_exec(i8*)\n");
	fprintf(out, "declare i8* @system_getenv(i8*)\n");
	fprintf(out, "declare i32 @system_setenv(i8*, i8*)\n\n");

	fprintf(out, "; GC Operations\n");
	fprintf(out, "declare i32 @gc_collect()\n");
	fprintf(out, "declare i32 @gc_allocated_bytes()\n");
	fprintf(out, "declare i32 @gc_total_objects()\n");
	fprintf(out, "declare i32 @gc_enable()\n");
	fprintf(out, "declare i32 @gc_disable()\n");
	fprintf(out, "declare i32 @gc_set_threshold(i32)\n");
	fprintf(out, "declare i32 @gc_dump()\n\n");

	/* 6. Write Functions IR */
	fprintf(out, "%s", functions_ir);

	/* 7. Write Main Entrypoint */
	fprintf(out, "define i32 @main(i32 %%argc, i8** %%argv) {\nentry:\n");
	fprintf(out, "  call void @xllvm_rt_init(i32 %%argc, i8** %%argv)\n");
	fprintf(out, "%s", main_ir);
	fprintf(out, "}\n");

	free(functions_ir);
	free(main_ir);
	if (emitter.code_buf) free(emitter.code_buf);
	for (int i = 0; i < emitter.string_constant_count; i++)
	{
		if (emitter.string_constants[i].text)
			free(emitter.string_constants[i].text);
	}

	return true;
}

bool xllvm_emit_file(const AstProgram* prog, const char* source_file, const char* out_path, const XLLVMConfig* config)
{
	FILE* f = fopen(out_path, "w");
	if (!f) return false;
	bool ok = xllvm_emit_program(prog, source_file, f, config);
	fclose(f);
	return ok;
}
