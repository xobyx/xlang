#include "xast_parser.h"
#include "xdiag.h"
#include "ximport.h"
#include "functions.h"
#include "xffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static AstExpr* parse_expr(AstArena* arena, token_stream_t* s);
static AstExpr* parse_expr_prec(AstArena* arena, token_stream_t* s, int min_prec);
static AstStmt* parse_statement(AstArena* arena, token_stream_t* s);
static AstStmt* parse_block(AstArena* arena, token_stream_t* s);

static AstProgram* s_current_program = NULL;
static int s_class_depth = 0;
static const char* s_current_class_name = NULL;

/* -------------------------------------------------------------------------
 * Registered Classes Registry (for E0010, E0011, E0012 diagnostics)
 * ------------------------------------------------------------------------- */
static char** s_registered_classes = NULL;
static int s_registered_class_count = 0;
static int s_registered_class_cap = 0;

static void register_class_name(const char* name)
{
	if (name == NULL || *name == '\0') return;
	for (int i = 0; i < s_registered_class_count; i++)
	{
		if (strcmp(s_registered_classes[i], name) == 0) return;
	}
	if (s_registered_class_count >= s_registered_class_cap)
	{
		s_registered_class_cap = s_registered_class_cap == 0 ? 32 : s_registered_class_cap * 2;
		s_registered_classes = (char**)realloc(s_registered_classes, s_registered_class_cap * sizeof(char*));
	}
	s_registered_classes[s_registered_class_count++] = strdup(name);
	if (get_type_by_name((char*)name) == NULL)
	{
		type_def* td = new_type();
		td->type_name = strdup(name);
	}
}

static bool is_primitive_type_name(const char* name)
{
	if (!name) return false;
	return (strcmp(name, "int") == 0 ||
	        strcmp(name, "float") == 0 ||
	        strcmp(name, "string") == 0 ||
	        strcmp(name, "bool") == 0 ||
	        strcmp(name, "char") == 0 ||
	        strcmp(name, "long") == 0 ||
	        strcmp(name, "void") == 0 ||
	        strcmp(name, "double") == 0);
}

static bool is_known_class_name(const char* name)
{
	if (!name) return false;
	if (is_primitive_type_name(name)) return false;

	for (int i = 0; i < s_registered_class_count; i++)
	{
		if (strcmp(s_registered_classes[i], name) == 0) return true;
	}

	type_def* t = get_type_by_name((char*)name);
	if (t != NULL && !is_base_type(t)) return true;

	return false;
}

/* -------------------------------------------------------------------------
 * Operator Precedence
 * ------------------------------------------------------------------------- */
static AstBinaryOp get_binop(token_type_t tt)
{
	switch (tt)
	{
		case TOK_PLUS: return BINOP_ADD;
		case TOK_MINUS: return BINOP_SUB;
		case TOK_STAR: return BINOP_MUL;
		case TOK_SLASH: return BINOP_DIV;
		case TOK_PERCENT: return BINOP_MOD;
		case TOK_EQ: return BINOP_EQ;
		case TOK_NEQ: return BINOP_NEQ;
		case TOK_LT: return BINOP_LT;
		case TOK_LTE: return BINOP_LTE;
		case TOK_GT: return BINOP_GT;
		case TOK_GTE: return BINOP_GTE;
		case TOK_AND: return BINOP_AND;
		case TOK_OR: return BINOP_OR;
		case TOK_BIT_AND: return BINOP_BIT_AND;
		case TOK_BIT_OR: return BINOP_BIT_OR;
		case TOK_BIT_XOR: return BINOP_BIT_XOR;
		case TOK_SHL: return BINOP_SHL;
		case TOK_SHR: return BINOP_SHR;
		default: return BINOP_NONE;
	}
}

static int get_precedence(AstBinaryOp op)
{
	switch (op)
	{
		case BINOP_OR: return 2;
		case BINOP_AND: return 3;
		case BINOP_BIT_OR: return 4;
		case BINOP_BIT_XOR: return 5;
		case BINOP_BIT_AND: return 6;
		case BINOP_EQ:
		case BINOP_NEQ: return 7;
		case BINOP_LT:
		case BINOP_LTE:
		case BINOP_GT:
		case BINOP_GTE: return 8;
		case BINOP_SHL:
		case BINOP_SHR: return 9;
		case BINOP_ADD:
		case BINOP_SUB: return 10;
		case BINOP_MUL:
		case BINOP_DIV:
		case BINOP_MOD: return 11;
		default: return 0;
	}
}

/* -------------------------------------------------------------------------
 * Expression Parsing
 * ------------------------------------------------------------------------- */
