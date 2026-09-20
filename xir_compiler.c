#include "xir_compiler.h"
#include "functions.h"

typedef struct XIrLocal {
	char* name;
	int depth;
} XIrLocal;

typedef struct XIrLoop {
	int loop_start;
	int break_jumps[64];
	int break_count;
	struct XIrLoop* enclosing;
} XIrLoop;

typedef struct XCompilerUpvalue {
	uint8_t index;
	bool is_local;
} XCompilerUpvalue;

typedef struct XCompiler {
	struct XCompiler* enclosing;
	XFunction* function;
	XIrChunk* chunk;
	XIrLocal locals[256];
	int local_count;
	int scope_depth;
	XCompilerUpvalue upvalues[64];
	int upvalue_count;
	XIrLoop* current_loop;
} XCompiler;

static void compiler_init(XCompiler* c, XIrChunk* chunk)
{
	c->enclosing = NULL;
	c->function = NULL;
	c->chunk = chunk;
	c->local_count = 0;
	c->scope_depth = 0;
	c->upvalue_count = 0;
	c->current_loop = NULL;
}

static void enter_scope(XCompiler* c)
{
	c->scope_depth++;
}

static void exit_scope(XCompiler* c, int line)
{
	c->scope_depth--;
	while (c->local_count > 0 && c->locals[c->local_count - 1].depth > c->scope_depth)
	{
		xir_emit_op(c->chunk, OP_POP, line);
		if (c->locals[c->local_count - 1].name)
		{
			free(c->locals[c->local_count - 1].name);
		}
		c->local_count--;
	}
}

static int resolve_local(XCompiler* c, const char* name)
{
	if (!c || !name) return -1;
	for (int i = c->local_count - 1; i >= 0; i--)
	{
		if (c->locals[i].name && strcmp(c->locals[i].name, name) == 0)
		{
			return i;
		}
	}
	return -1;
}

static void add_local(XCompiler* c, const char* name)
{
	if (c->local_count >= 256) return;
	c->locals[c->local_count].name = strdup(name ? name : "");
	c->locals[c->local_count].depth = c->scope_depth;
	c->local_count++;
}

static int add_upvalue(XCompiler* c, uint8_t index, bool is_local)
{
	for (int i = 0; i < c->upvalue_count; i++)
	{
		if (c->upvalues[i].index == index && c->upvalues[i].is_local == is_local)
		{
			return i;
		}
	}
	if (c->upvalue_count >= 64) return -1;
	c->upvalues[c->upvalue_count].index = index;
	c->upvalues[c->upvalue_count].is_local = is_local;
	return c->upvalue_count++;
}

static int resolve_upvalue(XCompiler* c, const char* name)
{
	if (c == NULL || c->enclosing == NULL || name == NULL) return -1;

	int local = resolve_local(c->enclosing, name);
	if (local != -1)
	{
		return add_upvalue(c, (uint8_t)local, true);
	}

	int upvalue = resolve_upvalue(c->enclosing, name);
	if (upvalue != -1)
	{
		return add_upvalue(c, (uint8_t)upvalue, false);
	}

	return -1;
}

/* Forward declarations */
static void compile_expr_node(XCompiler* c, const AstExpr* expr);
static void compile_stmt_node(XCompiler* c, const AstStmt* stmt);

