#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const char* token_type_name(token_type_t type)
{
	switch (type)
	{
		case TOK_EOF: return "EOF";
		case TOK_NEWLINE: return "NEWLINE";
		case TOK_IDENT: return "IDENT";
		case TOK_INT: return "INT";
		case TOK_FLOAT: return "FLOAT";
		case TOK_STRING: return "STRING";
		case TOK_CHAR: return "CHAR";
		case TOK_BOOL: return "BOOL";
		case TOK_NULL: return "null";
		case TOK_KW_IF: return "if";
		case TOK_KW_ELSE: return "else";
		case TOK_KW_EIF: return "eif";
		case TOK_KW_WHILE: return "while";
		case TOK_KW_FOR: return "for";
		case TOK_KW_DO: return "do";
		case TOK_KW_RETURN: return "return";
		case TOK_KW_CLASS: return "class";
		case TOK_KW_BREAK: return "break";
		case TOK_KW_CONTINUE: return "continue";
		case TOK_KW_IMPORT: return "import";
		case TOK_KW_STATIC: return "static";
		case TOK_KW_NEW: return "new";
		case TOK_KW_IN: return "in";
		case TOK_TYPE_INT: return "int";
		case TOK_TYPE_FLOAT: return "float";
		case TOK_TYPE_STRING: return "string";
		case TOK_TYPE_CHAR: return "char";
		case TOK_TYPE_BOOL: return "bool";
		case TOK_TYPE_VOID: return "void";
		case TOK_TYPE_LONG: return "long";
		case TOK_TYPE_DOUBLE: return "double";
		case TOK_PLUS: return "+";
		case TOK_MINUS: return "-";
		case TOK_STAR: return "*";
		case TOK_SLASH: return "/";
		case TOK_PERCENT: return "%";
		case TOK_ASSIGN: return "=";
		case TOK_PLUS_ASSIGN: return "+=";
		case TOK_MINUS_ASSIGN: return "-=";
		case TOK_STAR_ASSIGN: return "*=";
		case TOK_SLASH_ASSIGN: return "/=";
		case TOK_MOD_ASSIGN: return "%=";
		case TOK_INC: return "++";
		case TOK_DEC: return "--";
		case TOK_EQ: return "==";
		case TOK_NEQ: return "!=";
		case TOK_LT: return "<";
		case TOK_LTE: return "<=";
		case TOK_GT: return ">";
		case TOK_GTE: return ">=";
		case TOK_AND: return "&&";
		case TOK_OR: return "||";
		case TOK_NOT: return "!";
		case TOK_BIT_AND: return "&";
		case TOK_BIT_OR: return "|";
		case TOK_BIT_XOR: return "^";
		case TOK_BIT_NOT: return "~";
		case TOK_SHL: return "<<";
		case TOK_SHR: return ">>";
		case TOK_ARROW: return "->";
		case TOK_DOT: return ".";
		case TOK_COLON_COLON: return "::";
		case TOK_LPAREN: return "(";
		case TOK_RPAREN: return ")";
		case TOK_LBRACE: return "{";
		case TOK_RBRACE: return "}";
		case TOK_LBRACKET: return "[";
		case TOK_RBRACKET: return "]";
		case TOK_COMMA: return ",";
		case TOK_SEMICOLON: return ";";
		case TOK_COLON: return ":";
		default: return "UNKNOWN";
	}
}

void token_free(token_t* tok)
{
	if (tok != NULL && tok->text != NULL)
	{
		free(tok->text);
		tok->text = NULL;
	}
}

void lexer_init(lexer_t* lexer, const char* source)
{
	if (lexer == NULL) return;
	lexer->source = source != NULL ? source : "";
	lexer->cursor = lexer->source;
	lexer->line = 1;
	lexer->col = 1;
	lexer->last_was_newline = true; /* collapse any leading newlines */
}

static char* lex_strndup(const char* s, size_t n)
{
	char* p = (char*)malloc(n + 1);
	if (p != NULL)
	{
		memcpy(p, s, n);
		p[n] = '\0';
	}
	return p;
}

static token_t make_token(token_type_t type, const char* text, int len, int line, int col)
{
	token_t tok;
	tok.type = type;
	tok.text = lex_strndup(text, len);
	tok.length = len;
	tok.line = line;
	tok.col = col;
	tok.int_val = 0;
	tok.float_val = 0.0;
	return tok;
}