static AstExpr* parse_primary(AstArena* arena, token_stream_t* s)
{
	token_stream_skip_newlines(s);
	token_t* tok = token_stream_current(s);
	if (tok == NULL || tok->type == TOK_EOF) return NULL;

	int line = tok->line;
	int col = tok->col;

	/* Integer Literal */
	if (tok->type == TOK_INT)
	{
		token_stream_advance(s);
		return ast_expr_literal_int(arena, tok->int_val, line, col);
	}

	/* Float Literal */
	if (tok->type == TOK_FLOAT)
	{
		token_stream_advance(s);
		return ast_expr_literal_float(arena, tok->float_val, line, col);
	}

	/* String Literal */
	if (tok->type == TOK_STRING)
	{
		token_stream_advance(s);
		return ast_expr_literal_string(arena, tok->text ? tok->text : "", line, col);
	}

	/* Char Literal */
	if (tok->type == TOK_CHAR)
	{
		token_stream_advance(s);
		return ast_expr_literal_int(arena, tok->int_val, line, col);
	}

	/* Boolean Literal */
	if (tok->type == TOK_BOOL)
	{
		token_stream_advance(s);
		return ast_expr_literal_bool(arena, tok->int_val != 0, line, col);
	}

	/* Null Literal */
	if (tok->type == TOK_NULL)
	{
		token_stream_advance(s);
		return ast_expr_literal_null(arena, line, col);
	}

	/* List Literal: [elem1, elem2, ...] */
	if (tok->type == TOK_LBRACKET)
	{
		token_stream_advance(s); /* consume '[' */
		AstExpr* elems[128];
		int elem_count = 0;

		while (!token_stream_check(s, TOK_RBRACKET) && !token_stream_check(s, TOK_EOF))
		{
			token_stream_skip_newlines(s);
			if (token_stream_check(s, TOK_RBRACKET)) break;

			AstExpr* elem = parse_expr_prec(arena, s, 1);
			if (elem && elem_count < 128)
			{
				elems[elem_count++] = elem;
			}
			token_stream_skip_newlines(s);
			if (token_stream_check(s, TOK_COMMA))
			{
				token_stream_advance(s);
			}
			else
			{
				break;
			}
		}
		token_stream_skip_newlines(s);
		token_stream_match(s, TOK_RBRACKET);
		return ast_expr_list(arena, elems, elem_count, line, col);
	}

	/* Parenthesized Group: (expr) */
	if (tok->type == TOK_LPAREN)
	{
		token_stream_advance(s); /* consume '(' */
		token_stream_skip_newlines(s);
		AstExpr* sub = parse_expr(arena, s);
		token_stream_skip_newlines(s);
		token_stream_match(s, TOK_RPAREN);
		return sub;
	}

	/* Unary - */
	if (tok->type == TOK_MINUS)
	{
		token_stream_advance(s);
		AstExpr* opnd = parse_primary(arena, s);
		return ast_expr_unary(arena, UNOP_NEG, opnd, line, col);
	}

	/* Unary ! */
	if (tok->type == TOK_NOT)
	{
		token_stream_advance(s);
		AstExpr* opnd = parse_primary(arena, s);
		return ast_expr_unary(arena, UNOP_NOT, opnd, line, col);
	}

	/* Unary ~ */
	if (tok->type == TOK_BIT_NOT)
	{
		token_stream_advance(s);
		AstExpr* opnd = parse_primary(arena, s);
		return ast_expr_unary(arena, UNOP_BIT_NOT, opnd, line, col);
	}

	/* 'new' Expression: new ClassName(args...) */
	if (tok->type == TOK_KW_NEW)
	{
		token_stream_advance(s); /* consume 'new' */
		token_stream_skip_newlines(s);
		token_t* ctok = token_stream_current(s);
		char* class_name = ctok ? ctok->text : "Object";
		token_stream_advance(s); /* consume class name */

		AstExpr* args[32];
		int arg_count = 0;

		token_stream_skip_newlines(s);
		if (token_stream_match(s, TOK_LPAREN))
		{
			while (!token_stream_check(s, TOK_RPAREN) && !token_stream_check(s, TOK_EOF))
			{
				token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_RPAREN)) break;

				AstExpr* arg = parse_expr_prec(arena, s, 1);
				if (arg && arg_count < 32)
				{
					args[arg_count++] = arg;
				}
				token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_COMMA))
				{
					token_stream_advance(s);
				}
				else
				{
					break;
				}
			}
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);
		}

		return ast_expr_new(arena, class_name, args, arg_count, line, col);
	}

	/* Identifier or Function Call */
	if (tok->type == TOK_IDENT || tok->type == TOK_TYPE_INT || tok->type == TOK_TYPE_FLOAT ||
	    tok->type == TOK_TYPE_STRING || tok->type == TOK_TYPE_BOOL || tok->type == TOK_TYPE_CHAR ||
	    tok->type == TOK_TYPE_VOID || tok->type == TOK_TYPE_LONG || tok->type == TOK_TYPE_DOUBLE)
	{
		char* name = tok->text;
		token_stream_advance(s); /* consume identifier */

		/* Function Call: name(args...) */
		if (token_stream_check(s, TOK_LPAREN))
		{
			token_stream_advance(s); /* consume '(' */

			AstExpr* args[32];
			int arg_count = 0;

			while (!token_stream_check(s, TOK_RPAREN) && !token_stream_check(s, TOK_EOF))
			{
				token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_RPAREN)) break;

				AstExpr* arg = parse_expr_prec(arena, s, 1);
				if (arg && arg_count < 32)
				{
					args[arg_count++] = arg;
				}
				token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_COMMA))
				{
					token_stream_advance(s);
				}
				else
				{
					break;
				}
			}
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);

			/* Disallowed: Instantiation without 'new' (E0012) */
			if (is_known_class_name(name))
			{
				xdiag_error("E0012", NULL, line, col, 0, NULL,
				            "cannot instantiate class '%s' without 'new'; use 'new %s(...)'",
				            name, name);
				return NULL;
			}

			return ast_expr_call(arena, name ? name : "anon", args, arg_count, line, col);
		}

		return ast_expr_identifier(arena, name ? name : "unknown", line, col);
	}

	return NULL;
}

