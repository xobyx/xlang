#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========================================================================= */
/* Phase 1: High-performance drop-in replacements for PCRE match() in parse.c */
/* ========================================================================= */

find* lex_match_word(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL || *buff == '\0')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	if (!isalnum((unsigned char)*buff) && *buff != '_')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	int len = 0;
	while (isalnum((unsigned char)buff[len]) || buff[len] == '_')
	{
		len++;
	}

	char* s = (char*)malloc(len + 1);
	memcpy(s, buff, len);
	s[len] = '\0';

	res->isFind = true;
	res->bn = s;
	res->size = len;
	return res;
}

find* lex_match_string(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL || *buff != '"')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	const char* p = buff + 1;
	while (*p != '\0')
	{
		if (*p == '\\')
		{
			p++;
			if (*p != '\0')
				p++;
		}
		else if (*p == '"')
		{
			/* Found matching end quote */
			int content_len = (int)(p - (buff + 1));
			char* s = (char*)malloc(content_len + 1);
			memcpy(s, buff + 1, content_len);
			s[content_len] = '\0';

			res->isFind = true;
			res->bn = s;
			res->size = content_len;
			return res;
		}
		else
		{
			p++;
		}
	}

	res->isFind = false;
	res->bn = "";
	return res;
}

find* lex_match_char(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL || *buff != '\'')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	const char* p = buff + 1;
	if (*p == '\\')
	{
		p++;
		if (*p != '\0')
			p++;
	}
	else if (*p != '\'' && *p != '\0')
	{
		p++;
	}

	if (*p == '\'')
	{
		int content_len = (int)(p - (buff + 1));
		if (content_len >= 1 && content_len <= 2)
		{
			char* s = (char*)malloc(content_len + 1);
			memcpy(s, buff + 1, content_len);
			s[content_len] = '\0';

			res->isFind = true;
			res->bn = s;
			res->size = content_len;
			return res;
		}
	}

	res->isFind = false;
	res->bn = "";
	return res;
}

find* lex_match_bool(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL)
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	if (strncmp(buff, "true", 4) == 0 && !isalnum((unsigned char)buff[4]) && buff[4] != '_')
	{
		char* s = (char*)malloc(5);
		strcpy(s, "true");
		res->isFind = true;
		res->bn = s;
		res->size = 4;
		return res;
	}

	if (strncmp(buff, "false", 5) == 0 && !isalnum((unsigned char)buff[5]) && buff[5] != '_')
	{
		char* s = (char*)malloc(6);
		strcpy(s, "false");
		res->isFind = true;
		res->bn = s;
		res->size = 5;
		return res;
	}

	res->isFind = false;
	res->bn = "";
	return res;
}

find* lex_match_number(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL)
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	const char* p = buff;
	if (*p == '+' || *p == '-')
	{
		p++;
	}

	bool has_digits = false;

	if (*p == '.')
	{
		p++;
		while (isdigit((unsigned char)*p))
		{
			has_digits = true;
			p++;
		}
		if (!has_digits)
		{
			res->isFind = false;
			res->bn = "";
			return res;
		}
	}
	else if (isdigit((unsigned char)*p))
	{
		has_digits = true;
		while (isdigit((unsigned char)*p))
		{
			p++;
		}
		if (*p == '.')
		{
			p++;
			while (isdigit((unsigned char)*p))
			{
				p++;
			}
		}
	}
	else
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	int total_len = (int)(p - buff);
	char* s = (char*)malloc(total_len + 1);
	memcpy(s, buff, total_len);
	s[total_len] = '\0';

	res->isFind = true;
	res->bn = s;
	res->size = total_len;
	return res;
}