static void skip_spaces_and_comments(lexer_t* lexer)
{
	while (*lexer->cursor != '\0')
	{
		if (*lexer->cursor == ' ' || *lexer->cursor == '\t' || *lexer->cursor == '\r')
		{
			lexer->cursor++;
			lexer->col++;
		}
		else if (*lexer->cursor == '#' || (*lexer->cursor == '/' && *(lexer->cursor + 1) == '/'))
		{
			/* Single line comment: skip until newline or EOF */
			while (*lexer->cursor != '\0' && *lexer->cursor != '\n')
			{
				lexer->cursor++;
			}
		}
		else if (*lexer->cursor == '/' && *(lexer->cursor + 1) == '*')
		{
			/* Multi-line comment: skip until * / */
			lexer->cursor += 2;
			lexer->col += 2;
			while (*lexer->cursor != '\0')
			{
				if (*lexer->cursor == '*' && *(lexer->cursor + 1) == '/')
				{
					lexer->cursor += 2;
					lexer->col += 2;
					break;
				}
				if (*lexer->cursor == '\n')
				{
					lexer->cursor++;
					lexer->line++;
					lexer->col = 1;
				}
				else
				{
					lexer->cursor++;
					lexer->col++;
				}
			}
		}
		else
		{
			break;
		}
	}
}

static token_type_t check_keyword(const char* text, int len)
{
	switch (len)
	{
		case 2:
			if (text[0] == 'i' && text[1] == 'f') return TOK_KW_IF;
			if (text[0] == 'd' && text[1] == 'o') return TOK_KW_DO;
			if (text[0] == 'i' && text[1] == 'n') return TOK_KW_IN;
			break;
		case 3:
			if (strncmp(text, "for", 3) == 0) return TOK_KW_FOR;
			if (strncmp(text, "eif", 3) == 0) return TOK_KW_EIF;
			if (strncmp(text, "new", 3) == 0) return TOK_KW_NEW;
			if (strncmp(text, "int", 3) == 0) return TOK_TYPE_INT;
			break;
		case 4:
			if (strncmp(text, "else", 4) == 0) return TOK_KW_ELSE;
			if (strncmp(text, "char", 4) == 0) return TOK_TYPE_CHAR;
			if (strncmp(text, "bool", 4) == 0) return TOK_TYPE_BOOL;
			if (strncmp(text, "void", 4) == 0) return TOK_TYPE_VOID;
			if (strncmp(text, "long", 4) == 0) return TOK_TYPE_LONG;
			if (strncmp(text, "null", 4) == 0) return TOK_NULL;
			if (strncmp(text, "true", 4) == 0) return TOK_BOOL;
			break;
		case 5:
			if (strncmp(text, "while", 5) == 0) return TOK_KW_WHILE;
			if (strncmp(text, "class", 5) == 0) return TOK_KW_CLASS;
			if (strncmp(text, "break", 5) == 0) return TOK_KW_BREAK;
			if (strncmp(text, "float", 5) == 0) return TOK_TYPE_FLOAT;
			if (strncmp(text, "false", 5) == 0) return TOK_BOOL;
			break;
		case 6:
			if (strncmp(text, "return", 6) == 0) return TOK_KW_RETURN;
			if (strncmp(text, "import", 6) == 0) return TOK_KW_IMPORT;
			if (strncmp(text, "static", 6) == 0) return TOK_KW_STATIC;
			if (strncmp(text, "string", 6) == 0) return TOK_TYPE_STRING;
			if (strncmp(text, "double", 6) == 0) return TOK_TYPE_DOUBLE;
			break;
		case 8:
			if (strncmp(text, "continue", 8) == 0) return TOK_KW_CONTINUE;
			break;
	}
	return TOK_IDENT;
}

