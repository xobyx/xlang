#ifndef XAST_PARSER_H
#define XAST_PARSER_H

#include "xast.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Build Structured AST from the token node stream (nodes->root) */
AstProgram* xast_parse_node_stream(AstArena* arena, node* start_node, node* end_node);

/* Parse an xlang source code string into a Structured AST */
AstProgram* xast_parse_source(const char* source_code, const char* filename);

/* Parse an xlang source file into a Structured AST */
AstProgram* xast_parse_file(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* XAST_PARSER_H */