static void compile_expr_node(XCompiler* c, const AstExpr* expr)
{
	if (!expr) return;
	int line = expr->line;

	switch (expr->type)
	{
	case AST_EXPR_LITERAL_INT:
		xir_emit_op(c->chunk, OP_CONST_INT, line);
		xir_emit_int(c->chunk, (int32_t)expr->as.int_val, line);
		break;

	case AST_EXPR_LITERAL_FLOAT:
		{
			int c_idx = xir_add_constant(c->chunk, xval_float(expr->as.float_val));
			xir_emit_op(c->chunk, OP_CONST_FLOAT, line);
			xir_emit_short(c->chunk, (uint16_t)c_idx, line);
			break;
		}

	case AST_EXPR_LITERAL_STRING:
		{
			int c_idx = xir_add_constant(c->chunk, xval_str(expr->as.string_val));
			xir_emit_op(c->chunk, OP_CONST_STR, line);
			xir_emit_short(c->chunk, (uint16_t)c_idx, line);
			break;
		}

	case AST_EXPR_LITERAL_BOOL:
		xir_emit_op(c->chunk, expr->as.bool_val ? OP_CONST_TRUE : OP_CONST_FALSE, line);
		break;

	case AST_EXPR_LITERAL_NULL:
		xir_emit_op(c->chunk, OP_CONST_NULL, line);
		break;

	case AST_EXPR_IDENTIFIER:
		{
			int local_slot = resolve_local(c, expr->as.identifier_name);
			if (local_slot != -1)
			{
				xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
				xir_emit_short(c->chunk, (uint16_t)local_slot, line);
			}
			else
			{
				int upvalue_slot = resolve_upvalue(c, expr->as.identifier_name);
				if (upvalue_slot != -1)
				{
					xir_emit_op(c->chunk, OP_GET_UPVALUE, line);
					xir_emit_byte(c->chunk, (uint8_t)upvalue_slot, line);
				}
				else
				{
					int s_idx = xir_add_symbol(c->chunk, expr->as.identifier_name);
					xir_emit_op(c->chunk, OP_LOAD_GLOBAL, line);
					xir_emit_short(c->chunk, (uint16_t)s_idx, line);
				}
			}
			break;
		}

	case AST_EXPR_BINARY:
		{
			/* Short circuit for AND and OR */
			if (expr->as.binary.op == BINOP_AND)
			{
				compile_expr_node(c, expr->as.binary.left);
				int jump_false = xir_emit_jump(c->chunk, OP_JUMP_IF_FALSE, line);
				xir_emit_op(c->chunk, OP_POP, line);
				compile_expr_node(c, expr->as.binary.right);
				xir_patch_jump(c->chunk, jump_false);
				return;
			}
			if (expr->as.binary.op == BINOP_OR)
			{
				compile_expr_node(c, expr->as.binary.left);
				int jump_true = xir_emit_jump(c->chunk, OP_JUMP_IF_TRUE, line);
				xir_emit_op(c->chunk, OP_POP, line);
				compile_expr_node(c, expr->as.binary.right);
				xir_patch_jump(c->chunk, jump_true);
				return;
			}

			compile_expr_node(c, expr->as.binary.left);
			compile_expr_node(c, expr->as.binary.right);

			switch (expr->as.binary.op)
			{
			case BINOP_ADD:     xir_emit_op(c->chunk, OP_ADD, line); break;
			case BINOP_SUB:     xir_emit_op(c->chunk, OP_SUB, line); break;
			case BINOP_MUL:     xir_emit_op(c->chunk, OP_MUL, line); break;
			case BINOP_DIV:     xir_emit_op(c->chunk, OP_DIV, line); break;
			case BINOP_MOD:     xir_emit_op(c->chunk, OP_MOD, line); break;
			case BINOP_EQ:      xir_emit_op(c->chunk, OP_EQ, line); break;
			case BINOP_NEQ:     xir_emit_op(c->chunk, OP_NEQ, line); break;
			case BINOP_LT:      xir_emit_op(c->chunk, OP_LT, line); break;
			case BINOP_LTE:     xir_emit_op(c->chunk, OP_LTE, line); break;
			case BINOP_GT:      xir_emit_op(c->chunk, OP_GT, line); break;
			case BINOP_GTE:     xir_emit_op(c->chunk, OP_GTE, line); break;
			case BINOP_BIT_AND: xir_emit_op(c->chunk, OP_BIT_AND, line); break;
			case BINOP_BIT_OR:  xir_emit_op(c->chunk, OP_BIT_OR, line); break;
			case BINOP_BIT_XOR: xir_emit_op(c->chunk, OP_BIT_XOR, line); break;
			case BINOP_SHL:     xir_emit_op(c->chunk, OP_SHL, line); break;
			case BINOP_SHR:     xir_emit_op(c->chunk, OP_SHR, line); break;
			default: break;
			}
			break;
		}

	case AST_EXPR_UNARY:
		compile_expr_node(c, expr->as.unary.operand);
		switch (expr->as.unary.op)
		{
		case UNOP_NEG:     xir_emit_op(c->chunk, OP_NEG, line); break;
		case UNOP_NOT:     xir_emit_op(c->chunk, OP_NOT, line); break;
		case UNOP_BIT_NOT: xir_emit_op(c->chunk, OP_BIT_NOT, line); break;
		default: break;
		}
		break;

	case AST_EXPR_CALL:
		{
			/* Check built-in print */
			if (strcmp(expr->as.call.name, "print") == 0)
			{
				for (int i = 0; i < expr->as.call.arg_count; i++)
				{
					compile_expr_node(c, expr->as.call.args[i]);
				}
				xir_emit_op(c->chunk, OP_PRINT, line);
				xir_emit_byte(c->chunk, (uint8_t)expr->as.call.arg_count, line);
				return;
			}

			int local_slot = resolve_local(c, expr->as.call.name);
			int upvalue_slot = (local_slot == -1) ? resolve_upvalue(c, expr->as.call.name) : -1;

			if (local_slot != -1 || upvalue_slot != -1)
			{
				/* Local/upvalue function closure: push callee first, then args */
				if (local_slot != -1)
				{
					xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
					xir_emit_short(c->chunk, (uint16_t)local_slot, line);
				}
				else
				{
					xir_emit_op(c->chunk, OP_GET_UPVALUE, line);
					xir_emit_byte(c->chunk, (uint8_t)upvalue_slot, line);
				}

				for (int i = 0; i < expr->as.call.arg_count; i++)
				{
					compile_expr_node(c, expr->as.call.args[i]);
				}

				xir_emit_op(c->chunk, OP_CALL, line);
				xir_emit_short(c->chunk, 0xFFFF, line);
				xir_emit_byte(c->chunk, (uint8_t)expr->as.call.arg_count, line);
			}
			else
			{
				/* Check if calling a ClassName(...) constructor */
				type_def* class_td = get_type_by_name((char*)expr->as.call.name);
				if ((class_td != NULL && !is_base_type(class_td)) ||
				    strcmp(expr->as.call.name, "List") == 0 ||
				    strcmp(expr->as.call.name, "Map") == 0 ||
				    strcmp(expr->as.call.name, "HashMap") == 0 ||
				    strcmp(expr->as.call.name, "DateTime") == 0)
				{
					for (int i = 0; i < expr->as.call.arg_count; i++)
					{
						compile_expr_node(c, expr->as.call.args[i]);
					}
					int s_idx = xir_add_symbol(c->chunk, expr->as.call.name);
					xir_emit_op(c->chunk, OP_NEW_INSTANCE, line);
					xir_emit_short(c->chunk, (uint16_t)s_idx, line);
					xir_emit_byte(c->chunk, (uint8_t)expr->as.call.arg_count, line);
					break;
				}

				/* Named global call */
				for (int i = 0; i < expr->as.call.arg_count; i++)
				{
					compile_expr_node(c, expr->as.call.args[i]);
				}
				int s_idx = xir_add_symbol(c->chunk, expr->as.call.name);
				xir_emit_op(c->chunk, OP_CALL, line);
				xir_emit_short(c->chunk, (uint16_t)s_idx, line);
				xir_emit_byte(c->chunk, (uint8_t)expr->as.call.arg_count, line);
			}
			break;
		}

	case AST_EXPR_METHOD_CALL:
		{
			/* Check for static method call: ClassName.methodName(...) */
			if (expr->as.method_call.object->type == AST_EXPR_IDENTIFIER)
			{
				const char* obj_name = expr->as.method_call.object->as.identifier_name;
				if (resolve_local(c, obj_name) == -1 && resolve_upvalue(c, obj_name) == -1 &&
				    get_type_by_name((char*)obj_name) != NULL)
				{
					char static_full_name[256];
					snprintf(static_full_name, sizeof(static_full_name), "%s.%s", obj_name, expr->as.method_call.method_name);
					for (int i = 0; i < expr->as.method_call.arg_count; i++)
					{
						compile_expr_node(c, expr->as.method_call.args[i]);
					}
					int s_idx = xir_add_symbol(c->chunk, static_full_name);
					xir_emit_op(c->chunk, OP_CALL, line);
					xir_emit_short(c->chunk, (uint16_t)s_idx, line);
					xir_emit_byte(c->chunk, (uint8_t)expr->as.method_call.arg_count, line);
					break;
				}
			}

			compile_expr_node(c, expr->as.method_call.object);
			for (int i = 0; i < expr->as.method_call.arg_count; i++)
			{
				compile_expr_node(c, expr->as.method_call.args[i]);
			}
			int s_idx = xir_add_symbol(c->chunk, expr->as.method_call.method_name);
			xir_emit_op(c->chunk, OP_CALL_METHOD, line);
			xir_emit_short(c->chunk, (uint16_t)s_idx, line);
			xir_emit_byte(c->chunk, (uint8_t)expr->as.method_call.arg_count, line);
			break;
		}

	case AST_EXPR_MEMBER:
		{
			compile_expr_node(c, expr->as.member.object);
			int s_idx = xir_add_symbol(c->chunk, expr->as.member.member_name);
			xir_emit_op(c->chunk, OP_LOAD_FIELD, line);
			xir_emit_short(c->chunk, (uint16_t)s_idx, line);
			break;
		}

	case AST_EXPR_INDEX:
		compile_expr_node(c, expr->as.index.target);
		compile_expr_node(c, expr->as.index.index);
		xir_emit_op(c->chunk, OP_LOAD_INDEX, line);
		break;

	case AST_EXPR_LIST:
		for (int i = 0; i < expr->as.list.element_count; i++)
		{
			compile_expr_node(c, expr->as.list.elements[i]);
		}
		xir_emit_op(c->chunk, OP_BUILD_LIST, line);
		xir_emit_short(c->chunk, (uint16_t)expr->as.list.element_count, line);
		break;

	case AST_EXPR_ASSIGN:
		{
			const AstExpr* target = expr->as.assign.target;
			const char* op = expr->as.assign.op;

			/* Handle compound assignment: target += val -> target = target + val */
			bool is_compound = (op && strcmp(op, "=") != 0);

			if (is_compound)
			{
				compile_expr_node(c, target);
				compile_expr_node(c, expr->as.assign.value);
				if (strcmp(op, "+=") == 0) xir_emit_op(c->chunk, OP_ADD, line);
				else if (strcmp(op, "-=") == 0) xir_emit_op(c->chunk, OP_SUB, line);
				else if (strcmp(op, "*=") == 0) xir_emit_op(c->chunk, OP_MUL, line);
				else if (strcmp(op, "/=") == 0) xir_emit_op(c->chunk, OP_DIV, line);
				else if (strcmp(op, "%=") == 0) xir_emit_op(c->chunk, OP_MOD, line);
			}
			else
			{
				compile_expr_node(c, expr->as.assign.value);
			}

			/* Duplicate value on stack if assignment is an expression */
			xir_emit_op(c->chunk, OP_DUP, line);

			if (target->type == AST_EXPR_IDENTIFIER)
			{
				int slot = resolve_local(c, target->as.identifier_name);
				if (slot != -1)
				{
					xir_emit_op(c->chunk, OP_STORE_LOCAL, line);
					xir_emit_short(c->chunk, (uint16_t)slot, line);
				}
				else
				{
					int upvalue_slot = resolve_upvalue(c, target->as.identifier_name);
					if (upvalue_slot != -1)
					{
						xir_emit_op(c->chunk, OP_SET_UPVALUE, line);
						xir_emit_byte(c->chunk, (uint8_t)upvalue_slot, line);
					}
					else
					{
						int s_idx = xir_add_symbol(c->chunk, target->as.identifier_name);
						xir_emit_op(c->chunk, OP_STORE_GLOBAL, line);
						xir_emit_short(c->chunk, (uint16_t)s_idx, line);
					}
				}
			}
			else if (target->type == AST_EXPR_MEMBER)
			{
				compile_expr_node(c, target->as.member.object);
				int s_idx = xir_add_symbol(c->chunk, target->as.member.member_name);
				xir_emit_op(c->chunk, OP_STORE_FIELD, line);
				xir_emit_short(c->chunk, (uint16_t)s_idx, line);
			}
			else if (target->type == AST_EXPR_INDEX)
			{
				compile_expr_node(c, target->as.index.target);
				compile_expr_node(c, target->as.index.index);
				xir_emit_op(c->chunk, OP_STORE_INDEX, line);
			}
			break;
		}

	default:
		break;
	}
}