token_t lexer_next_token(lexer_t* lexer)
{
	while (true)
	{
		skip_spaces_and_comments(lexer);

		if (*lexer->cursor == '\0')
		{
			token_t tok = make_token(TOK_EOF, "", 0, lexer->line, lexer->col);
			return tok;
		}

		/* Check newline */
		if (*lexer->cursor == '\n')
		{
			int start_line = lexer->line;
			int start_col = lexer->col;
			lexer->cursor++;
			lexer->line++;
			lexer->col = 1;

			if (!lexer->last_was_newline)
			{
				lexer->last_was_newline = true;
				return make_token(TOK_NEWLINE, "\n", 1, start_line, start_col);
			}
			continue; /* collapse consecutive newlines */
		}

		/* We are about to emit a non-newline token */
		lexer->last_was_newline = false;
		break;
	}

	int start_line = lexer->line;
	int start_col = lexer->col;
	const char* start_p = lexer->cursor;
	char c = *lexer->cursor;
	char c2 = *(lexer->cursor + 1);

	/* Two-character operators */
	if (c == '=' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_EQ, "==", 2, start_line, start_col); }
	if (c == '!' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_NEQ, "!=", 2, start_line, start_col); }
	if (c == '<' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_LTE, "<=", 2, start_line, start_col); }
	if (c == '>' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_GTE, ">=", 2, start_line, start_col); }
	if (c == '+' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_PLUS_ASSIGN, "+=", 2, start_line, start_col); }
	if (c == '-' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_MINUS_ASSIGN, "-=", 2, start_line, start_col); }
	if (c == '*' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_STAR_ASSIGN, "*=", 2, start_line, start_col); }
	if (c == '/' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_SLASH_ASSIGN, "/=", 2, start_line, start_col); }
	if (c == '%' && c2 == '=') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_MOD_ASSIGN, "%=", 2, start_line, start_col); }
	if (c == '+' && c2 == '+') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_INC, "++", 2, start_line, start_col); }
	if (c == '-' && c2 == '-') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_DEC, "--", 2, start_line, start_col); }
	if (c == '&' && c2 == '&') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_AND, "&&", 2, start_line, start_col); }
	if (c == '|' && c2 == '|') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_OR, "||", 2, start_line, start_col); }
	if (c == '<' && c2 == '<') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_SHL, "<<", 2, start_line, start_col); }
	if (c == '>' && c2 == '>') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_SHR, ">>", 2, start_line, start_col); }
	if (c == '-' && c2 == '>') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_ARROW, "->", 2, start_line, start_col); }
	if (c == ':' && c2 == ':') { lexer->cursor += 2; lexer->col += 2; return make_token(TOK_COLON_COLON, "::", 2, start_line, start_col); }

	/* Single-character operators and delimiters */
	switch (c)
	{
		case '+': lexer->cursor++; lexer->col++; return make_token(TOK_PLUS, "+", 1, start_line, start_col);
		case '-': lexer->cursor++; lexer->col++; return make_token(TOK_MINUS, "-", 1, start_line, start_col);
		case '*': lexer->cursor++; lexer->col++; return make_token(TOK_STAR, "*", 1, start_line, start_col);
		case '/': lexer->cursor++; lexer->col++; return make_token(TOK_SLASH, "/", 1, start_line, start_col);
		case '%': lexer->cursor++; lexer->col++; return make_token(TOK_PERCENT, "%", 1, start_line, start_col);
		case '=': lexer->cursor++; lexer->col++; return make_token(TOK_ASSIGN, "=", 1, start_line, start_col);
		case '<': lexer->cursor++; lexer->col++; return make_token(TOK_LT, "<", 1, start_line, start_col);
		case '>': lexer->cursor++; lexer->col++; return make_token(TOK_GT, ">", 1, start_line, start_col);
		case '!': lexer->cursor++; lexer->col++; return make_token(TOK_NOT, "!", 1, start_line, start_col);
		case '&': lexer->cursor++; lexer->col++; return make_token(TOK_BIT_AND, "&", 1, start_line, start_col);
		case '|': lexer->cursor++; lexer->col++; return make_token(TOK_BIT_OR, "|", 1, start_line, start_col);
		case '^': lexer->cursor++; lexer->col++; return make_token(TOK_BIT_XOR, "^", 1, start_line, start_col);
		case '~': lexer->cursor++; lexer->col++; return make_token(TOK_BIT_NOT, "~", 1, start_line, start_col);
		case '(': lexer->cursor++; lexer->col++; return make_token(TOK_LPAREN, "(", 1, start_line, start_col);
		case ')': lexer->cursor++; lexer->col++; return make_token(TOK_RPAREN, ")", 1, start_line, start_col);
		case '{': lexer->cursor++; lexer->col++; return make_token(TOK_LBRACE, "{", 1, start_line, start_col);
		case '}': lexer->cursor++; lexer->col++; return make_token(TOK_RBRACE, "}", 1, start_line, start_col);
		case '[': lexer->cursor++; lexer->col++; return make_token(TOK_LBRACKET, "[", 1, start_line, start_col);
		case ']': lexer->cursor++; lexer->col++; return make_token(TOK_RBRACKET, "]", 1, start_line, start_col);
		case ',': lexer->cursor++; lexer->col++; return make_token(TOK_COMMA, ",", 1, start_line, start_col);
		case ';': lexer->cursor++; lexer->col++; return make_token(TOK_SEMICOLON, ";", 1, start_line, start_col);
		case ':': lexer->cursor++; lexer->col++; return make_token(TOK_COLON, ":", 1, start_line, start_col);
		case '.': lexer->cursor++; lexer->col++; return make_token(TOK_DOT, ".", 1, start_line, start_col);
	}

	/* String literal */
	if (c == '"')
	{
		lexer->cursor++;
		lexer->col++;
		size_t cap = 64;
		size_t len = 0;
		char* s = (char*)malloc(cap);

		while (*lexer->cursor != '\0' && *lexer->cursor != '"')
		{
			char ch = *lexer->cursor;
			if (ch == '\\')
			{
				lexer->cursor++;
				lexer->col++;
				ch = *lexer->cursor;
				if (ch == '\0') break;
				switch (ch)
				{
					case 'n': ch = '\n'; break;
					case 't': ch = '\t'; break;
					case 'r': ch = '\r'; break;
					case '\\': ch = '\\'; break;
					case '"': ch = '"'; break;
					case '\'': ch = '\''; break;
					case '0': ch = '\0'; break;
					default:
						/* Preserve backslash for unknown escape sequences (e.g. \d, \s in regex) */
						if (len + 2 >= cap) { cap *= 2; s = (char*)realloc(s, cap); }
						s[len++] = '\\';
						break;
				}
			}
			if (len + 2 >= cap)
			{
				cap *= 2;
				s = (char*)realloc(s, cap);
			}
			s[len++] = ch;
			if (ch == '\n')
			{
				lexer->line++;
				lexer->col = 1;
			}
			else
			{
				lexer->col++;
			}
			lexer->cursor++;
		}
		s[len] = '\0';

		if (*lexer->cursor == '"')
		{
			lexer->cursor++;
			lexer->col++;
		}

		token_t tok;
		tok.type = TOK_STRING;
		tok.text = s;
		tok.length = (int)len;
		tok.line = start_line;
		tok.col = start_col;
		tok.int_val = 0;
		tok.float_val = 0.0;
		return tok;
	}

	/* Char literal */
	if (c == '\'')
	{
		lexer->cursor++;
		lexer->col++;
		char ch = *lexer->cursor;
		if (ch == '\\')
		{
			lexer->cursor++;
			lexer->col++;
			ch = *lexer->cursor;
			switch (ch)
			{
				case 'n': ch = '\n'; break;
				case 't': ch = '\t'; break;
				case 'r': ch = '\r'; break;
				case '\\': ch = '\\'; break;
				case '\'': ch = '\''; break;
				case '0': ch = '\0'; break;
			}
		}
		if (*lexer->cursor != '\0')
		{
			lexer->cursor++;
			lexer->col++;
		}
		if (*lexer->cursor == '\'')
		{
			lexer->cursor++;
			lexer->col++;
		}
		char buf[2] = { ch, '\0' };
		token_t tok = make_token(TOK_CHAR, buf, 1, start_line, start_col);
		tok.int_val = (int64_t)(unsigned char)ch;
		return tok;
	}

	/* Number literals: hex, binary, octal, float, decimal integer */
	if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)c2)))
	{
		if (c == '0' && (c2 == 'x' || c2 == 'X'))
		{
			/* Hexadecimal */
			lexer->cursor += 2;
			lexer->col += 2;
			const char* hex_start = lexer->cursor;
			while (isxdigit((unsigned char)*lexer->cursor))
			{
				lexer->cursor++;
				lexer->col++;
			}
			int raw_len = (int)(lexer->cursor - start_p);
			token_t tok = make_token(TOK_INT, start_p, raw_len, start_line, start_col);
			tok.int_val = (int64_t)strtoull(hex_start, NULL, 16);
			return tok;
		}

		if (c == '0' && (c2 == 'b' || c2 == 'B'))
		{
			/* Binary */
			lexer->cursor += 2;
			lexer->col += 2;
			const char* bin_start = lexer->cursor;
			while (*lexer->cursor == '0' || *lexer->cursor == '1')
			{
				lexer->cursor++;
				lexer->col++;
			}
			int raw_len = (int)(lexer->cursor - start_p);
			token_t tok = make_token(TOK_INT, start_p, raw_len, start_line, start_col);
			tok.int_val = (int64_t)strtoull(bin_start, NULL, 2);
			return tok;
		}

		bool is_float = false;
		while (isdigit((unsigned char)*lexer->cursor))
		{
			lexer->cursor++;
			lexer->col++;
		}

		if (*lexer->cursor == '.' && isdigit((unsigned char)*(lexer->cursor + 1)))
		{
			is_float = true;
			lexer->cursor++;
			lexer->col++;
			while (isdigit((unsigned char)*lexer->cursor))
			{
				lexer->cursor++;
				lexer->col++;
			}
		}

		if (*lexer->cursor == 'e' || *lexer->cursor == 'E')
		{
			is_float = true;
			lexer->cursor++;
			lexer->col++;
			if (*lexer->cursor == '+' || *lexer->cursor == '-')
			{
				lexer->cursor++;
				lexer->col++;
			}
			while (isdigit((unsigned char)*lexer->cursor))
			{
				lexer->cursor++;
				lexer->col++;
			}
		}

		int num_len = (int)(lexer->cursor - start_p);
		token_t tok = make_token(is_float ? TOK_FLOAT : TOK_INT, start_p, num_len, start_line, start_col);
		if (is_float)
			tok.float_val = strtod(tok.text, NULL);
		else
			tok.int_val = (int64_t)strtoll(tok.text, NULL, 10);

		return tok;
	}

	/* Identifiers and keywords */
	if (isalpha((unsigned char)c) || c == '_')
	{
		while (isalnum((unsigned char)*lexer->cursor) || *lexer->cursor == '_')
		{
			lexer->cursor++;
			lexer->col++;
		}

		int id_len = (int)(lexer->cursor - start_p);
		token_type_t tt = check_keyword(start_p, id_len);
		token_t tok = make_token(tt, start_p, id_len, start_line, start_col);
		if (tt == TOK_BOOL)
		{
			tok.int_val = (strcmp(tok.text, "true") == 0) ? 1 : 0;
		}
		return tok;
	}

	/* Unknown character */
	lexer->cursor++;
	lexer->col++;
	return make_token(TOK_UNKNOWN, start_p, 1, start_line, start_col);
}

