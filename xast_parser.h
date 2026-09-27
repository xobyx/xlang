#ifndef XAST_PARSER_H
#define XAST_PARSER_H

#include "xast.h"
#include "lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Build Structured AST from the token stream */
AstProgram* xast_parse_token_stream(AstArena* arena, token_stream_t* stream);

/* Parse into an existing AstProgram (used by imports) */
bool xast_parse_into_program(AstProgram* prog, token_stream_t* stream);

/* Parse an xlang source code string into a Structured AST */
AstProgram* xast_parse_source(const char* source_code, const char* filename);

/* Parse an xlang source file into a Structured AST */
AstProgram* xast_parse_file(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* XAST_PARSER_H */
