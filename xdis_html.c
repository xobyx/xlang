/* xdis_html.c — Side-by-side .xb source / XBC bytecode HTML viewer
 *
 * Generates a self-contained HTML file from a compiled XIrChunk, correlating
 * each bytecode instruction back to its source line number so source and
 * disassembly are shown side by side with bi-directional highlighting.
 *
 * Entry point:
 *   bool xdis_html_write(const XIrChunk* chunk, const char* source_path,
 *                        const char* out_html);
 */
#include "xdis_html.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* --------------------------------------------------------------------------
 * Internal helpers — decode one instruction into a human-readable string
 * -------------------------------------------------------------------------- */
static int decode_instruction(const XIrChunk* chunk, int offset,
                               int* out_line, char* buf, int buf_sz)
{
	if (offset >= chunk->count) return -1;

	int line = chunk->lines[offset];
	if (out_line) *out_line = line;

	uint8_t opcode = chunk->code[offset];

	/* Get the opcode name from the canonical helper in xir.c */
	const char* opname = xir_opcode_name((XIrOpCode)opcode);

	char ops[256] = "";
	int next = offset + 1;

#define PEEK2() ((uint16_t)((chunk->code[next] << 8) | chunk->code[next+1]))
#define PEEK4() ((int32_t)((chunk->code[next]<<24)|(chunk->code[next+1]<<16)|(chunk->code[next+2]<<8)|chunk->code[next+3]))
#define SYM(i)  ((i) < chunk->symbols.count ? chunk->symbols.symbols[(i)] : "?")

	switch ((XIrOpCode)opcode)
	{
	/* No operands — single byte instructions */
	case OP_NOP:
	case OP_CONST_NULL: case OP_CONST_TRUE: case OP_CONST_FALSE:
	case OP_POP: case OP_DUP:
	case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD: case OP_NEG:
	case OP_BIT_AND: case OP_BIT_OR: case OP_BIT_XOR: case OP_BIT_NOT:
	case OP_SHL: case OP_SHR:
	case OP_EQ: case OP_NEQ: case OP_LT: case OP_LTE: case OP_GT: case OP_GTE: case OP_NOT:
	case OP_LOAD_INDEX: case OP_STORE_INDEX:
	case OP_RETURN: case OP_HALT: case OP_CLOSE_UPVALUE:
		next = offset + 1;
		break;

	case OP_CONST_INT: {
		int32_t v = PEEK4(); snprintf(ops, sizeof(ops), "%d", v); next += 4; break;
	}

	case OP_CONST_FLOAT:
	case OP_CONST_STR: {
		uint16_t idx = PEEK2();
		if (idx < chunk->constants.count) {
			XValue val = chunk->constants.values[idx];
			if (val.type == VAL_STRING && val.as.sval)
				snprintf(ops, sizeof(ops), "\"%s\"", val.as.sval);
			else if (val.type == VAL_FLOAT)
				snprintf(ops, sizeof(ops), "%g", val.as.fval);
			else
				snprintf(ops, sizeof(ops), "[%d]", idx);
		}
		next += 2; break;
	}

	case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL:
	case OP_LOAD_FIELD:  case OP_STORE_FIELD: {
		uint16_t idx = PEEK2(); snprintf(ops, sizeof(ops), "%s", SYM(idx)); next += 2; break;
	}

	case OP_GET_FIELD_INDEX: case OP_SET_FIELD_INDEX: {
		uint16_t slot = PEEK2(); snprintf(ops, sizeof(ops), "slot [%d]", slot); next += 2; break;
	}

	case OP_CLASS: {
		uint16_t si = PEEK2();
		uint16_t fc = (chunk->code[next+4] << 8) | chunk->code[next+5];
		snprintf(ops, sizeof(ops), "%s (%d fields)", SYM(si), fc);
		next += 6 + fc * 2; break;
	}

	case OP_METHOD: {
		uint16_t cs = PEEK2(); uint16_t ms = (chunk->code[next+2] << 8) | chunk->code[next+3];
		snprintf(ops, sizeof(ops), "%s.%s", SYM(cs), SYM(ms)); next += 5; break;
	}

	case OP_LOAD_LOCAL: case OP_STORE_LOCAL: {
		uint16_t slot = PEEK2(); snprintf(ops, sizeof(ops), "slot %d", slot); next += 2; break;
	}

	case OP_BUILD_LIST: {
		uint16_t cnt = PEEK2(); snprintf(ops, sizeof(ops), "%d items", cnt); next += 2; break;
	}

	case OP_NEW_INSTANCE: {
		uint16_t si = PEEK2(); uint8_t ac = chunk->code[next+2];
		snprintf(ops, sizeof(ops), "%s (%d args)", SYM(si), ac); next += 3; break;
	}

	case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE: case OP_LOOP: {
		uint16_t tgt = PEEK2(); snprintf(ops, sizeof(ops), "-> %04d", tgt); next += 2; break;
	}

	case OP_CALL: {
		uint16_t si = PEEK2(); uint8_t ac = chunk->code[next+2];
		snprintf(ops, sizeof(ops), "%s (%d args)", si == 0xFFFF ? "<stack>" : SYM(si), ac);
		next += 3; break;
	}

	case OP_CALL_METHOD: {
		uint16_t si = PEEK2(); uint8_t ac = chunk->code[next+2];
		snprintf(ops, sizeof(ops), ".%s (%d args)", SYM(si), ac); next += 3; break;
	}

	case OP_PRINT: {
		uint8_t ac = chunk->code[next]; snprintf(ops, sizeof(ops), "%d args", ac); next += 1; break;
	}

	case OP_CLOSURE: {
		uint16_t ci = PEEK2();
		if (ci < chunk->constants.count && chunk->constants.values[ci].type == VAL_FUNCTION) {
			XFunction* fn = chunk->constants.values[ci].as.fnval;
			snprintf(ops, sizeof(ops), "<%s arity=%d>", fn->name ? fn->name : "fn", fn->arity);
			next += 2 + fn->upvalue_count * 2;
		} else { next += 2; }
		break;
	}

	case OP_GET_UPVALUE: case OP_SET_UPVALUE: {
		uint8_t slot = chunk->code[next]; snprintf(ops, sizeof(ops), "upv %d", slot); next += 1; break;
	}

	default: next = offset + 1; break;
	}

#undef PEEK2
#undef PEEK4
#undef SYM

	snprintf(buf, buf_sz, "<span class=\"op\">%-22s</span><span class=\"operand\">%s</span>",
	         opname, ops);
	return next;
}