void token_stream_init(token_stream_t* stream)
{
	if (stream == NULL) return;
	stream->tokens = NULL;
	stream->count = 0;
	stream->capacity = 0;
	stream->cursor = 0;
}

void token_stream_free(token_stream_t* stream)
{
	if (stream == NULL) return;
	for (int i = 0; i < stream->count; i++)
	{
		token_free(&stream->tokens[i]);
	}
	if (stream->tokens != NULL)
	{
		free(stream->tokens);
		stream->tokens = NULL;
	}
	stream->count = 0;
	stream->capacity = 0;
	stream->cursor = 0;
}

void token_stream_add(token_stream_t* stream, token_t tok)
{
	if (stream == NULL) return;
	if (stream->count >= stream->capacity)
	{
		stream->capacity = stream->capacity == 0 ? 128 : stream->capacity * 2;
		stream->tokens = (token_t*)realloc(stream->tokens, stream->capacity * sizeof(token_t));
	}
	stream->tokens[stream->count++] = tok;
}

token_stream_t* token_stream_tokenize(const char* source)
{
	token_stream_t* stream = (token_stream_t*)malloc(sizeof(token_stream_t));
	token_stream_init(stream);

	lexer_t lexer;
	lexer_init(&lexer, source);

	while (true)
	{
		token_t tok = lexer_next_token(&lexer);
		token_stream_add(stream, tok);
		if (tok.type == TOK_EOF) break;
	}

	return stream;
}

