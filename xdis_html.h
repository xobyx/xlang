#ifndef XDIS_HTML_H
#define XDIS_HTML_H

#include "xir.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Generates a self-contained HTML file pairing source lines with their
 * bytecode IR instructions side by side.
 *
 * chunk       — compiled bytecode chunk (main + nested functions)
 * source_path — path to the original .xb source file (may be NULL)
 * out_html    — output path for the generated HTML file
 *
 * Returns true on success.
 */
bool xdis_html_write(const XIrChunk* chunk, const char* source_path,
                     const char* out_html);

#ifdef __cplusplus
}
#endif

#endif /* XDIS_HTML_H */
