#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TOK_EOF = 0,
	TOK_NEWLINE,
	TOK_IDENT,
	TOK_INT,
	TOK_FLOAT,
	TOK_STRING,
	TOK_CHAR,
	TOK_BOOL,
	TOK_NULL,

	/* Keywords */
	TOK_KW_IF,
	TOK_KW_ELSE,
	TOK_KW_EIF,
	TOK_KW_WHILE,
	TOK_KW_FOR,
	TOK_KW_DO,
	TOK_KW_RETURN,
	TOK_KW_CLASS,
	TOK_KW_BREAK,
	TOK_KW_CONTINUE,
	TOK_KW_IMPORT,
	TOK_KW_STATIC,
	TOK_KW_NEW,
	TOK_KW_IN,
	TOK_KW_EXTERN,

	/* Types */
	TOK_TYPE_INT,
	TOK_TYPE_FLOAT,
	TOK_TYPE_STRING,
	TOK_TYPE_CHAR,
	TOK_TYPE_BOOL,
	TOK_TYPE_VOID,
	TOK_TYPE_LONG,
	TOK_TYPE_DOUBLE,

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
	TOK_MOD_ASSIGN,   /* %= */
	TOK_INC,          /* ++ */
	TOK_DEC,          /* -- */
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
	char* text;
	int length;
	int line;
	int col;
	int64_t int_val;
	double float_val;
} token_t;

typedef struct {
	const char* source;
	const char* cursor;
	int line;
	int col;
	bool last_was_newline;
} lexer_t;

typedef struct {
	token_t* tokens;
	int count;
	int capacity;
	int cursor;
} token_stream_t;

/* Token functions */
const char* token_type_name(token_type_t type);
void token_free(token_t* tok);

/* Lexer functions */
void lexer_init(lexer_t* lexer, const char* source);
token_t lexer_next_token(lexer_t* lexer);

/* Token stream functions */
void token_stream_init(token_stream_t* stream);
void token_stream_free(token_stream_t* stream);
void token_stream_add(token_stream_t* stream, token_t tok);
token_stream_t* token_stream_tokenize(const char* source);

token_t* token_stream_peek(token_stream_t* stream, int offset);
token_t* token_stream_current(token_stream_t* stream);
token_t* token_stream_advance(token_stream_t* stream);
bool token_stream_match(token_stream_t* stream, token_type_t type);
bool token_stream_check(token_stream_t* stream, token_type_t type);
void token_stream_skip_newlines(token_stream_t* stream);

#ifdef __cplusplus
}
#endif

#endif /* LEXER_H */
