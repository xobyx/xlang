#ifndef XAST_H
#define XAST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * AST Memory Arena
 * Provides high-speed bump allocation and single-pass teardown.
 * ------------------------------------------------------------------------- */
typedef struct AstArenaBlock {
	struct AstArenaBlock* next;
	size_t capacity;
	size_t used;
	char* data;
} AstArenaBlock;

typedef struct AstArena {
	AstArenaBlock* first;
	AstArenaBlock* current;
	size_t total_allocated;
} AstArena;

AstArena* ast_arena_create(size_t initial_capacity);
void* ast_arena_alloc(AstArena* arena, size_t size);
char* ast_arena_strdup(AstArena* arena, const char* str);
void ast_arena_destroy(AstArena* arena);

/* -------------------------------------------------------------------------
 * AST Node Types & Operator Enums
 * ------------------------------------------------------------------------- */
typedef enum AstNodeType {
	/* Expressions */
	AST_EXPR_LITERAL_INT = 1,
	AST_EXPR_LITERAL_FLOAT,
	AST_EXPR_LITERAL_STRING,
	AST_EXPR_LITERAL_BOOL,
	AST_EXPR_LITERAL_NULL,
	AST_EXPR_IDENTIFIER,
	AST_EXPR_BINARY,
	AST_EXPR_UNARY,
	AST_EXPR_CALL,
	AST_EXPR_METHOD_CALL,
	AST_EXPR_MEMBER,
	AST_EXPR_INDEX,
	AST_EXPR_LIST,
	AST_EXPR_ASSIGN,

	/* Statements */
	AST_STMT_EXPR,
	AST_STMT_VAR_DECL,
	AST_STMT_BLOCK,
	AST_STMT_IF,
	AST_STMT_WHILE,
	AST_STMT_DO_WHILE,
	AST_STMT_FOR_C,
	AST_STMT_FOR_IN,
	AST_STMT_RETURN,
	AST_STMT_BREAK,
	AST_STMT_CONTINUE,
	AST_STMT_FUNC_DECL,
	AST_STMT_CLASS_DECL
} AstNodeType;

typedef enum AstBinaryOp {
	BINOP_NONE = 0,
	BINOP_ADD,
	BINOP_SUB,
	BINOP_MUL,
	BINOP_DIV,
	BINOP_MOD,
	BINOP_EQ,
	BINOP_NEQ,
	BINOP_LT,
	BINOP_LTE,
	BINOP_GT,
	BINOP_GTE,
	BINOP_AND,
	BINOP_OR,
	BINOP_BIT_AND,
	BINOP_BIT_OR,
	BINOP_BIT_XOR,
	BINOP_SHL,
	BINOP_SHR
} AstBinaryOp;

typedef enum AstUnaryOp {
	UNOP_NONE = 0,
	UNOP_NEG,
	UNOP_NOT,
	UNOP_BIT_NOT
} AstUnaryOp;

const char* ast_binop_to_string(AstBinaryOp op);
const char* ast_unop_to_string(AstUnaryOp op);
AstBinaryOp ast_binop_from_string(const char* op_str);

/* Forward Declarations */
typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;

/* Parameter for Function Declarations */
typedef struct AstParam {
	char* name;
	char* type_name;
} AstParam;

/* -------------------------------------------------------------------------
 * Expression Node
 * ------------------------------------------------------------------------- */
struct AstExpr {
	AstNodeType type;
	int line;
	int col;

	union {
		/* AST_EXPR_LITERAL_INT */
		int64_t int_val;

		/* AST_EXPR_LITERAL_FLOAT */
		double float_val;

		/* AST_EXPR_LITERAL_STRING */
		char* string_val;

		/* AST_EXPR_LITERAL_BOOL */
		bool bool_val;

		/* AST_EXPR_IDENTIFIER */
		char* identifier_name;

		/* AST_EXPR_BINARY */
		struct {
			AstBinaryOp op;
			AstExpr* left;
			AstExpr* right;
		} binary;

		/* AST_EXPR_UNARY */
		struct {
			AstUnaryOp op;
			AstExpr* operand;
		} unary;

		/* AST_EXPR_CALL */
		struct {
			char* name;
			AstExpr** args;
			int arg_count;
		} call;

		/* AST_EXPR_METHOD_CALL */
		struct {
			AstExpr* object;
			char* method_name;
			AstExpr** args;
			int arg_count;
		} method_call;

		/* AST_EXPR_MEMBER */
		struct {
			AstExpr* object;
			char* member_name;
		} member;

		/* AST_EXPR_INDEX */
		struct {
			AstExpr* target;
			AstExpr* index;
		} index;

		/* AST_EXPR_LIST */
		struct {
			AstExpr** elements;
			int element_count;
		} list;

		/* AST_EXPR_ASSIGN */
		struct {
			AstExpr* target;
			char* op; /* "=", "+=", "-=", etc. */
			AstExpr* value;
		} assign;
	} as;
};

/* -------------------------------------------------------------------------
 * Statement Node
 * ------------------------------------------------------------------------- */
struct AstStmt {
	AstNodeType type;
	int line;
	int col;

	union {
		/* AST_STMT_EXPR */
		AstExpr* expr;

		/* AST_STMT_VAR_DECL */
		struct {
			char* var_name;
			char* type_name;
			AstExpr* init_expr;
			bool is_static;
		} var_decl;

		/* AST_STMT_BLOCK */
		struct {
			AstStmt** stmts;
			int stmt_count;
		} block;

