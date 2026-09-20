#include "xast.h"

/* -------------------------------------------------------------------------
 * AST Arena Allocator Implementation
 * ------------------------------------------------------------------------- */
AstArena* ast_arena_create(size_t initial_capacity)
{
	if (initial_capacity < 4096)
		initial_capacity = 4096;

	AstArena* arena = (AstArena*)malloc(sizeof(AstArena));
	if (!arena) return NULL;

	AstArenaBlock* block = (AstArenaBlock*)malloc(sizeof(AstArenaBlock));
	if (!block)
	{
		free(arena);
		return NULL;
	}

	block->data = (char*)malloc(initial_capacity);
	if (!block->data)
	{
		free(block);
		free(arena);
		return NULL;
	}

	block->next = NULL;
	block->capacity = initial_capacity;
	block->used = 0;

	arena->first = block;
	arena->current = block;
	arena->total_allocated = initial_capacity;
	return arena;
}

void* ast_arena_alloc(AstArena* arena, size_t size)
{
	if (!arena) return malloc(size);

	/* 8-byte alignment */
	size_t aligned = (size + 7) & ~((size_t)7);

	if (arena->current->used + aligned > arena->current->capacity)
	{
		size_t next_cap = arena->current->capacity * 2;
		if (next_cap < aligned * 2) next_cap = aligned * 2;

		AstArenaBlock* block = (AstArenaBlock*)malloc(sizeof(AstArenaBlock));
		if (!block) return NULL;

		block->data = (char*)malloc(next_cap);
		if (!block->data)
		{
			free(block);
			return NULL;
		}

		block->next = NULL;
		block->capacity = next_cap;
		block->used = 0;

		arena->current->next = block;
		arena->current = block;
		arena->total_allocated += next_cap;
	}

	void* ptr = arena->current->data + arena->current->used;
	arena->current->used += aligned;
	return ptr;
}

char* ast_arena_strdup(AstArena* arena, const char* str)
{
	if (!str) return NULL;
	size_t len = strlen(str);
	char* copy = (char*)ast_arena_alloc(arena, len + 1);
	if (copy)
	{
		memcpy(copy, str, len + 1);
	}
	return copy;
}

void ast_arena_destroy(AstArena* arena)
{
	if (!arena) return;
	AstArenaBlock* curr = arena->first;
	while (curr != NULL)
	{
		AstArenaBlock* next = curr->next;
		if (curr->data) free(curr->data);
		free(curr);
		curr = next;
	}
	free(arena);
}

/* -------------------------------------------------------------------------
 * Operator Conversions
 * ------------------------------------------------------------------------- */
const char* ast_binop_to_string(AstBinaryOp op)
{
	switch (op)
	{
	case BINOP_ADD:     return "+";
	case BINOP_SUB:     return "-";
	case BINOP_MUL:     return "*";
	case BINOP_DIV:     return "/";
	case BINOP_MOD:     return "%";
	case BINOP_EQ:      return "==";
	case BINOP_NEQ:     return "!=";
	case BINOP_LT:      return "<";
	case BINOP_LTE:     return "<=";
	case BINOP_GT:      return ">";
	case BINOP_GTE:     return ">=";
	case BINOP_AND:     return "&&";
	case BINOP_OR:      return "||";
	case BINOP_BIT_AND: return "&";
	case BINOP_BIT_OR:  return "|";
	case BINOP_BIT_XOR: return "^";
	case BINOP_SHL:     return "<<";
	case BINOP_SHR:     return ">>";
	default:            return "?";
	}
}

const char* ast_unop_to_string(AstUnaryOp op)
{
	switch (op)
	{
	case UNOP_NEG:     return "-";
	case UNOP_NOT:     return "!";
	case UNOP_BIT_NOT: return "~";
	default:           return "?";
	}
}

