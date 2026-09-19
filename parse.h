#pragma once
#include "xlang_main.h"

typedef enum ScopeKind {
	SCOPE_GLOBAL = 0,
	SCOPE_CLASS,
	SCOPE_FUNCTION,
	SCOPE_BLOCK
} ScopeKind;

typedef struct ScopeEntry {
	ScopeKind kind;
	char name[128];
	node* opening_node;
	int line;
} ScopeEntry;

/* ParserContext: Dynamic, re-entrant parser state encapsulating delimiter stack,
 * scope stack, line tracking, and multi-line continuation pointers. */
typedef struct ParserContext {
	/* Dynamic Delimiter Stack (No 10-item limit!) */
	fl* delim_stack;
	int delim_capacity;

	/* Dynamic Scope Stack (Class / Function / Block nesting) */
	ScopeEntry* scope_stack;
	int scope_top;
	int scope_capacity;

	node* save;
	const char* current_parsing_class_name;
	int current_line;
	bool interactive;
	bool has_error;
	char error_msg[256];

	struct ParserContext* prev_ctx; /* Nested execution link (imports, eval) */
} ParserContext;

/* Active thread/call parser context pointer */
extern ParserContext* current_parser_ctx;

/* Parser context lifecycle */
ParserContext* parser_context_create(bool interactive);
void parser_context_init(ParserContext* ctx, bool interactive);
void parser_context_cleanup(ParserContext* ctx);
void parser_context_free(ParserContext* ctx);
void parser_interactive_cleanup(void);

/* Delimiter stack operations (auto-expanding, unlimited depth) */
node* parser_delim_op(ParserContext* ctx, const node_type mtype, node* w_node, bool added);
bool parser_delim_check_unclosed(ParserContext* ctx, node_type* m);
fl* parser_delim_get_first_unclosed(ParserContext* ctx);
bool parser_is_inside_func_param(ParserContext* ctx);
bool parser_has_open_brace(ParserContext* ctx);

/* Dynamic Scope Stack operations */
void parser_scope_push(ParserContext* ctx, ScopeKind kind, const char* name, node* opening_node, int line);
ScopeEntry* parser_scope_pop(ParserContext* ctx);
ScopeEntry* parser_scope_current(ParserContext* ctx);
const char* parser_current_class_name(ParserContext* ctx);
bool parser_is_in_class(ParserContext* ctx);
bool parser_is_in_function(ParserContext* ctx);

/* Token-level parsing entry points */
void parse_line(char* buff, node* n_node, const int line);
void parse_line_ctx(ParserContext* ctx, char* buff, node* n_node, const int line);

/* Line and buffer parsing entry points */
/*XLANG*/ void pre_parse_line(char* ts, const int linex);
/*XLANG*/ void pre_parse_line_ctx(ParserContext* ctx, char* ts, const int linex);
/*XLANG*/ void start_parse_lines(char* text, bool interactive);
/*XLANG*/ void start_parse_lines_ctx(ParserContext* ctx, char* text, bool interactive);
/*XLANG*/ char** get_lines_array(char* subject);