static AstExpr* parse_postfix(AstArena* arena, token_stream_t* s)
{
	AstExpr* left = parse_primary(arena, s);
	if (!left) return NULL;

	while (true)
	{
		/* Member Access or Method Call: .member or .method(args...) */
		if (token_stream_check(s, TOK_DOT))
		{
			token_stream_advance(s); /* consume '.' */
			token_t* mtok = token_stream_current(s);
			if (mtok == NULL || (mtok->type != TOK_IDENT && mtok->type < TOK_TYPE_INT))
			{
				break;
			}
			char* mem_name = mtok->text;
			int mline = mtok->line;
			int mcol = mtok->col;
			token_stream_advance(s); /* consume member identifier */

			if (token_stream_check(s, TOK_LPAREN))
			{
				token_stream_advance(s); /* consume '(' */
				AstExpr* args[32];
				int arg_count = 0;

				while (!token_stream_check(s, TOK_RPAREN) && !token_stream_check(s, TOK_EOF))
				{
					token_stream_skip_newlines(s);
					if (token_stream_check(s, TOK_RPAREN)) break;

					AstExpr* arg = parse_expr_prec(arena, s, 1);
					if (arg && arg_count < 32)
					{
						args[arg_count++] = arg;
					}
					token_stream_skip_newlines(s);
					if (token_stream_check(s, TOK_COMMA))
					{
						token_stream_advance(s);
					}
					else
					{
						break;
					}
				}
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);

				left = ast_expr_method_call(arena, left, mem_name ? mem_name : "", args, arg_count, mline, mcol);
			}
			else
			{
				left = ast_expr_member(arena, left, mem_name ? mem_name : "", mline, mcol);
			}
			continue;
		}

		/* Indexing: [idx] */
		if (token_stream_check(s, TOK_LBRACKET))
		{
			int iline = token_stream_current(s)->line;
			int icol = token_stream_current(s)->col;
			token_stream_advance(s); /* consume '[' */

			token_stream_skip_newlines(s);
			AstExpr* idx = parse_expr(arena, s);
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RBRACKET);

			left = ast_expr_index(arena, left, idx, iline, icol);
			continue;
		}

		/* Postfix ++ */
		if (token_stream_check(s, TOK_INC))
		{
			token_t* itok = token_stream_current(s);
			int iline = itok->line;
			int icol = itok->col;
			token_stream_advance(s);
			AstExpr* one_lit = ast_expr_literal_int(arena, 1, iline, icol);
			left = ast_expr_assign(arena, left, "+=", one_lit, iline, icol);
			continue;
		}

		/* Postfix -- */
		if (token_stream_check(s, TOK_DEC))
		{
			token_t* dtok = token_stream_current(s);
			int dline = dtok->line;
			int dcol = dtok->col;
			token_stream_advance(s);
			AstExpr* one_lit = ast_expr_literal_int(arena, 1, dline, dcol);
			left = ast_expr_assign(arena, left, "-=", one_lit, dline, dcol);
			continue;
		}

		break;
	}

	return left;
}

static AstExpr* parse_expr_prec(AstArena* arena, token_stream_t* s, int min_prec)
{
	AstExpr* left = parse_postfix(arena, s);
	if (!left) return NULL;

	while (true)
	{
		token_t* tok = token_stream_current(s);
		if (tok == NULL || tok->type == TOK_EOF) break;

		AstBinaryOp op = get_binop(tok->type);
		if (op == BINOP_NONE) break;

		int prec = get_precedence(op);
		if (prec < min_prec) break;

		int oline = tok->line;
		int ocol = tok->col;
		token_stream_advance(s); /* consume binary operator */

		token_stream_skip_newlines(s);
		AstExpr* right = parse_expr_prec(arena, s, prec + 1);
		if (!right) break;

		left = ast_expr_binary(arena, op, left, right, oline, ocol);
	}

	return left;
}

static AstExpr* parse_expr(AstArena* arena, token_stream_t* s)
{
	return parse_expr_prec(arena, s, 1);
}

/* -------------------------------------------------------------------------
 * Statement Parser
 * ------------------------------------------------------------------------- */
static AstStmt* parse_block(AstArena* arena, token_stream_t* s)
{
	token_stream_skip_newlines(s);
	if (token_stream_check(s, TOK_EOF)) return NULL;

	if (!token_stream_check(s, TOK_LBRACE))
	{
		/* Single statement body */
		AstStmt* single = parse_statement(arena, s);
		if (!single) return NULL;
		AstStmt* stmts[1] = { single };
		return ast_stmt_block(arena, stmts, 1, single->line, single->col);
	}

	token_t* btok = token_stream_current(s);
	int line = btok->line;
	int col = btok->col;
	token_stream_advance(s); /* consume '{' */

	AstStmt* stmt_buf[128];
	int count = 0;

	while (!token_stream_check(s, TOK_RBRACE) && !token_stream_check(s, TOK_EOF))
	{
		if (xdiag_get_error_count() > 0) break;

		token_stream_skip_newlines(s);
		while (token_stream_match(s, TOK_SEMICOLON))
		{
			token_stream_skip_newlines(s);
		}
		if (token_stream_check(s, TOK_RBRACE) || token_stream_check(s, TOK_EOF)) break;

		token_t* prev_tok = token_stream_current(s);
		AstStmt* st = parse_statement(arena, s);
		if (xdiag_get_error_count() > 0) break;
		if (st && count < 128)
		{
			stmt_buf[count++] = st;
		}
		else
		{
			if (token_stream_current(s) == prev_tok && !token_stream_check(s, TOK_EOF) && !token_stream_check(s, TOK_RBRACE))
			{
				token_stream_advance(s);
			}
		}
		token_stream_skip_newlines(s);
		while (token_stream_match(s, TOK_SEMICOLON))
		{
			token_stream_skip_newlines(s);
		}
	}

	token_stream_match(s, TOK_RBRACE);
	return ast_stmt_block(arena, stmt_buf, count, line, col);
}