static void compile_stmt_node(XCompiler* c, const AstStmt* stmt)
{
	if (!stmt) return;
	int line = stmt->line;

	switch (stmt->type)
	{
	case AST_STMT_EXPR:
		compile_expr_node(c, stmt->as.expr);
		xir_emit_op(c->chunk, OP_POP, line);
		break;

	case AST_STMT_VAR_DECL:
		if (stmt->as.var_decl.init_expr)
		{
			compile_expr_node(c, stmt->as.var_decl.init_expr);
		}
		else if (stmt->as.var_decl.type_name &&
		         (strcmp(stmt->as.var_decl.type_name, "List") == 0 ||
		          strcmp(stmt->as.var_decl.type_name, "Map") == 0 ||
		          strcmp(stmt->as.var_decl.type_name, "HashMap") == 0 ||
		          (get_type_by_name((char*)stmt->as.var_decl.type_name) != NULL && !is_base_type(get_type_by_name((char*)stmt->as.var_decl.type_name)))))
		{
			int s_idx = xir_add_symbol(c->chunk, stmt->as.var_decl.type_name);
			xir_emit_op(c->chunk, OP_NEW_INSTANCE, line);
			xir_emit_short(c->chunk, (uint16_t)s_idx, line);
			xir_emit_byte(c->chunk, 0, line);
		}
		else
		{
			xir_emit_op(c->chunk, OP_CONST_NULL, line);
		}

		if (c->scope_depth > 0)
		{
			/* Local variable in stack frame */
			add_local(c, stmt->as.var_decl.var_name);
		}
		else
		{
			/* Global variable */
			int s_idx = xir_add_symbol(c->chunk, stmt->as.var_decl.var_name);
			xir_emit_op(c->chunk, OP_STORE_GLOBAL, line);
			xir_emit_short(c->chunk, (uint16_t)s_idx, line);
		}
		break;

	case AST_STMT_BLOCK:
		enter_scope(c);
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
		{
			compile_stmt_node(c, stmt->as.block.stmts[i]);
		}
		exit_scope(c, line);
		break;

	case AST_STMT_IF:
		{
			compile_expr_node(c, stmt->as.if_stmt.condition);
			int jump_false = xir_emit_jump(c->chunk, OP_JUMP_IF_FALSE, line);
			xir_emit_op(c->chunk, OP_POP, line); /* pop condition */

			compile_stmt_node(c, stmt->as.if_stmt.then_branch);

			int jump_else = xir_emit_jump(c->chunk, OP_JUMP, line);
			xir_patch_jump(c->chunk, jump_false);
			xir_emit_op(c->chunk, OP_POP, line); /* pop condition on false */

			if (stmt->as.if_stmt.else_branch)
			{
				compile_stmt_node(c, stmt->as.if_stmt.else_branch);
			}
			xir_patch_jump(c->chunk, jump_else);
			break;
		}

	case AST_STMT_WHILE:
		{
			XIrLoop loop;
			loop.loop_start = c->chunk->count;
			loop.break_count = 0;
			loop.enclosing = c->current_loop;
			c->current_loop = &loop;

			compile_expr_node(c, stmt->as.while_stmt.condition);
			int exit_jump = xir_emit_jump(c->chunk, OP_JUMP_IF_FALSE, line);
			xir_emit_op(c->chunk, OP_POP, line);

			compile_stmt_node(c, stmt->as.while_stmt.body);

			xir_emit_loop(c->chunk, loop.loop_start, line);
			xir_patch_jump(c->chunk, exit_jump);
			xir_emit_op(c->chunk, OP_POP, line);

			/* Patch breaks */
			for (int i = 0; i < loop.break_count; i++)
			{
				xir_patch_jump(c->chunk, loop.break_jumps[i]);
			}
			c->current_loop = loop.enclosing;
			break;
		}

	case AST_STMT_DO_WHILE:
		{
			XIrLoop loop;
			loop.loop_start = c->chunk->count;
			loop.break_count = 0;
			loop.enclosing = c->current_loop;
			c->current_loop = &loop;

			compile_stmt_node(c, stmt->as.do_while_stmt.body);

			compile_expr_node(c, stmt->as.do_while_stmt.condition);
			int repeat_jump = xir_emit_jump(c->chunk, OP_JUMP_IF_TRUE, line);
			/* if not true, fall through */
			xir_emit_op(c->chunk, OP_POP, line);

			/* Patch repeat jump back to loop_start */
			uint16_t target = (uint16_t)loop.loop_start;
			c->chunk->code[repeat_jump] = (uint8_t)((target >> 8) & 0xff);
			c->chunk->code[repeat_jump + 1] = (uint8_t)(target & 0xff);

			for (int i = 0; i < loop.break_count; i++)
			{
				xir_patch_jump(c->chunk, loop.break_jumps[i]);
			}
			c->current_loop = loop.enclosing;
			break;
		}

	case AST_STMT_FOR_C:
		{
			enter_scope(c);
			if (stmt->as.for_c.init)
			{
				compile_stmt_node(c, stmt->as.for_c.init);
			}

			XIrLoop loop;
			loop.loop_start = c->chunk->count;
			loop.break_count = 0;
			loop.enclosing = c->current_loop;
			c->current_loop = &loop;

			int exit_jump = -1;
			if (stmt->as.for_c.condition)
			{
				compile_expr_node(c, stmt->as.for_c.condition);
				exit_jump = xir_emit_jump(c->chunk, OP_JUMP_IF_FALSE, line);
				xir_emit_op(c->chunk, OP_POP, line);
			}

			compile_stmt_node(c, stmt->as.for_c.body);

			if (stmt->as.for_c.step)
			{
				compile_expr_node(c, stmt->as.for_c.step);
				xir_emit_op(c->chunk, OP_POP, line);
			}

			xir_emit_loop(c->chunk, loop.loop_start, line);
			if (exit_jump != -1)
			{
				xir_patch_jump(c->chunk, exit_jump);
				xir_emit_op(c->chunk, OP_POP, line);
			}

			for (int i = 0; i < loop.break_count; i++)
			{
				xir_patch_jump(c->chunk, loop.break_jumps[i]);
			}
			c->current_loop = loop.enclosing;
			exit_scope(c, line);
			break;
		}

	case AST_STMT_FOR_IN:
		{
			/* Desugar for-in: evaluate collection, iterate via helper */
			enter_scope(c);
			compile_expr_node(c, stmt->as.for_in.collection);
			/* We have collection on stack */
			add_local(c, "__coll__");
			int coll_slot = resolve_local(c, "__coll__");

			/* index counter = 0 */
			xir_emit_op(c->chunk, OP_CONST_INT, line);
			xir_emit_int(c->chunk, 0, line);
			add_local(c, "__idx__");
			int idx_slot = resolve_local(c, "__idx__");

			/* loop start */
			XIrLoop loop;
			loop.loop_start = c->chunk->count;
			loop.break_count = 0;
			loop.enclosing = c->current_loop;
			c->current_loop = &loop;

			/* condition: __idx__ < __coll__.size() */
			xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)coll_slot, line);
			int s_size = xir_add_symbol(c->chunk, "size");
			xir_emit_op(c->chunk, OP_CALL_METHOD, line);
			xir_emit_short(c->chunk, (uint16_t)s_size, line);
			xir_emit_byte(c->chunk, 0, line); /* 0 args */

			xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)idx_slot, line);

			xir_emit_op(c->chunk, OP_LT, line);
			int exit_jump = xir_emit_jump(c->chunk, OP_JUMP_IF_FALSE, line);
			xir_emit_op(c->chunk, OP_POP, line);

			/* item_var = __coll__.get(__idx__) */
			xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)coll_slot, line);
			xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)idx_slot, line);
			int s_get = xir_add_symbol(c->chunk, "get");
			xir_emit_op(c->chunk, OP_CALL_METHOD, line);
			xir_emit_short(c->chunk, (uint16_t)s_get, line);
			xir_emit_byte(c->chunk, 1, line); /* 1 arg */

			add_local(c, stmt->as.for_in.item_var);

			/* body */
			compile_stmt_node(c, stmt->as.for_in.body);

			/* __idx__++ */
			xir_emit_op(c->chunk, OP_LOAD_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)idx_slot, line);
			xir_emit_op(c->chunk, OP_CONST_INT, line);
			xir_emit_int(c->chunk, 1, line);
			xir_emit_op(c->chunk, OP_ADD, line);
			xir_emit_op(c->chunk, OP_STORE_LOCAL, line);
			xir_emit_short(c->chunk, (uint16_t)idx_slot, line);
			xir_emit_op(c->chunk, OP_POP, line);

			xir_emit_loop(c->chunk, loop.loop_start, line);
			xir_patch_jump(c->chunk, exit_jump);
			xir_emit_op(c->chunk, OP_POP, line);

			for (int i = 0; i < loop.break_count; i++)
			{
				xir_patch_jump(c->chunk, loop.break_jumps[i]);
			}
			c->current_loop = loop.enclosing;
			exit_scope(c, line);
			break;
		}

	case AST_STMT_RETURN:
		if (stmt->as.return_expr)
		{
			compile_expr_node(c, stmt->as.return_expr);
		}
		else
		{
			xir_emit_op(c->chunk, OP_CONST_NULL, line);
		}
		xir_emit_op(c->chunk, OP_RETURN, line);
		break;

	case AST_STMT_BREAK:
		if (c->current_loop && c->current_loop->break_count < 64)
		{
			int b_jump = xir_emit_jump(c->chunk, OP_JUMP, line);
			c->current_loop->break_jumps[c->current_loop->break_count++] = b_jump;
		}
		break;

	case AST_STMT_CONTINUE:
		if (c->current_loop)
		{
			xir_emit_loop(c->chunk, c->current_loop->loop_start, line);
		}
		break;

	case AST_STMT_FUNC_DECL:
		{
			const char* fname = stmt->as.func_decl.name ? stmt->as.func_decl.name : "fn";
			char full_name[256];
			if (stmt->as.func_decl.class_name && strlen(stmt->as.func_decl.class_name) > 0)
			{
				snprintf(full_name, sizeof(full_name), "%s.%s", stmt->as.func_decl.class_name, fname);
			}
			else
			{
				snprintf(full_name, sizeof(full_name), "%s", fname);
			}

			XFunction* fn = xfunc_create(full_name, stmt->as.func_decl.param_count);
			XCompiler fn_compiler;
			compiler_init(&fn_compiler, &fn->chunk);
			fn_compiler.enclosing = c;
			fn_compiler.function = fn;

			if (c->scope_depth > 0)
			{
				add_local(c, fname);
			}

			/* Parameters are initial local variables of the function (slots 0..param_count-1) */
			for (int p = 0; p < stmt->as.func_decl.param_count; p++)
			{
				add_local(&fn_compiler, stmt->as.func_decl.params[p].name);
			}

			if (stmt->as.func_decl.body)
			{
				compile_stmt_node(&fn_compiler, stmt->as.func_decl.body);
			}

			/* Implicit return null and halt */
			xir_emit_op(&fn->chunk, OP_CONST_NULL, line);
			xir_emit_op(&fn->chunk, OP_RETURN, line);

			fn->upvalue_count = fn_compiler.upvalue_count;

			/* Free local names in fn_compiler */
			for (int l = 0; l < fn_compiler.local_count; l++)
			{
				if (fn_compiler.locals[l].name) free(fn_compiler.locals[l].name);
			}

			/* Add function to enclosing chunk's constant pool */
			int c_idx = xir_add_constant(c->chunk, xval_func(fn));

			/* Emit OP_CLOSURE and upvalue descriptors */
			xir_emit_op(c->chunk, OP_CLOSURE, line);
			xir_emit_short(c->chunk, (uint16_t)c_idx, line);
			for (int u = 0; u < fn->upvalue_count; u++)
			{
				xir_emit_byte(c->chunk, fn_compiler.upvalues[u].is_local ? 1 : 0, line);
				xir_emit_byte(c->chunk, fn_compiler.upvalues[u].index, line);
			}

			/* Store in local or global */
			if (c->scope_depth == 0)
			{
				int s_idx = xir_add_symbol(c->chunk, full_name);
				xir_emit_op(c->chunk, OP_STORE_GLOBAL, line);
				xir_emit_short(c->chunk, (uint16_t)s_idx, line);
			}
			/* If local (c->scope_depth > 0), the closure value stays on the stack in its local slot */
			break;
		}

	case AST_STMT_CLASS_DECL:
		/* Class decls compile their member methods */
		for (int i = 0; i < stmt->as.class_decl.member_count; i++)
		{
			AstStmt* m = stmt->as.class_decl.members[i];
			if (m && m->type == AST_STMT_FUNC_DECL)
			{
				if (!m->as.func_decl.class_name)
				{
					m->as.func_decl.class_name = stmt->as.class_decl.name;
				}
				compile_stmt_node(c, m);
			}
		}
		break;

	default:
		break;
	}
}

