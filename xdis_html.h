#ifndef XDIS_HTML_H
#define XDIS_HTML_H

#include "xir.h"
#include "xast.h"

struct XVmTraceLog;

#ifdef __cplusplus
extern "C" {
#endif

/* Generates an advanced, interactive self-contained HTML compiler explorer
 * pairing source lines with bytecode IR, continuous linear disassembly,
 * LLVM IR, constant pool & symbol inspectors, opcode analytics, and
 * interactive VM execution trace stepping.
 *
 * chunk       — compiled bytecode chunk (main + nested functions)
 * prog        — parsed AST program (optional, enables LLVM IR tab; may be NULL)
 * source_path — path to the original .xb source file (may be NULL)
 * out_html    — output path for the generated HTML file
 * trace       — recorded VM execution trace (optional; may be NULL)
 *
 * Returns true on success.
 */
bool xdis_html_write_full(const XIrChunk* chunk, const AstProgram* prog,
                          const char* source_path, const char* out_html,
                          const struct XVmTraceLog* trace);

/* Convenience wrapper matching original signature (prog = NULL, trace = NULL) */
bool xdis_html_write(const XIrChunk* chunk, const char* source_path,
                     const char* out_html);

#ifdef __cplusplus
}
#endif

#endif /* XDIS_HTML_H */

