#include "xast_parser.h"
#include "functions.h"
#include "parse.h"
#include "lexer.h"
#include "xdiag.h"

/* Forward declarations */
static AstExpr* parse_expr(AstArena* arena, node** n, node* stop);
static AstExpr* parse_expr_prec(AstArena* arena, node** n, node* stop, int min_prec);
static AstStmt* parse_statement(AstArena* arena, node** n, node* stop);
static AstStmt* parse_block(AstArena* arena, node** n, node* stop);

static inline node* node_advance(node* curr)
{
	if (curr == NULL) return NULL;
	if (curr->next != NULL) return curr->next;
	return curr->stack_next;
}

static bool is_end(node* n, node* stop)
{
	if (n == NULL) return true;
	if (stop != NULL && (n == stop || (n->parent != NULL && n->parent == stop)))
		return true;
	return false;
}

static void skip_endl(node** n, node* stop)
{
	while (*n != NULL && !is_end(*n, stop) && ((*n)->type_ & endl))
	{
		*n = node_advance(*n);
	}
}

static AstBinaryOp get_binop(node* n, int* out_tokens)
{
	if (out_tokens) *out_tokens = 1;
	if (n == NULL) return BINOP_NONE;

	node* next = node_advance(n);

	/* Check two-token operators */
	if (n->type_ == equles && next != NULL && next->type_ == equles)
	{
		if (out_tokens) *out_tokens = 2;
		return BINOP_EQ;
	}

	if (n->type_ == operators_n && n->value_char_ptr != NULL)
	{
		char c = *n->value_char_ptr;
		if (next != NULL && next->type_ == equles)
		{
			if (c == '!') { if (out_tokens) *out_tokens = 2; return BINOP_NEQ; }
			if (c == '<') { if (out_tokens) *out_tokens = 2; return BINOP_LTE; }
			if (c == '>') { if (out_tokens) *out_tokens = 2; return BINOP_GTE; }
			return BINOP_NONE; /* +=, -=, *=, /=, %= are assignments */
		}

		if (next != NULL && next->type_ == operators_n && next->value_char_ptr != NULL)
		{
			char c2 = *next->value_char_ptr;
			if (c == '&' && c2 == '&') { if (out_tokens) *out_tokens = 2; return BINOP_AND; }
			if (c == '|' && c2 == '|') { if (out_tokens) *out_tokens = 2; return BINOP_OR; }
			if (c == '<' && c2 == '<') { if (out_tokens) *out_tokens = 2; return BINOP_SHL; }
			if (c == '>' && c2 == '>') { if (out_tokens) *out_tokens = 2; return BINOP_SHR; }
		}

		return ast_binop_from_string(n->value_char_ptr);
	}

	return BINOP_NONE;
}

static int get_precedence(AstBinaryOp op)
{
	switch (op)
	{
	case BINOP_OR:      return 1;
	case BINOP_AND:     return 2;
	case BINOP_BIT_OR:  return 3;
	case BINOP_BIT_XOR: return 4;
	case BINOP_BIT_AND: return 5;
	case BINOP_EQ:
	case BINOP_NEQ:     return 6;
	case BINOP_LT:
	case BINOP_LTE:
	case BINOP_GT:
	case BINOP_GTE:     return 7;
	case BINOP_SHL:
	case BINOP_SHR:     return 8;
	case BINOP_ADD:
	case BINOP_SUB:     return 9;
	case BINOP_MUL:
	case BINOP_DIV:
	case BINOP_MOD:     return 10;
	default:            return 0;
	}
}