		/* AST_STMT_IF */
		struct {
			AstExpr* condition;
			AstStmt* then_branch;
			AstStmt* else_branch; /* may be NULL */
		} if_stmt;

		/* AST_STMT_WHILE */
		struct {
			AstExpr* condition;
			AstStmt* body;
		} while_stmt;

		/* AST_STMT_DO_WHILE */
		struct {
			AstStmt* body;
			AstExpr* condition;
		} do_while_stmt;

		/* AST_STMT_FOR_C */
		struct {
			AstStmt* init;
			AstExpr* condition;
			AstExpr* step;
			AstStmt* body;
		} for_c;

		/* AST_STMT_FOR_IN */
		struct {
			char* item_var;
			AstExpr* collection;
			AstStmt* body;
		} for_in;

		/* AST_STMT_RETURN */
		AstExpr* return_expr; /* may be NULL */

		/* AST_STMT_FUNC_DECL */
		struct {
			char* name;
			char* return_type;
			AstParam* params;
			int param_count;
			AstStmt* body;
			bool is_static;
			char* class_name; /* NULL if top-level */
		} func_decl;

		/* AST_STMT_CLASS_DECL */
		struct {
			char* name;
			char* base_name; /* may be NULL */
			AstStmt** members;
			int member_count;
		} class_decl;
	} as;
};

/* -------------------------------------------------------------------------
 * Complete AST Program
 * ------------------------------------------------------------------------- */
typedef struct AstProgram {
	AstArena* arena;
	AstStmt** statements;
	int statement_count;
	int statement_capacity;
} AstProgram;

/* -------------------------------------------------------------------------
 * AST Node Constructors
 * ------------------------------------------------------------------------- */
AstExpr* ast_expr_literal_int(AstArena* arena, int64_t val, int line, int col);
AstExpr* ast_expr_literal_float(AstArena* arena, double val, int line, int col);
AstExpr* ast_expr_literal_string(AstArena* arena, const char* val, int line, int col);
AstExpr* ast_expr_literal_bool(AstArena* arena, bool val, int line, int col);
AstExpr* ast_expr_literal_null(AstArena* arena, int line, int col);
AstExpr* ast_expr_identifier(AstArena* arena, const char* name, int line, int col);
AstExpr* ast_expr_binary(AstArena* arena, AstBinaryOp op, AstExpr* left, AstExpr* right, int line, int col);
AstExpr* ast_expr_unary(AstArena* arena, AstUnaryOp op, AstExpr* operand, int line, int col);
AstExpr* ast_expr_call(AstArena* arena, const char* name, AstExpr** args, int arg_count, int line, int col);
AstExpr* ast_expr_method_call(AstArena* arena, AstExpr* obj, const char* method, AstExpr** args, int arg_count, int line, int col);
AstExpr* ast_expr_member(AstArena* arena, AstExpr* obj, const char* member, int line, int col);
AstExpr* ast_expr_index(AstArena* arena, AstExpr* target, AstExpr* index, int line, int col);
AstExpr* ast_expr_list(AstArena* arena, AstExpr** elements, int count, int line, int col);
AstExpr* ast_expr_assign(AstArena* arena, AstExpr* target, const char* op, AstExpr* value, int line, int col);

AstStmt* ast_stmt_expr(AstArena* arena, AstExpr* expr, int line, int col);
AstStmt* ast_stmt_var_decl(AstArena* arena, const char* type, const char* name, AstExpr* init, bool is_static, int line, int col);
AstStmt* ast_stmt_block(AstArena* arena, AstStmt** stmts, int count, int line, int col);
AstStmt* ast_stmt_if(AstArena* arena, AstExpr* cond, AstStmt* then_b, AstStmt* else_b, int line, int col);
AstStmt* ast_stmt_while(AstArena* arena, AstExpr* cond, AstStmt* body, int line, int col);
AstStmt* ast_stmt_do_while(AstArena* arena, AstStmt* body, AstExpr* cond, int line, int col);
AstStmt* ast_stmt_for_c(AstArena* arena, AstStmt* init, AstExpr* cond, AstExpr* step, AstStmt* body, int line, int col);
AstStmt* ast_stmt_for_in(AstArena* arena, const char* item_var, AstExpr* coll, AstStmt* body, int line, int col);
AstStmt* ast_stmt_return(AstArena* arena, AstExpr* expr, int line, int col);
AstStmt* ast_stmt_break(AstArena* arena, int line, int col);
AstStmt* ast_stmt_continue(AstArena* arena, int line, int col);
AstStmt* ast_stmt_func_decl(AstArena* arena, const char* name, const char* ret_type, AstParam* params, int pcount, AstStmt* body, bool is_static, const char* class_name, int line, int col);
AstStmt* ast_stmt_class_decl(AstArena* arena, const char* name, const char* base, AstStmt** members, int mcount, int line, int col);

AstProgram* ast_program_create(AstArena* arena);
void ast_program_add_stmt(AstProgram* prog, AstStmt* stmt);
void ast_program_destroy(AstProgram* prog);

/* -------------------------------------------------------------------------
 * AST Visual Formatter / Pretty Printer
 * ------------------------------------------------------------------------- */
void xast_dump_program(const AstProgram* prog);
void xast_dump_stmt(const AstStmt* stmt, int indent, bool is_last);
void xast_dump_expr(const AstExpr* expr, int indent, bool is_last);

#ifdef __cplusplus
}
#endif

#endif /* XAST_H */