AstBinaryOp ast_binop_from_string(const char* op_str)
{
	if (!op_str) return BINOP_NONE;
	if (strcmp(op_str, "+") == 0) return BINOP_ADD;
	if (strcmp(op_str, "-") == 0) return BINOP_SUB;
	if (strcmp(op_str, "*") == 0) return BINOP_MUL;
	if (strcmp(op_str, "/") == 0) return BINOP_DIV;
	if (strcmp(op_str, "%") == 0) return BINOP_MOD;
	if (strcmp(op_str, "==") == 0) return BINOP_EQ;
	if (strcmp(op_str, "!=") == 0) return BINOP_NEQ;
	if (strcmp(op_str, "<") == 0) return BINOP_LT;
	if (strcmp(op_str, "<=") == 0) return BINOP_LTE;
	if (strcmp(op_str, ">") == 0) return BINOP_GT;
	if (strcmp(op_str, ">=") == 0) return BINOP_GTE;
	if (strcmp(op_str, "&&") == 0) return BINOP_AND;
	if (strcmp(op_str, "||") == 0) return BINOP_OR;
	if (strcmp(op_str, "&") == 0) return BINOP_BIT_AND;
	if (strcmp(op_str, "|") == 0) return BINOP_BIT_OR;
	if (strcmp(op_str, "^") == 0) return BINOP_BIT_XOR;
	if (strcmp(op_str, "<<") == 0) return BINOP_SHL;
	if (strcmp(op_str, ">>") == 0) return BINOP_SHR;
	return BINOP_NONE;
}

/* -------------------------------------------------------------------------
 * AST Expression Constructors
 * ------------------------------------------------------------------------- */
static AstExpr* alloc_expr(AstArena* arena, AstNodeType type, int line, int col)
{
	AstExpr* e = (AstExpr*)ast_arena_alloc(arena, sizeof(AstExpr));
	memset(e, 0, sizeof(AstExpr));
	e->type = type;
	e->line = line;
	e->col = col;
	return e;
}

AstExpr* ast_expr_literal_int(AstArena* arena, int64_t val, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_LITERAL_INT, line, col);
	e->as.int_val = val;
	return e;
}

AstExpr* ast_expr_literal_float(AstArena* arena, double val, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_LITERAL_FLOAT, line, col);
	e->as.float_val = val;
	return e;
}

AstExpr* ast_expr_literal_string(AstArena* arena, const char* val, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_LITERAL_STRING, line, col);
	e->as.string_val = ast_arena_strdup(arena, val);
	return e;
}

AstExpr* ast_expr_literal_bool(AstArena* arena, bool val, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_LITERAL_BOOL, line, col);
	e->as.bool_val = val;
	return e;
}

AstExpr* ast_expr_literal_null(AstArena* arena, int line, int col)
{
	return alloc_expr(arena, AST_EXPR_LITERAL_NULL, line, col);
}

AstExpr* ast_expr_identifier(AstArena* arena, const char* name, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_IDENTIFIER, line, col);
	e->as.identifier_name = ast_arena_strdup(arena, name);
	return e;
}

AstExpr* ast_expr_binary(AstArena* arena, AstBinaryOp op, AstExpr* left, AstExpr* right, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_BINARY, line, col);
	e->as.binary.op = op;
	e->as.binary.left = left;
	e->as.binary.right = right;
	return e;
}

AstExpr* ast_expr_unary(AstArena* arena, AstUnaryOp op, AstExpr* operand, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_UNARY, line, col);
	e->as.unary.op = op;
	e->as.unary.operand = operand;
	return e;
}

AstExpr* ast_expr_call(AstArena* arena, const char* name, AstExpr** args, int arg_count, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_CALL, line, col);
	e->as.call.name = ast_arena_strdup(arena, name);
	e->as.call.arg_count = arg_count;
	if (arg_count > 0 && args != NULL)
	{
		e->as.call.args = (AstExpr**)ast_arena_alloc(arena, sizeof(AstExpr*) * arg_count);
		memcpy(e->as.call.args, args, sizeof(AstExpr*) * arg_count);
	}
	else
	{
		e->as.call.args = NULL;
	}
	return e;
}

