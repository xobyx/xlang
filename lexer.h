#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "xlang_main.h"

#ifndef FIND_STRUCT_DEF
#define FIND_STRUCT_DEF
typedef struct find
{
	bool isFind;
	char* bn;
	int size;
} find;
#endif

/* Token types for the standalone lexer (Phase 1 foundational / Phase 2 ready) */
typedef enum {
	TOK_EOF = 0,
	TOK_IDENT,
	TOK_INT,
	TOK_FLOAT,
	TOK_STRING,
	TOK_CHAR,
	TOK_BOOL,
	
	/* Keywords */
	TOK_KW_IF,
	TOK_KW_ELSE,
	TOK_KW_WHILE,
	TOK_KW_FOR,
	TOK_KW_DO,
	TOK_KW_RETURN,
	TOK_KW_CLASS,
	TOK_KW_BREAK,
	TOK_KW_CONTINUE,
	TOK_KW_IMPORT,
	
	/* Types */
	TOK_TYPE_INT,
	TOK_TYPE_FLOAT,
	TOK_TYPE_STRING,
	TOK_TYPE_CHAR,
	TOK_TYPE_BOOL,
	TOK_TYPE_VOID,
	
	/* Operators */
	TOK_PLUS,         /* + */
	TOK_MINUS,        /* - */
	TOK_STAR,         /* * */
	TOK_SLASH,        /* / */
	TOK_PERCENT,      /* % */
	TOK_ASSIGN,       /* = */
	TOK_PLUS_ASSIGN,  /* += */
	TOK_MINUS_ASSIGN, /* -= */
	TOK_STAR_ASSIGN,  /* *= */
	TOK_SLASH_ASSIGN, /* /= */
	TOK_EQ,           /* == */
	TOK_NEQ,          /* != */
	TOK_LT,           /* < */
	TOK_LTE,          /* <= */
	TOK_GT,           /* > */
	TOK_GTE,          /* >= */
	TOK_AND,          /* && */
	TOK_OR,           /* || */
	TOK_NOT,          /* ! */
	TOK_BIT_AND,      /* & */
	TOK_BIT_OR,       /* | */
	TOK_BIT_XOR,      /* ^ */
	TOK_BIT_NOT,      /* ~ */
	TOK_SHL,          /* << */
	TOK_SHR,          /* >> */
	TOK_ARROW,        /* -> */
	TOK_DOT,          /* . */
	TOK_COLON_COLON,  /* :: */
	
	/* Delimiters */
	TOK_LPAREN,       /* ( */
	TOK_RPAREN,       /* ) */
	TOK_LBRACE,       /* { */
	TOK_RBRACE,       /* } */
	TOK_LBRACKET,     /* [ */
	TOK_RBRACKET,     /* ] */
	TOK_COMMA,        /* , */
	TOK_SEMICOLON,    /* ; */
	TOK_COLON,        /* : */
	
	TOK_UNKNOWN
} token_type_t;

typedef struct {
	token_type_t type;
	const char* start;
	int length;
	char* text;
	int line;
	int col;
} token_t;

typedef struct {
	const char* source;
	const char* cursor;
	int line;
	int col;
} lexer_t;

/* Phase 1: Zero-breakage fast character-level scanner functions for parse.c */
find* lex_match_word(const char* buff);
find* lex_match_string(const char* buff);
find* lex_match_char(const char* buff);
find* lex_match_bool(const char* buff);
find* lex_match_number(const char* buff);
find* lex_match_var_name(const char* buff);

/* Standalone token stream lexer functions */
void lexer_init(lexer_t* lexer, const char* source);
token_t lexer_next_token(lexer_t* lexer);
const char* token_type_name(token_type_t type);
void token_free(token_t* tok);

#endif /* LEXER_H */