find* lex_match_var_name(const char* buff)
{
	find* res = (find*)calloc(1, sizeof(find));
	if (buff == NULL || *buff == '\0')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	const char* p = buff;
	if (*p == '&')
	{
		p++;
	}

	if (!isalpha((unsigned char)*p) && *p != '_')
	{
		res->isFind = false;
		res->bn = "";
		return res;
	}

	while (isalnum((unsigned char)*p) || *p == '_')
	{
		p++;
	}

	int len = (int)(p - buff);
	char* s = (char*)malloc(len + 1);
	memcpy(s, buff, len);
	s[len] = '\0';

	res->isFind = true;
	res->bn = s;
	res->size = len;
	return res;
}

/* ========================================================================= */
/* Phase 2 Foundation: Standalone Tokenizer Implementation                   */
/* ========================================================================= */

void lexer_init(lexer_t* lexer, const char* source)
{
	if (lexer == NULL)
		return;
	lexer->source = source;
	lexer->cursor = source != NULL ? source : "";
	lexer->line = 1;
	lexer->col = 1;
}

static void skip_whitespace_and_comments(lexer_t* lexer)
{
	while (*lexer->cursor != '\0')
	{
		if (*lexer->cursor == ' ' || *lexer->cursor == '\t' || *lexer->cursor == '\r')
		{
			lexer->cursor++;
			lexer->col++;
		}
		else if (*lexer->cursor == '\n')
		{
			lexer->cursor++;
			lexer->line++;
			lexer->col = 1;
		}
		else if (*lexer->cursor == '#' || (*lexer->cursor == '/' && *(lexer->cursor + 1) == '/'))
		{
			/* Single-line comment */
			while (*lexer->cursor != '\0' && *lexer->cursor != '\n')
			{
				lexer->cursor++;
			}
		}
		else if (*lexer->cursor == '/' && *(lexer->cursor + 1) == '*')
		{
			/* Multi-line comment */
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
					lexer->line++;
					lexer->col = 1;
				}
				else
				{
					lexer->col++;
				}
				lexer->cursor++;
			}
		}
		else
		{
			break;
		}
	}
}

static token_t make_simple_token(lexer_t* lexer, token_type_t type, int len)
{
	token_t tok;
	tok.type = type;
	tok.start = lexer->cursor;
	tok.length = len;
	tok.line = lexer->line;
	tok.col = lexer->col;

	char* text = (char*)malloc(len + 1);
	memcpy(text, lexer->cursor, len);
	text[len] = '\0';
	tok.text = text;

	lexer->cursor += len;
	lexer->col += len;
	return tok;
}

static token_type_t check_keyword_or_type(const char* text, int len)
{
	if (len == 2)
	{
		if (strncmp(text, "if", 2) == 0) return TOK_KW_IF;
		if (strncmp(text, "do", 2) == 0) return TOK_KW_DO;
	}
	else if (len == 3)
	{
		if (strncmp(text, "for", 3) == 0) return TOK_KW_FOR;
		if (strncmp(text, "int", 3) == 0) return TOK_TYPE_INT;
	}
	else if (len == 4)
	{
		if (strncmp(text, "else", 4) == 0) return TOK_KW_ELSE;
		if (strncmp(text, "char", 4) == 0) return TOK_TYPE_CHAR;
		if (strncmp(text, "bool", 4) == 0) return TOK_TYPE_BOOL;
		if (strncmp(text, "void", 4) == 0) return TOK_TYPE_VOID;
		if (strncmp(text, "true", 4) == 0) return TOK_BOOL;
	}
	else if (len == 5)
	{
		if (strncmp(text, "while", 5) == 0) return TOK_KW_WHILE;
		if (strncmp(text, "class", 5) == 0) return TOK_KW_CLASS;
		if (strncmp(text, "break", 5) == 0) return TOK_KW_BREAK;
		if (strncmp(text, "float", 5) == 0) return TOK_TYPE_FLOAT;
		if (strncmp(text, "false", 5) == 0) return TOK_BOOL;
	}
	else if (len == 6)
	{
		if (strncmp(text, "return", 6) == 0) return TOK_KW_RETURN;
		if (strncmp(text, "import", 6) == 0) return TOK_KW_IMPORT;
		if (strncmp(text, "string", 6) == 0) return TOK_TYPE_STRING;
	}
	else if (len == 8)
	{
		if (strncmp(text, "continue", 8) == 0) return TOK_KW_CONTINUE;
	}
	return TOK_IDENT;
}