token_t* token_stream_peek(token_stream_t* stream, int offset)
{
	if (stream == NULL || stream->count == 0) return NULL;
	int idx = stream->cursor + offset;
	if (idx < 0) idx = 0;
	if (idx >= stream->count) return &stream->tokens[stream->count - 1]; /* return EOF token */
	return &stream->tokens[idx];
}

token_t* token_stream_current(token_stream_t* stream)
{
	return token_stream_peek(stream, 0);
}

token_t* token_stream_advance(token_stream_t* stream)
{
	if (stream == NULL || stream->cursor >= stream->count) return NULL;
	token_t* cur = &stream->tokens[stream->cursor];
	if (stream->cursor < stream->count - 1)
		stream->cursor++;
	return cur;
}

bool token_stream_match(token_stream_t* stream, token_type_t type)
{
	token_t* cur = token_stream_current(stream);
	if (cur != NULL && cur->type == type)
	{
		token_stream_advance(stream);
		return true;
	}
	return false;
}

bool token_stream_check(token_stream_t* stream, token_type_t type)
{
	token_t* cur = token_stream_current(stream);
	return (cur != NULL && cur->type == type);
}

void token_stream_skip_newlines(token_stream_t* stream)
{
	while (token_stream_check(stream, TOK_NEWLINE))
	{
		token_stream_advance(stream);
	}
}
