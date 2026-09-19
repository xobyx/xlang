#pragma once
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum XDiagLevel {
	DIAG_ERROR = 0,
	DIAG_WARNING,
	DIAG_NOTE
} XDiagLevel;

/* Global or context-bound source file info */
void xdiag_set_current_file(const char* filepath);
const char* xdiag_get_current_file(void);
void xdiag_set_source_code(const char* src);

/* Extract a single 1-indexed line from source text */
bool xdiag_get_line_snippet(const char* source, int target_line, char* out_buf, size_t out_len);

/* Industrial diagnostic reporter with caret pointers and hints */
void xdiag_report(XDiagLevel level, const char* code, const char* file, int line, int col,
                  int token_len, const char* line_text, const char* message, const char* hint);

/* Convenience helpers */
void xdiag_error(const char* code, const char* file, int line, int col, int token_len,
                 const char* line_text, const char* fmt, ...);

void xdiag_warning(const char* code, const char* file, int line, int col, int token_len,
                   const char* line_text, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