/* --------------------------------------------------------------------------
 * HTML-escape a string into dst buffer
 * -------------------------------------------------------------------------- */
static void html_escape(const char* src, char* dst, int dst_sz)
{
	int j = 0;
	for (const char* p = src; *p && j < dst_sz - 8; p++)
	{
		switch (*p)
		{
		case '&':  memcpy(dst+j,"&amp;", 5);  j+=5; break;
		case '<':  memcpy(dst+j,"&lt;",  4);  j+=4; break;
		case '>':  memcpy(dst+j,"&gt;",  4);  j+=4; break;
		case '"':  memcpy(dst+j,"&quot;",6);  j+=6; break;
		default:   dst[j++] = *p; break;
		}
	}
	dst[j] = '\0';
}

/* --------------------------------------------------------------------------
 * Read source file into an array of lines
 * -------------------------------------------------------------------------- */
static char** read_source_lines(const char* path, int* count_out)
{
	*count_out = 0;
	FILE* f = fopen(path, "r");
	if (!f) return NULL;

	/* count lines */
	int lines = 0;
	char ch;
	while ((ch = (char)fgetc(f)) != EOF) if (ch == '\n') lines++;
	lines++; /* last line might not end with \n */
	rewind(f);

	char** arr = (char**)calloc(lines + 1, sizeof(char*));
	char line_buf[4096];
	int idx = 0;
	while (fgets(line_buf, sizeof(line_buf), f) && idx < lines)
	{
		/* strip trailing newline */
		size_t ln = strlen(line_buf);
		while (ln > 0 && (line_buf[ln-1] == '\n' || line_buf[ln-1] == '\r')) line_buf[--ln] = '\0';
		arr[idx++] = strdup(line_buf);
	}
	fclose(f);
	*count_out = idx;
	return arr;
}