/* -------------------------------------------------------------------------
 * Public Entry Points
 * ------------------------------------------------------------------------- */
bool xir_compile_program(const AstProgram* prog, XIrChunk* out_chunk)
{
	if (!prog || !out_chunk) return false;

	XCompiler compiler;
	compiler_init(&compiler, out_chunk);

	for (int i = 0; i < prog->statement_count; i++)
	{
		compile_stmt_node(&compiler, prog->statements[i]);
	}

	xir_emit_op(out_chunk, OP_HALT, prog->statement_count > 0 ? prog->statements[prog->statement_count - 1]->line : 1);
	return true;
}

bool xir_compile_stmt(const AstStmt* stmt, XIrChunk* out_chunk)
{
	if (!stmt || !out_chunk) return false;

	XCompiler compiler;
	compiler_init(&compiler, out_chunk);
	compile_stmt_node(&compiler, stmt);
	xir_emit_op(out_chunk, OP_HALT, stmt->line);
	return true;
}

bool xir_compile_expr(const AstExpr* expr, XIrChunk* out_chunk)
{
	if (!expr || !out_chunk) return false;

	XCompiler compiler;
	compiler_init(&compiler, out_chunk);
	compile_expr_node(&compiler, expr);
	xir_emit_op(out_chunk, OP_HALT, expr->line);
	return true;
}