static AstStmt* parse_statement(AstArena* arena, token_stream_t* s)
{
	token_stream_skip_newlines(s);
	while (token_stream_match(s, TOK_SEMICOLON))
	{
		token_stream_skip_newlines(s);
	}
	if (token_stream_check(s, TOK_EOF)) return NULL;

	token_t* curr = token_stream_current(s);
	if (curr == NULL) return NULL;

	int line = curr->line;
	int col = curr->col;

	/* Block: { stmts... } */
	if (curr->type == TOK_LBRACE)
	{
		return parse_block(arena, s);
	}

	/* IF / EIF Statement */
	if (curr->type == TOK_KW_IF || curr->type == TOK_KW_EIF)
	{
		token_stream_advance(s); /* consume 'if' or 'eif' */
		token_stream_skip_newlines(s);

		if (token_stream_match(s, TOK_LPAREN))
		{
			token_stream_skip_newlines(s);
			AstExpr* cond = parse_expr(arena, s);
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);

			token_stream_skip_newlines(s);
			AstStmt* then_b = parse_block(arena, s);

			token_stream_skip_newlines(s);
			AstStmt* else_b = NULL;

			if (token_stream_check(s, TOK_KW_ELSE))
			{
				token_stream_advance(s); /* consume 'else' */
				token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_KW_IF))
				{
					/* else if -> recursive */
					else_b = parse_statement(arena, s);
				}
				else
				{
					else_b = parse_block(arena, s);
				}
			}
			else if (token_stream_check(s, TOK_KW_EIF))
			{
				/* eif -> recursive */
				else_b = parse_statement(arena, s);
			}

			return ast_stmt_if(arena, cond, then_b, else_b, line, col);
		}
	}

	/* WHILE Statement */
	if (curr->type == TOK_KW_WHILE)
	{
		token_stream_advance(s); /* consume 'while' */
		token_stream_skip_newlines(s);
		if (token_stream_match(s, TOK_LPAREN))
		{
			token_stream_skip_newlines(s);
			AstExpr* cond = parse_expr(arena, s);
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);

			token_stream_skip_newlines(s);
			AstStmt* body = parse_block(arena, s);
			return ast_stmt_while(arena, cond, body, line, col);
		}
	}

	/* DO-WHILE Statement */
	if (curr->type == TOK_KW_DO)
	{
		token_stream_advance(s); /* consume 'do' */
		token_stream_skip_newlines(s);
		AstStmt* body = parse_block(arena, s);

		token_stream_skip_newlines(s);
		AstExpr* cond = NULL;
		if (token_stream_match(s, TOK_KW_WHILE))
		{
			token_stream_skip_newlines(s);
			if (token_stream_match(s, TOK_LPAREN))
			{
				token_stream_skip_newlines(s);
				cond = parse_expr(arena, s);
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);
			}
		}
		return ast_stmt_do_while(arena, body, cond, line, col);
	}

	/* FOR Statement */
	if (curr->type == TOK_KW_FOR)
	{
		token_stream_advance(s); /* consume 'for' */
		token_stream_skip_newlines(s);

		/* Case 1: for (item in coll) */
		if (token_stream_check(s, TOK_LPAREN))
		{
			/* Lookahead to see if there is an 'in' keyword before ')' */
			int offset = 1;
			bool has_in = false;
			while (true)
			{
				token_t* t = token_stream_peek(s, offset);
				if (t == NULL || t->type == TOK_EOF || t->type == TOK_RPAREN) break;
				if (t->type == TOK_KW_IN) { has_in = true; break; }
				offset++;
			}

			if (has_in)
			{
				token_stream_advance(s); /* consume '(' */
				token_stream_skip_newlines(s);
				token_t* itok = token_stream_current(s);
				char* item_name = itok ? itok->text : "item";
				token_stream_advance(s); /* consume item identifier */

				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_KW_IN); /* consume 'in' */

				token_stream_skip_newlines(s);
				AstExpr* coll = parse_expr(arena, s);
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);

				token_stream_skip_newlines(s);
				AstStmt* body = parse_block(arena, s);
				return ast_stmt_for_in(arena, item_name, coll, body, line, col);
			}
			else
			{
				/* 3-part for loop inside parentheses:
				 * e.g. for (int i = 0, i++, i < 5)
				 *      for (int i = 0, i = i + 1, i < 5)
				 *      for (i = 0, i++, i < 5)
				 *      for (int i = 0; i < 5; i++)
				 */
				token_stream_advance(s); /* consume '(' */
				token_stream_skip_newlines(s);

				/* 1. Parse Init */
				AstStmt* init_stmt = NULL;
				char* loop_var_name = NULL;

				token_t* ctok = token_stream_current(s);
				bool is_type = (ctok != NULL && (ctok->type == TOK_TYPE_INT || ctok->type == TOK_TYPE_FLOAT ||
				                                 ctok->type == TOK_TYPE_STRING || ctok->type == TOK_TYPE_BOOL ||
				                                 ctok->type == TOK_TYPE_CHAR || ctok->type == TOK_TYPE_VOID ||
				                                 ctok->type == TOK_TYPE_LONG || ctok->type == TOK_TYPE_DOUBLE));
				if (!is_type && ctok != NULL && ctok->type == TOK_IDENT)
				{
					token_t* nxt = token_stream_peek(s, 1);
					if (nxt != NULL && nxt->type == TOK_IDENT)
					{
						is_type = true;
					}
				}

				if (is_type)
				{
					char* type_name = ctok->text;
					token_stream_advance(s); /* consume type */
					token_stream_skip_newlines(s);

					token_t* ntok = token_stream_current(s);
					char* var_name = ntok ? ntok->text : "i";
					loop_var_name = var_name;
					if (ntok && ntok->type == TOK_IDENT)
					{
						token_stream_advance(s); /* consume var name */
					}
					token_stream_skip_newlines(s);

					AstExpr* init_expr = NULL;
					if (token_stream_match(s, TOK_ASSIGN))
					{
						token_stream_skip_newlines(s);
						init_expr = parse_expr_prec(arena, s, 1);
					}
					init_stmt = ast_stmt_var_decl(arena, type_name, var_name, init_expr, false, line, col);
				}
				else
				{
					/* Expression or assignment init: e.g. i = 0 */
					AstExpr* ie = parse_expr_prec(arena, s, 1);
					if (ie != NULL)
					{
						if (ie->type == AST_EXPR_IDENTIFIER)
						{
							loop_var_name = ie->as.identifier_name;
						}
						token_stream_skip_newlines(s);
						if (token_stream_check(s, TOK_ASSIGN) ||
						    token_stream_check(s, TOK_PLUS_ASSIGN) ||
						    token_stream_check(s, TOK_MINUS_ASSIGN) ||
						    token_stream_check(s, TOK_STAR_ASSIGN) ||
						    token_stream_check(s, TOK_SLASH_ASSIGN) ||
						    token_stream_check(s, TOK_MOD_ASSIGN))
						{
							token_t* atok = token_stream_current(s);
							char* op_str = atok->text;
							token_stream_advance(s);
							token_stream_skip_newlines(s);
							AstExpr* val_expr = parse_expr_prec(arena, s, 1);
							ie = ast_expr_assign(arena, ie, op_str, val_expr, line, col);
						}
						init_stmt = ast_stmt_expr(arena, ie, line, col);
					}
				}

				/* Delimiter between part 1 and part 2 */
				token_stream_skip_newlines(s);
				bool is_semi = token_stream_match(s, TOK_SEMICOLON);
				if (!is_semi)
				{
					token_stream_match(s, TOK_COMMA);
				}

				/* 2. Parse Part 2 */
				token_stream_skip_newlines(s);
				AstExpr* part2 = parse_expr_prec(arena, s, 1);
				token_stream_skip_newlines(s);
				if (part2 != NULL && (token_stream_check(s, TOK_ASSIGN) ||
				                      token_stream_check(s, TOK_PLUS_ASSIGN) ||
				                      token_stream_check(s, TOK_MINUS_ASSIGN) ||
				                      token_stream_check(s, TOK_STAR_ASSIGN) ||
				                      token_stream_check(s, TOK_SLASH_ASSIGN) ||
				                      token_stream_check(s, TOK_MOD_ASSIGN)))
				{
					token_t* atok = token_stream_current(s);
					char* op_str = atok->text;
					token_stream_advance(s);
					token_stream_skip_newlines(s);
					AstExpr* val_expr = parse_expr_prec(arena, s, 1);
					part2 = ast_expr_assign(arena, part2, op_str, val_expr, line, col);
				}

				/* Delimiter between part 2 and part 3 */
				token_stream_skip_newlines(s);
				if (is_semi)
				{
					token_stream_match(s, TOK_SEMICOLON);
				}
				else
				{
					token_stream_match(s, TOK_COMMA);
				}

				/* 3. Parse Part 3 */
				token_stream_skip_newlines(s);
				AstExpr* part3 = parse_expr_prec(arena, s, 1);
				token_stream_skip_newlines(s);
				if (part3 != NULL && (token_stream_check(s, TOK_ASSIGN) ||
				                      token_stream_check(s, TOK_PLUS_ASSIGN) ||
				                      token_stream_check(s, TOK_MINUS_ASSIGN) ||
				                      token_stream_check(s, TOK_STAR_ASSIGN) ||
				                      token_stream_check(s, TOK_SLASH_ASSIGN) ||
				                      token_stream_check(s, TOK_MOD_ASSIGN)))
				{
					token_t* atok = token_stream_current(s);
					char* op_str = atok->text;
					token_stream_advance(s);
					token_stream_skip_newlines(s);
					AstExpr* val_expr = parse_expr_prec(arena, s, 1);
					part3 = ast_expr_assign(arena, part3, op_str, val_expr, line, col);
				}

				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);

				token_stream_skip_newlines(s);
				AstStmt* body = parse_block(arena, s);

				AstExpr* cond_expr = NULL;
				AstExpr* step_expr = NULL;

				if (is_semi)
				{
					/* for (init; cond; step) */
					cond_expr = part2;
					step_expr = part3;
				}
				else
				{
					/* Check if part2 is comparison and part3 is step/assign */
					bool part2_is_cond = (part2 != NULL && part2->type == AST_EXPR_BINARY &&
					                      (part2->as.binary.op == BINOP_LT ||
					                       part2->as.binary.op == BINOP_LTE ||
					                       part2->as.binary.op == BINOP_GT ||
					                       part2->as.binary.op == BINOP_GTE ||
					                       part2->as.binary.op == BINOP_EQ ||
					                       part2->as.binary.op == BINOP_NEQ));
					bool part3_is_step = (part3 != NULL && (part3->type == AST_EXPR_ASSIGN));

					if (part2_is_cond && (part3_is_step || part3->type != AST_EXPR_BINARY))
					{
						/* for (init, cond, step) */
						cond_expr = part2;
						step_expr = part3;
					}
					else
					{
						/* for (init, step, cond) - user's exact specification */
						step_expr = part2;
						cond_expr = part3;
					}
				}

				/* If step_expr is not an assignment (e.g. i + 1), wrap as i = i + 1 */
				if (step_expr != NULL && step_expr->type != AST_EXPR_ASSIGN && loop_var_name != NULL)
				{
					AstExpr* v_target = ast_expr_identifier(arena, loop_var_name, line, col);
					step_expr = ast_expr_assign(arena, v_target, "=", step_expr, line, col);
				}

				return ast_stmt_for_c(arena, init_stmt, cond_expr, step_expr, body, line, col);
			}
		}

		/* Case 2: for item in coll */
		token_t* p1 = token_stream_peek(s, 0);
		token_t* p2 = token_stream_peek(s, 1);
		if (p1 != NULL && p2 != NULL && p1->type == TOK_IDENT && p2->type == TOK_KW_IN)
		{
			char* item_name = p1->text;
			token_stream_advance(s); /* consume item */
			token_stream_advance(s); /* consume 'in' */

			token_stream_skip_newlines(s);
			AstExpr* coll = parse_expr(arena, s);

			token_stream_skip_newlines(s);
			AstStmt* body = parse_block(arena, s);
			return ast_stmt_for_in(arena, item_name, coll, body, line, col);
		}

		/* Case 3: classic for i (0, i+1, i < 10) */
		if (p1 != NULL && p1->type == TOK_IDENT)
		{
			char* var_name = p1->text;
			token_stream_advance(s); /* consume loop var name */
			token_stream_skip_newlines(s);

			if (token_stream_match(s, TOK_LPAREN))
			{
				token_stream_skip_newlines(s);
				AstExpr* init_val = parse_expr_prec(arena, s, 1);
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_COMMA);

				token_stream_skip_newlines(s);
				AstExpr* step_expr = parse_expr_prec(arena, s, 1);
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_COMMA);

				token_stream_skip_newlines(s);
				AstExpr* cond_expr = parse_expr_prec(arena, s, 1);
				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);

				token_stream_skip_newlines(s);
				AstStmt* body = parse_block(arena, s);

				AstExpr* v_target = ast_expr_identifier(arena, var_name, line, col);
				AstStmt* init_stmt = ast_stmt_var_decl(arena, "int", var_name, init_val, false, line, col);
				if (step_expr && step_expr->type != AST_EXPR_ASSIGN)
				{
					step_expr = ast_expr_assign(arena, v_target, "=", step_expr, line, col);
				}

				return ast_stmt_for_c(arena, init_stmt, cond_expr, step_expr, body, line, col);
			}
		}
	}

	/* RETURN Statement */
	if (curr->type == TOK_KW_RETURN)
	{
		token_stream_advance(s); /* consume 'return' */
		AstExpr* ret_expr = NULL;

		token_t* next = token_stream_current(s);
		if (next != NULL && next->type != TOK_NEWLINE && next->type != TOK_SEMICOLON &&
		    next->type != TOK_RBRACE && next->type != TOK_EOF)
		{
			ret_expr = parse_expr(arena, s);
		}
		return ast_stmt_return(arena, ret_expr, line, col);
	}

	/* BREAK Statement */
	if (curr->type == TOK_KW_BREAK)
	{
		token_stream_advance(s);
		return ast_stmt_break(arena, line, col);
	}

	/* CONTINUE Statement */
	if (curr->type == TOK_KW_CONTINUE)
	{
		token_stream_advance(s);
		return ast_stmt_continue(arena, line, col);
	}

	/* IMPORT Statement */
	if (curr->type == TOK_KW_IMPORT)
	{
		token_stream_advance(s); /* consume 'import' */
		token_stream_skip_newlines(s);
		bool has_paren = token_stream_match(s, TOK_LPAREN);
		token_stream_skip_newlines(s);
		token_t* mtok = token_stream_current(s);
		if (mtok != NULL && (mtok->type == TOK_STRING || mtok->type == TOK_IDENT))
		{
			char* mod_name = mtok->text;
			token_stream_advance(s);
			if (s_current_program != NULL)
			{
				x_import_module_ast(s_current_program, mod_name);
			}
		}
		if (has_paren)
		{
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);
		}
		return NULL;
	}

	/* EXTERN Declaration Block */
	if (curr->type == TOK_KW_EXTERN)
	{
		token_stream_advance(s); /* consume 'extern' */
		token_stream_skip_newlines(s);
		token_t* ltok = token_stream_current(s);
		char* lib_name = (ltok && (ltok->type == TOK_STRING || ltok->type == TOK_IDENT)) ? ltok->text : "";
		if (ltok && (ltok->type == TOK_STRING || ltok->type == TOK_IDENT))
		{
			token_stream_advance(s); /* consume library name */
		}
		token_stream_skip_newlines(s);

		AstStmt* func_decls[128];
		int func_count = 0;

		if (token_stream_match(s, TOK_LBRACE))
		{
			while (!token_stream_check(s, TOK_RBRACE) && !token_stream_check(s, TOK_EOF))
			{
				if (xdiag_get_error_count() > 0) break;
				token_stream_skip_newlines(s);
				while (token_stream_match(s, TOK_SEMICOLON)) token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_RBRACE) || token_stream_check(s, TOK_EOF)) break;

				token_t* fcurr = token_stream_current(s);
				if (fcurr == NULL || fcurr->type == TOK_EOF) break;

				int f_line = fcurr->line;
				int f_col = fcurr->col;

				/* Parse return type */
				char* ret_type = fcurr->text;
				token_stream_advance(s);
				token_stream_skip_newlines(s);

				/* Parse function name */
				token_t* ntok = token_stream_current(s);
				char* fn_name = ntok ? ntok->text : "extern_fn";
				if (ntok && ntok->type == TOK_IDENT)
				{
					token_stream_advance(s);
				}
				token_stream_skip_newlines(s);

				/* Parse parameter list: '(' [ type [name], ... ] ')' */
				AstParam params[32];
				int pcount = 0;
				if (token_stream_match(s, TOK_LPAREN))
				{
					while (!token_stream_check(s, TOK_RPAREN) && !token_stream_check(s, TOK_EOF))
					{
						token_stream_skip_newlines(s);
						if (token_stream_check(s, TOK_RPAREN)) break;

						token_t* ptype_tok = token_stream_current(s);
						char* ptype = ptype_tok ? ptype_tok->text : "int";
						token_stream_advance(s); /* consume param type */

						token_stream_skip_newlines(s);
						token_t* pname_tok = token_stream_current(s);
						char* pname = "arg";
						if (pname_tok && pname_tok->type == TOK_IDENT)
						{
							pname = pname_tok->text;
							token_stream_advance(s); /* consume param name */
						}

						if (pcount < 32)
						{
							params[pcount].type_name = ptype;
							params[pcount].name = pname;
							pcount++;
						}

						token_stream_skip_newlines(s);
						if (token_stream_check(s, TOK_COMMA))
						{
							token_stream_advance(s);
						}
						else
						{
							break;
						}
					}
					token_stream_skip_newlines(s);
					token_stream_match(s, TOK_RPAREN);
				}

				/* Optional trailing semicolon */
				token_stream_skip_newlines(s);
				while (token_stream_match(s, TOK_SEMICOLON)) token_stream_skip_newlines(s);

				AstStmt* fs = ast_stmt_func_decl(arena, fn_name, ret_type, params, pcount, NULL, false, NULL, f_line, f_col);
				if (func_count < 128)
				{
					func_decls[func_count++] = fs;
				}
			}
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RBRACE);
		}

		AstStmt* ext_stmt = ast_stmt_extern_block(arena, lib_name, func_decls, func_count, line, col);
		xffi_process_extern_block(ext_stmt);
		return ext_stmt;
	}

	/* CLASS Declaration */
	if (curr->type == TOK_KW_CLASS)
	{
		token_stream_advance(s); /* consume 'class' */
		token_stream_skip_newlines(s);
		token_t* ctok = token_stream_current(s);
		char* class_name = ctok ? ctok->text : "AnonClass";
		token_stream_advance(s); /* consume class name */

		register_class_name(class_name);

		char* base_name = NULL;
		token_stream_skip_newlines(s);
		if (token_stream_match(s, TOK_LPAREN))
		{
			token_stream_skip_newlines(s);
			token_t* btok = token_stream_current(s);
			if (btok != NULL && btok->type == TOK_IDENT)
			{
				base_name = btok->text;
				token_stream_advance(s);
			}
			token_stream_skip_newlines(s);
			token_stream_match(s, TOK_RPAREN);
		}

		token_stream_skip_newlines(s);
		AstStmt* members[64];
		int member_count = 0;

		if (token_stream_match(s, TOK_LBRACE))
		{
			s_class_depth++;
			const char* prev_class_name = s_current_class_name;
			s_current_class_name = class_name;

			while (!token_stream_check(s, TOK_RBRACE) && !token_stream_check(s, TOK_EOF))
			{
				if (xdiag_get_error_count() > 0) break;
				token_stream_skip_newlines(s);
				while (token_stream_match(s, TOK_SEMICOLON)) token_stream_skip_newlines(s);
				if (token_stream_check(s, TOK_RBRACE) || token_stream_check(s, TOK_EOF)) break;

				token_t* prev_tok = token_stream_current(s);
				AstStmt* m = parse_statement(arena, s);
				if (xdiag_get_error_count() > 0) break;
				if (m && member_count < 64)
				{
					members[member_count++] = m;
				}
				else
				{
					if (token_stream_current(s) == prev_tok && !token_stream_check(s, TOK_EOF) && !token_stream_check(s, TOK_RBRACE))
					{
						token_stream_advance(s);
					}
				}
				token_stream_skip_newlines(s);
				while (token_stream_match(s, TOK_SEMICOLON)) token_stream_skip_newlines(s);
			}

			s_current_class_name = prev_class_name;
			s_class_depth--;
			token_stream_match(s, TOK_RBRACE);
		}

		type_def* td = get_type_by_name(class_name);
		if (td == NULL)
		{
			td = new_type();
			td->type_name = strdup(class_name);
		}
		if (base_name != NULL)
		{
			td->base = get_type_by_name(base_name);
		}
		for (int i = 0; i < member_count; i++)
		{
			AstStmt* m = members[i];
			if (m && m->type == AST_STMT_VAR_DECL && !m->as.var_decl.is_static)
			{
				type_def* ftype = get_type_by_name(m->as.var_decl.type_name);
				if (!ftype) ftype = T_INT;
				var* dummy = NULL;
				define_class_property(m->as.var_decl.var_name, td, ftype, &dummy);
			}
		}
		type_def_compute_field_offsets(td);

		return ast_stmt_class_decl(arena, class_name, base_name, members, member_count, line, col);
	}

	/* Variable or Function Declaration */
	bool is_static = false;
	if (curr->type == TOK_KW_STATIC)
	{
		is_static = true;
		token_stream_advance(s);
		token_stream_skip_newlines(s);
		curr = token_stream_current(s);
		if (curr == NULL || curr->type == TOK_EOF) return NULL;
	}

	/* Check if it's a type keyword: int, float, string, bool, char, void, long, double */
	bool is_type_kw = (curr->type == TOK_TYPE_INT || curr->type == TOK_TYPE_FLOAT ||
	                   curr->type == TOK_TYPE_STRING || curr->type == TOK_TYPE_BOOL ||
	                   curr->type == TOK_TYPE_CHAR || curr->type == TOK_TYPE_VOID ||
	                   curr->type == TOK_TYPE_LONG || curr->type == TOK_TYPE_DOUBLE);

	/* Check if it's an Identifier followed by an Identifier: Point p ... */
	bool is_class_type_decl = false;
	if (curr->type == TOK_IDENT)
	{
		token_t* next_tok = token_stream_peek(s, 1);
		if (next_tok != NULL && (next_tok->type == TOK_IDENT || next_tok->type == TOK_LPAREN))
		{
			if (next_tok->type == TOK_IDENT)
			{
				is_class_type_decl = true;
			}
			else if (s_current_class_name != NULL && strcmp(curr->text, s_current_class_name) == 0)
			{
				/* Constructor without return type: Point(int a, int b) { ... } */
				is_type_kw = true;
			}
		}
	}

	if (is_type_kw || is_class_type_decl)
	{
		char* type_name = curr->text;
		token_stream_advance(s); /* consume type name */
		token_stream_skip_newlines(s);

		token_t* name_tok = token_stream_current(s);
		char* decl_name = name_tok ? name_tok->text : "var";
		if (name_tok && name_tok->type == TOK_IDENT)
		{
			token_stream_advance(s); /* consume decl name */
		}
		else if (s_current_class_name != NULL && strcmp(type_name, s_current_class_name) == 0)
		{
			/* Constructor named after class */
			decl_name = type_name;
		}

		token_stream_skip_newlines(s);

		/* Check if followed by '(' -> Function Declaration or Constructor Init */
		if (token_stream_check(s, TOK_LPAREN))
		{
			/* Lookahead past closing ')' to check if followed by '{' */
			int offset = 1;
			int depth = 1;
			while (true)
			{
				token_t* t = token_stream_peek(s, offset);
				if (t == NULL || t->type == TOK_EOF) break;
				if (t->type == TOK_LPAREN) depth++;
				else if (t->type == TOK_RPAREN)
				{
					depth--;
					if (depth == 0)
					{
						offset++;
						break;
					}
				}
				offset++;
			}

			/* Skip any newlines after ')' */
			while (true)
			{
				token_t* t = token_stream_peek(s, offset);
				if (t != NULL && t->type == TOK_NEWLINE) offset++;
				else break;
			}

			token_t* after_paren = token_stream_peek(s, offset);
			bool is_func_def = (after_paren != NULL && after_paren->type == TOK_LBRACE);

			if (is_func_def)
			{
				token_stream_advance(s); /* consume '(' */
				AstParam params[16];
				int param_count = 0;

				while (!token_stream_check(s, TOK_RPAREN) && !token_stream_check(s, TOK_EOF))
				{
					token_stream_skip_newlines(s);
					if (token_stream_check(s, TOK_RPAREN)) break;

					token_t* ptype_tok = token_stream_current(s);
					char* ptype = ptype_tok ? ptype_tok->text : "object";
					token_stream_advance(s); /* consume param type */

					token_stream_skip_newlines(s);
					token_t* pname_tok = token_stream_current(s);
					char* pname = (pname_tok && pname_tok->type == TOK_IDENT) ? pname_tok->text : "arg";
					if (pname_tok && pname_tok->type == TOK_IDENT)
					{
						token_stream_advance(s); /* consume param name */
					}

					if (param_count < 16)
					{
						params[param_count].type_name = ptype;
						params[param_count].name = pname;
						param_count++;
					}

					token_stream_skip_newlines(s);
					if (token_stream_check(s, TOK_COMMA))
					{
						token_stream_advance(s);
					}
					else
					{
						break;
					}
				}

				token_stream_skip_newlines(s);
				token_stream_match(s, TOK_RPAREN);

				token_stream_skip_newlines(s);
				AstStmt* body = parse_block(arena, s);
				const char* ret_type = (s_current_class_name != NULL && strcmp(decl_name, s_current_class_name) == 0) ? "void" : type_name;
				return ast_stmt_func_decl(arena, decl_name, ret_type, params, param_count, body, is_static, s_current_class_name, line, col);
			}
			else
			{
				/* Disallowed: Direct constructor call in declaration (E0010) */
				xdiag_error("E0010", NULL, line, col, 0, NULL,
				            "direct constructor call in declaration is disallowed; use '%s %s = new %s(...)'",
				            type_name, decl_name, type_name);
				while (!token_stream_check(s, TOK_NEWLINE) && !token_stream_check(s, TOK_SEMICOLON) && !token_stream_check(s, TOK_EOF))
				{
					token_stream_advance(s);
				}
				return NULL;
			}
		}

		/* Variable Declaration */
		AstExpr* init_expr = NULL;
		if (token_stream_match(s, TOK_ASSIGN))
		{
			token_stream_skip_newlines(s);
			init_expr = parse_expr(arena, s);
		}

		if (init_expr == NULL && s_class_depth == 0 && !is_primitive_type_name(type_name))
		{
			/* Disallowed: Implicit default instantiation (E0011) */
			xdiag_error("E0011", NULL, line, col, 0, NULL,
			            "implicit default instantiation is disallowed; use '%s %s = new %s()'",
			            type_name, decl_name, type_name);
			while (!token_stream_check(s, TOK_NEWLINE) && !token_stream_check(s, TOK_SEMICOLON) && !token_stream_check(s, TOK_EOF))
			{
				token_stream_advance(s);
			}
			return NULL;
		}

		return ast_stmt_var_decl(arena, type_name, decl_name, init_expr, is_static, line, col);
	}

	/* Expression / Assignment Statement */
	AstExpr* expr = parse_expr(arena, s);
	if (!expr)
	{
		return NULL;
	}

	/* Check assignment: =, +=, -=, *=, /=, %= */
	token_t* atok = token_stream_current(s);
	if (atok != NULL)
	{
		char* op_str = NULL;
		if (atok->type == TOK_ASSIGN) op_str = "=";
		else if (atok->type == TOK_PLUS_ASSIGN) op_str = "+=";
		else if (atok->type == TOK_MINUS_ASSIGN) op_str = "-=";
		else if (atok->type == TOK_STAR_ASSIGN) op_str = "*=";
		else if (atok->type == TOK_SLASH_ASSIGN) op_str = "/=";
		else if (atok->type == TOK_MOD_ASSIGN) op_str = "%=";

		if (op_str != NULL)
		{
			token_stream_advance(s); /* consume assign operator */
			token_stream_skip_newlines(s);
			AstExpr* val_expr = parse_expr(arena, s);
			expr = ast_expr_assign(arena, expr, op_str, val_expr, line, col);
		}
	}

	return ast_stmt_expr(arena, expr, line, col);
}

