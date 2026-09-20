#ifndef XIR_COMPILER_H
#define XIR_COMPILER_H

#include "xast.h"
#include "xir.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compiles a Structured AST Program into a Bytecode Chunk */
bool xir_compile_program(const AstProgram* prog, XIrChunk* out_chunk);

/* Compiles a standalone AST Statement */
bool xir_compile_stmt(const AstStmt* stmt, XIrChunk* out_chunk);

/* Compiles a standalone AST Expression */
bool xir_compile_expr(const AstExpr* expr, XIrChunk* out_chunk);

#ifdef __cplusplus
}
#endif

#endif /* XIR_COMPILER_H */