AstExpr* ast_expr_method_call(AstArena* arena, AstExpr* obj, const char* method, AstExpr** args, int arg_count, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_METHOD_CALL, line, col);
	e->as.method_call.object = obj;
	e->as.method_call.method_name = ast_arena_strdup(arena, method);
	e->as.method_call.arg_count = arg_count;
	if (arg_count > 0 && args != NULL)
	{
		e->as.method_call.args = (AstExpr**)ast_arena_alloc(arena, sizeof(AstExpr*) * arg_count);
		memcpy(e->as.method_call.args, args, sizeof(AstExpr*) * arg_count);
	}
	else
	{
		e->as.method_call.args = NULL;
	}
	return e;
}

AstExpr* ast_expr_member(AstArena* arena, AstExpr* obj, const char* member, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_MEMBER, line, col);
	e->as.member.object = obj;
	e->as.member.member_name = ast_arena_strdup(arena, member);
	return e;
}

AstExpr* ast_expr_index(AstArena* arena, AstExpr* target, AstExpr* index, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_INDEX, line, col);
	e->as.index.target = target;
	e->as.index.index = index;
	return e;
}

AstExpr* ast_expr_list(AstArena* arena, AstExpr** elements, int count, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_LIST, line, col);
	e->as.list.element_count = count;
	if (count > 0 && elements != NULL)
	{
		e->as.list.elements = (AstExpr**)ast_arena_alloc(arena, sizeof(AstExpr*) * count);
		memcpy(e->as.list.elements, elements, sizeof(AstExpr*) * count);
	}
	else
	{
		e->as.list.elements = NULL;
	}
	return e;
}

AstExpr* ast_expr_assign(AstArena* arena, AstExpr* target, const char* op, AstExpr* value, int line, int col)
{
	AstExpr* e = alloc_expr(arena, AST_EXPR_ASSIGN, line, col);
	e->as.assign.target = target;
	e->as.assign.op = ast_arena_strdup(arena, op);
	e->as.assign.value = value;
	return e;
}

/* -------------------------------------------------------------------------
 * AST Statement Constructors
 * ------------------------------------------------------------------------- */
static AstStmt* alloc_stmt(AstArena* arena, AstNodeType type, int line, int col)
{
	AstStmt* s = (AstStmt*)ast_arena_alloc(arena, sizeof(AstStmt));
	memset(s, 0, sizeof(AstStmt));
	s->type = type;
	s->line = line;
	s->col = col;
	return s;
}

AstStmt* ast_stmt_expr(AstArena* arena, AstExpr* expr, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_EXPR, line, col);
	s->as.expr = expr;
	return s;
}

AstStmt* ast_stmt_var_decl(AstArena* arena, const char* type, const char* name, AstExpr* init, bool is_static, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_VAR_DECL, line, col);
	s->as.var_decl.type_name = ast_arena_strdup(arena, type);
	s->as.var_decl.var_name = ast_arena_strdup(arena, name);
	s->as.var_decl.init_expr = init;
	s->as.var_decl.is_static = is_static;
	return s;
}

AstStmt* ast_stmt_block(AstArena* arena, AstStmt** stmts, int count, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_BLOCK, line, col);
	s->as.block.stmt_count = count;
	if (count > 0 && stmts != NULL)
	{
		s->as.block.stmts = (AstStmt**)ast_arena_alloc(arena, sizeof(AstStmt*) * count);
		memcpy(s->as.block.stmts, stmts, sizeof(AstStmt*) * count);
	}
	else
	{
		s->as.block.stmts = NULL;
	}
	return s;
}

AstStmt* ast_stmt_if(AstArena* arena, AstExpr* cond, AstStmt* then_b, AstStmt* else_b, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_IF, line, col);
	s->as.if_stmt.condition = cond;
	s->as.if_stmt.then_branch = then_b;
	s->as.if_stmt.else_branch = else_b;
	return s;
}