/* --------------------------------------------------------------------------
 * Collect all instructions for each source line
 * -------------------------------------------------------------------------- */
typedef struct InstrList {
	char** instrs;
	int count;
	int cap;
} InstrList;

static void instr_append(InstrList* il, const char* s)
{
	if (il->count >= il->cap)
	{
		il->cap = il->cap == 0 ? 8 : il->cap * 2;
		il->instrs = (char**)realloc(il->instrs, il->cap * sizeof(char*));
	}
	il->instrs[il->count++] = strdup(s);
}

static void instr_free(InstrList* il)
{
	for (int i = 0; i < il->count; i++) free(il->instrs[i]);
	free(il->instrs);
}

/* Collect instructions from chunk (and recurse into nested functions) */
static void collect_chunk_instrs(const XIrChunk* chunk, InstrList* by_line,
                                  int max_line, const char* fn_label)
{
	char labeled[640];
	int offset = 0;
	while (offset < chunk->count)
	{
		int line = 0;
		char opbuf[256] = "";
		int next = decode_instruction(chunk, offset, &line, opbuf, sizeof(opbuf));
		if (next <= offset) break;

		if (line >= 1 && line <= max_line)
		{
			if (fn_label && fn_label[0])
				snprintf(labeled, sizeof(labeled), "<span class=\"fn-label\">[%s]</span> %04d %s",
				         fn_label, offset, opbuf);
			else
				snprintf(labeled, sizeof(labeled), "%04d %s", offset, opbuf);
			instr_append(&by_line[line], labeled);
		}
		offset = next;
	}

	/* Recurse into nested function constants */
	for (int i = 0; i < chunk->constants.count; i++)
	{
		if (chunk->constants.values[i].type == VAL_FUNCTION &&
		    chunk->constants.values[i].as.fnval)
		{
			XFunction* fn = chunk->constants.values[i].as.fnval;
			collect_chunk_instrs(&fn->chunk, by_line, max_line,
			                     fn->name ? fn->name : "fn");
		}
	}
}

/* --------------------------------------------------------------------------
 * Main public entry point
 * -------------------------------------------------------------------------- */