static AstExpr* parse_primary(AstArena* arena, node** n, node* stop)
{
	if (is_end(*n, stop)) return NULL;

	node* curr = *n;
	int line = curr->line;
	int col = curr->col;

	/* Literals */
	if (curr->type_ == value)
	{
		*n = curr->next;
		if (curr->opt_type_ptr == T_INT)
		{
			int64_t v = curr->value_char_ptr ? atoll(curr->value_char_ptr) : 0;
			return ast_expr_literal_int(arena, v, line, col);
		}
		if (curr->opt_type_ptr == T_FLOAT)
		{
			double v = curr->value_char_ptr ? atof(curr->value_char_ptr) : 0.0;
			return ast_expr_literal_float(arena, v, line, col);
		}
		if (curr->opt_type_ptr == T_STRING)
		{
			return ast_expr_literal_string(arena, curr->value_char_ptr ? curr->value_char_ptr : "", line, col);
		}
		if (curr->opt_type_ptr == T_BOOL)
		{
			bool b = (curr->value_char_ptr && strcmp(curr->value_char_ptr, "true") == 0);
			return ast_expr_literal_bool(arena, b, line, col);
		}
		/* Fallback: guess from string */
		if (curr->value_char_ptr != NULL)
		{
			if (strcmp(curr->value_char_ptr, "null") == 0)
				return ast_expr_literal_null(arena, line, col);
			if (strchr(curr->value_char_ptr, '.'))
				return ast_expr_literal_float(arena, atof(curr->value_char_ptr), line, col);
			return ast_expr_literal_int(arena, atoll(curr->value_char_ptr), line, col);
		}
		return ast_expr_literal_null(arena, line, col);
	}

	/* List Literal: [elem1, elem2, ...] */
	if (curr->type_ == s_index)
	{
		node* close = get_close_part(curr);
		*n = curr->next;

		AstExpr* elems[64];
		int elem_count = 0;

		while (!is_end(*n, close) && *n != close)
		{
			if ((*n)->type_ == comma)
			{
				*n = (*n)->next;
				continue;
			}
			AstExpr* elem = parse_expr_prec(arena, n, close, 1);
			if (elem && elem_count < 64)
			{
				elems[elem_count++] = elem;
			}
			if (!is_end(*n, close) && (*n)->type_ == comma)
			{
				*n = (*n)->next;
			}
		}
		if (*n == close) *n = (*n)->next;
		return ast_expr_list(arena, elems, elem_count, line, col);
	}

	/* Parenthesized Group: (expr) */
	if (curr->type_ == parentheses4)
	{
		node* close = get_close_part(curr);
		*n = curr->next;
		AstExpr* sub = parse_expr(arena, n, close);
		if (*n == close) *n = (*n)->next;
		return sub;
	}

	/* Unary Operator: -expr or !expr */
	if (curr->type_ == operators_n && curr->value_char_ptr != NULL)
	{
		if (strcmp(curr->value_char_ptr, "-") == 0)
		{
			*n = curr->next;
			AstExpr* opnd = parse_primary(arena, n, stop);
			return ast_expr_unary(arena, UNOP_NEG, opnd, line, col);
		}
		if (strcmp(curr->value_char_ptr, "!") == 0)
		{
			*n = curr->next;
			AstExpr* opnd = parse_primary(arena, n, stop);
			return ast_expr_unary(arena, UNOP_NOT, opnd, line, col);
		}
	}

	/* Variable or Function Call */
	if (curr->type_ == var_name || curr->type_ == itype)
	{
		char* name = curr->value_char_ptr;
		if (curr->type_ == itype && curr->value_type != NULL && curr->value_type->type_name != NULL)
			name = curr->value_type->type_name;

		*n = curr->next;

		/* Function Call: name(args...) */
		if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
		{
			node* pclose = get_close_part(*n);
			*n = (*n)->next;

			AstExpr* args[32];
			int arg_count = 0;

			while (!is_end(*n, pclose) && *n != pclose)
			{
				if ((*n)->type_ == comma)
				{
					*n = (*n)->next;
					continue;
				}
				AstExpr* arg = parse_expr_prec(arena, n, pclose, 1);
				if (arg && arg_count < 32)
				{
					args[arg_count++] = arg;
				}
				if (!is_end(*n, pclose) && (*n)->type_ == comma)
				{
					*n = (*n)->next;
				}
			}
			if (*n == pclose) *n = (*n)->next;
			return ast_expr_call(arena, name ? name : "anon", args, arg_count, line, col);
		}

		/* Simple Identifier */
		return ast_expr_identifier(arena, name ? name : "unknown", line, col);
	}

	*n = curr->next;
	return NULL;
}