/* -------------------------------------------------------------------------
 * Public Entry Points
 * ------------------------------------------------------------------------- */
bool xast_parse_into_program(AstProgram* prog, token_stream_t* stream)
{
	if (prog == NULL || stream == NULL) return false;

	AstProgram* prev_prog = s_current_program;
	s_current_program = prog;

	while (!token_stream_check(stream, TOK_EOF))
	{
		if (xdiag_get_error_count() > 0) break;

		token_stream_skip_newlines(stream);
		while (token_stream_match(stream, TOK_SEMICOLON))
		{
			token_stream_skip_newlines(stream);
		}
		if (token_stream_check(stream, TOK_EOF)) break;

		token_t* prev_tok = token_stream_current(stream);
		AstStmt* stmt = parse_statement(prog->arena, stream);
		if (xdiag_get_error_count() > 0) break;
		if (stmt != NULL)
		{
			ast_program_add_stmt(prog, stmt);
		}
		else
		{
			if (token_stream_current(stream) == prev_tok && !token_stream_check(stream, TOK_EOF))
			{
				token_stream_advance(stream);
			}
		}
		token_stream_skip_newlines(stream);
		while (token_stream_match(stream, TOK_SEMICOLON))
		{
			token_stream_skip_newlines(stream);
		}
	}

	s_current_program = prev_prog;
	return (xdiag_get_error_count() == 0);
}