bool xdis_html_write(const XIrChunk* chunk, const char* source_path,
                     const char* out_html)
{
	if (!chunk || !out_html) return false;

	/* Read source */
	int src_line_count = 0;
	char** src_lines = source_path ? read_source_lines(source_path, &src_line_count) : NULL;
	if (!src_lines || src_line_count == 0)
	{
		/* No source — just disassemble without pairing */
		src_line_count = 0;
	}

	/* Collect instructions per source line */
	InstrList* by_line = (InstrList*)calloc(src_line_count + 2, sizeof(InstrList));
	collect_chunk_instrs(chunk, by_line, src_line_count, NULL);

	/* Write HTML */
	FILE* out = fopen(out_html, "w");
	if (!out)
	{
		for (int i = 0; i < src_line_count; i++) if (src_lines[i]) free(src_lines[i]);
		free(src_lines);
		for (int i = 0; i <= src_line_count; i++) instr_free(&by_line[i]);
		free(by_line);
		return false;
	}

	const char* src_basename = source_path ? source_path : "unknown";
	/* Find last path separator */
	const char* b = strrchr(src_basename, '/');
	if (!b) b = strrchr(src_basename, '\\');
	if (b) src_basename = b + 1;

	fprintf(out,
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<title>xlang IR Viewer — %s</title>\n"
"<style>\n"
"  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
"  body { font-family: 'Cascadia Code', 'Fira Code', Consolas, monospace;\n"
"         font-size: 13px; background: #1e1e2e; color: #cdd6f4; }\n"
"  header { background: #181825; border-bottom: 1px solid #313244;\n"
"           padding: 10px 20px; display: flex; align-items: center; gap: 16px; }\n"
"  header h1 { font-size: 16px; color: #89dceb; }\n"
"  header span { color: #6c7086; font-size: 12px; }\n"
"  .badge { background: #313244; border-radius: 4px; padding: 2px 8px;\n"
"           font-size: 11px; color: #a6adc8; }\n"
"  .container { display: grid; grid-template-columns: 1fr 1fr;\n"
"               height: calc(100vh - 49px); }\n"
"  .pane { overflow-y: auto; }\n"
"  .pane-header { position: sticky; top: 0; background: #1e1e2e;\n"
"                 border-bottom: 1px solid #313244; padding: 6px 12px;\n"
"                 font-size: 11px; color: #6c7086; text-transform: uppercase;\n"
"                 letter-spacing: 1px; z-index: 10; }\n"
"  #src-pane { border-right: 1px solid #313244; }\n"
"  .src-row { display: flex; align-items: flex-start;\n"
"             border-bottom: 1px solid #1a1a2e; cursor: pointer;\n"
"             transition: background 0.1s; }\n"
"  .src-row:hover { background: #2a2a3e !important; }\n"
"  .src-row.active { background: #303050 !important; }\n"
"  .src-row.has-code { border-left: 3px solid #45475a; }\n"
"  .src-row.has-code:hover { border-left-color: #89b4fa; }\n"
"  .src-row.active.has-code { border-left-color: #89b4fa; }\n"
"  .line-num { width: 44px; min-width: 44px; padding: 3px 8px;\n"
"              text-align: right; color: #45475a; user-select: none;\n"
"              font-size: 11px; line-height: 20px; }\n"
"  .src-code { padding: 3px 8px; white-space: pre; color: #cdd6f4;\n"
"              line-height: 20px; flex: 1; }\n"
"  .ir-section { padding: 4px 12px; border-bottom: 1px solid #1a1a2e; }\n"
"  .ir-section.active { background: #25253a; }\n"
"  .ir-label { font-size: 11px; color: #6c7086; padding: 2px 0; }\n"
"  .ir-instr { padding: 1px 0 1px 12px; line-height: 20px; }\n"
"  .ir-instr:hover { background: #2a2a3e; }\n"
"  .op { color: #89b4fa; min-width: 200px; display: inline-block; }\n"
"  .operand { color: #a6e3a1; }\n"
"  .fn-label { color: #f38ba8; font-size: 10px; }\n"
"  .offset { color: #45475a; margin-right: 6px; }\n"
"  .empty-ir { color: #45475a; font-size: 11px; padding: 3px 12px 3px 24px; }\n"
"  ::-webkit-scrollbar { width: 8px; height: 8px; }\n"
"  ::-webkit-scrollbar-track { background: #181825; }\n"
"  ::-webkit-scrollbar-thumb { background: #45475a; border-radius: 4px; }\n"
"  .stats { display: flex; gap: 12px; margin-left: auto; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<header>\n"
"  <h1>&#x1F9EE; xlang IR Viewer</h1>\n"
"  <span class=\"badge\">%s</span>\n"
"  <span class=\"badge\">%d source lines</span>\n"
"  <span class=\"badge\">%d bytecode bytes</span>\n"
"  <span style=\"margin-left:auto;color:#6c7086;font-size:11px\">"
         "Click a source line to highlight its bytecode &bull; "
         "Hover to see correspondence</span>\n"
"</header>\n"
"<div class=\"container\">\n"
"  <div class=\"pane\" id=\"src-pane\">\n"
"    <div class=\"pane-header\">&#x1F4C4; Source: %s</div>\n",
	         src_basename, src_basename,
	         src_line_count, chunk->count,
	         src_basename);

	/* Source pane rows */
	for (int ln = 1; ln <= src_line_count; ln++)
	{
		const char* raw = src_lines[ln-1] ? src_lines[ln-1] : "";
		char esc[4096];
		html_escape(raw, esc, sizeof(esc));
		int has = by_line[ln].count > 0;
		fprintf(out,
		        "    <div class=\"src-row%s%s\" data-line=\"%d\" onclick=\"selectLine(%d)\" "
		        "onmouseenter=\"hoverLine(%d)\" onmouseleave=\"unhover()\">\n"
		        "      <span class=\"line-num\">%d</span>\n"
		        "      <span class=\"src-code\">%s</span>\n"
		        "    </div>\n",
		        has ? " has-code" : "",
		        ln == 1 ? " active" : "",
		        ln, ln, ln, ln, esc);
	}

	fprintf(out,
"  </div>\n"
"  <div class=\"pane\" id=\"ir-pane\">\n"
"    <div class=\"pane-header\">&#x26A1; Bytecode IR</div>\n");

	/* IR pane sections — one per source line */
	for (int ln = 1; ln <= src_line_count; ln++)
	{
		InstrList* il = &by_line[ln];
		fprintf(out,
		        "    <div class=\"ir-section%s\" id=\"ir-line-%d\">\n"
		        "      <div class=\"ir-label\">Line %d</div>\n",
		        ln == 1 ? " active" : "", ln, ln);

		if (il->count == 0)
		{
			fprintf(out, "      <div class=\"empty-ir\">&mdash;</div>\n");
		}
		else
		{
			for (int k = 0; k < il->count; k++)
			{
				fprintf(out, "      <div class=\"ir-instr\">%s</div>\n", il->instrs[k]);
			}
		}
		fprintf(out, "    </div>\n");
	}

	fprintf(out,
"  </div>\n"
"</div>\n"
"<script>\n"
"let activeLine = 1;\n"
"function selectLine(n) {\n"
"  document.querySelectorAll('.src-row.active').forEach(e => e.classList.remove('active'));\n"
"  document.querySelectorAll('.ir-section.active').forEach(e => e.classList.remove('active'));\n"
"  const sr = document.querySelector('.src-row[data-line=\"'+n+'\"]');\n"
"  const ir = document.getElementById('ir-line-'+n);\n"
"  if (sr) sr.classList.add('active');\n"
"  if (ir) { ir.classList.add('active'); ir.scrollIntoView({behavior:'smooth',block:'nearest'}); }\n"
"  activeLine = n;\n"
"}\n"
"function hoverLine(n) {\n"
"  document.querySelectorAll('.ir-section').forEach(e => e.style.outline='');\n"
"  const ir = document.getElementById('ir-line-'+n);\n"
"  if (ir) ir.style.outline = '1px solid #89b4fa';\n"
"}\n"
"function unhover() {\n"
"  document.querySelectorAll('.ir-section').forEach(e => e.style.outline='');\n"
"}\n"
"/* Keyboard navigation */\n"
"document.addEventListener('keydown', e => {\n"
"  if (e.key === 'ArrowDown') { selectLine(Math.min(activeLine+1, %d)); e.preventDefault(); }\n"
"  if (e.key === 'ArrowUp')   { selectLine(Math.max(activeLine-1, 1));  e.preventDefault(); }\n"
"});\n"
"selectLine(1);\n"
"</script>\n"
"</body>\n"
"</html>\n",
	src_line_count);

	fclose(out);

	/* Cleanup */
	for (int i = 0; i < src_line_count; i++) if (src_lines[i]) free(src_lines[i]);
	free(src_lines);
	for (int i = 0; i <= src_line_count; i++) instr_free(&by_line[i]);
	free(by_line);

	return true;
}