AstStmt* ast_stmt_while(AstArena* arena, AstExpr* cond, AstStmt* body, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_WHILE, line, col);
	s->as.while_stmt.condition = cond;
	s->as.while_stmt.body = body;
	return s;
}

AstStmt* ast_stmt_do_while(AstArena* arena, AstStmt* body, AstExpr* cond, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_DO_WHILE, line, col);
	s->as.do_while_stmt.body = body;
	s->as.do_while_stmt.condition = cond;
	return s;
}

AstStmt* ast_stmt_for_c(AstArena* arena, AstStmt* init, AstExpr* cond, AstExpr* step, AstStmt* body, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_FOR_C, line, col);
	s->as.for_c.init = init;
	s->as.for_c.condition = cond;
	s->as.for_c.step = step;
	s->as.for_c.body = body;
	return s;
}

AstStmt* ast_stmt_for_in(AstArena* arena, const char* item_var, AstExpr* coll, AstStmt* body, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_FOR_IN, line, col);
	s->as.for_in.item_var = ast_arena_strdup(arena, item_var);
	s->as.for_in.collection = coll;
	s->as.for_in.body = body;
	return s;
}

AstStmt* ast_stmt_return(AstArena* arena, AstExpr* expr, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_RETURN, line, col);
	s->as.return_expr = expr;
	return s;
}

AstStmt* ast_stmt_break(AstArena* arena, int line, int col)
{
	return alloc_stmt(arena, AST_STMT_BREAK, line, col);
}

AstStmt* ast_stmt_continue(AstArena* arena, int line, int col)
{
	return alloc_stmt(arena, AST_STMT_CONTINUE, line, col);
}

AstStmt* ast_stmt_func_decl(AstArena* arena, const char* name, const char* ret_type, AstParam* params, int pcount, AstStmt* body, bool is_static, const char* class_name, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_FUNC_DECL, line, col);
	s->as.func_decl.name = ast_arena_strdup(arena, name);
	s->as.func_decl.return_type = ast_arena_strdup(arena, ret_type ? ret_type : "void");
	s->as.func_decl.param_count = pcount;
	s->as.func_decl.body = body;
	s->as.func_decl.is_static = is_static;
	s->as.func_decl.class_name = class_name ? ast_arena_strdup(arena, class_name) : NULL;

	if (pcount > 0 && params != NULL)
	{
		s->as.func_decl.params = (AstParam*)ast_arena_alloc(arena, sizeof(AstParam) * pcount);
		for (int i = 0; i < pcount; i++)
		{
			s->as.func_decl.params[i].name = ast_arena_strdup(arena, params[i].name);
			s->as.func_decl.params[i].type_name = ast_arena_strdup(arena, params[i].type_name);
		}
	}
	else
	{
		s->as.func_decl.params = NULL;
	}
	return s;
}

AstStmt* ast_stmt_class_decl(AstArena* arena, const char* name, const char* base, AstStmt** members, int mcount, int line, int col)
{
	AstStmt* s = alloc_stmt(arena, AST_STMT_CLASS_DECL, line, col);
	s->as.class_decl.name = ast_arena_strdup(arena, name);
	s->as.class_decl.base_name = base ? ast_arena_strdup(arena, base) : NULL;
	s->as.class_decl.member_count = mcount;
	if (mcount > 0 && members != NULL)
	{
		s->as.class_decl.members = (AstStmt**)ast_arena_alloc(arena, sizeof(AstStmt*) * mcount);
		memcpy(s->as.class_decl.members, members, sizeof(AstStmt*) * mcount);
	}
	else
	{
		s->as.class_decl.members = NULL;
	}
	return s;
}

/* -------------------------------------------------------------------------
 * Complete AST Program
 * ------------------------------------------------------------------------- */
