#include "xdiag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

static char g_current_file[512] = "<script>";
static char* g_source_code = NULL;

void xdiag_set_current_file(const char* filepath)
{
	if (filepath != NULL && filepath[0] != '\0')
	{
		strncpy(g_current_file, filepath, sizeof(g_current_file) - 1);
		g_current_file[sizeof(g_current_file) - 1] = '\0';
	}
	else
	{
		strncpy(g_current_file, "<interactive>", sizeof(g_current_file) - 1);
	}
}

const char* xdiag_get_current_file(void)
{
	return g_current_file;
}

void xdiag_set_source_code(const char* src)
{
	if (g_source_code != NULL)
	{
		free(g_source_code);
		g_source_code = NULL;
	}
	if (src != NULL)
	{
		size_t len = strlen(src);
		g_source_code = (char*)malloc(len + 1);
		if (g_source_code != NULL)
		{
			memcpy(g_source_code, src, len + 1);
		}
	}
}

bool xdiag_get_line_snippet(const char* source, int target_line, char* out_buf, size_t out_len)
{
	if (out_buf == NULL || out_len == 0)
		return false;
	out_buf[0] = '\0';

	const char* src = source != NULL ? source : g_source_code;
	if (src == NULL || target_line <= 0)
		return false;

	int cur_line = 1;
	const char* p = src;
	const char* line_start = src;

	while (*p != '\0')
	{
		if (cur_line == target_line)
		{
			line_start = p;
			while (*p != '\0' && *p != '\n' && *p != '\r')
			{
				p++;
			}
			size_t len = (size_t)(p - line_start);
			if (len >= out_len)
				len = out_len - 1;
			memcpy(out_buf, line_start, len);
			out_buf[len] = '\0';
			return true;
		}
		if (*p == '\n')
		{
			cur_line++;
		}
		p++;
	}

	return false;
}

static bool use_color(void)
{
	if (getenv("NO_COLOR") != NULL)
		return false;
	return isatty(fileno(stderr));
}

void xdiag_report(XDiagLevel level, const char* code, const char* file, int line, int col,
                  int token_len, const char* line_text, const char* message, const char* hint)
{
	bool color = use_color();

	const char* c_red = color ? "\033[1;31m" : "";
	const char* c_yellow = color ? "\033[1;33m" : "";
	const char* c_blue = color ? "\033[1;36m" : "";
	const char* c_bold = color ? "\033[1m" : "";
	const char* c_reset = color ? "\033[0m" : "";

	const char* level_name = "error";
	const char* level_color = c_red;
	if (level == DIAG_WARNING)
	{
		level_name = "warning";
		level_color = c_yellow;
	}
	else if (level == DIAG_NOTE)
	{
		level_name = "note";
		level_color = c_blue;
	}

	const char* fpath = (file != NULL && file[0] != '\0') ? file : g_current_file;
	int safe_line = line > 0 ? line : 1;
	int safe_col = col > 0 ? col : 1;
	int safe_tlen = token_len > 0 ? token_len : 1;

	/* Header: error[E0001]: message */
	if (code != NULL && code[0] != '\0')
	{
		fprintf(stderr, "%s%s[%s]%s: %s%s\n", level_color, level_name, code, c_bold, message, c_reset);
	}
	else
	{
		fprintf(stderr, "%s%s%s: %s%s\n", level_color, level_name, c_bold, message, c_reset);
	}

	/* Location: --> file:line:col */
	fprintf(stderr, "  %s-->%s %s:%d:%d\n", c_blue, c_reset, fpath, safe_line, safe_col);

	/* Line snippet lookup if not provided */
	char snippet_buf[1024];
	const char* snippet = line_text;
	if ((snippet == NULL || snippet[0] == '\0') && g_source_code != NULL)
	{
		if (xdiag_get_line_snippet(g_source_code, safe_line, snippet_buf, sizeof(snippet_buf)))
		{
			snippet = snippet_buf;
		}
	}

	if (snippet != NULL && snippet[0] != '\0')
	{
		/* Gutter and source line */
		fprintf(stderr, "   %s|%s\n", c_blue, c_reset);
		fprintf(stderr, "%s%3d |%s %s\n", c_blue, safe_line, c_reset, snippet);

		/* Caret underline:   |     ^~~~~ */
		fprintf(stderr, "   %s|%s ", c_blue, c_reset);
		for (int i = 1; i < safe_col; i++)
		{
			if (i - 1 < (int)strlen(snippet) && snippet[i - 1] == '\t')
				fputc('\t', stderr);
			else
				fputc(' ', stderr);
		}
		fprintf(stderr, "%s^", level_color);
		for (int i = 1; i < safe_tlen; i++)
		{
			fputc('~', stderr);
		}
		fprintf(stderr, "%s\n", c_reset);
	}

	/* Optional hint */
	if (hint != NULL && hint[0] != '\0')
	{
		fprintf(stderr, "   %s=%s %shint%s: %s\n", c_blue, c_reset, c_yellow, c_reset, hint);
	}
	fprintf(stderr, "\n");
}

void xdiag_error(const char* code, const char* file, int line, int col, int token_len,
                 const char* line_text, const char* fmt, ...)
{
	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	xdiag_report(DIAG_ERROR, code, file, line, col, token_len, line_text, msg, NULL);
}

void xdiag_warning(const char* code, const char* file, int line, int col, int token_len,
                   const char* line_text, const char* fmt, ...)
{
	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	xdiag_report(DIAG_WARNING, code, file, line, col, token_len, line_text, msg, NULL);
}