AstProgram* xast_parse_token_stream(AstArena* arena, token_stream_t* stream)
{
	if (arena == NULL || stream == NULL) return NULL;
	AstProgram* prog = ast_program_create(arena);
	if (!xast_parse_into_program(prog, stream))
	{
		return NULL;
	}
	return prog;
}

AstProgram* xast_parse_source(const char* source_code, const char* filename)
{
	if (source_code == NULL) return NULL;
	xdiag_set_current_file(filename ? filename : "<source>");
	xdiag_set_source_code(source_code);

	token_stream_t* stream = token_stream_tokenize(source_code);
	AstArena* arena = ast_arena_create(64 * 1024);
	AstProgram* prog = xast_parse_token_stream(arena, stream);
	token_stream_free(stream);
	free(stream);

	if (xdiag_get_error_count() > 0)
	{
		if (prog != NULL)
		{
			ast_program_destroy(prog);
			prog = NULL;
		}
		return NULL;
	}
	return prog;
}

AstProgram* xast_parse_file(const char* filepath)
{
	FILE* f = fopen(filepath, "r");
	if (f == NULL) return NULL;
	char* buf = get_file_buffer(f);
	fclose(f);
	if (buf == NULL) return NULL;

	AstProgram* prog = xast_parse_source(buf, filepath);
	free(buf);
	return prog;
}