AstProgram* ast_program_create(AstArena* arena)
{
	AstProgram* prog = (AstProgram*)ast_arena_alloc(arena, sizeof(AstProgram));
	prog->arena = arena;
	prog->statement_count = 0;
	prog->statement_capacity = 32;
	prog->statements = (AstStmt**)ast_arena_alloc(arena, sizeof(AstStmt*) * prog->statement_capacity);
	return prog;
}

void ast_program_add_stmt(AstProgram* prog, AstStmt* stmt)
{
	if (!prog || !stmt) return;
	if (prog->statement_count >= prog->statement_capacity)
	{
		int new_cap = prog->statement_capacity * 2;
		AstStmt** new_arr = (AstStmt**)ast_arena_alloc(prog->arena, sizeof(AstStmt*) * new_cap);
		memcpy(new_arr, prog->statements, sizeof(AstStmt*) * prog->statement_count);
		prog->statements = new_arr;
		prog->statement_capacity = new_cap;
	}
	prog->statements[prog->statement_count++] = stmt;
}

void ast_program_destroy(AstProgram* prog)
{
	if (!prog) return;
	if (prog->arena)
	{
		ast_arena_destroy(prog->arena);
	}
}

/* -------------------------------------------------------------------------
 * AST Visual Pretty Printer / Formatter
 * ------------------------------------------------------------------------- */
static void print_indent(int indent)
{
	for (int i = 0; i < indent; i++)
		printf("  ");
}

void xast_dump_expr(const AstExpr* expr, int indent, bool is_last)
{
	if (!expr)
	{
		print_indent(indent);
		printf("(null expr)\n");
		return;
	}

	print_indent(indent);
	switch (expr->type)
	{
	case AST_EXPR_LITERAL_INT:
		printf("\033[33mLiteralInt\033[0m: %ld\n", (long)expr->as.int_val);
		break;
	case AST_EXPR_LITERAL_FLOAT:
		printf("\033[33mLiteralFloat\033[0m: %f\n", expr->as.float_val);
		break;
	case AST_EXPR_LITERAL_STRING:
		printf("\033[32mLiteralString\033[0m: \"%s\"\n", expr->as.string_val ? expr->as.string_val : "");
		break;
	case AST_EXPR_LITERAL_BOOL:
		printf("\033[35mLiteralBool\033[0m: %s\n", expr->as.bool_val ? "true" : "false");
		break;
	case AST_EXPR_LITERAL_NULL:
		printf("\033[31mLiteralNull\033[0m\n");
		break;
	case AST_EXPR_IDENTIFIER:
		printf("\033[36mIdentifier\033[0m: %s\n", expr->as.identifier_name);
		break;
	case AST_EXPR_BINARY:
		printf("\033[1;34mBinaryExpr\033[0m (%s)\n", ast_binop_to_string(expr->as.binary.op));
		xast_dump_expr(expr->as.binary.left, indent + 1, false);
		xast_dump_expr(expr->as.binary.right, indent + 1, true);
		break;
	case AST_EXPR_UNARY:
		printf("\033[1;34mUnaryExpr\033[0m (%s)\n", ast_unop_to_string(expr->as.unary.op));
		xast_dump_expr(expr->as.unary.operand, indent + 1, true);
		break;
	case AST_EXPR_CALL:
		printf("\033[1;32mCallExpr\033[0m: %s (%d args)\n", expr->as.call.name, expr->as.call.arg_count);
		for (int i = 0; i < expr->as.call.arg_count; i++)
			xast_dump_expr(expr->as.call.args[i], indent + 1, i == expr->as.call.arg_count - 1);
		break;
	case AST_EXPR_METHOD_CALL:
		printf("\033[1;32mMethodCall\033[0m: .%s (%d args)\n", expr->as.method_call.method_name, expr->as.method_call.arg_count);
		print_indent(indent + 1);
		printf("Receiver:\n");
		xast_dump_expr(expr->as.method_call.object, indent + 2, false);
		for (int i = 0; i < expr->as.method_call.arg_count; i++)
			xast_dump_expr(expr->as.method_call.args[i], indent + 1, i == expr->as.method_call.arg_count - 1);
		break;
	case AST_EXPR_MEMBER:
		printf("\033[36mMemberExpr\033[0m: .%s\n", expr->as.member.member_name);
		xast_dump_expr(expr->as.member.object, indent + 1, true);
		break;
	case AST_EXPR_INDEX:
		printf("\033[34mIndexExpr\033[0m []\n");
		xast_dump_expr(expr->as.index.target, indent + 1, false);
		xast_dump_expr(expr->as.index.index, indent + 1, true);
		break;
	case AST_EXPR_LIST:
		printf("\033[1;33mListLiteral\033[0m (%d items)\n", expr->as.list.element_count);
		for (int i = 0; i < expr->as.list.element_count; i++)
			xast_dump_expr(expr->as.list.elements[i], indent + 1, i == expr->as.list.element_count - 1);
		break;
	case AST_EXPR_ASSIGN:
		printf("\033[1;31mAssignExpr\033[0m (%s)\n", expr->as.assign.op);
		xast_dump_expr(expr->as.assign.target, indent + 1, false);
		xast_dump_expr(expr->as.assign.value, indent + 1, true);
		break;
	default:
		printf("UnknownExpr (%d)\n", expr->type);
		break;
	}
}