static AstExpr* parse_postfix(AstArena* arena, node** n, node* stop)
{
	AstExpr* left = parse_primary(arena, n, stop);
	if (!left) return NULL;

	while (!is_end(*n, stop))
	{
		/* Member access: .member or .method(args) */
		if ((*n)->type_ == dot)
		{
			*n = (*n)->next;
			if (is_end(*n, stop)) break;

			node* mem_node = *n;
			char* mem_name = mem_node->value_char_ptr;
			if (!mem_name && mem_node->value_type)
				mem_name = mem_node->value_type->type_name;
			*n = mem_node->next;

			/* Check if followed by (args) -> Method Call */
			if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
			{
				node* pclose = get_close_part(*n);
				*n = (*n)->next;

				AstExpr* args[32];
				int arg_count = 0;
				while (!is_end(*n, pclose) && *n != pclose)
				{
					if ((*n)->type_ == comma)
					{
						*n = (*n)->next;
						continue;
					}
					AstExpr* arg = parse_expr_prec(arena, n, pclose, 1);
					if (arg && arg_count < 32)
					{
						args[arg_count++] = arg;
					}
					if (!is_end(*n, pclose) && (*n)->type_ == comma)
					{
						*n = (*n)->next;
					}
				}
				if (*n == pclose) *n = (*n)->next;
				left = ast_expr_method_call(arena, left, mem_name ? mem_name : "", args, arg_count, mem_node->line, mem_node->col);
			}
			else
			{
				left = ast_expr_member(arena, left, mem_name ? mem_name : "", mem_node->line, mem_node->col);
			}
			continue;
		}

		/* Indexing: [idx] */
		if ((*n)->type_ == s_index)
		{
			node* iclose = get_close_part(*n);
			int line = (*n)->line;
			int col = (*n)->col;
			*n = (*n)->next;

			AstExpr* idx = parse_expr(arena, n, iclose);
			if (*n == iclose) *n = (*n)->next;
			left = ast_expr_index(arena, left, idx, line, col);
			continue;
		}

		break;
	}

	return left;
}

static AstExpr* parse_expr_prec(AstArena* arena, node** n, node* stop, int min_prec)
{
	AstExpr* left = parse_postfix(arena, n, stop);
	if (!left) return NULL;

	while (!is_end(*n, stop))
	{
		int token_count = 1;
		AstBinaryOp op = get_binop(*n, &token_count);
		if (op == BINOP_NONE) break;

		int prec = get_precedence(op);
		if (prec < min_prec) break;

		node* op_node = *n;
		for (int t = 0; t < token_count; t++)
		{
			*n = node_advance(*n);
		}

		AstExpr* right = parse_expr_prec(arena, n, stop, prec + 1);
		if (!right) break;

		left = ast_expr_binary(arena, op, left, right, op_node->line, op_node->col);
	}

	return left;
}

static AstExpr* parse_expr(AstArena* arena, node** n, node* stop)
{
	return parse_expr_prec(arena, n, stop, 1);
}

/* -------------------------------------------------------------------------
 * Statement Parser
 * ------------------------------------------------------------------------- */
static AstStmt* parse_block(AstArena* arena, node** n, node* stop)
{
	if (is_end(*n, stop)) return NULL;

	node* b_open = *n;
	if (b_open->type_ != parentheses1)
	{
		/* Single statement block */
		AstStmt* s = parse_statement(arena, n, stop);
		if (!s) return NULL;
		AstStmt* stmts[1] = { s };
		return ast_stmt_block(arena, stmts, 1, s->line, s->col);
	}

	node* b_close = get_close_part(b_open);
	int line = b_open->line;
	int col = b_open->col;
	*n = b_open->next;

	AstStmt* stmt_buf[128];
	int count = 0;

	while (!is_end(*n, b_close) && *n != b_close)
	{
		skip_endl(n, b_close);
		if (*n == b_close || is_end(*n, b_close)) break;

		AstStmt* s = parse_statement(arena, n, b_close);
		if (s && count < 128)
		{
			stmt_buf[count++] = s;
		}
		skip_endl(n, b_close);
	}

	if (*n == b_close) *n = (*n)->next;
	return ast_stmt_block(arena, stmt_buf, count, line, col);
}