token_t lexer_next_token(lexer_t* lexer)
{
	skip_whitespace_and_comments(lexer);

	if (*lexer->cursor == '\0')
	{
		token_t tok;
		tok.type = TOK_EOF;
		tok.start = lexer->cursor;
		tok.length = 0;
		tok.text = strdup("");
		tok.line = lexer->line;
		tok.col = lexer->col;
		return tok;
	}

	const char c = *lexer->cursor;
	const char c2 = *(lexer->cursor + 1);

	/* Two-character operators */
	if (c == '=' && c2 == '=') return make_simple_token(lexer, TOK_EQ, 2);
	if (c == '!' && c2 == '=') return make_simple_token(lexer, TOK_NEQ, 2);
	if (c == '<' && c2 == '=') return make_simple_token(lexer, TOK_LTE, 2);
	if (c == '>' && c2 == '=') return make_simple_token(lexer, TOK_GTE, 2);
	if (c == '+' && c2 == '=') return make_simple_token(lexer, TOK_PLUS_ASSIGN, 2);
	if (c == '-' && c2 == '=') return make_simple_token(lexer, TOK_MINUS_ASSIGN, 2);
	if (c == '*' && c2 == '=') return make_simple_token(lexer, TOK_STAR_ASSIGN, 2);
	if (c == '/' && c2 == '=') return make_simple_token(lexer, TOK_SLASH_ASSIGN, 2);
	if (c == '&' && c2 == '&') return make_simple_token(lexer, TOK_AND, 2);
	if (c == '|' && c2 == '|') return make_simple_token(lexer, TOK_OR, 2);
	if (c == '-' && c2 == '>') return make_simple_token(lexer, TOK_ARROW, 2);
	if (c == ':' && c2 == ':') return make_simple_token(lexer, TOK_COLON_COLON, 2);
	if (c == '<' && c2 == '<') return make_simple_token(lexer, TOK_SHL, 2);
	if (c == '>' && c2 == '>') return make_simple_token(lexer, TOK_SHR, 2);

	/* Single-character operators and delimiters */
	switch (c)
	{
		case '+': return make_simple_token(lexer, TOK_PLUS, 1);
		case '-': return make_simple_token(lexer, TOK_MINUS, 1);
		case '*': return make_simple_token(lexer, TOK_STAR, 1);
		case '/': return make_simple_token(lexer, TOK_SLASH, 1);
		case '%': return make_simple_token(lexer, TOK_PERCENT, 1);
		case '=': return make_simple_token(lexer, TOK_ASSIGN, 1);
		case '<': return make_simple_token(lexer, TOK_LT, 1);
		case '>': return make_simple_token(lexer, TOK_GT, 1);
		case '!': return make_simple_token(lexer, TOK_NOT, 1);
		case '&': return make_simple_token(lexer, TOK_BIT_AND, 1);
		case '|': return make_simple_token(lexer, TOK_BIT_OR, 1);
		case '^': return make_simple_token(lexer, TOK_BIT_XOR, 1);
		case '~': return make_simple_token(lexer, TOK_BIT_NOT, 1);
		case '(': return make_simple_token(lexer, TOK_LPAREN, 1);
		case ')': return make_simple_token(lexer, TOK_RPAREN, 1);
		case '{': return make_simple_token(lexer, TOK_LBRACE, 1);
		case '}': return make_simple_token(lexer, TOK_RBRACE, 1);
		case '[': return make_simple_token(lexer, TOK_LBRACKET, 1);
		case ']': return make_simple_token(lexer, TOK_RBRACKET, 1);
		case ',': return make_simple_token(lexer, TOK_COMMA, 1);
		case ';': return make_simple_token(lexer, TOK_SEMICOLON, 1);
		case ':': return make_simple_token(lexer, TOK_COLON, 1);
		case '.': return make_simple_token(lexer, TOK_DOT, 1);
	}

	/* String literal */
	if (c == '"')
	{
		find* mf = lex_match_string(lexer->cursor);
		if (mf->isFind)
		{
			token_t tok;
			tok.type = TOK_STRING;
			tok.start = lexer->cursor;
			tok.length = mf->size + 2;
			tok.text = mf->bn;
			tok.line = lexer->line;
			tok.col = lexer->col;

			lexer->cursor += tok.length;
			lexer->col += tok.length;
			free(mf);
			return tok;
		}
		free(mf);
	}

	/* Char literal */
	if (c == '\'')
	{
		find* mf = lex_match_char(lexer->cursor);
		if (mf->isFind)
		{
			token_t tok;
			tok.type = TOK_CHAR;
			tok.start = lexer->cursor;
			tok.length = mf->size + 2;
			tok.text = mf->bn;
			tok.line = lexer->line;
			tok.col = lexer->col;

			lexer->cursor += tok.length;
			lexer->col += tok.length;
			free(mf);
			return tok;
		}
		free(mf);
	}

	/* Number literal */
	if (isdigit((unsigned char)c))
	{
		find* mf = lex_match_number(lexer->cursor);
		if (mf->isFind)
		{
			token_t tok;
			tok.type = (strstr(mf->bn, ".") != NULL) ? TOK_FLOAT : TOK_INT;
			tok.start = lexer->cursor;
			tok.length = mf->size;
			tok.text = mf->bn;
			tok.line = lexer->line;
			tok.col = lexer->col;

			lexer->cursor += tok.length;
			lexer->col += tok.length;
			free(mf);
			return tok;
		}
		free(mf);
	}

	/* Identifier or Keyword */
	if (isalpha((unsigned char)c) || c == '_')
	{
		int len = 0;
		while (isalnum((unsigned char)lexer->cursor[len]) || lexer->cursor[len] == '_')
		{
			len++;
		}
		char* text = (char*)malloc(len + 1);
		memcpy(text, lexer->cursor, len);
		text[len] = '\0';

		token_type_t tt = check_keyword_or_type(text, len);
		token_t tok;
		tok.type = tt;
		tok.start = lexer->cursor;
		tok.length = len;
		tok.text = text;
		tok.line = lexer->line;
		tok.col = lexer->col;

		lexer->cursor += len;
		lexer->col += len;
		return tok;
	}

	/* Unknown character */
	return make_simple_token(lexer, TOK_UNKNOWN, 1);
}

const char* token_type_name(token_type_t type)
{
	switch (type)
	{
		case TOK_EOF: return "EOF";
		case TOK_IDENT: return "IDENT";
		case TOK_INT: return "INT";
		case TOK_FLOAT: return "FLOAT";
		case TOK_STRING: return "STRING";
		case TOK_CHAR: return "CHAR";
		case TOK_BOOL: return "BOOL";
		case TOK_KW_IF: return "if";
		case TOK_KW_ELSE: return "else";
		case TOK_KW_WHILE: return "while";
		case TOK_KW_FOR: return "for";
		case TOK_KW_DO: return "do";
		case TOK_KW_RETURN: return "return";
		case TOK_KW_CLASS: return "class";
		case TOK_KW_BREAK: return "break";
		case TOK_KW_CONTINUE: return "continue";
		case TOK_KW_IMPORT: return "import";
		case TOK_TYPE_INT: return "int";
		case TOK_TYPE_FLOAT: return "float";
		case TOK_TYPE_STRING: return "string";
		case TOK_TYPE_CHAR: return "char";
		case TOK_TYPE_BOOL: return "bool";
		case TOK_TYPE_VOID: return "void";
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