void xast_dump_stmt(const AstStmt* stmt, int indent, bool is_last)
{
	if (!stmt) return;

	print_indent(indent);
	switch (stmt->type)
	{
	case AST_STMT_EXPR:
		printf("\033[1mExprStmt\033[0m [line %d]\n", stmt->line);
		xast_dump_expr(stmt->as.expr, indent + 1, true);
		break;
	case AST_STMT_VAR_DECL:
		printf("\033[1;32mVarDecl\033[0m %s %s [line %d]%s\n",
		       stmt->as.var_decl.type_name, stmt->as.var_decl.var_name, stmt->line,
		       stmt->as.var_decl.is_static ? " (static)" : "");
		if (stmt->as.var_decl.init_expr)
		{
			xast_dump_expr(stmt->as.var_decl.init_expr, indent + 1, true);
		}
		break;
	case AST_STMT_BLOCK:
		printf("\033[1mBlockStmt\033[0m (%d statements)\n", stmt->as.block.stmt_count);
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
			xast_dump_stmt(stmt->as.block.stmts[i], indent + 1, i == stmt->as.block.stmt_count - 1);
		break;
	case AST_STMT_IF:
		printf("\033[1;35mIfStmt\033[0m [line %d]\n", stmt->line);
		print_indent(indent + 1);
		printf("Condition:\n");
		xast_dump_expr(stmt->as.if_stmt.condition, indent + 2, false);
		print_indent(indent + 1);
		printf("Then:\n");
		xast_dump_stmt(stmt->as.if_stmt.then_branch, indent + 2, stmt->as.if_stmt.else_branch == NULL);
		if (stmt->as.if_stmt.else_branch)
		{
			print_indent(indent + 1);
			printf("Else:\n");
			xast_dump_stmt(stmt->as.if_stmt.else_branch, indent + 2, true);
		}
		break;
	case AST_STMT_WHILE:
		printf("\033[1;35mWhileStmt\033[0m [line %d]\n", stmt->line);
		print_indent(indent + 1);
		printf("Condition:\n");
		xast_dump_expr(stmt->as.while_stmt.condition, indent + 2, false);
		print_indent(indent + 1);
		printf("Body:\n");
		xast_dump_stmt(stmt->as.while_stmt.body, indent + 2, true);
		break;
	case AST_STMT_DO_WHILE:
		printf("\033[1;35mDoWhileStmt\033[0m [line %d]\n", stmt->line);
		print_indent(indent + 1);
		printf("Body:\n");
		xast_dump_stmt(stmt->as.do_while_stmt.body, indent + 2, false);
		print_indent(indent + 1);
		printf("Condition:\n");
		xast_dump_expr(stmt->as.do_while_stmt.condition, indent + 2, true);
		break;
	case AST_STMT_FOR_C:
		printf("\033[1;35mForCStmt\033[0m [line %d]\n", stmt->line);
		if (stmt->as.for_c.init)
		{
			print_indent(indent + 1);
			printf("Init:\n");
			xast_dump_stmt(stmt->as.for_c.init, indent + 2, false);
		}
		if (stmt->as.for_c.condition)
		{
			print_indent(indent + 1);
			printf("Condition:\n");
			xast_dump_expr(stmt->as.for_c.condition, indent + 2, false);
		}
		if (stmt->as.for_c.step)
		{
			print_indent(indent + 1);
			printf("Step:\n");
			xast_dump_expr(stmt->as.for_c.step, indent + 2, false);
		}
		print_indent(indent + 1);
		printf("Body:\n");
		xast_dump_stmt(stmt->as.for_c.body, indent + 2, true);
		break;
	case AST_STMT_FOR_IN:
		printf("\033[1;35mForInStmt\033[0m (var %s) [line %d]\n", stmt->as.for_in.item_var, stmt->line);
		print_indent(indent + 1);
		printf("Collection:\n");
		xast_dump_expr(stmt->as.for_in.collection, indent + 2, false);
		print_indent(indent + 1);
		printf("Body:\n");
		xast_dump_stmt(stmt->as.for_in.body, indent + 2, true);
		break;
	case AST_STMT_RETURN:
		printf("\033[1;33mReturnStmt\033[0m [line %d]\n", stmt->line);
		if (stmt->as.return_expr)
			xast_dump_expr(stmt->as.return_expr, indent + 1, true);
		break;
	case AST_STMT_BREAK:
		printf("\033[1;33mBreakStmt\033[0m [line %d]\n", stmt->line);
		break;
	case AST_STMT_CONTINUE:
		printf("\033[1;33mContinueStmt\033[0m [line %d]\n", stmt->line);
		break;
	case AST_STMT_FUNC_DECL:
		printf("\033[1;36mFuncDecl\033[0m %s %s(%d params) [line %d]%s\n",
		       stmt->as.func_decl.return_type, stmt->as.func_decl.name,
		       stmt->as.func_decl.param_count, stmt->line,
		       stmt->as.func_decl.is_static ? " (static)" : "");
		for (int i = 0; i < stmt->as.func_decl.param_count; i++)
		{
			print_indent(indent + 1);
			printf("Param: %s %s\n", stmt->as.func_decl.params[i].type_name, stmt->as.func_decl.params[i].name);
		}
		if (stmt->as.func_decl.body)
		{
			print_indent(indent + 1);
			printf("Body:\n");
			xast_dump_stmt(stmt->as.func_decl.body, indent + 2, true);
		}
		break;
	case AST_STMT_CLASS_DECL:
		printf("\033[1;34mClassDecl\033[0m %s%s%s [line %d]\n",
		       stmt->as.class_decl.name,
		       stmt->as.class_decl.base_name ? " : " : "",
		       stmt->as.class_decl.base_name ? stmt->as.class_decl.base_name : "",
		       stmt->line);
		for (int i = 0; i < stmt->as.class_decl.member_count; i++)
			xast_dump_stmt(stmt->as.class_decl.members[i], indent + 1, i == stmt->as.class_decl.member_count - 1);
		break;
	default:
		printf("UnknownStmt (%d)\n", stmt->type);
		break;
	}
}

void xast_dump_program(const AstProgram* prog)
{
	if (!prog) return;
	printf("\n\033[1;36m=== Structured AST Program (%d statements) ===\033[0m\n", prog->statement_count);
	for (int i = 0; i < prog->statement_count; i++)
	{
		xast_dump_stmt(prog->statements[i], 0, i == prog->statement_count - 1);
	}
	printf("\033[1;36m===============================================\033[0m\n\n");
}