static AstStmt* parse_statement(AstArena* arena, node** n, node* stop)
{
	skip_endl(n, stop);
	if (is_end(*n, stop)) return NULL;

	node* curr = *n;
	int line = curr->line;
	int col = curr->col;

	/* Block: { stmts... } */
	if (curr->type_ == parentheses1)
	{
		return parse_block(arena, n, stop);
	}

	/* Keywords */
	if (curr->type_ == keyword)
	{
		/* IF Statement */
		if (curr->value_keyword == _if_)
		{
			*n = curr->next;
			if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
			{
				node* pclose = get_close_part(*n);
				*n = (*n)->next;
				AstExpr* cond = parse_expr(arena, n, pclose);
				if (*n == pclose) *n = (*n)->next;

				skip_endl(n, stop);
				AstStmt* then_b = parse_block(arena, n, stop);

				skip_endl(n, stop);
				AstStmt* else_b = NULL;
				if (!is_end(*n, stop) && (*n)->type_ == keyword)
				{
					if ((*n)->value_keyword == _else_)
					{
						*n = (*n)->next;
						skip_endl(n, stop);
						else_b = parse_block(arena, n, stop);
					}
					else if ((*n)->value_keyword == _eif_)
					{
						/* eif / else if handled recursively */
						else_b = parse_statement(arena, n, stop);
					}
				}
				return ast_stmt_if(arena, cond, then_b, else_b, line, col);
			}
		}

		/* WHILE Statement */
		if (curr->value_keyword == _while_)
		{
			*n = curr->next;
			if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
			{
				node* pclose = get_close_part(*n);
				*n = (*n)->next;
				AstExpr* cond = parse_expr(arena, n, pclose);
				if (*n == pclose) *n = (*n)->next;

				skip_endl(n, stop);
				AstStmt* body = parse_block(arena, n, stop);
				return ast_stmt_while(arena, cond, body, line, col);
			}
		}

		/* DO-WHILE Statement */
		if (curr->value_keyword == _do_)
		{
			*n = curr->next;
			skip_endl(n, stop);
			AstStmt* body = parse_block(arena, n, stop);

			skip_endl(n, stop);
			AstExpr* cond = NULL;
			if (!is_end(*n, stop) && (*n)->type_ == keyword && (*n)->value_keyword == _while_)
			{
				*n = (*n)->next;
				if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
				{
					node* pclose = get_close_part(*n);
					*n = (*n)->next;
					cond = parse_expr(arena, n, pclose);
					if (*n == pclose) *n = (*n)->next;
				}
			}
			return ast_stmt_do_while(arena, body, cond, line, col);
		}

		/* FOR Statement (for-in or classic for) */
		if (curr->value_keyword == _for_)
		{
			*n = curr->next;
			/* Check if next token is for (item in coll) or for item in coll */
			bool has_paren = (!is_end(*n, stop) && (*n)->type_ == parentheses4);
			node* pclose = has_paren ? get_close_part(*n) : NULL;
			if (has_paren) *n = (*n)->next;

			/* Check if it has keyword _in_ inside */
			bool is_for_in = false;
			node* check = *n;
			while (!is_end(check, has_paren ? pclose : stop) && check != (has_paren ? pclose : NULL))
			{
				if (check->type_ == keyword && check->value_keyword == _in_)
				{
					is_for_in = true;
					break;
				}
				check = check->next;
			}

			if (is_for_in)
			{
				/* for [item] in [collection] */
				char* item_name = (*n)->value_char_ptr;
				*n = (*n)->next; /* skip item */

				if (!is_end(*n, stop) && (*n)->type_ == keyword && (*n)->value_keyword == _in_)
				{
					*n = (*n)->next; /* skip 'in' */
				}

				AstExpr* coll = parse_expr(arena, n, has_paren ? pclose : stop);
				if (has_paren && *n == pclose) *n = (*n)->next;

				skip_endl(n, stop);
				AstStmt* body = parse_block(arena, n, stop);
				return ast_stmt_for_in(arena, item_name ? item_name : "item", coll, body, line, col);
			}
			else
			{
				/* Classic for i (0, i+1, i<10) */
				char* var_name = (*n)->value_char_ptr;
				*n = (*n)->next; /* skip loop var name */

				node* f_close = NULL;
				if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
				{
					f_close = get_close_part(*n);
					*n = (*n)->next;
				}

				AstExpr* init_val = parse_expr_prec(arena, n, f_close, 1);
				if (!is_end(*n, f_close) && (*n)->type_ == comma) *n = (*n)->next;

				AstExpr* step_expr = parse_expr_prec(arena, n, f_close, 1);
				if (!is_end(*n, f_close) && (*n)->type_ == comma) *n = (*n)->next;

				AstExpr* cond_expr = parse_expr_prec(arena, n, f_close, 1);
				if (f_close && *n == f_close) *n = (*n)->next;

				skip_endl(n, stop);
				AstStmt* body = parse_block(arena, n, stop);

				AstExpr* v_target = ast_expr_identifier(arena, var_name ? var_name : "i", line, col);
				AstExpr* assign_init = ast_expr_assign(arena, v_target, "=", init_val, line, col);
				AstStmt* init_stmt = ast_stmt_expr(arena, assign_init, line, col);

				return ast_stmt_for_c(arena, init_stmt, cond_expr, step_expr, body, line, col);
			}
		}

		/* RETURN Statement */
		if (curr->value_keyword == _return_)
		{
			*n = curr->next;
			AstExpr* ret_expr = NULL;
			if (!is_end(*n, stop) && !((*n)->type_ & endl))
			{
				ret_expr = parse_expr(arena, n, stop);
			}
			skip_endl(n, stop);
			return ast_stmt_return(arena, ret_expr, line, col);
		}

		/* BREAK & CONTINUE */
		if (curr->value_keyword == _break_)
		{
			*n = curr->next;
			skip_endl(n, stop);
			return ast_stmt_break(arena, line, col);
		}

		/* CLASS Declaration */
		if (curr->value_keyword == _class_)
		{
			*n = curr->next;
			char* class_name = (*n)->value_char_ptr;
			*n = (*n)->next;

			char* base_name = NULL;
			if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
			{
				node* pclose = get_close_part(*n);
				*n = (*n)->next;
				if (!is_end(*n, pclose) && (*n)->type_ == var_name)
				{
					base_name = (*n)->value_char_ptr;
				}
				if (pclose) *n = pclose->next;
			}

			skip_endl(n, stop);
			AstStmt* members[64];
			int member_count = 0;

			if (!is_end(*n, stop) && (*n)->type_ == parentheses1)
			{
				node* c_close = get_close_part(*n);
				*n = (*n)->next;

				while (!is_end(*n, c_close) && *n != c_close)
				{
					skip_endl(n, c_close);
					if (*n == c_close || is_end(*n, c_close)) break;

					AstStmt* m = parse_statement(arena, n, c_close);
					if (m && member_count < 64)
					{
						members[member_count++] = m;
					}
					skip_endl(n, c_close);
				}
				if (*n == c_close) *n = (*n)->next;
			}

			return ast_stmt_class_decl(arena, class_name ? class_name : "AnonClass", base_name, members, member_count, line, col);
		}
	}

	/* Type Declaration (Variable or Function): int x = 10; or int add(...) { ... } */
	bool is_static = false;
	if (curr->type_ == keyword && curr->value_keyword == _static_)
	{
		is_static = true;
		*n = curr->next;
		curr = *n;
		if (is_end(curr, stop)) return NULL;
	}

	if (curr->type_ == itype)
	{
		char* type_name = curr->value_type ? curr->value_type->type_name : "object";
		*n = curr->next;

		if (!is_end(*n, stop) && ((*n)->type_ == var_name || (*n)->type_ == itype))
		{
			node* name_node = *n;
			char* decl_name = name_node->value_char_ptr;
			*n = name_node->next;

			/* Check if followed by '(' -> Function Declaration */
			if (!is_end(*n, stop) && (*n)->type_ == parentheses4)
			{
				node* pclose = get_close_part(*n);
				*n = (*n)->next;

				AstParam params[16];
				int param_count = 0;

				while (!is_end(*n, pclose) && *n != pclose)
				{
					if ((*n)->type_ == comma)
					{
						*n = (*n)->next;
						continue;
					}
					if ((*n)->type_ == itype)
					{
						char* ptype = (*n)->value_type ? (*n)->value_type->type_name : "object";
						*n = (*n)->next;
						char* pname = (!is_end(*n, pclose) && (*n)->type_ == var_name) ? (*n)->value_char_ptr : "arg";
						if (!is_end(*n, pclose) && (*n)->type_ == var_name) *n = (*n)->next;

						if (param_count < 16)
						{
							params[param_count].type_name = ptype;
							params[param_count].name = pname;
							param_count++;
						}
					}
					else
					{
						*n = (*n)->next;
					}
				}
				if (*n == pclose) *n = (*n)->next;

				skip_endl(n, stop);
				AstStmt* body = parse_block(arena, n, stop);
				return ast_stmt_func_decl(arena, decl_name ? decl_name : "fn", type_name, params, param_count, body, is_static, NULL, line, col);
			}

			/* Variable Declaration */
			AstExpr* init_expr = NULL;
			if (!is_end(*n, stop) && ((*n)->type_ == equles || (*n)->value_keyword == _new_))
			{
				*n = (*n)->next;
				init_expr = parse_expr(arena, n, stop);
			}
			skip_endl(n, stop);
			return ast_stmt_var_decl(arena, type_name, decl_name ? decl_name : "var", init_expr, is_static, line, col);
		}
	}

	/* Expression / Assignment Statement */
	AstExpr* expr = parse_expr(arena, n, stop);
	if (!expr)
	{
		if (!is_end(*n, stop)) *n = node_advance(*n);
		return NULL;
	}

	/* Check if followed by '=' or compound assignment operator (+=, -=, *=, /=, %=) */
	if (!is_end(*n, stop))
	{
		char* op_str = NULL;
		node* next = node_advance(*n);

		if ((*n)->type_ == equles && (next == NULL || next->type_ != equles))
		{
			op_str = "=";
			*n = node_advance(*n);
		}
		else if ((*n)->type_ == operators_n && (*n)->value_char_ptr != NULL && next != NULL && next->type_ == equles)
		{
			char c = *(*n)->value_char_ptr;
			if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%')
			{
				char op_buf[4] = { c, '=', '\0' };
				op_str = ast_arena_strdup(arena, op_buf);
				*n = node_advance(next);
			}
		}

		if (op_str != NULL)
		{
			AstExpr* val_expr = parse_expr(arena, n, stop);
			expr = ast_expr_assign(arena, expr, op_str, val_expr, line, col);
		}
	}

	skip_endl(n, stop);
	return ast_stmt_expr(arena, expr, line, col);
}

/* -------------------------------------------------------------------------
 * Public Entry Points
 * ------------------------------------------------------------------------- */
AstProgram* xast_parse_node_stream(AstArena* arena, node* start_node, node* end_node)
{
	if (!arena || !start_node) return NULL;

	AstProgram* prog = ast_program_create(arena);
	node* curr = start_node;

	while (!is_end(curr, end_node))
	{
		skip_endl(&curr, end_node);
		if (is_end(curr, end_node)) break;

		AstStmt* stmt = parse_statement(arena, &curr, end_node);
		if (stmt)
		{
			ast_program_add_stmt(prog, stmt);
		}
		skip_endl(&curr, end_node);
	}

	return prog;
}

AstProgram* xast_parse_source(const char* source_code, const char* filename)
{
	if (!source_code) return NULL;

	AstArena* arena = ast_arena_create(64 * 1024);
	if (!arena) return NULL;

	xdiag_set_current_file(filename ? filename : "<source>");
	xdiag_set_source_code(source_code);

	/* Use xlang parser to tokenize into node stream */
	char* dup_src = strdup(source_code);
	start_parse_lines(dup_src, false);
	free(dup_src);

	extern node_stack* nodes;
	node* start_node = (nodes != NULL) ? nodes->root : NULL;

	AstProgram* prog = xast_parse_node_stream(arena, start_node, NULL);
	return prog;
}

AstProgram* xast_parse_file(const char* filepath)
{
	FILE* f = fopen(filepath, "r");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* buf = (char*)malloc(len + 1);
	if (!buf)
	{
		fclose(f);
		return NULL;
	}

	size_t read_bytes = fread(buf, 1, len, f);
	buf[read_bytes] = '\0';
	fclose(f);

	AstProgram* prog = xast_parse_source(buf, filepath);
	free(buf);
	return prog;
}
