/* xdis_html.c — Advanced Interactive Compiler Explorer & IR Viewer for xlang
 *
 * Generates a self-contained, offline HTML visualizer featuring:
 *   1. Split View: Source code side-by-side with correlated Bytecode IR instructions.
 *   2. Linear Bytecode Stream: Continuous disassembly with hex dump, opcodes, operands, and jump links.
 *   3. AST Tree & Inspector: Interactive expandable syntax tree and formatted AST text dump.
 *   4. LLVM IR: Formatted and syntax-highlighted typed LLVM IR (.ll) representation.
 *   5. Constants & Symbols Inspector: Interactive tables of constant pool values and symbol tables.
 *   6. Opcode Analytics: Bytecode instruction frequency breakdown and category distribution.
 *   7. Interactive controls: Clickable jump targets, bi-directional line highlighting, live search & filter,
 *      draggable split pane, multiple themes (Catppuccin Mocha, Tokyo Night, Light), and font zoom.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "xdis_html.h"
#include "xllvm.h"
#include "xvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>

/* --------------------------------------------------------------------------
 * Opcode Metadata & Categorization
 * -------------------------------------------------------------------------- */
typedef struct OpcodeInfo {
	XIrOpCode op;
	const char* name;
	const char* category;       /* CSS class: flow, math, bitwise, logic, var, obj, coll, call, const, stack, system */
	const char* category_label; /* Human readable */
	const char* stack_eff;      /* e.g. [a, b] -> [a + b] */
	const char* desc;           /* Concise runtime description */
} OpcodeInfo;

static const OpcodeInfo OP_METAS[] = {
	{ OP_NOP,             "OP_NOP",             "stack",   "Stack / Spacer", "[] -> []",                  "No operation; VM spacer or alignment padding" },
	{ OP_CONST_NULL,      "OP_CONST_NULL",      "const",   "Constants",      "[] -> [null]",              "Push null value onto the evaluation stack" },
	{ OP_CONST_TRUE,      "OP_CONST_TRUE",      "const",   "Constants",      "[] -> [true]",              "Push boolean true onto the evaluation stack" },
	{ OP_CONST_FALSE,     "OP_CONST_FALSE",     "const",   "Constants",      "[] -> [false]",             "Push boolean false onto the evaluation stack" },
	{ OP_CONST_INT,       "OP_CONST_INT",       "const",   "Constants",      "[] -> [int]",               "Push 32-bit signed integer immediate onto stack" },
	{ OP_CONST_FLOAT,     "OP_CONST_FLOAT",     "const",   "Constants",      "[] -> [float]",             "Push 64-bit float from constant pool onto stack" },
	{ OP_CONST_STR,       "OP_CONST_STR",       "const",   "Constants",      "[] -> [str]",               "Push string literal from constant pool onto stack" },

	{ OP_LOAD_GLOBAL,     "OP_LOAD_GLOBAL",     "var",     "Variables",      "[] -> [val]",               "Load global variable value onto stack by symbol" },
	{ OP_STORE_GLOBAL,    "OP_STORE_GLOBAL",    "var",     "Variables",      "[val] -> [val]",            "Assign top of stack to global variable" },
	{ OP_LOAD_LOCAL,      "OP_LOAD_LOCAL",      "var",     "Variables",      "[] -> [val]",               "Load local variable from frame stack slot" },
	{ OP_STORE_LOCAL,     "OP_STORE_LOCAL",     "var",     "Variables",      "[val] -> [val]",            "Assign top of stack to local frame stack slot" },

	{ OP_LOAD_FIELD,      "OP_LOAD_FIELD",      "obj",     "Objects",        "[obj] -> [field]",          "Load object field property by symbol name" },
	{ OP_STORE_FIELD,     "OP_STORE_FIELD",     "obj",     "Objects",        "[obj, val] -> [val]",       "Assign value to object field property by symbol name" },
	{ OP_GET_FIELD_INDEX, "OP_GET_FIELD_INDEX", "obj",     "Objects",        "[obj] -> [field]",          "Fast direct property access by struct field slot" },
	{ OP_SET_FIELD_INDEX, "OP_SET_FIELD_INDEX", "obj",     "Objects",        "[obj, val] -> [val]",       "Fast direct property write by struct field slot" },

	{ OP_LOAD_INDEX,      "OP_LOAD_INDEX",      "coll",    "Collections",    "[coll, idx] -> [elem]",     "Index subscript access on array, string, or map" },
	{ OP_STORE_INDEX,     "OP_STORE_INDEX",     "coll",    "Collections",    "[coll, idx, val] -> [val]", "Index subscript assignment on array or map" },
	{ OP_BUILD_LIST,      "OP_BUILD_LIST",      "coll",    "Collections",    "[e0..eN-1] -> [list]",      "Construct new dynamic list from N stack elements" },
	{ OP_NEW_INSTANCE,    "OP_NEW_INSTANCE",    "obj",     "Objects",        "[arg0..argN-1] -> [inst]",  "Instantiate class, invoke constructor, push instance" },
	{ OP_CLASS,           "OP_CLASS",           "obj",     "Objects",        "[] -> []",                  "Define class layout and field descriptor table" },
	{ OP_METHOD,          "OP_METHOD",          "obj",     "Objects",        "[fn] -> []",                "Register method function in class method table" },

	{ OP_POP,             "OP_POP",             "stack",   "Stack Ops",      "[val] -> []",               "Pop and discard the top evaluation stack value" },
	{ OP_DUP,             "OP_DUP",             "stack",   "Stack Ops",      "[val] -> [val, val]",       "Duplicate the top value on the evaluation stack" },

	{ OP_ADD,             "OP_ADD",             "math",    "Arithmetic",     "[a, b] -> [a + b]",         "Addition or string concatenation" },
	{ OP_SUB,             "OP_SUB",             "math",    "Arithmetic",     "[a, b] -> [a - b]",         "Arithmetic subtraction" },
	{ OP_MUL,             "OP_MUL",             "math",    "Arithmetic",     "[a, b] -> [a * b]",         "Arithmetic multiplication" },
	{ OP_DIV,             "OP_DIV",             "math",    "Arithmetic",     "[a, b] -> [a / b]",         "Arithmetic division" },
	{ OP_MOD,             "OP_MOD",             "math",    "Arithmetic",     "[a, b] -> [a % b]",         "Modulo remainder" },
	{ OP_NEG,             "OP_NEG",             "math",    "Arithmetic",     "[a] -> [-a]",               "Arithmetic unary negation" },

	{ OP_BIT_AND,         "OP_BIT_AND",         "bitwise", "Bitwise",        "[a, b] -> [a & b]",         "Bitwise AND" },
	{ OP_BIT_OR,          "OP_BIT_OR",          "bitwise", "Bitwise",        "[a, b] -> [a | b]",         "Bitwise OR" },
	{ OP_BIT_XOR,         "OP_BIT_XOR",         "bitwise", "Bitwise",        "[a, b] -> [a ^ b]",         "Bitwise XOR" },
	{ OP_BIT_NOT,         "OP_BIT_NOT",         "bitwise", "Bitwise",        "[a] -> [~a]",               "Bitwise NOT / inversion" },
	{ OP_SHL,             "OP_SHL",             "bitwise", "Bitwise",        "[a, b] -> [a << b]",        "Bitwise shift left" },
	{ OP_SHR,             "OP_SHR",             "bitwise", "Bitwise",        "[a, b] -> [a >> b]",        "Bitwise shift right" },

	{ OP_EQ,              "OP_EQ",              "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare equality (a == b)" },
	{ OP_NEQ,             "OP_NEQ",             "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare inequality (a != b)" },
	{ OP_LT,              "OP_LT",              "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare less than (a < b)" },
	{ OP_LTE,             "OP_LTE",             "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare less than or equal (a <= b)" },
	{ OP_GT,              "OP_GT",              "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare greater than (a > b)" },
	{ OP_GTE,             "OP_GTE",             "logic",   "Comparison",     "[a, b] -> [bool]",          "Compare greater than or equal (a >= b)" },
	{ OP_NOT,             "OP_NOT",             "logic",   "Logic",          "[val] -> [bool]",           "Boolean logical NOT (!val)" },

	{ OP_JUMP,            "OP_JUMP",            "flow",    "Control Flow",   "[] -> []",                  "Unconditional jump to instruction offset" },
	{ OP_JUMP_IF_FALSE,   "OP_JUMP_IF_FALSE",   "flow",    "Control Flow",   "[cond] -> []",              "Jump to offset if top condition evaluates to false" },
	{ OP_JUMP_IF_TRUE,    "OP_JUMP_IF_TRUE",    "flow",    "Control Flow",   "[cond] -> []",              "Jump to offset if top condition evaluates to true" },
	{ OP_LOOP,            "OP_LOOP",            "flow",    "Control Flow",   "[] -> []",                  "Backward loop jump to instruction offset" },

	{ OP_CALL,            "OP_CALL",            "call",    "Calls",          "[a0..aN-1] -> [res]",       "Invoke function or native runtime helper" },
	{ OP_CALL_METHOD,     "OP_CALL_METHOD",     "call",    "Calls",          "[obj, a0..aN-1] -> [res]",  "Invoke object method by symbol with N arguments" },
	{ OP_PRINT,           "OP_PRINT",           "system",  "System",         "[a0..aN-1] -> []",          "Built-in print of N values to stdout" },
	{ OP_RETURN,          "OP_RETURN",          "flow",    "Control Flow",   "[res] -> exit",             "Return from current function with return value" },

	{ OP_CLOSURE,         "OP_CLOSURE",         "call",    "Closures",       "[] -> [closure]",           "Instantiate closure wrapping function and upvalues" },
	{ OP_GET_UPVALUE,     "OP_GET_UPVALUE",     "var",     "Closures",       "[] -> [val]",               "Load captured upvalue value from closure" },
	{ OP_SET_UPVALUE,     "OP_SET_UPVALUE",     "var",     "Closures",       "[val] -> [val]",            "Store value into captured upvalue in closure" },
	{ OP_CLOSE_UPVALUE,   "OP_CLOSE_UPVALUE",   "var",     "Closures",       "[] -> []",                  "Close open upvalues above stack slot" },

	{ OP_HALT,            "OP_HALT",            "flow",    "Control Flow",   "[] -> halt",                "Cleanly terminate VM execution" }
};

static const OpcodeInfo* get_opcode_info(uint8_t op)
{
	for (size_t i = 0; i < sizeof(OP_METAS)/sizeof(OP_METAS[0]); i++)
	{
		if (OP_METAS[i].op == (XIrOpCode)op) return &OP_METAS[i];
	}
	static const OpcodeInfo unknown_info = {
		(XIrOpCode)255, "OP_UNKNOWN", "system", "Unknown", "[] -> []", "Unrecognized bytecode opcode"
	};
	return &unknown_info;
}

/* --------------------------------------------------------------------------
 * HTML Escaping Helper
 * -------------------------------------------------------------------------- */
static void html_escape(const char* src, char* dst, int dst_sz)
{
	if (!src || !dst || dst_sz <= 0) return;
	int j = 0;
	for (const char* p = src; *p && j < dst_sz - 8; p++)
	{
		switch (*p)
		{
		case '&':  memcpy(dst+j,"&amp;", 5);  j+=5; break;
		case '<':  memcpy(dst+j,"&lt;",  4);  j+=4; break;
		case '>':  memcpy(dst+j,"&gt;",  4);  j+=4; break;
		case '"':  memcpy(dst+j,"&quot;",6);  j+=6; break;
		case '\'': memcpy(dst+j,"&#39;", 5);  j+=5; break;
		default:   dst[j++] = *p; break;
		}
	}
	dst[j] = '\0';
}

static void js_escape(const char* src, char* dst, int dst_sz)
{
	if (!src || !dst || dst_sz <= 0) return;
	int j = 0;
	for (const char* p = src; *p && j < dst_sz - 8; p++)
	{
		switch (*p)
		{
		case '\\': memcpy(dst+j, "\\\\", 2); j+=2; break;
		case '"':  memcpy(dst+j, "\\\"", 2); j+=2; break;
		case '\'': memcpy(dst+j, "\\'",  2); j+=2; break;
		case '\n': memcpy(dst+j, "\\n",  2); j+=2; break;
		case '\r': memcpy(dst+j, "\\r",  2); j+=2; break;
		case '\t': memcpy(dst+j, "\\t",  2); j+=2; break;
		case '<':  memcpy(dst+j, "\\u003c", 6); j+=6; break;
		case '>':  memcpy(dst+j, "\\u003e", 6); j+=6; break;
		default:   dst[j++] = *p; break;
		}
	}
	dst[j] = '\0';
}

/* --------------------------------------------------------------------------
 * Source Code Syntax Highlighter for xlang
 * -------------------------------------------------------------------------- */
static bool is_ident_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static bool is_ident_char(char c)  { return isalnum((unsigned char)c) || c == '_'; }

static bool is_xlang_keyword(const char* s, int len)
{
	static const char* const kws[] = {
		/* The 14 reserved words from VOCABULARY.md section 2 ... */
		"if", "eif", "else", "while", "for", "do", "return", "break", "continue",
		"class", "static", "new", "import", "in",
		/* ... plus built-in names worth highlighting */
		"this", "print", "echo", "assert", NULL
	};
	for (int i = 0; kws[i]; i++) {
		if ((int)strlen(kws[i]) == len && memcmp(s, kws[i], len) == 0) return true;
	}
	return false;
}

static bool is_xlang_type(const char* s, int len)
{
	static const char* const types[] = {
		"int", "char", "string", "bool", "float", "long", "double", "object", NULL
	};
	for (int i = 0; types[i]; i++) {
		if ((int)strlen(types[i]) == len && memcmp(s, types[i], len) == 0) return true;
	}
	return false;
}

static bool is_xlang_literal(const char* s, int len)
{
	static const char* const lits[] = { "true", "false", "null", NULL };
	for (int i = 0; lits[i]; i++) {
		if ((int)strlen(lits[i]) == len && memcmp(s, lits[i], len) == 0) return true;
	}
	return false;
}

static void highlight_xlang_line(const char* src, char* dst, int dst_sz, bool* in_comment)
{
	if (!src || !dst || dst_sz <= 0) return;
	int j = 0;
	const char* p = src;

#define EMIT_CHAR(c) do { if (j < dst_sz - 8) dst[j++] = (c); } while(0)
#define EMIT_STR(s)  do { \
	int l = (int)strlen(s); \
	if (j + l < dst_sz - 8) { memcpy(dst+j, s, l); j += l; } \
} while(0)
#define EMIT_ESC(c)  do { \
	if ((c) == '&') EMIT_STR("&amp;"); \
	else if ((c) == '<') EMIT_STR("&lt;"); \
	else if ((c) == '>') EMIT_STR("&gt;"); \
	else if ((c) == '"') EMIT_STR("&quot;"); \
	else if ((c) == '\'') EMIT_STR("&#39;"); \
	else EMIT_CHAR(c); \
} while(0)

	while (*p && j < dst_sz - 32)
	{
		/* Check block comment continuation */
		if (in_comment && *in_comment)
		{
			EMIT_STR("<span class=\"tok-comment\">");
			while (*p && j < dst_sz - 32)
			{
				if (p[0] == '*' && p[1] == '/')
				{
					EMIT_STR("*/</span>");
					p += 2;
					*in_comment = false;
					break;
				}
				EMIT_ESC(*p);
				p++;
			}
			if (in_comment && *in_comment)
			{
				EMIT_STR("</span>");
			}
			continue;
		}

		/* Check block comment start */
		if (p[0] == '/' && p[1] == '*')
		{
			EMIT_STR("<span class=\"tok-comment\">/*");
			p += 2;
			if (in_comment) *in_comment = true;
			while (*p && j < dst_sz - 32)
			{
				if (p[0] == '*' && p[1] == '/')
				{
					EMIT_STR("*/</span>");
					p += 2;
					if (in_comment) *in_comment = false;
					break;
				}
				EMIT_ESC(*p);
				p++;
			}
			if (in_comment && *in_comment) EMIT_STR("</span>");
			continue;
		}

		/* Single-line comments: '#' or '//' to end of line (VOCABULARY.md 1.1) */
		if (*p == '#' || (p[0] == '/' && p[1] == '/'))
		{
			EMIT_STR("<span class=\"tok-comment\">");
			while (*p && j < dst_sz - 16) { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			break;
		}

		/* Check string literals */
		if (*p == '"' || *p == '\'')
		{
			char quote = *p;
			EMIT_STR("<span class=\"tok-str\">");
			EMIT_ESC(*p);
			p++;
			while (*p && *p != quote && j < dst_sz - 16)
			{
				if (*p == '\\' && *(p+1)) {
					EMIT_ESC(*p); p++;
					EMIT_ESC(*p); p++;
				} else {
					EMIT_ESC(*p);
					p++;
				}
			}
			if (*p == quote) { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			continue;
		}

		/* Check number literals */
		if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)*(p+1))))
		{
			EMIT_STR("<span class=\"tok-num\">");
			if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
				EMIT_ESC(*p); p++;
				EMIT_ESC(*p); p++;
				while (isxdigit((unsigned char)*p)) { EMIT_ESC(*p); p++; }
			} else {
				while (isdigit((unsigned char)*p) || *p == '.' || *p == 'f' || *p == 'L' || *p == 'e' || *p == 'E') {
					EMIT_ESC(*p); p++;
				}
			}
			EMIT_STR("</span>");
			continue;
		}

		/* Check identifiers / keywords */
		if (is_ident_start(*p))
		{
			const char* start = p;
			while (is_ident_char(*p)) p++;
			int len = (int)(p - start);

			/* Look ahead for function call: foo( */
			const char* q = p;
			while (*q == ' ' || *q == '\t') q++;
			bool is_call = (*q == '(');

			if (is_xlang_keyword(start, len))
			{
				EMIT_STR("<span class=\"tok-kw\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			else if (is_xlang_type(start, len))
			{
				EMIT_STR("<span class=\"tok-type\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			else if (is_xlang_literal(start, len))
			{
				EMIT_STR("<span class=\"tok-const\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			else if (is_call)
			{
				EMIT_STR("<span class=\"tok-fn\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			else
			{
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
			}
			continue;
		}

		/* Operators and punctuation */
		if (strchr("+-*/%=!<>|&^~?:", *p))
		{
			EMIT_STR("<span class=\"tok-op\">");
			EMIT_ESC(*p);
			p++;
			if (*p && strchr("+-*/%=!<>|&^~?:", *p)) {
				EMIT_ESC(*p);
				p++;
			}
			EMIT_STR("</span>");
			continue;
		}

		/* Normal character */
		EMIT_ESC(*p);
		p++;
	}

#undef EMIT_CHAR
#undef EMIT_STR
#undef EMIT_ESC

	dst[j] = '\0';
}

/* --------------------------------------------------------------------------
 * LLVM IR Syntax Highlighter
 * -------------------------------------------------------------------------- */
static void highlight_llvm_line(const char* src, char* dst, int dst_sz)
{
	if (!src || !dst || dst_sz <= 0) return;
	int j = 0;
	const char* p = src;

#define EMIT_CHAR(c) do { if (j < dst_sz - 8) dst[j++] = (c); } while(0)
#define EMIT_STR(s)  do { \
	int l = (int)strlen(s); \
	if (j + l < dst_sz - 8) { memcpy(dst+j, s, l); j += l; } \
} while(0)
#define EMIT_ESC(c)  do { \
	if ((c) == '&') EMIT_STR("&amp;"); \
	else if ((c) == '<') EMIT_STR("&lt;"); \
	else if ((c) == '>') EMIT_STR("&gt;"); \
	else if ((c) == '"') EMIT_STR("&quot;"); \
	else if ((c) == '\'') EMIT_STR("&#39;"); \
	else EMIT_CHAR(c); \
} while(0)

	while (*p && j < dst_sz - 32)
	{
		/* Comment */
		if (*p == ';')
		{
			EMIT_STR("<span class=\"tok-comment\">");
			while (*p && j < dst_sz - 16) { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			break;
		}

		/* Registers / Local IDs %name */
		if (*p == '%')
		{
			EMIT_STR("<span class=\"llvm-reg\">%");
			p++;
			while (isalnum((unsigned char)*p) || *p == '_' || *p == '.') { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			continue;
		}

		/* Globals @name */
		if (*p == '@')
		{
			EMIT_STR("<span class=\"llvm-global\">@");
			p++;
			while (isalnum((unsigned char)*p) || *p == '_' || *p == '.') { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			continue;
		}

		/* String literals */
		if (*p == '"')
		{
			EMIT_STR("<span class=\"tok-str\">&quot;");
			p++;
			while (*p && *p != '"' && j < dst_sz - 16)
			{
				if (*p == '\\' && *(p+1)) { EMIT_ESC(*p); p++; EMIT_ESC(*p); p++; }
				else { EMIT_ESC(*p); p++; }
			}
			if (*p == '"') { EMIT_STR("&quot;"); p++; }
			EMIT_STR("</span>");
			continue;
		}

		/* Keywords and types */
		if (is_ident_start(*p))
		{
			const char* start = p;
			while (isalnum((unsigned char)*p) || *p == '_' || *p == '.') p++;
			int len = (int)(p - start);

			char word[64];
			if (len < (int)sizeof(word)) {
				memcpy(word, start, len);
				word[len] = '\0';
			} else {
				word[0] = '\0';
			}

			/* Types */
			if (strcmp(word, "i1") == 0 || strcmp(word, "i8") == 0 || strcmp(word, "i16") == 0 ||
			    strcmp(word, "i32") == 0 || strcmp(word, "i64") == 0 || strcmp(word, "void") == 0 ||
			    strcmp(word, "float") == 0 || strcmp(word, "double") == 0 || strcmp(word, "ptr") == 0 ||
			    strcmp(word, "label") == 0 || strcmp(word, "metadata") == 0)
			{
				EMIT_STR("<span class=\"tok-type\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			/* Instructions / Keywords */
			else if (strcmp(word, "define") == 0 || strcmp(word, "declare") == 0 || strcmp(word, "ret") == 0 ||
			         strcmp(word, "br") == 0 || strcmp(word, "alloca") == 0 || strcmp(word, "load") == 0 ||
			         strcmp(word, "store") == 0 || strcmp(word, "call") == 0 || strcmp(word, "tail") == 0 ||
			         strcmp(word, "getelementptr") == 0 || strcmp(word, "icmp") == 0 || strcmp(word, "fcmp") == 0 ||
			         strcmp(word, "phi") == 0 || strcmp(word, "add") == 0 || strcmp(word, "sub") == 0 ||
			         strcmp(word, "mul") == 0 || strcmp(word, "sdiv") == 0 || strcmp(word, "udiv") == 0 ||
			         strcmp(word, "bitcast") == 0 || strcmp(word, "trunc") == 0 || strcmp(word, "zext") == 0 ||
			         strcmp(word, "sext") == 0 || strcmp(word, "switch") == 0 || strcmp(word, "select") == 0)
			{
				EMIT_STR("<span class=\"tok-kw\">");
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
				EMIT_STR("</span>");
			}
			else
			{
				for (int k = 0; k < len; k++) EMIT_ESC(start[k]);
			}
			continue;
		}

		/* Numbers */
		if (isdigit((unsigned char)*p) || (*p == '-' && isdigit((unsigned char)*(p+1))))
		{
			EMIT_STR("<span class=\"tok-num\">");
			if (*p == '-') { EMIT_CHAR('-'); p++; }
			while (isdigit((unsigned char)*p) || *p == '.') { EMIT_ESC(*p); p++; }
			EMIT_STR("</span>");
			continue;
		}

		EMIT_ESC(*p);
		p++;
	}

#undef EMIT_CHAR
#undef EMIT_STR
#undef EMIT_ESC

	dst[j] = '\0';
}

/* --------------------------------------------------------------------------
 * LLVM IR In-Memory Capture
 * -------------------------------------------------------------------------- */
static char* capture_llvm_ir(const AstProgram* prog, const char* source_path)
{
	if (!prog) return NULL;
	char* buf = NULL;
	size_t sz = 0;

#if defined(__linux__) || defined(__APPLE__)
	FILE* mem = open_memstream(&buf, &sz);
	if (!mem) return NULL;
	XLLVMConfig cfg = xllvm_default_config();
	cfg.emit_comments = true;
	xllvm_emit_program(prog, source_path ? source_path : "source.xb", mem, &cfg);
	fclose(mem);
	return buf;
#else
	FILE* tmp = tmpfile();
	if (!tmp) return NULL;
	XLLVMConfig cfg = xllvm_default_config();
	cfg.emit_comments = true;
	xllvm_emit_program(prog, source_path ? source_path : "source.xb", tmp, &cfg);
	long len = ftell(tmp);
	rewind(tmp);
	buf = (char*)malloc(len + 1);
	if (buf) {
		size_t rd = fread(buf, 1, len, tmp);
		buf[rd] = '\0';
	}
	fclose(tmp);
	return buf;
#endif
}

/* --------------------------------------------------------------------------
 * AST Interactive Tree & Text Formatter Helpers
 * -------------------------------------------------------------------------- */
static void render_ast_expr_tree(FILE* out, const AstExpr* expr);
static void render_ast_stmt_tree(FILE* out, const AstStmt* stmt);
static void render_ast_expr_text(FILE* out, const AstExpr* expr, int indent);
static void render_ast_stmt_text(FILE* out, const AstStmt* stmt, int indent);

static void count_expr_stats(const AstExpr* expr, int* total_exprs, int depth, int* max_depth)
{
	if (!expr) return;
	(*total_exprs)++;
	if (depth > *max_depth) *max_depth = depth;

	switch (expr->type)
	{
	case AST_EXPR_BINARY:
		count_expr_stats(expr->as.binary.left, total_exprs, depth + 1, max_depth);
		count_expr_stats(expr->as.binary.right, total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_UNARY:
		count_expr_stats(expr->as.unary.operand, total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_CALL:
		for (int i = 0; i < expr->as.call.arg_count; i++)
			count_expr_stats(expr->as.call.args[i], total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_METHOD_CALL:
		count_expr_stats(expr->as.method_call.object, total_exprs, depth + 1, max_depth);
		for (int i = 0; i < expr->as.method_call.arg_count; i++)
			count_expr_stats(expr->as.method_call.args[i], total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_MEMBER:
		count_expr_stats(expr->as.member.object, total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_INDEX:
		count_expr_stats(expr->as.index.target, total_exprs, depth + 1, max_depth);
		count_expr_stats(expr->as.index.index, total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_LIST:
		for (int i = 0; i < expr->as.list.element_count; i++)
			count_expr_stats(expr->as.list.elements[i], total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_ASSIGN:
		count_expr_stats(expr->as.assign.target, total_exprs, depth + 1, max_depth);
		count_expr_stats(expr->as.assign.value, total_exprs, depth + 1, max_depth);
		break;
	case AST_EXPR_NEW:
		for (int i = 0; i < expr->as.new_expr.arg_count; i++)
			count_expr_stats(expr->as.new_expr.args[i], total_exprs, depth + 1, max_depth);
		break;
	default:
		break;
	}
}

static void count_stmt_stats(const AstStmt* stmt, int* total_stmts, int* total_exprs,
                             int* total_funcs, int* total_classes, int depth, int* max_depth)
{
	if (!stmt) return;
	(*total_stmts)++;
	if (depth > *max_depth) *max_depth = depth;

	switch (stmt->type)
	{
	case AST_STMT_EXPR:
		count_expr_stats(stmt->as.expr, total_exprs, depth + 1, max_depth);
		break;
	case AST_STMT_VAR_DECL:
		if (stmt->as.var_decl.init_expr)
			count_expr_stats(stmt->as.var_decl.init_expr, total_exprs, depth + 1, max_depth);
		break;
	case AST_STMT_BLOCK:
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
			count_stmt_stats(stmt->as.block.stmts[i], total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_IF:
		count_expr_stats(stmt->as.if_stmt.condition, total_exprs, depth + 1, max_depth);
		count_stmt_stats(stmt->as.if_stmt.then_branch, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		if (stmt->as.if_stmt.else_branch)
			count_stmt_stats(stmt->as.if_stmt.else_branch, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_WHILE:
		count_expr_stats(stmt->as.while_stmt.condition, total_exprs, depth + 1, max_depth);
		count_stmt_stats(stmt->as.while_stmt.body, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_DO_WHILE:
		count_stmt_stats(stmt->as.do_while_stmt.body, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		count_expr_stats(stmt->as.do_while_stmt.condition, total_exprs, depth + 1, max_depth);
		break;
	case AST_STMT_FOR_C:
		if (stmt->as.for_c.init)
			count_stmt_stats(stmt->as.for_c.init, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		if (stmt->as.for_c.condition)
			count_expr_stats(stmt->as.for_c.condition, total_exprs, depth + 1, max_depth);
		if (stmt->as.for_c.step)
			count_expr_stats(stmt->as.for_c.step, total_exprs, depth + 1, max_depth);
		count_stmt_stats(stmt->as.for_c.body, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_FOR_IN:
		count_expr_stats(stmt->as.for_in.collection, total_exprs, depth + 1, max_depth);
		count_stmt_stats(stmt->as.for_in.body, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_RETURN:
		if (stmt->as.return_expr)
			count_expr_stats(stmt->as.return_expr, total_exprs, depth + 1, max_depth);
		break;
	case AST_STMT_FUNC_DECL:
		(*total_funcs)++;
		if (stmt->as.func_decl.body)
			count_stmt_stats(stmt->as.func_decl.body, total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	case AST_STMT_CLASS_DECL:
		(*total_classes)++;
		for (int i = 0; i < stmt->as.class_decl.member_count; i++)
			count_stmt_stats(stmt->as.class_decl.members[i], total_stmts, total_exprs, total_funcs, total_classes, depth + 1, max_depth);
		break;
	default:
		break;
	}
}

static void render_ast_expr_tree(FILE* out, const AstExpr* expr)
{
	if (!expr) {
		fprintf(out, "<div class=\"ast-leaf\"><span style=\"color:var(--text-muted)\">(null expr)</span></div>\n");
		return;
	}

	char esc[256];
	switch (expr->type)
	{
	case AST_EXPR_LITERAL_INT:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-lit\">LiteralInt</span> <span class=\"ast-val\">%" PRId64 "</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        expr->as.int_val, expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_FLOAT:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-lit\">LiteralFloat</span> <span class=\"ast-val\">%g</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        expr->as.float_val, expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_STRING:
		html_escape(expr->as.string_val ? expr->as.string_val : "", esc, sizeof(esc));
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-lit\">LiteralString</span> <span class=\"val-str\">&quot;%s&quot;</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        esc, expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_BOOL:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-lit\">LiteralBool</span> <span class=\"tok-const\">%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        expr->as.bool_val ? "true" : "false", expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_NULL:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-lit\">LiteralNull</span> <span class=\"tok-const\">null</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_IDENTIFIER:
		html_escape(expr->as.identifier_name ? expr->as.identifier_name : "", esc, sizeof(esc));
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-ident\">Identifier</span> <span class=\"val-sym\">%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        esc, expr->line, expr->line, expr->col);
		break;
	case AST_EXPR_BINARY:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">BinaryExpr</span> <span class=\"ast-name\">(%s)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        ast_binop_to_string(expr->as.binary.op), expr->line, expr->line, expr->col);
		render_ast_expr_tree(out, expr->as.binary.left);
		render_ast_expr_tree(out, expr->as.binary.right);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_UNARY:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">UnaryExpr</span> <span class=\"ast-name\">(%s)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        ast_unop_to_string(expr->as.unary.op), expr->line, expr->line, expr->col);
		render_ast_expr_tree(out, expr->as.unary.operand);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_CALL:
		html_escape(expr->as.call.name ? expr->as.call.name : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">CallExpr</span> <span class=\"ast-name\">%s (%d args)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, expr->as.call.arg_count, expr->line, expr->line, expr->col);
		for (int i = 0; i < expr->as.call.arg_count; i++)
			render_ast_expr_tree(out, expr->as.call.args[i]);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_METHOD_CALL:
		html_escape(expr->as.method_call.method_name ? expr->as.method_call.method_name : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">MethodCall</span> <span class=\"ast-name\">.%s (%d args)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, expr->as.method_call.arg_count, expr->line, expr->line, expr->col);
		fprintf(out, "<div class=\"ast-role-label\">Receiver:</div>\n");
		render_ast_expr_tree(out, expr->as.method_call.object);
		if (expr->as.method_call.arg_count > 0) fprintf(out, "<div class=\"ast-role-label\">Arguments:</div>\n");
		for (int i = 0; i < expr->as.method_call.arg_count; i++)
			render_ast_expr_tree(out, expr->as.method_call.args[i]);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_MEMBER:
		html_escape(expr->as.member.member_name ? expr->as.member.member_name : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">MemberExpr</span> <span class=\"ast-name\">.%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, expr->line, expr->line, expr->col);
		render_ast_expr_tree(out, expr->as.member.object);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_INDEX:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">IndexExpr</span> <span class=\"ast-name\">[]</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        expr->line, expr->line, expr->col);
		fprintf(out, "<div class=\"ast-role-label\">Target:</div>\n");
		render_ast_expr_tree(out, expr->as.index.target);
		fprintf(out, "<div class=\"ast-role-label\">Index:</div>\n");
		render_ast_expr_tree(out, expr->as.index.index);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_LIST:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">ListExpr</span> <span class=\"ast-name\">(%d items)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        expr->as.list.element_count, expr->line, expr->line, expr->col);
		for (int i = 0; i < expr->as.list.element_count; i++)
			render_ast_expr_tree(out, expr->as.list.elements[i]);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_ASSIGN:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">AssignExpr</span> <span class=\"ast-name\">(%s)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        expr->as.assign.op ? expr->as.assign.op : "=", expr->line, expr->line, expr->col);
		fprintf(out, "<div class=\"ast-role-label\">Target:</div>\n");
		render_ast_expr_tree(out, expr->as.assign.target);
		fprintf(out, "<div class=\"ast-role-label\">Value:</div>\n");
		render_ast_expr_tree(out, expr->as.assign.value);
		fprintf(out, "</div></details>\n");
		break;
	case AST_EXPR_NEW:
		html_escape(expr->as.new_expr.class_name ? expr->as.new_expr.class_name : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-expr\">NewExpr</span> <span class=\"ast-name\">new %s(%d args)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, expr->as.new_expr.arg_count, expr->line, expr->line, expr->col);
		for (int i = 0; i < expr->as.new_expr.arg_count; i++)
			render_ast_expr_tree(out, expr->as.new_expr.args[i]);
		fprintf(out, "</div></details>\n");
		break;
	default:
		fprintf(out, "<div class=\"ast-leaf\">UnknownExpr (%d)</div>\n", expr->type);
		break;
	}
}

static void render_ast_stmt_tree(FILE* out, const AstStmt* stmt)
{
	if (!stmt) return;

	char esc[256], esc2[256];
	switch (stmt->type)
	{
	case AST_STMT_EXPR:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">ExprStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->line, stmt->line, stmt->col);
		render_ast_expr_tree(out, stmt->as.expr);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_VAR_DECL:
		html_escape(stmt->as.var_decl.type_name ? stmt->as.var_decl.type_name : "var", esc, sizeof(esc));
		html_escape(stmt->as.var_decl.var_name ? stmt->as.var_decl.var_name : "", esc2, sizeof(esc2));
		if (stmt->as.var_decl.init_expr) {
			fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-var\">VarDecl</span> <span class=\"ast-name\">%s <span class=\"val-sym\">%s</span>%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
			        esc, esc2, stmt->as.var_decl.is_static ? " (static)" : "", stmt->line, stmt->line, stmt->col);
			fprintf(out, "<div class=\"ast-role-label\">Initializer:</div>\n");
			render_ast_expr_tree(out, stmt->as.var_decl.init_expr);
			fprintf(out, "</div></details>\n");
		} else {
			fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-var\">VarDecl</span> <span class=\"ast-name\">%s <span class=\"val-sym\">%s</span>%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
			        esc, esc2, stmt->as.var_decl.is_static ? " (static)" : "", stmt->line, stmt->line, stmt->col);
		}
		break;
	case AST_STMT_BLOCK:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-block\">BlockStmt</span> <span class=\"val-meta\">(%d statements)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->as.block.stmt_count, stmt->line, stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
			render_ast_stmt_tree(out, stmt->as.block.stmts[i]);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_IF:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">IfStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->line, stmt->line, stmt->col);
		fprintf(out, "<div class=\"ast-role-label\">Condition:</div>\n");
		render_ast_expr_tree(out, stmt->as.if_stmt.condition);
		fprintf(out, "<div class=\"ast-role-label\">Then Branch:</div>\n");
		render_ast_stmt_tree(out, stmt->as.if_stmt.then_branch);
		if (stmt->as.if_stmt.else_branch) {
			fprintf(out, "<div class=\"ast-role-label\">Else Branch:</div>\n");
			render_ast_stmt_tree(out, stmt->as.if_stmt.else_branch);
		}
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_WHILE:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">WhileStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->line, stmt->line, stmt->col);
		fprintf(out, "<div class=\"ast-role-label\">Condition:</div>\n");
		render_ast_expr_tree(out, stmt->as.while_stmt.condition);
		fprintf(out, "<div class=\"ast-role-label\">Body:</div>\n");
		render_ast_stmt_tree(out, stmt->as.while_stmt.body);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_DO_WHILE:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">DoWhileStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->line, stmt->line, stmt->col);
		fprintf(out, "<div class=\"ast-role-label\">Body:</div>\n");
		render_ast_stmt_tree(out, stmt->as.do_while_stmt.body);
		fprintf(out, "<div class=\"ast-role-label\">Condition:</div>\n");
		render_ast_expr_tree(out, stmt->as.do_while_stmt.condition);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_FOR_C:
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">ForCStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        stmt->line, stmt->line, stmt->col);
		if (stmt->as.for_c.init) {
			fprintf(out, "<div class=\"ast-role-label\">Init:</div>\n");
			render_ast_stmt_tree(out, stmt->as.for_c.init);
		}
		if (stmt->as.for_c.condition) {
			fprintf(out, "<div class=\"ast-role-label\">Condition:</div>\n");
			render_ast_expr_tree(out, stmt->as.for_c.condition);
		}
		if (stmt->as.for_c.step) {
			fprintf(out, "<div class=\"ast-role-label\">Step:</div>\n");
			render_ast_expr_tree(out, stmt->as.for_c.step);
		}
		fprintf(out, "<div class=\"ast-role-label\">Body:</div>\n");
		render_ast_stmt_tree(out, stmt->as.for_c.body);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_FOR_IN:
		html_escape(stmt->as.for_in.item_var ? stmt->as.for_in.item_var : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">ForInStmt</span> <span class=\"ast-name\">(var %s)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, stmt->line, stmt->line, stmt->col);
		fprintf(out, "<div class=\"ast-role-label\">Collection:</div>\n");
		render_ast_expr_tree(out, stmt->as.for_in.collection);
		fprintf(out, "<div class=\"ast-role-label\">Body:</div>\n");
		render_ast_stmt_tree(out, stmt->as.for_in.body);
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_RETURN:
		if (stmt->as.return_expr) {
			fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-ctrl\">ReturnStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
			        stmt->line, stmt->line, stmt->col);
			render_ast_expr_tree(out, stmt->as.return_expr);
			fprintf(out, "</div></details>\n");
		} else {
			fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-ctrl\">ReturnStmt</span> <span class=\"val-meta\">(void)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
			        stmt->line, stmt->line, stmt->col);
		}
		break;
	case AST_STMT_BREAK:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-ctrl\">BreakStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        stmt->line, stmt->line, stmt->col);
		break;
	case AST_STMT_CONTINUE:
		fprintf(out, "<div class=\"ast-leaf\"><span class=\"ast-type-badge ast-t-ctrl\">ContinueStmt</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></div>\n",
		        stmt->line, stmt->line, stmt->col);
		break;
	case AST_STMT_FUNC_DECL:
		html_escape(stmt->as.func_decl.return_type ? stmt->as.func_decl.return_type : "void", esc, sizeof(esc));
		html_escape(stmt->as.func_decl.name ? stmt->as.func_decl.name : "", esc2, sizeof(esc2));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-func\">FuncDecl</span> <span class=\"ast-name\">%s <span class=\"val-sym\">%s</span>(%d params)%s</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, esc2, stmt->as.func_decl.param_count, stmt->as.func_decl.is_static ? " (static)" : "", stmt->line, stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.func_decl.param_count; i++) {
			char pt[128], pn[128];
			html_escape(stmt->as.func_decl.params[i].type_name ? stmt->as.func_decl.params[i].type_name : "any", pt, sizeof(pt));
			html_escape(stmt->as.func_decl.params[i].name ? stmt->as.func_decl.params[i].name : "", pn, sizeof(pn));
			fprintf(out, "<div class=\"ast-leaf\"><span class=\"val-meta\">Param #%d:</span> <span class=\"tok-type\">%s</span> <span class=\"val-sym\">%s</span></div>\n", i, pt, pn);
		}
		if (stmt->as.func_decl.body) {
			fprintf(out, "<div class=\"ast-role-label\">Body:</div>\n");
			render_ast_stmt_tree(out, stmt->as.func_decl.body);
		}
		fprintf(out, "</div></details>\n");
		break;
	case AST_STMT_CLASS_DECL:
		html_escape(stmt->as.class_decl.name ? stmt->as.class_decl.name : "", esc, sizeof(esc));
		fprintf(out, "<details open class=\"ast-node\"><summary class=\"ast-summary\"><span class=\"ast-type-badge ast-t-class\">ClassDecl</span> <span class=\"ast-name\">class <span class=\"val-sym\">%s</span>%s%s (%d members)</span> <a href=\"javascript:void(0)\" class=\"ast-line-link\" onclick=\"goToSourceLine(%d)\">L%d:%d</a></summary><div class=\"ast-children\">\n",
		        esc, stmt->as.class_decl.base_name ? " : " : "", stmt->as.class_decl.base_name ? stmt->as.class_decl.base_name : "",
		        stmt->as.class_decl.member_count, stmt->line, stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.class_decl.member_count; i++)
			render_ast_stmt_tree(out, stmt->as.class_decl.members[i]);
		fprintf(out, "</div></details>\n");
		break;
	default:
		fprintf(out, "<div class=\"ast-leaf\">UnknownStmt (%d)</div>\n", stmt->type);
		break;
	}
}

static void print_ast_indent(FILE* out, int indent)
{
	for (int i = 0; i < indent; i++) fprintf(out, "  ");
}

static void render_ast_expr_text(FILE* out, const AstExpr* expr, int indent)
{
	if (!expr) {
		print_ast_indent(out, indent);
		fprintf(out, "(null expr)\n");
		return;
	}
	print_ast_indent(out, indent);
	switch (expr->type)
	{
	case AST_EXPR_LITERAL_INT:
		fprintf(out, "LiteralInt: %" PRId64 " [line %d:%d]\n", expr->as.int_val, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_FLOAT:
		fprintf(out, "LiteralFloat: %g [line %d:%d]\n", expr->as.float_val, expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_STRING: {
		char esc[256];
		html_escape(expr->as.string_val ? expr->as.string_val : "", esc, sizeof(esc));
		fprintf(out, "LiteralString: &quot;%s&quot; [line %d:%d]\n", esc, expr->line, expr->col);
		break;
	}
	case AST_EXPR_LITERAL_BOOL:
		fprintf(out, "LiteralBool: %s [line %d:%d]\n", expr->as.bool_val ? "true" : "false", expr->line, expr->col);
		break;
	case AST_EXPR_LITERAL_NULL:
		fprintf(out, "LiteralNull [line %d:%d]\n", expr->line, expr->col);
		break;
	case AST_EXPR_IDENTIFIER: {
		char esc[128];
		html_escape(expr->as.identifier_name ? expr->as.identifier_name : "", esc, sizeof(esc));
		fprintf(out, "Identifier: %s [line %d:%d]\n", esc, expr->line, expr->col);
		break;
	}
	case AST_EXPR_BINARY:
		fprintf(out, "BinaryExpr (%s) [line %d:%d]\n", ast_binop_to_string(expr->as.binary.op), expr->line, expr->col);
		render_ast_expr_text(out, expr->as.binary.left, indent + 1);
		render_ast_expr_text(out, expr->as.binary.right, indent + 1);
		break;
	case AST_EXPR_UNARY:
		fprintf(out, "UnaryExpr (%s) [line %d:%d]\n", ast_unop_to_string(expr->as.unary.op), expr->line, expr->col);
		render_ast_expr_text(out, expr->as.unary.operand, indent + 1);
		break;
	case AST_EXPR_CALL: {
		char esc[128];
		html_escape(expr->as.call.name ? expr->as.call.name : "", esc, sizeof(esc));
		fprintf(out, "CallExpr: %s (%d args) [line %d:%d]\n", esc, expr->as.call.arg_count, expr->line, expr->col);
		for (int i = 0; i < expr->as.call.arg_count; i++)
			render_ast_expr_text(out, expr->as.call.args[i], indent + 1);
		break;
	}
	case AST_EXPR_METHOD_CALL: {
		char esc[128];
		html_escape(expr->as.method_call.method_name ? expr->as.method_call.method_name : "", esc, sizeof(esc));
		fprintf(out, "MethodCall: .%s (%d args) [line %d:%d]\n", esc, expr->as.method_call.arg_count, expr->line, expr->col);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Receiver:\n");
		render_ast_expr_text(out, expr->as.method_call.object, indent + 2);
		for (int i = 0; i < expr->as.method_call.arg_count; i++)
			render_ast_expr_text(out, expr->as.method_call.args[i], indent + 1);
		break;
	}
	case AST_EXPR_MEMBER: {
		char esc[128];
		html_escape(expr->as.member.member_name ? expr->as.member.member_name : "", esc, sizeof(esc));
		fprintf(out, "MemberExpr: .%s [line %d:%d]\n", esc, expr->line, expr->col);
		render_ast_expr_text(out, expr->as.member.object, indent + 1);
		break;
	}
	case AST_EXPR_INDEX:
		fprintf(out, "IndexExpr [] [line %d:%d]\n", expr->line, expr->col);
		render_ast_expr_text(out, expr->as.index.target, indent + 1);
		render_ast_expr_text(out, expr->as.index.index, indent + 1);
		break;
	case AST_EXPR_LIST:
		fprintf(out, "ListLiteral (%d items) [line %d:%d]\n", expr->as.list.element_count, expr->line, expr->col);
		for (int i = 0; i < expr->as.list.element_count; i++)
			render_ast_expr_text(out, expr->as.list.elements[i], indent + 1);
		break;
	case AST_EXPR_ASSIGN:
		fprintf(out, "AssignExpr (%s) [line %d:%d]\n", expr->as.assign.op ? expr->as.assign.op : "=", expr->line, expr->col);
		render_ast_expr_text(out, expr->as.assign.target, indent + 1);
		render_ast_expr_text(out, expr->as.assign.value, indent + 1);
		break;
	case AST_EXPR_NEW: {
		char esc[128];
		html_escape(expr->as.new_expr.class_name ? expr->as.new_expr.class_name : "", esc, sizeof(esc));
		fprintf(out, "NewExpr: %s (%d args) [line %d:%d]\n", esc, expr->as.new_expr.arg_count, expr->line, expr->col);
		for (int i = 0; i < expr->as.new_expr.arg_count; i++)
			render_ast_expr_text(out, expr->as.new_expr.args[i], indent + 1);
		break;
	}
	default:
		fprintf(out, "UnknownExpr (%d)\n", expr->type);
		break;
	}
}

static void render_ast_stmt_text(FILE* out, const AstStmt* stmt, int indent)
{
	if (!stmt) return;

	char esc[256], esc2[256];
	print_ast_indent(out, indent);

	switch (stmt->type)
	{
	case AST_STMT_EXPR:
		fprintf(out, "ExprStmt [line %d:%d]\n", stmt->line, stmt->col);
		render_ast_expr_text(out, stmt->as.expr, indent + 1);
		break;
	case AST_STMT_VAR_DECL:
		html_escape(stmt->as.var_decl.type_name ? stmt->as.var_decl.type_name : "var", esc, sizeof(esc));
		html_escape(stmt->as.var_decl.var_name ? stmt->as.var_decl.var_name : "", esc2, sizeof(esc2));
		fprintf(out, "VarDecl: %s %s%s [line %d:%d]\n", esc, esc2, stmt->as.var_decl.is_static ? " (static)" : "", stmt->line, stmt->col);
		if (stmt->as.var_decl.init_expr)
			render_ast_expr_text(out, stmt->as.var_decl.init_expr, indent + 1);
		break;
	case AST_STMT_BLOCK:
		fprintf(out, "BlockStmt (%d stmts) [line %d:%d]\n", stmt->as.block.stmt_count, stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.block.stmt_count; i++)
			render_ast_stmt_text(out, stmt->as.block.stmts[i], indent + 1);
		break;
	case AST_STMT_IF:
		fprintf(out, "IfStmt [line %d:%d]\n", stmt->line, stmt->col);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Condition:\n");
		render_ast_expr_text(out, stmt->as.if_stmt.condition, indent + 2);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Then:\n");
		render_ast_stmt_text(out, stmt->as.if_stmt.then_branch, indent + 2);
		if (stmt->as.if_stmt.else_branch) {
			print_ast_indent(out, indent + 1);
			fprintf(out, "Else:\n");
			render_ast_stmt_text(out, stmt->as.if_stmt.else_branch, indent + 2);
		}
		break;
	case AST_STMT_WHILE:
		fprintf(out, "WhileStmt [line %d:%d]\n", stmt->line, stmt->col);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Condition:\n");
		render_ast_expr_text(out, stmt->as.while_stmt.condition, indent + 2);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Body:\n");
		render_ast_stmt_text(out, stmt->as.while_stmt.body, indent + 2);
		break;
	case AST_STMT_DO_WHILE:
		fprintf(out, "DoWhileStmt [line %d:%d]\n", stmt->line, stmt->col);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Body:\n");
		render_ast_stmt_text(out, stmt->as.do_while_stmt.body, indent + 2);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Condition:\n");
		render_ast_expr_text(out, stmt->as.do_while_stmt.condition, indent + 2);
		break;
	case AST_STMT_FOR_C:
		fprintf(out, "ForCStmt [line %d:%d]\n", stmt->line, stmt->col);
		if (stmt->as.for_c.init) {
			print_ast_indent(out, indent + 1);
			fprintf(out, "Init:\n");
			render_ast_stmt_text(out, stmt->as.for_c.init, indent + 2);
		}
		if (stmt->as.for_c.condition) {
			print_ast_indent(out, indent + 1);
			fprintf(out, "Condition:\n");
			render_ast_expr_text(out, stmt->as.for_c.condition, indent + 2);
		}
		if (stmt->as.for_c.step) {
			print_ast_indent(out, indent + 1);
			fprintf(out, "Step:\n");
			render_ast_expr_text(out, stmt->as.for_c.step, indent + 2);
		}
		print_ast_indent(out, indent + 1);
		fprintf(out, "Body:\n");
		render_ast_stmt_text(out, stmt->as.for_c.body, indent + 2);
		break;
	case AST_STMT_FOR_IN:
		html_escape(stmt->as.for_in.item_var ? stmt->as.for_in.item_var : "", esc, sizeof(esc));
		fprintf(out, "ForInStmt (var %s) [line %d:%d]\n", esc, stmt->line, stmt->col);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Collection:\n");
		render_ast_expr_text(out, stmt->as.for_in.collection, indent + 2);
		print_ast_indent(out, indent + 1);
		fprintf(out, "Body:\n");
		render_ast_stmt_text(out, stmt->as.for_in.body, indent + 2);
		break;
	case AST_STMT_RETURN:
		fprintf(out, "ReturnStmt [line %d:%d]\n", stmt->line, stmt->col);
		if (stmt->as.return_expr)
			render_ast_expr_text(out, stmt->as.return_expr, indent + 1);
		break;
	case AST_STMT_BREAK:
		fprintf(out, "BreakStmt [line %d:%d]\n", stmt->line, stmt->col);
		break;
	case AST_STMT_CONTINUE:
		fprintf(out, "ContinueStmt [line %d:%d]\n", stmt->line, stmt->col);
		break;
	case AST_STMT_FUNC_DECL:
		html_escape(stmt->as.func_decl.return_type ? stmt->as.func_decl.return_type : "void", esc, sizeof(esc));
		html_escape(stmt->as.func_decl.name ? stmt->as.func_decl.name : "", esc2, sizeof(esc2));
		fprintf(out, "FuncDecl: %s %s(%d params)%s [line %d:%d]\n",
		        esc, esc2, stmt->as.func_decl.param_count, stmt->as.func_decl.is_static ? " (static)" : "", stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.func_decl.param_count; i++) {
			char pt[128], pn[128];
			html_escape(stmt->as.func_decl.params[i].type_name ? stmt->as.func_decl.params[i].type_name : "any", pt, sizeof(pt));
			html_escape(stmt->as.func_decl.params[i].name ? stmt->as.func_decl.params[i].name : "", pn, sizeof(pn));
			print_ast_indent(out, indent + 1);
			fprintf(out, "Param: %s %s\n", pt, pn);
		}
		if (stmt->as.func_decl.body)
			render_ast_stmt_text(out, stmt->as.func_decl.body, indent + 1);
		break;
	case AST_STMT_CLASS_DECL:
		html_escape(stmt->as.class_decl.name ? stmt->as.class_decl.name : "", esc, sizeof(esc));
		fprintf(out, "ClassDecl: %s%s%s [line %d:%d]\n",
		        esc, stmt->as.class_decl.base_name ? " : " : "", stmt->as.class_decl.base_name ? stmt->as.class_decl.base_name : "", stmt->line, stmt->col);
		for (int i = 0; i < stmt->as.class_decl.member_count; i++)
			render_ast_stmt_text(out, stmt->as.class_decl.members[i], indent + 1);
		break;
	default:
		fprintf(out, "UnknownStmt (%d)\n", stmt->type);
		break;
	}
}

/* --------------------------------------------------------------------------
 * Instruction Decoding Structure & Helper
 * -------------------------------------------------------------------------- */
typedef struct DecodedInstr {
	int offset;
	int next_offset;
	int line;
	uint8_t opcode;
	const OpcodeInfo* info;
	char hex_bytes[40];
	char operand_str[256];
	char operand_html[512];
	bool is_jump;
	int jump_target;
	char func_id[24];   /* owning function, used for unique DOM ids */
} DecodedInstr;

static int decode_instruction_rich(const XIrChunk* chunk, int offset,
                                   const char* func_id, DecodedInstr* out)
{
	if (!chunk || offset < 0 || offset >= chunk->count) return -1;

	memset(out, 0, sizeof(DecodedInstr));
	out->offset = offset;
	snprintf(out->func_id, sizeof(out->func_id), "%s", func_id ? func_id : "fn_0");
	out->line = chunk->lines ? chunk->lines[offset] : 0;
	out->opcode = chunk->code[offset];
	out->info = get_opcode_info(out->opcode);

	int next = offset + 1;

	/* Minimum operand bytes each opcode reads; refuse to decode past the end. */
	{
		int need = 0;
		switch ((XIrOpCode)out->opcode)
		{
		case OP_CONST_INT: need = 4; break;
		case OP_CONST_FLOAT: case OP_CONST_STR:
		case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL: case OP_LOAD_FIELD: case OP_STORE_FIELD:
		case OP_GET_FIELD_INDEX: case OP_SET_FIELD_INDEX:
		case OP_LOAD_LOCAL: case OP_STORE_LOCAL: case OP_BUILD_LIST:
		case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE: case OP_LOOP:
		case OP_CLOSURE: need = 2; break;
		case OP_NEW_INSTANCE: case OP_CALL: case OP_CALL_METHOD: need = 3; break;
		case OP_METHOD: need = 5; break;
		case OP_CLASS: need = 6; break;
		case OP_PRINT: case OP_GET_UPVALUE: case OP_SET_UPVALUE: need = 1; break;
		default: break;
		}
		if (next + need > chunk->count)
		{
			snprintf(out->operand_str, sizeof(out->operand_str), "<truncated>");
			snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-meta\">&lt;truncated&gt;</span>");
			out->next_offset = chunk->count;
			snprintf(out->hex_bytes, sizeof(out->hex_bytes), "%02X ", chunk->code[offset]);
			return chunk->count;
		}
	}

#define PEEK2() ((uint16_t)((chunk->code[next] << 8) | chunk->code[next+1]))
#define PEEK4() ((int32_t)((chunk->code[next]<<24)|(chunk->code[next+1]<<16)|(chunk->code[next+2]<<8)|chunk->code[next+3]))
#define SYM(i)  ((i) < chunk->symbols.count ? chunk->symbols.symbols[(i)] : "?")

	switch ((XIrOpCode)out->opcode)
	{
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
		int32_t v = PEEK4();
		snprintf(out->operand_str, sizeof(out->operand_str), "%d", v);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-num\">%d</span>", v);
		next += 4;
		break;
	}

	case OP_CONST_FLOAT:
	case OP_CONST_STR: {
		uint16_t idx = PEEK2();
		if (idx < chunk->constants.count) {
			XValue val = chunk->constants.values[idx];
			if (val.type == VAL_STRING && val.as.sval) {
				char esc[128];
				html_escape(val.as.sval, esc, sizeof(esc));
				snprintf(out->operand_str, sizeof(out->operand_str), "\"%s\"", val.as.sval);
				snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-str\">&quot;%s&quot;</span> <span class=\"val-meta\">(#%d)</span>", esc, idx);
			} else if (val.type == VAL_FLOAT) {
				snprintf(out->operand_str, sizeof(out->operand_str), "%g", val.as.fval);
				snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-num\">%g</span> <span class=\"val-meta\">(#%d)</span>", val.as.fval, idx);
			} else {
				snprintf(out->operand_str, sizeof(out->operand_str), "[#%d]", idx);
				snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-meta\">#%d</span>", idx);
			}
		} else {
			snprintf(out->operand_str, sizeof(out->operand_str), "[#%d]", idx);
			snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-meta\">#%d</span>", idx);
		}
		next += 2;
		break;
	}

	case OP_LOAD_GLOBAL: case OP_STORE_GLOBAL:
	case OP_LOAD_FIELD:  case OP_STORE_FIELD: {
		uint16_t idx = PEEK2();
		const char* sym = SYM(idx);
		char esc[128];
		html_escape(sym, esc, sizeof(esc));
		snprintf(out->operand_str, sizeof(out->operand_str), "%s", sym);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-sym\">%s</span> <span class=\"val-meta\">(sym #%d)</span>", esc, idx);
		next += 2;
		break;
	}

	case OP_GET_FIELD_INDEX: case OP_SET_FIELD_INDEX: {
		uint16_t slot = PEEK2();
		snprintf(out->operand_str, sizeof(out->operand_str), "slot [%d]", slot);
		snprintf(out->operand_html, sizeof(out->operand_html), "slot <span class=\"val-num\">[%d]</span>", slot);
		next += 2;
		break;
	}

	case OP_LOAD_LOCAL: case OP_STORE_LOCAL: {
		uint16_t slot = PEEK2();
		snprintf(out->operand_str, sizeof(out->operand_str), "slot %d", slot);
		snprintf(out->operand_html, sizeof(out->operand_html), "slot <span class=\"val-num\">%d</span>", slot);
		next += 2;
		break;
	}

	case OP_BUILD_LIST: {
		uint16_t cnt = PEEK2();
		snprintf(out->operand_str, sizeof(out->operand_str), "%d items", cnt);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-num\">%d</span> items", cnt);
		next += 2;
		break;
	}

	case OP_NEW_INSTANCE: {
		uint16_t si = PEEK2();
		uint8_t ac = chunk->code[next+2];
		const char* sym = SYM(si);
		char esc[128];
		html_escape(sym, esc, sizeof(esc));
		snprintf(out->operand_str, sizeof(out->operand_str), "%s (%d args)", sym, ac);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-sym\">%s</span> <span class=\"val-meta\">(%d args)</span>", esc, ac);
		next += 3;
		break;
	}

	case OP_CLASS: {
		uint16_t si = PEEK2();
		uint16_t fc = (chunk->code[next+4] << 8) | chunk->code[next+5];
		const char* sym = SYM(si);
		char esc[128];
		html_escape(sym, esc, sizeof(esc));
		snprintf(out->operand_str, sizeof(out->operand_str), "%s (%d fields)", sym, fc);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-sym\">%s</span> <span class=\"val-meta\">(%d fields)</span>", esc, fc);
		next += 6 + fc * 2;
		break;
	}

	case OP_METHOD: {
		uint16_t cs = PEEK2();
		uint16_t ms = (chunk->code[next+2] << 8) | chunk->code[next+3];
		const char* cs_name = SYM(cs);
		const char* ms_name = SYM(ms);
		char esc1[64], esc2[64];
		html_escape(cs_name, esc1, sizeof(esc1));
		html_escape(ms_name, esc2, sizeof(esc2));
		snprintf(out->operand_str, sizeof(out->operand_str), "%s.%s", cs_name, ms_name);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-sym\">%s</span>.<span class=\"val-sym\">%s</span>", esc1, esc2);
		next += 5;
		break;
	}

	case OP_JUMP: case OP_JUMP_IF_FALSE: case OP_JUMP_IF_TRUE: case OP_LOOP: {
		uint16_t tgt = PEEK2();
		out->is_jump = true;
		out->jump_target = tgt;
		snprintf(out->operand_str, sizeof(out->operand_str), "-> %04d", tgt);
		snprintf(out->operand_html, sizeof(out->operand_html),
		         "<a href=\"javascript:void(0)\" class=\"jump-link\" onclick=\"jumpToOffset('%s', %d, event)\" title=\"Jump to bytecode offset %04d\">&rarr; %04d</a>",
		         func_id ? func_id : "fn_0", tgt, tgt, tgt);
		next += 2;
		break;
	}

	case OP_CALL: {
		uint16_t si = PEEK2();
		uint8_t ac = chunk->code[next+2];
		const char* sym = (si == 0xFFFF) ? "<stack>" : SYM(si);
		char esc[128];
		html_escape(sym, esc, sizeof(esc));
		snprintf(out->operand_str, sizeof(out->operand_str), "%s (%d args)", sym, ac);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-sym\">%s</span> <span class=\"val-meta\">(%d args)</span>", esc, ac);
		next += 3;
		break;
	}

	case OP_CALL_METHOD: {
		uint16_t si = PEEK2();
		uint8_t ac = chunk->code[next+2];
		const char* sym = SYM(si);
		char esc[128];
		html_escape(sym, esc, sizeof(esc));
		snprintf(out->operand_str, sizeof(out->operand_str), ".%s (%d args)", sym, ac);
		snprintf(out->operand_html, sizeof(out->operand_html), ".<span class=\"val-sym\">%s</span> <span class=\"val-meta\">(%d args)</span>", esc, ac);
		next += 3;
		break;
	}

	case OP_PRINT: {
		uint8_t ac = chunk->code[next];
		snprintf(out->operand_str, sizeof(out->operand_str), "%d args", ac);
		snprintf(out->operand_html, sizeof(out->operand_html), "<span class=\"val-num\">%d</span> args", ac);
		next += 1;
		break;
	}

	case OP_CLOSURE: {
		uint16_t ci = PEEK2();
		if (ci < chunk->constants.count && chunk->constants.values[ci].type == VAL_FUNCTION &&
		    chunk->constants.values[ci].as.fnval) {
			XFunction* fn = chunk->constants.values[ci].as.fnval;
			char esc[128];
			html_escape(fn->name ? fn->name : "fn", esc, sizeof(esc));
			snprintf(out->operand_str, sizeof(out->operand_str), "<%s arity=%d>", fn->name ? fn->name : "fn", fn->arity);
			snprintf(out->operand_html, sizeof(out->operand_html), "&lt;<span class=\"val-sym\">%s</span> <span class=\"val-meta\">arity=%d upv=%d</span>&gt;",
			         esc, fn->arity, fn->upvalue_count);
			next += 2 + fn->upvalue_count * 2;
		} else {
			snprintf(out->operand_str, sizeof(out->operand_str), "<closure #%d>", ci);
			snprintf(out->operand_html, sizeof(out->operand_html), "&lt;closure #%d&gt;", ci);
			next += 2;
		}
		break;
	}

	case OP_GET_UPVALUE: case OP_SET_UPVALUE: {
		uint8_t slot = chunk->code[next];
		snprintf(out->operand_str, sizeof(out->operand_str), "upv %d", slot);
		snprintf(out->operand_html, sizeof(out->operand_html), "upv <span class=\"val-num\">%d</span>", slot);
		next += 1;
		break;
	}

	default:
		next = offset + 1;
		break;
	}

#undef PEEK2
#undef PEEK4
#undef SYM

	/* Variable-length operands (OP_CLASS fields, OP_CLOSURE upvalues) may claim more than exists */
	if (next > chunk->count) next = chunk->count;
	out->next_offset = next;

	/* Format Hex Byte Dump */
	char* hptr = out->hex_bytes;
	int hrem = sizeof(out->hex_bytes);
	for (int i = offset; i < next && hrem > 4; i++) {
		int written = snprintf(hptr, hrem, "%02X ", chunk->code[i]);
		if (written > 0 && written < hrem) {
			hptr += written;
			hrem -= written;
		}
	}

	return next;
}

/* --------------------------------------------------------------------------
 * Function Entry Catalog
 * -------------------------------------------------------------------------- */
typedef struct FuncEntry {
	char id[32];
	char name[64];
	int arity;
	int upvalues;
	const XIrChunk* chunk;
} FuncEntry;

typedef struct FuncCatalog {
	FuncEntry* entries;
	int count;
	int capacity;
} FuncCatalog;

static void catalog_init(FuncCatalog* cat)
{
	cat->entries = NULL;
	cat->count = 0;
	cat->capacity = 0;
}

static void catalog_add(FuncCatalog* cat, const char* name, int arity, int upv, const XIrChunk* chunk)
{
	if (cat->count >= cat->capacity) {
		int new_cap = cat->capacity == 0 ? 8 : cat->capacity * 2;
		FuncEntry* grown = (FuncEntry*)realloc(cat->entries, (size_t)new_cap * sizeof(FuncEntry));
		if (!grown) return;   /* keep existing entries valid; function is skipped */
		cat->entries = grown;
		cat->capacity = new_cap;
	}
	FuncEntry* fe = &cat->entries[cat->count];
	snprintf(fe->id, sizeof(fe->id), "fn_%d", cat->count);
	snprintf(fe->name, sizeof(fe->name), "%s", name && name[0] ? name : "main");
	fe->arity = arity;
	fe->upvalues = upv;
	fe->chunk = chunk;
	cat->count++;
}

static void catalog_collect(FuncCatalog* cat, const XIrChunk* chunk, const char* name, int arity, int upv)
{
	catalog_add(cat, name, arity, upv, chunk);
	for (int i = 0; i < chunk->constants.count; i++) {
		if (chunk->constants.values[i].type == VAL_FUNCTION &&
		    chunk->constants.values[i].as.fnval) {
			XFunction* fn = chunk->constants.values[i].as.fnval;
			catalog_collect(cat, &fn->chunk, fn->name ? fn->name : "anonymous", fn->arity, fn->upvalue_count);
		}
	}
}

static void catalog_free(FuncCatalog* cat)
{
	if (cat->entries) free(cat->entries);
	cat->entries = NULL;
	cat->count = cat->capacity = 0;
}

/* --------------------------------------------------------------------------
 * Read source file into line array
 * -------------------------------------------------------------------------- */
static char** read_source_lines(const char* path, int* count_out)
{
	*count_out = 0;
	if (!path) return NULL;
	FILE* f = fopen(path, "r");
	if (!f) return NULL;

	int lines = 0;
	int ch;
	while ((ch = fgetc(f)) != EOF) if (ch == '\n') lines++;
	lines++;
	rewind(f);

	char** arr = (char**)calloc((size_t)lines + 2, sizeof(char*));
	if (!arr) { fclose(f); return NULL; }

	/* Lines of any length: keep appending chunks until a newline is seen,
	 * so one source line is always exactly one entry. */
	char chunk_buf[1024];
	char* acc = NULL;
	size_t acc_len = 0;
	int idx = 0;
	while (idx < lines && fgets(chunk_buf, sizeof(chunk_buf), f))
	{
		size_t cl = strlen(chunk_buf);
		bool eol = cl > 0 && chunk_buf[cl-1] == '\n';
		char* grown = (char*)realloc(acc, acc_len + cl + 1);
		if (!grown) { free(acc); acc = NULL; break; }
		acc = grown;
		memcpy(acc + acc_len, chunk_buf, cl + 1);
		acc_len += cl;
		if (!eol && !feof(f)) continue;

		while (acc_len > 0 && (acc[acc_len-1] == '\n' || acc[acc_len-1] == '\r')) acc[--acc_len] = '\0';
		arr[idx++] = acc;      /* ownership moves to arr */
		acc = NULL;
		acc_len = 0;
	}
	free(acc);
	fclose(f);
	*count_out = idx;
	return arr;
}

/* --------------------------------------------------------------------------
 * Collect instructions grouped by source line
 * -------------------------------------------------------------------------- */
typedef struct InstrList {
	DecodedInstr* instrs;
	int count;
	int capacity;
} InstrList;

static void instr_list_append(InstrList* il, const DecodedInstr* d)
{
	if (il->count >= il->capacity) {
		int new_cap = il->capacity == 0 ? 8 : il->capacity * 2;
		DecodedInstr* grown = (DecodedInstr*)realloc(il->instrs, (size_t)new_cap * sizeof(DecodedInstr));
		if (!grown) return;   /* drop this instruction rather than crash */
		il->instrs = grown;
		il->capacity = new_cap;
	}
	il->instrs[il->count++] = *d;
}

static void collect_line_instrs(const FuncCatalog* cat, InstrList* by_line, int max_line)
{
	for (int f = 0; f < cat->count; f++) {
		const FuncEntry* fe = &cat->entries[f];
		int offset = 0;
		while (offset < fe->chunk->count) {
			DecodedInstr d;
			int next = decode_instruction_rich(fe->chunk, offset, fe->id, &d);
			if (next <= offset) break;

			if (d.line >= 1 && d.line <= max_line) {
				instr_list_append(&by_line[d.line], &d);
			}
			offset = next;
		}
	}
}

static void write_trace_view(FILE* out, const XVmTraceLog* trace, int src_line_count)
{
	(void)src_line_count;
	fprintf(out, "<div class=\"tab-content\" id=\"view-trace\">\n");

	if (!trace || trace->count == 0)
	{
		fprintf(out,
"  <div style=\"padding:60px 20px;text-align:center;\">\n"
"    <div style=\"font-size:36px;margin-bottom:12px;\">&#x25B6;</div>\n"
"    <div style=\"font-size:16px;font-weight:bold;margin-bottom:6px;color:var(--text-primary);\">No VM Execution Trace Captured</div>\n"
"    <div style=\"font-size:12px;color:var(--text-muted);\">Run <code>xlang --view &lt;script.xb&gt;</code> to execute the program in the Bytecode VM and capture the execution trace.</div>\n"
"  </div>\n"
"</div>\n");
		return;
	}

	int total_steps = trace->count;
	int max_stack = 0;
	int max_call = 1;
	for (int i = 0; i < total_steps; i++) {
		if (trace->steps[i].stack_depth > max_stack) max_stack = trace->steps[i].stack_depth;
		if (trace->steps[i].call_depth > max_call) max_call = trace->steps[i].call_depth;
	}

	/* Summary Metric Cards */
	fprintf(out,
"  <div class=\"metric-cards\">\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Executed Instructions</div>\n"
"      <div class=\"metric-val\">%d %s</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Max Stack Depth</div>\n"
"      <div class=\"metric-val\">%d <span style=\"font-size:12px;color:var(--text-muted)\">slots</span></div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Max Call Depth</div>\n"
"      <div class=\"metric-val\">%d <span style=\"font-size:12px;color:var(--text-muted)\">frames</span></div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">VM Exit Status</div>\n"
"      <div class=\"metric-val\"><span class=\"badge %s\">%s</span></div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Execution Engine</div>\n"
"      <div class=\"metric-val\" style=\"font-size:16px;\">Bytecode VM</div>\n"
"    </div>\n"
"  </div>\n\n",
		total_steps, trace->truncated ? "<span style=\"font-size:11px;color:var(--accent-peach)\">(truncated)</span>" : "",
		max_stack, max_call,
		strcmp(trace->exit_status, "VM_OK") == 0 ? "badge-highlight" : "badge-warn",
		trace->exit_status[0] ? trace->exit_status : "VM_OK");

	/* Stepper & Playback Controls Bar */
	fprintf(out,
"  <!-- Trace Stepper Toolbar -->\n"
"  <div class=\"trace-toolbar\">\n"
"    <div class=\"trace-ctrl-group\">\n"
"      <button class=\"trace-btn\" id=\"btn-trace-first\" onclick=\"traceJump(0)\" title=\"Jump to Start (Home)\">|&#x25C0;</button>\n"
"      <button class=\"trace-btn\" id=\"btn-trace-prev\" onclick=\"traceStep(-1)\" title=\"Step Back (Left Arrow / [)\">&#x25C0; Prev</button>\n"
"      <button class=\"trace-btn btn-primary\" id=\"btn-trace-play\" onclick=\"traceTogglePlay()\" title=\"Play / Pause (Space)\">&#x25B6; Play</button>\n"
"      <button class=\"trace-btn\" id=\"btn-trace-next\" onclick=\"traceStep(1)\" title=\"Step Forward (Right Arrow / ])\">Next &#x25B6;</button>\n"
"      <button class=\"trace-btn\" id=\"btn-trace-last\" onclick=\"traceJump(%d)\" title=\"Jump to End (End)\">&#x25B6;|</button>\n"
"    </div>\n"
"    <div class=\"trace-speed-group\">\n"
"      <span style=\"font-size:12px;color:var(--text-muted)\">Speed:</span>\n"
"      <select id=\"trace-speed\" onchange=\"traceSetSpeed(+this.value)\">\n"
"        <option value=\"400\">0.5x</option>\n"
"        <option value=\"200\" selected>1x</option>\n"
"        <option value=\"100\">2x</option>\n"
"        <option value=\"40\">5x</option>\n"
"        <option value=\"10\">10x</option>\n"
"      </select>\n"
"    </div>\n"
"    <div class=\"trace-slider-wrap\">\n"
"      <input type=\"range\" id=\"trace-slider\" min=\"0\" max=\"%d\" value=\"0\" oninput=\"traceJump(+this.value)\">\n"
"      <span id=\"trace-counter\" class=\"trace-counter-badge\">Step 1 / %d</span>\n"
"    </div>\n"
"    <div class=\"trace-filter-wrap\">\n"
"      <input type=\"text\" id=\"trace-filter\" placeholder=\"Filter opcode / func...\" oninput=\"filterTraceTable(this.value)\">\n"
"    </div>\n"
"  </div>\n\n",
		total_steps - 1, total_steps - 1, total_steps);

	/* Main Workspace */
	fprintf(out,
"  <!-- Trace Workspace: Timeline Table on Left, Live State Inspector on Right -->\n"
"  <div class=\"trace-workspace\">\n"
"    <div class=\"trace-table-pane\">\n"
"      <table class=\"data-grid trace-table\" id=\"trace-table\">\n"
"        <thead>\n"
"          <tr>\n"
"            <th style=\"width:55px;\">Step</th>\n"
"            <th style=\"width:100px;\">Function</th>\n"
"            <th style=\"width:50px;\">Line</th>\n"
"            <th style=\"width:60px;\">Offset</th>\n"
"            <th style=\"width:150px;\">Opcode</th>\n"
"            <th>Operands</th>\n"
"            <th style=\"width:150px;\">Stack Preview</th>\n"
"          </tr>\n"
"        </thead>\n"
"        <tbody id=\"trace-tbody\">\n");

	for (int i = 0; i < total_steps; i++) {
		const XVmTraceStep* s = &trace->steps[i];
		char esc_func[128]; html_escape(s->func_name, esc_func, sizeof(esc_func));
		char esc_opnd[256]; html_escape(s->operands, esc_opnd, sizeof(esc_opnd));
		char esc_stack[512]; html_escape(s->stack_summary, esc_stack, sizeof(esc_stack));

		fprintf(out,
"          <tr id=\"tr-step-%d\" data-step=\"%d\" data-line=\"%d\" data-off=\"%d\" onclick=\"traceJump(%d)\"%s>\n"
"            <td style=\"color:var(--text-muted);font-family:var(--font-mono);\">#%d</td>\n"
"            <td><span class=\"badge\" style=\"font-size:10px;\">%s</span></td>\n"
"            <td><a href=\"#\" onclick=\"event.stopPropagation();switchTab('split');selectLine(%d,true);return false;\" style=\"color:var(--accent-blue);\">L%d</a></td>\n"
"            <td style=\"font-family:var(--font-mono);color:var(--accent-yellow);\">%04d</td>\n"
"            <td><span class=\"op-badge\">%s</span></td>\n"
"            <td style=\"font-family:var(--font-mono);color:var(--text-primary);\">%s</td>\n"
"            <td style=\"font-family:var(--font-mono);color:var(--text-muted);font-size:11px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;\">%s</td>\n"
"          </tr>\n",
			i, i, s->line, s->offset, i, i == 0 ? " class=\"active-trace-row\"" : "",
			i + 1, esc_func, s->line, s->line, s->offset, s->op_name, esc_opnd, esc_stack);
	}

	char* esc_console = NULL;
	if (trace->captured_output && trace->output_len > 0)
	{
		size_t clen = trace->output_len * 4 + 1;
		esc_console = (char*)malloc(clen);
		if (esc_console)
		{
			html_escape(trace->captured_output, esc_console, (int)clen);
		}
	}

	fprintf(out,
"        </tbody>\n"
"      </table>\n"
"    </div>\n\n"
"    <!-- Right Inspector Pane -->\n"
"    <div class=\"trace-inspector-pane\">\n"
"      <div class=\"trace-panel-card\">\n"
"        <div class=\"section-title\">&#x1F50D; Current Step Inspector</div>\n"
"        <div id=\"inspector-step-info\">\n"
"          <div style=\"display:flex;justify-content:space-between;margin-bottom:8px;\">\n"
"            <span style=\"font-size:12px;color:var(--text-muted);\">Step:</span>\n"
"            <b id=\"insp-step-num\" style=\"font-family:var(--font-mono);color:var(--accent-cyan);\">#1</b>\n"
"          </div>\n"
"          <div style=\"display:flex;justify-content:space-between;margin-bottom:8px;\">\n"
"            <span style=\"font-size:12px;color:var(--text-muted);\">Function:</span>\n"
"            <span id=\"insp-func\" class=\"badge\">main</span>\n"
"          </div>\n"
"          <div style=\"display:flex;justify-content:space-between;margin-bottom:8px;\">\n"
"            <span style=\"font-size:12px;color:var(--text-muted);\">Source Location:</span>\n"
"            <span id=\"insp-loc\" style=\"font-family:var(--font-mono);color:var(--accent-blue);\">Line 1 (offset 0000)</span>\n"
"          </div>\n"
"          <div style=\"display:flex;justify-content:space-between;margin-bottom:8px;\">\n"
"            <span style=\"font-size:12px;color:var(--text-muted);\">Opcode &amp; Operands:</span>\n"
"            <b id=\"insp-op\" style=\"font-family:var(--font-mono);color:var(--accent-yellow);\">-</b>\n"
"          </div>\n"
"        </div>\n"
"      </div>\n\n"
"      <div class=\"trace-panel-card\">\n"
"        <div class=\"section-title\">&#x1F4DA; VM Value Stack (<span id=\"insp-stack-count\">0</span> values)</div>\n"
"        <div id=\"insp-stack-slots\" class=\"stack-slots-container\">\n"
"          <div style=\"color:var(--text-muted);font-size:12px;padding:8px;\">(Stack is empty)</div>\n"
"        </div>\n"
"      </div>\n\n"
"      <div class=\"trace-panel-card\">\n"
"        <div class=\"section-title\">&#x1F4DF; Program Console Output</div>\n"
"        <pre class=\"trace-console-output\" id=\"trace-console\">%s</pre>\n"
"      </div>\n"
"    </div>\n"
"  </div>\n"
"</div>\n",
		esc_console ? esc_console : "(No console output produced)");

	if (esc_console) free(esc_console);

	/* Embed TRACE_DATA */
	fprintf(out, "<script>\nconst TRACE_DATA = [\n");
	for (int i = 0; i < total_steps; i++) {
		const XVmTraceStep* s = &trace->steps[i];
		char esc_func[128]; js_escape(s->func_name, esc_func, sizeof(esc_func));
		char esc_opnd[256]; js_escape(s->operands, esc_opnd, sizeof(esc_opnd));
		char esc_stack[512]; js_escape(s->stack_summary, esc_stack, sizeof(esc_stack));
		fprintf(out, "  {step:%d,fn:\"%s\",line:%d,off:%d,op:\"%s\",opnd:\"%s\",sdepth:%d,cdepth:%d,stack:\"%s\"}%s\n",
		        i, esc_func, s->line, s->offset, s->op_name, esc_opnd, s->stack_depth, s->call_depth, esc_stack,
		        i < total_steps - 1 ? "," : "");
	}
	fprintf(out, "];\n</script>\n");
}

/* --------------------------------------------------------------------------
 * Full HTML Generation
 * -------------------------------------------------------------------------- */
bool xdis_html_write_full(const XIrChunk* chunk, const AstProgram* prog,
                          const char* source_path, const char* out_html,
                          const XVmTraceLog* trace)
{
	if (!chunk || !out_html) return false;

	/* 1. Build Function Catalog */
	FuncCatalog cat;
	catalog_init(&cat);
	catalog_collect(&cat, chunk, "main", 0, 0);

	/* 2. Read Source Lines */
	int src_line_count = 0;
	char** src_lines = read_source_lines(source_path, &src_line_count);

	/* 3. Collect instructions by line */
	InstrList* by_line = (InstrList*)calloc((size_t)src_line_count + 2, sizeof(InstrList));
	if (!by_line) src_line_count = 0;  /* degrade: no source/IR pairing, but no crash */
	if (src_line_count > 0) {
		collect_line_instrs(&cat, by_line, src_line_count);
	}

	/* 4. Capture LLVM IR if AST is provided */
	char* llvm_ir = capture_llvm_ir(prog, source_path);

	/* 5. Calculate AST Statistics */
	int ast_total_stmts = 0;
	int ast_total_exprs = 0;
	int ast_total_funcs = 0;
	int ast_total_classes = 0;
	int ast_max_depth = 0;
	if (prog) {
		for (int i = 0; i < prog->statement_count; i++) {
			count_stmt_stats(prog->statements[i], &ast_total_stmts, &ast_total_exprs,
			                 &ast_total_funcs, &ast_total_classes, 1, &ast_max_depth);
		}
	}

	/* 6. Calculate Bytecode Analytics */
	int total_instructions = 0;
	int total_bytes = 0;
	int op_counts[256] = {0};
	int cat_counts[12] = {0}; /* flow, math, bitwise, logic, var, obj, coll, call, const, stack, system, other */

	for (int f = 0; f < cat.count; f++) {
		const XIrChunk* c = cat.entries[f].chunk;
		total_bytes += c->count;
		int offset = 0;
		while (offset < c->count) {
			DecodedInstr d;
			int next = decode_instruction_rich(c, offset, cat.entries[f].id, &d);
			if (next <= offset) break;
			total_instructions++;
			op_counts[d.opcode]++;

			if (strcmp(d.info->category, "flow") == 0) cat_counts[0]++;
			else if (strcmp(d.info->category, "math") == 0) cat_counts[1]++;
			else if (strcmp(d.info->category, "bitwise") == 0) cat_counts[2]++;
			else if (strcmp(d.info->category, "logic") == 0) cat_counts[3]++;
			else if (strcmp(d.info->category, "var") == 0) cat_counts[4]++;
			else if (strcmp(d.info->category, "obj") == 0) cat_counts[5]++;
			else if (strcmp(d.info->category, "coll") == 0) cat_counts[6]++;
			else if (strcmp(d.info->category, "call") == 0) cat_counts[7]++;
			else if (strcmp(d.info->category, "const") == 0) cat_counts[8]++;
			else if (strcmp(d.info->category, "stack") == 0) cat_counts[9]++;
			else if (strcmp(d.info->category, "system") == 0) cat_counts[10]++;
			else cat_counts[11]++;

			offset = next;
		}
	}

	int unique_opcodes = 0;
	for (int i = 0; i < 256; i++) if (op_counts[i] > 0) unique_opcodes++;

	/* Open output file */
	FILE* out = fopen(out_html, "w");
	if (!out) {
		catalog_free(&cat);
		if (src_lines) {
			for (int i = 0; i < src_line_count; i++) free(src_lines[i]);
			free(src_lines);
		}
		if (by_line) {
			for (int i = 0; i <= src_line_count; i++) {
				if (by_line[i].instrs) free(by_line[i].instrs);
			}
			free(by_line);
		}
		if (llvm_ir) free(llvm_ir);
		return false;
	}

	const char* src_basename = source_path ? source_path : "program.xb";
	const char* b = strrchr(src_basename, '/');
	if (!b) b = strrchr(src_basename, '\\');
	if (b) src_basename = b + 1;
	char src_basename_esc[1024];
	html_escape(src_basename, src_basename_esc, sizeof(src_basename_esc));

	/* ----------------------------------------------------------------------
	 * HTML Document Header & CSS Styles
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<!DOCTYPE html>\n"
"<html lang=\"en\" data-theme=\"mocha\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"<title>xlang IR Explorer — %s</title>\n"
"<style>\n"
"  :root {\n"
"    --base-font-size: 13px;\n"
"    --font-mono: 'Cascadia Code', 'Fira Code', 'JetBrains Mono', Consolas, Menlo, monospace;\n"
"  }\n"
"  html[data-theme=\"mocha\"] {\n"
"    --bg-page: #1e1e2e; --bg-surface: #181825; --bg-panel: #11111b; --bg-active: #282a3f;\n"
"    --bg-hover: #262738; --border-color: #313244; --border-subtle: #24253a;\n"
"    --text-primary: #cdd6f4; --text-secondary: #a6adc8; --text-muted: #6c7086;\n"
"    --accent-blue: #89b4fa; --accent-cyan: #89dceb; --accent-green: #a6e3a1;\n"
"    --accent-yellow: #f9e2af; --accent-orange: #fab387; --accent-red: #f38ba8;\n"
"    --accent-purple: #cba6f7; --accent-teal: #94e2d5; --pulse-highlight: rgba(137, 180, 250, 0.45);\n"
"  }\n"
"  html[data-theme=\"tokyo\"] {\n"
"    --bg-page: #1a1b26; --bg-surface: #16161e; --bg-panel: #13141c; --bg-active: #24283b;\n"
"    --bg-hover: #202334; --border-color: #292e42; --border-subtle: #1e2233;\n"
"    --text-primary: #c0caf5; --text-secondary: #9aa5ce; --text-muted: #565f89;\n"
"    --accent-blue: #7aa2f7; --accent-cyan: #7dcfff; --accent-green: #9ece6a;\n"
"    --accent-yellow: #e0af68; --accent-orange: #ff9e64; --accent-red: #f7768e;\n"
"    --accent-purple: #bb9af7; --accent-teal: #73daca; --pulse-highlight: rgba(122, 162, 247, 0.45);\n"
"  }\n"
"  html[data-theme=\"light\"] {\n"
"    --bg-page: #eff1f5; --bg-surface: #e6e9ef; --bg-panel: #dce0e8; --bg-active: #ccd0da;\n"
"    --bg-hover: #bcc0cc; --border-color: #acb0be; --border-subtle: #bcc0cc;\n"
"    --text-primary: #4c4f69; --text-secondary: #5c5f77; --text-muted: #8c8fa1;\n"
"    --accent-blue: #1e66f5; --accent-cyan: #04a5e5; --accent-green: #40a02b;\n"
"    --accent-yellow: #df8e1d; --accent-orange: #fe640b; --accent-red: #d20f39;\n"
"    --accent-purple: #8839ef; --accent-teal: #179299; --pulse-highlight: rgba(30, 102, 245, 0.3);\n"
"  }\n"
"  * { box-sizing: border-box; margin: 0; padding: 0; }\n"
"  body {\n"
"    font-family: var(--font-mono);\n"
"    font-size: var(--base-font-size);\n"
"    background: var(--bg-page);\n"
"    color: var(--text-primary);\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    height: 100vh;\n"
"    overflow: hidden;\n"
"  }\n"
"  header {\n"
"    background: var(--bg-surface);\n"
"    border-bottom: 1px solid var(--border-color);\n"
"    padding: 8px 18px;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 14px;\n"
"    flex-wrap: wrap;\n"
"    z-index: 100;\n"
"  }\n"
"  .logo {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"    font-weight: bold;\n"
"    font-size: 15px;\n"
"    color: var(--accent-cyan);\n"
"  }\n"
"  .badge {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 4px;\n"
"    padding: 2px 7px;\n"
"    font-size: 11px;\n"
"    color: var(--text-secondary);\n"
"  }\n"
"  .badge-highlight {\n"
"    color: var(--accent-green);\n"
"    border-color: var(--accent-green);\n"
"  }\n"
"  .nav-tabs {\n"
"    display: flex;\n"
"    gap: 4px;\n"
"    margin-left: 10px;\n"
"  }\n"
"  .tab-btn {\n"
"    background: transparent;\n"
"    border: 1px solid transparent;\n"
"    color: var(--text-secondary);\n"
"    padding: 5px 12px;\n"
"    border-radius: 5px;\n"
"    cursor: pointer;\n"
"    font-family: inherit;\n"
"    font-size: 12px;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 6px;\n"
"    transition: all 0.15s ease;\n"
"  }\n"
"  .tab-btn:hover { background: var(--bg-hover); color: var(--text-primary); }\n"
"  .tab-btn.active {\n"
"    background: var(--bg-active);\n"
"    border-color: var(--border-color);\n"
"    color: var(--accent-blue);\n"
"    font-weight: bold;\n"
"  }\n"
"  .header-controls {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"    margin-left: auto;\n"
"  }\n"
"  .search-wrap {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 4px;\n"
"    padding: 2px 8px;\n"
"    gap: 4px;\n"
"  }\n"
"  .search-wrap input {\n"
"    background: transparent;\n"
"    border: none;\n"
"    outline: none;\n"
"    color: var(--text-primary);\n"
"    font-family: inherit;\n"
"    font-size: 11px;\n"
"    width: 140px;\n"
"  }\n"
"  .search-wrap input:focus { width: 200px; }\n"
"  .search-nav-btn {\n"
"    background: transparent;\n"
"    border: none;\n"
"    color: var(--text-muted);\n"
"    cursor: pointer;\n"
"    font-size: 11px;\n"
"    padding: 0 3px;\n"
"  }\n"
"  .search-nav-btn:hover { color: var(--accent-cyan); }\n"
"  .theme-select, .font-ctrl {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    color: var(--text-secondary);\n"
"    border-radius: 4px;\n"
"    padding: 3px 8px;\n"
"    font-family: inherit;\n"
"    font-size: 11px;\n"
"    cursor: pointer;\n"
"  }\n"
"  /* View Containers */\n"
"  .tab-content {\n"
"    flex: 1;\n"
"    display: none;\n"
"    min-height: 0;\n"
"    overflow: hidden;\n"
"  }\n"
"  .tab-content.active { display: flex; }\n"
"\n"
"  /* Tab 1: Split View */\n"
"  #view-split { display: none; }\n"
"  #view-split.active { display: flex; flex-direction: row; width: 100%%; }\n"
"  .pane {\n"
"    overflow-y: auto;\n"
"    height: 100%%;\n"
"  }\n"
"  #src-pane { width: 50%%; min-width: 250px; }\n"
"  #ir-pane { flex: 1; min-width: 250px; }\n"
"  .resizer {\n"
"    width: 6px;\n"
"    background: var(--border-color);\n"
"    cursor: col-resize;\n"
"    transition: background 0.15s;\n"
"    user-select: none;\n"
"    z-index: 20;\n"
"  }\n"
"  .resizer:hover, .resizer.dragging { background: var(--accent-blue); }\n"
"  .pane-header {\n"
"    position: sticky;\n"
"    top: 0;\n"
"    background: var(--bg-surface);\n"
"    border-bottom: 1px solid var(--border-color);\n"
"    padding: 7px 14px;\n"
"    font-size: 11px;\n"
"    color: var(--text-muted);\n"
"    text-transform: uppercase;\n"
"    letter-spacing: 1px;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    justify-content: space-between;\n"
"    z-index: 15;\n"
"  }\n"
"\n"
"  /* Source rows */\n"
"  .src-row {\n"
"    display: flex;\n"
"    align-items: flex-start;\n"
"    border-bottom: 1px solid var(--border-subtle);\n"
"    cursor: pointer;\n"
"    line-height: 20px;\n"
"    transition: background 0.08s ease;\n"
"  }\n"
"  .src-row:hover { background: var(--bg-hover) !important; }\n"
"  .src-row.active { background: var(--bg-active) !important; }\n"
"  .src-row.has-code { border-left: 3px solid var(--text-muted); }\n"
"  .src-row.has-code:hover { border-left-color: var(--accent-blue); }\n"
"  .src-row.active.has-code { border-left-color: var(--accent-cyan); }\n"
"  .line-num {\n"
"    width: 48px;\n"
"    min-width: 48px;\n"
"    padding: 2px 10px 2px 4px;\n"
"    text-align: right;\n"
"    color: var(--text-muted);\n"
"    user-select: none;\n"
"    font-size: 11px;\n"
"  }\n"
"  .src-code { padding: 2px 10px; white-space: pre; flex: 1; overflow-x: auto; }\n"
"  .line-ops-badge {\n"
"    font-size: 10px;\n"
"    color: var(--accent-teal);\n"
"    background: var(--bg-panel);\n"
"    padding: 1px 5px;\n"
"    border-radius: 3px;\n"
"    margin-right: 8px;\n"
"    align-self: center;\n"
"  }\n"
"\n"
"  /* IR Sections */\n"
"  .ir-section {\n"
"    padding: 6px 14px;\n"
"    border-bottom: 1px solid var(--border-subtle);\n"
"  }\n"
"  .ir-section.active { background: var(--bg-active); border-left: 3px solid var(--accent-blue); }\n"
"  .ir-sec-hdr {\n"
"    font-size: 11px;\n"
"    color: var(--text-muted);\n"
"    margin-bottom: 4px;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"  }\n"
"  .ir-instr-row {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    padding: 2px 8px;\n"
"    border-radius: 4px;\n"
"    line-height: 22px;\n"
"    transition: background 0.1s;\n"
"  }\n"
"  .ir-instr-row:hover { background: var(--bg-hover); }\n"
"  .ir-instr-row.target-highlight {\n"
"    animation: pulseGlow 1.4s ease-out;\n"
"  }\n"
"  @keyframes pulseGlow {\n"
"    0%% { background-color: var(--pulse-highlight); }\n"
"    100%% { background-color: transparent; }\n"
"  }\n"
"  .offset-tag {\n"
"    color: var(--text-muted);\n"
"    width: 48px;\n"
"    font-size: 11px;\n"
"  }\n"
"  .fn-badge {\n"
"    font-size: 10px;\n"
"    color: var(--accent-orange);\n"
"    margin-right: 6px;\n"
"  }\n"
"\n"
"  /* Opcode Pill Badges & Categories */\n"
"  .op-badge {\n"
"    display: inline-block;\n"
"    padding: 1px 7px;\n"
"    border-radius: 3px;\n"
"    font-size: 11px;\n"
"    font-weight: 600;\n"
"    min-width: 175px;\n"
"    cursor: help;\n"
"    position: relative;\n"
"  }\n"
"  .op-flow    { color: #cba6f7; background: rgba(203, 166, 247, 0.12); border: 1px solid rgba(203, 166, 247, 0.25); }\n"
"  .op-math    { color: #89dceb; background: rgba(137, 220, 235, 0.12); border: 1px solid rgba(137, 220, 235, 0.25); }\n"
"  .op-bitwise { color: #94e2d5; background: rgba(148, 226, 213, 0.12); border: 1px solid rgba(148, 226, 213, 0.25); }\n"
"  .op-logic   { color: #a6e3a1; background: rgba(166, 227, 161, 0.12); border: 1px solid rgba(166, 227, 161, 0.25); }\n"
"  .op-var     { color: #fab387; background: rgba(250, 179, 135, 0.12); border: 1px solid rgba(250, 179, 135, 0.25); }\n"
"  .op-obj     { color: #f9e2af; background: rgba(249, 226, 175, 0.12); border: 1px solid rgba(249, 226, 175, 0.25); }\n"
"  .op-coll    { color: #eba0ac; background: rgba(235, 160, 172, 0.12); border: 1px solid rgba(235, 160, 172, 0.25); }\n"
"  .op-call    { color: #f38ba8; background: rgba(243, 139, 168, 0.12); border: 1px solid rgba(243, 139, 168, 0.25); }\n"
"  .op-const   { color: #b4befe; background: rgba(180, 190, 254, 0.12); border: 1px solid rgba(180, 190, 254, 0.25); }\n"
"  .op-stack   { color: #9399b2; background: rgba(147, 153, 178, 0.12); border: 1px solid rgba(147, 153, 178, 0.25); }\n"
"  .op-system  { color: #f5c2e7; background: rgba(245, 194, 231, 0.12); border: 1px solid rgba(245, 194, 231, 0.25); }\n"
"\n"
"  .operand-cell { margin-left: 12px; color: var(--text-primary); }\n"
"  .jump-link {\n"
"    color: var(--accent-cyan);\n"
"    text-decoration: none;\n"
"    padding: 1px 6px;\n"
"    border-radius: 3px;\n"
"    background: rgba(137, 220, 235, 0.12);\n"
"    border: 1px solid rgba(137, 220, 235, 0.3);\n"
"    font-weight: bold;\n"
"    cursor: pointer;\n"
"  }\n"
"  .jump-link:hover {\n"
"    background: var(--accent-cyan);\n"
"    color: var(--bg-surface);\n"
"  }\n"
"  .val-sym  { color: var(--accent-yellow); font-weight: 500; }\n"
"  .val-num  { color: var(--accent-orange); }\n"
"  .val-str  { color: var(--accent-green); }\n"
"  .val-meta { color: var(--text-muted); font-size: 11px; }\n"
"\n"
"  /* Syntax Highlighting Tokens */\n"
"  .tok-kw      { color: var(--accent-purple); font-weight: bold; }\n"
"  .tok-type    { color: var(--accent-cyan); font-weight: 500; }\n"
"  .tok-const   { color: var(--accent-yellow); }\n"
"  .tok-str     { color: var(--accent-green); }\n"
"  .tok-num     { color: var(--accent-orange); }\n"
"  .tok-comment { color: var(--text-muted); font-style: italic; }\n"
"  .tok-fn      { color: var(--accent-blue); }\n"
"  .tok-op      { color: var(--accent-teal); }\n"
"  .llvm-reg    { color: var(--accent-red); }\n"
"  .llvm-global { color: var(--accent-yellow); }\n"
"\n"
"  /* Tooltip */\n"
"  #tooltip {\n"
"    position: fixed;\n"
"    display: none;\n"
"    background: var(--bg-surface);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 8px 12px;\n"
"    box-shadow: 0 8px 24px rgba(0,0,0,0.45);\n"
"    z-index: 1000;\n"
"    pointer-events: none;\n"
"    font-size: 12px;\n"
"    max-width: 320px;\n"
"  }\n"
"  #tooltip .tt-title { font-weight: bold; color: var(--accent-cyan); margin-bottom: 3px; }\n"
"  #tooltip .tt-cat   { font-size: 10px; color: var(--accent-purple); text-transform: uppercase; margin-bottom: 4px; }\n"
"  #tooltip .tt-stack { color: var(--accent-green); font-family: inherit; font-size: 11px; margin-bottom: 4px; }\n"
"  #tooltip .tt-desc  { color: var(--text-secondary); font-size: 11px; line-height: 1.4; }\n"
"\n"
"  /* Tab 2: Linear Bytecode */\n"
"  #view-linear {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 16px 24px;\n"
"    width: 100%%;\n"
"  }\n"
"  .linear-table {\n"
"    width: 100%%;\n"
"    border-collapse: collapse;\n"
"  }\n"
"  .linear-table th {\n"
"    text-align: left;\n"
"    padding: 6px 10px;\n"
"    border-bottom: 2px solid var(--border-color);\n"
"    color: var(--text-muted);\n"
"    font-size: 11px;\n"
"    text-transform: uppercase;\n"
"  }\n"
"  .linear-table td {\n"
"    padding: 4px 10px;\n"
"    border-bottom: 1px solid var(--border-subtle);\n"
"    font-size: 12px;\n"
"  }\n"
"  .linear-table tr:hover td { background: var(--bg-hover); }\n"
"  .linear-func-hdr td {\n"
"    background: var(--bg-panel) !important;\n"
"    color: var(--accent-yellow);\n"
"    font-weight: bold;\n"
"    padding: 10px 10px;\n"
"    border-top: 1px solid var(--border-color);\n"
"    border-bottom: 1px solid var(--border-color);\n"
"  }\n"
"  .hex-cell { color: var(--text-muted); font-size: 11px; width: 130px; }\n"
"  .src-link-badge {\n"
"    color: var(--accent-blue);\n"
"    text-decoration: none;\n"
"    background: var(--bg-panel);\n"
"    padding: 1px 6px;\n"
"    border-radius: 3px;\n"
"    font-size: 11px;\n"
"    cursor: pointer;\n"
"  }\n"
"  .src-link-badge:hover { background: var(--accent-blue); color: var(--bg-surface); }\n"
"\n"
"  /* Tab 3: AST Explorer */\n"
"  #view-ast {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 18px 24px;\n"
"    width: 100%%;\n"
"    gap: 16px;\n"
"  }\n"
"  .ast-toolbar {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 10px;\n"
"    background: var(--bg-surface);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 8px 14px;\n"
"    flex-wrap: wrap;\n"
"  }\n"
"  .ast-mode-btn {\n"
"    background: transparent;\n"
"    border: 1px solid var(--border-color);\n"
"    color: var(--text-secondary);\n"
"    padding: 3px 10px;\n"
"    border-radius: 4px;\n"
"    cursor: pointer;\n"
"    font-family: inherit;\n"
"    font-size: 11px;\n"
"  }\n"
"  .ast-mode-btn.active {\n"
"    background: var(--bg-active);\n"
"    color: var(--accent-cyan);\n"
"    border-color: var(--accent-cyan);\n"
"    font-weight: bold;\n"
"  }\n"
"  .ast-tree-container {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 16px 20px;\n"
"    overflow-x: auto;\n"
"    line-height: 24px;\n"
"  }\n"
"  .ast-node { margin: 2px 0; }\n"
"  .ast-summary {\n"
"    cursor: pointer;\n"
"    display: inline-flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"    user-select: none;\n"
"    padding: 1px 6px;\n"
"    border-radius: 4px;\n"
"  }\n"
"  .ast-summary:hover { background: var(--bg-hover); }\n"
"  .ast-leaf {\n"
"    display: inline-flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"    padding: 1px 6px;\n"
"  }\n"
"  .ast-children {\n"
"    border-left: 1px dashed var(--border-color);\n"
"    margin-left: 18px;\n"
"    padding-left: 14px;\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    gap: 2px;\n"
"  }\n"
"  .ast-role-label {\n"
"    font-size: 10px;\n"
"    color: var(--text-muted);\n"
"    text-transform: uppercase;\n"
"    margin-top: 4px;\n"
"  }\n"
"  .ast-type-badge {\n"
"    display: inline-block;\n"
"    padding: 1px 6px;\n"
"    border-radius: 3px;\n"
"    font-size: 11px;\n"
"    font-weight: bold;\n"
"  }\n"
"  .ast-t-func   { color: #cba6f7; background: rgba(203, 166, 247, 0.15); border: 1px solid rgba(203, 166, 247, 0.3); }\n"
"  .ast-t-class  { color: #89b4fa; background: rgba(137, 180, 250, 0.15); border: 1px solid rgba(137, 180, 250, 0.3); }\n"
"  .ast-t-var    { color: #fab387; background: rgba(250, 179, 135, 0.15); border: 1px solid rgba(250, 179, 135, 0.3); }\n"
"  .ast-t-ctrl   { color: #f9e2af; background: rgba(249, 226, 175, 0.15); border: 1px solid rgba(249, 226, 175, 0.3); }\n"
"  .ast-t-expr   { color: #89dceb; background: rgba(137, 220, 235, 0.15); border: 1px solid rgba(137, 220, 235, 0.3); }\n"
"  .ast-t-lit    { color: #a6e3a1; background: rgba(166, 227, 161, 0.15); border: 1px solid rgba(166, 227, 161, 0.3); }\n"
"  .ast-t-ident  { color: #94e2d5; background: rgba(148, 226, 213, 0.15); border: 1px solid rgba(148, 226, 213, 0.3); }\n"
"  .ast-t-block  { color: #9399b2; background: rgba(147, 153, 178, 0.15); border: 1px solid rgba(147, 153, 178, 0.3); }\n"
"  .ast-name     { color: var(--text-primary); font-weight: 500; font-size: 12px; }\n"
"  .ast-val      { color: var(--accent-orange); font-size: 12px; }\n"
"  .ast-line-link {\n"
"    color: var(--text-muted);\n"
"    font-size: 10px;\n"
"    text-decoration: none;\n"
"    background: var(--bg-surface);\n"
"    border: 1px solid var(--border-color);\n"
"    padding: 0 4px;\n"
"    border-radius: 3px;\n"
"    cursor: pointer;\n"
"  }\n"
"  .ast-line-link:hover { color: var(--accent-cyan); border-color: var(--accent-cyan); }\n"
"\n"
"  /* Tab 4: LLVM IR */\n"
"  #view-llvm {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 16px 24px;\n"
"    width: 100%%;\n"
"  }\n"
"  .code-viewer {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 12px;\n"
"    overflow-x: auto;\n"
"    line-height: 20px;\n"
"  }\n"
"  .llvm-line { display: flex; align-items: flex-start; }\n"
"  .llvm-num { width: 44px; color: var(--text-muted); text-align: right; margin-right: 14px; user-select: none; font-size: 11px; }\n"
"  .llvm-content { flex: 1; white-space: pre; }\n"
"\n"
"  /* Tab 5: Constants & Symbols */\n"
"  #view-tables {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 20px 28px;\n"
"    width: 100%%;\n"
"    gap: 24px;\n"
"  }\n"
"  .section-title {\n"
"    font-size: 14px;\n"
"    color: var(--accent-cyan);\n"
"    margin-bottom: 8px;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 8px;\n"
"  }\n"
"  .data-grid {\n"
"    width: 100%%;\n"
"    border-collapse: collapse;\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    overflow: hidden;\n"
"  }\n"
"  .data-grid th {\n"
"    background: var(--bg-surface);\n"
"    padding: 7px 12px;\n"
"    text-align: left;\n"
"    font-size: 11px;\n"
"    color: var(--text-muted);\n"
"    text-transform: uppercase;\n"
"    border-bottom: 1px solid var(--border-color);\n"
"  }\n"
"  .data-grid td {\n"
"    padding: 6px 12px;\n"
"    border-bottom: 1px solid var(--border-subtle);\n"
"    font-size: 12px;\n"
"  }\n"
"  .data-grid tr:hover td { background: var(--bg-hover); }\n"
"\n"
"  /* Tab 6: Opcode Analytics */\n"
"  #view-analytics {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 20px 28px;\n"
"    width: 100%%;\n"
"    gap: 20px;\n"
"  }\n"
"  .metric-cards {\n"
"    display: grid;\n"
"    grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));\n"
"    gap: 14px;\n"
"  }\n"
"  .metric-card {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 12px 16px;\n"
"  }\n"
"  .metric-label { font-size: 11px; color: var(--text-muted); text-transform: uppercase; margin-bottom: 4px; }\n"
"  .metric-val { font-size: 20px; font-weight: bold; color: var(--accent-cyan); }\n"
"  .dist-bar {\n"
"    height: 16px;\n"
"    border-radius: 4px;\n"
"    overflow: hidden;\n"
"    display: flex;\n"
"    margin: 8px 0 16px 0;\n"
"    background: var(--bg-panel);\n"
"  }\n"
"  .dist-seg {\n"
"    height: 100%%;\n"
"    transition: width 0.3s ease;\n"
"  }\n"
"  .legend {\n"
"    display: flex;\n"
"    flex-wrap: wrap;\n"
"    gap: 14px;\n"
"    margin-bottom: 16px;\n"
"  }\n"
"  .legend-item {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 6px;\n"
"    font-size: 11px;\n"
"    color: var(--text-secondary);\n"
"  }\n"
"  .legend-color {\n"
"    width: 10px;\n"
"    height: 10px;\n"
"    border-radius: 2px;\n"
"  }\n"
"\n"
"  /* Tab 7: VM Execution Trace */\n"
"  #view-trace {\n"
"    flex-direction: column;\n"
"    overflow-y: auto;\n"
"    padding: 20px 28px;\n"
"    width: 100%%;\n"
"    gap: 16px;\n"
"    height: 100%%;\n"
"  }\n"
"  .trace-toolbar {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 12px;\n"
"    background: var(--bg-surface);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 8px 14px;\n"
"    flex-wrap: wrap;\n"
"  }\n"
"  .trace-ctrl-group {\n"
"    display: flex;\n"
"    gap: 4px;\n"
"    align-items: center;\n"
"  }\n"
"  .trace-btn {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    color: var(--text-primary);\n"
"    padding: 5px 10px;\n"
"    border-radius: 4px;\n"
"    font-size: 12px;\n"
"    cursor: pointer;\n"
"    transition: all 0.15s ease;\n"
"  }\n"
"  .trace-btn:hover {\n"
"    background: var(--bg-hover);\n"
"    border-color: var(--accent-blue);\n"
"  }\n"
"  .trace-btn.btn-primary {\n"
"    background: var(--accent-blue);\n"
"    color: #11111b;\n"
"    font-weight: bold;\n"
"    border-color: var(--accent-blue);\n"
"  }\n"
"  .trace-speed-group {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 6px;\n"
"  }\n"
"  .trace-speed-group select {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    color: var(--text-primary);\n"
"    padding: 4px 8px;\n"
"    border-radius: 4px;\n"
"    font-size: 12px;\n"
"  }\n"
"  .trace-slider-wrap {\n"
"    flex: 1;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    gap: 10px;\n"
"    min-width: 200px;\n"
"  }\n"
"  .trace-slider-wrap input[type=range] {\n"
"    flex: 1;\n"
"    cursor: pointer;\n"
"  }\n"
"  .trace-counter-badge {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    padding: 4px 8px;\n"
"    border-radius: 4px;\n"
"    font-size: 11px;\n"
"    font-family: var(--font-mono);\n"
"    color: var(--accent-yellow);\n"
"    white-space: nowrap;\n"
"  }\n"
"  .trace-filter-wrap input {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    color: var(--text-primary);\n"
"    padding: 5px 10px;\n"
"    border-radius: 4px;\n"
"    font-size: 12px;\n"
"    width: 180px;\n"
"  }\n"
"  .trace-workspace {\n"
"    display: flex;\n"
"    gap: 16px;\n"
"    flex: 1;\n"
"    min-height: 480px;\n"
"  }\n"
"  .trace-table-pane {\n"
"    flex: 3;\n"
"    overflow-y: auto;\n"
"    max-height: 600px;\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    background: var(--bg-panel);\n"
"  }\n"
"  .trace-table {\n"
"    width: 100%%;\n"
"    border-collapse: collapse;\n"
"  }\n"
"  .trace-table tr {\n"
"    cursor: pointer;\n"
"    transition: background 0.1s ease;\n"
"  }\n"
"  .trace-table tr.active-trace-row {\n"
"    background: var(--bg-active) !important;\n"
"    border-left: 3px solid var(--accent-yellow);\n"
"  }\n"
"  .trace-table tr:hover td {\n"
"    background: var(--bg-hover);\n"
"  }\n"
"  .trace-inspector-pane {\n"
"    flex: 2;\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    gap: 14px;\n"
"    max-height: 600px;\n"
"    overflow-y: auto;\n"
"  }\n"
"  .trace-panel-card {\n"
"    background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 6px;\n"
"    padding: 12px 16px;\n"
"  }\n"
"  .stack-slots-container {\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    gap: 6px;\n"
"    max-height: 180px;\n"
"    overflow-y: auto;\n"
"    margin-top: 8px;\n"
"  }\n"
"  .stack-slot {\n"
"    display: flex;\n"
"    align-items: center;\n"
"    justify-content: space-between;\n"
"    background: var(--bg-surface);\n"
"    border: 1px solid var(--border-subtle);\n"
"    border-radius: 4px;\n"
"    padding: 4px 10px;\n"
"    font-family: var(--font-mono);\n"
"    font-size: 11px;\n"
"  }\n"
"  .stack-slot.stack-top-slot {\n"
"    border-color: var(--accent-cyan);\n"
"    background: var(--pulse-highlight);\n"
"  }\n"
"  .trace-console-output {\n"
"    background: #0d1117;\n"
"    color: #c9d1d9;\n"
"    border: 1px solid var(--border-color);\n"
"    border-radius: 4px;\n"
"    padding: 10px;\n"
"    font-family: var(--font-mono);\n"
"    font-size: 12px;\n"
"    max-height: 160px;\n"
"    overflow-y: auto;\n"
"    white-space: pre-wrap;\n"
"    word-break: break-all;\n"
"    margin-top: 6px;\n"
"  }\n"
"\n"
"  /* Search match highlights */\n"
"  .search-match {\n"
"    background: rgba(249, 226, 175, 0.4);\n"
"    color: var(--accent-yellow);\n"
"    border-radius: 2px;\n"
"  }\n"
"  .search-match.current {\n"
"    background: var(--accent-orange);\n"
"    color: #11111b;\n"
"    font-weight: bold;\n"
"  }\n"
"\n"
"  /* Scrollbar */\n"
"  ::-webkit-scrollbar { width: 7px; height: 7px; }\n"
"  ::-webkit-scrollbar-track { background: var(--bg-surface); }\n"
"  ::-webkit-scrollbar-thumb { background: var(--border-color); border-radius: 4px; }\n"
"  ::-webkit-scrollbar-thumb:hover { background: var(--text-muted); }\n"
"\n"
"  /* ===================== UI v2 ===================== */\n"
"  html { color-scheme: dark; }\n"
"  html[data-theme=\"light\"] { color-scheme: light; }\n"
"  html[data-theme=\"mocha\"], html[data-theme=\"tokyo\"] {\n"
"    --c-coll: #eba0ac; --c-const: #b4befe; --c-stack: #9399b2; --c-system: #f5c2e7;\n"
"    --shadow: 0 8px 24px rgba(0,0,0,0.45);\n"
"  }\n"
"  html[data-theme=\"light\"] {\n"
"    --c-coll: #e64553; --c-const: #7287fd; --c-stack: #6c6f85; --c-system: #ea76cb;\n"
"    --shadow: 0 8px 24px rgba(76,79,105,0.25);\n"
"  }\n"
"  * { scrollbar-width: thin; scrollbar-color: var(--border-color) transparent; }\n"
"  ::selection { background: var(--pulse-highlight); }\n"
"  body { height: 100vh; height: 100dvh; }\n"
"  header { gap: 8px 14px; }\n"
"  .nav-tabs { margin-left: 6px; overflow-x: auto; scrollbar-width: none; max-width: 100%%; }\n"
"  .tab-btn { white-space: nowrap; }\n"
"  .tab-btn:focus-visible, .font-ctrl:focus-visible, .theme-select:focus-visible,\n"
"  .ast-mode-btn:focus-visible, .chip:focus-visible, .search-nav-btn:focus-visible,\n"
"  a:focus-visible, .src-row:focus-visible {\n"
"    outline: 2px solid var(--accent-blue); outline-offset: 1px;\n"
"  }\n"
"  .search-wrap:focus-within { border-color: var(--accent-blue); }\n"
"  .src-row, .ir-section, .ir-instr-row, tr[id], .linear-func-hdr { scroll-margin-top: 40px; scroll-margin-bottom: 8px; }\n"
"  .ir-section.active { box-shadow: inset 0 0 0 1px var(--border-color); }\n"
"\n"
"  /* Theme-aware opcode and AST badge colours (readable on the light theme too) */\n"
"  .op-flow { --c: var(--accent-purple); } .op-math { --c: var(--accent-cyan); }\n"
"  .op-bitwise { --c: var(--accent-teal); } .op-logic { --c: var(--accent-green); }\n"
"  .op-var { --c: var(--accent-orange); } .op-obj { --c: var(--accent-yellow); }\n"
"  .op-coll { --c: var(--c-coll); } .op-call { --c: var(--accent-red); }\n"
"  .op-const { --c: var(--c-const); } .op-stack { --c: var(--c-stack); }\n"
"  .op-system { --c: var(--c-system); }\n"
"  .op-badge {\n"
"    color: var(--c, var(--text-secondary));\n"
"    background: color-mix(in srgb, var(--c, var(--text-muted)) 14%%, transparent);\n"
"    border: 1px solid color-mix(in srgb, var(--c, var(--text-muted)) 34%%, transparent);\n"
"  }\n"
"  .ast-type-badge {\n"
"    color: var(--c, var(--text-muted));\n"
"    background: color-mix(in srgb, var(--c, var(--text-muted)) 15%%, transparent);\n"
"    border: 1px solid color-mix(in srgb, var(--c, var(--text-muted)) 34%%, transparent);\n"
"  }\n"
"  .ast-t-func { --c: var(--accent-purple); } .ast-t-class { --c: var(--accent-blue); }\n"
"  .ast-t-var { --c: var(--accent-orange); } .ast-t-ctrl { --c: var(--accent-yellow); }\n"
"  .ast-t-expr { --c: var(--accent-cyan); } .ast-t-lit { --c: var(--accent-green); }\n"
"  .ast-t-ident { --c: var(--accent-teal); } .ast-t-block { --c: var(--text-muted); }\n"
"  .jump-link { background: color-mix(in srgb, var(--accent-cyan) 14%%, transparent); border-color: color-mix(in srgb, var(--accent-cyan) 38%%, transparent); }\n"
"  #tooltip { box-shadow: var(--shadow); }\n"
"\n"
"  /* Search highlights */\n"
"  mark.search-match {\n"
"    background: color-mix(in srgb, var(--accent-yellow) 40%%, transparent);\n"
"    color: inherit; border-radius: 2px;\n"
"    box-shadow: 0 0 0 1px color-mix(in srgb, var(--accent-yellow) 60%%, transparent);\n"
"  }\n"
"  mark.search-match.current { background: var(--accent-orange); color: var(--bg-page); box-shadow: none; }\n"
"\n"
"  /* Linear view: sticky column header + toolbar */\n"
"  .linear-table th { position: sticky; top: 0; background: var(--bg-surface); z-index: 5; }\n"
"  .lin-toolbar { display: flex; flex-wrap: wrap; align-items: center; gap: 8px; margin-bottom: 12px; }\n"
"  .lin-toolbar select {\n"
"    background: var(--bg-panel); color: var(--text-secondary); border: 1px solid var(--border-color);\n"
"    border-radius: 4px; padding: 3px 8px; font-family: inherit; font-size: 11px; max-width: 360px;\n"
"  }\n"
"  .lin-toolbar .lbl { font-size: 11px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; }\n"
"  .chip {\n"
"    border: 1px solid var(--border-color); background: transparent; color: var(--text-secondary);\n"
"    border-radius: 999px; padding: 2px 10px; font-family: inherit; font-size: 11px; cursor: pointer;\n"
"  }\n"
"  .chip:hover { background: var(--bg-hover); }\n"
"  .chip.on { color: var(--bg-page); background: var(--c, var(--accent-blue)); border-color: var(--c, var(--accent-blue)); font-weight: bold; }\n"
"  .chip[data-cat=\"flow\"] { --c: var(--accent-purple); } .chip[data-cat=\"math\"] { --c: var(--accent-cyan); }\n"
"  .chip[data-cat=\"bitwise\"] { --c: var(--accent-teal); } .chip[data-cat=\"logic\"] { --c: var(--accent-green); }\n"
"  .chip[data-cat=\"var\"] { --c: var(--accent-orange); } .chip[data-cat=\"obj\"] { --c: var(--accent-yellow); }\n"
"  .chip[data-cat=\"coll\"] { --c: var(--c-coll); } .chip[data-cat=\"call\"] { --c: var(--accent-red); }\n"
"  .chip[data-cat=\"const\"] { --c: var(--c-const); } .chip[data-cat=\"stack\"] { --c: var(--c-stack); }\n"
"  .chip[data-cat=\"system\"] { --c: var(--c-system); }\n"
"\n"
"  /* Status bar, toast, help overlay */\n"
"  #statusbar {\n"
"    display: flex; align-items: center; gap: 18px; flex-shrink: 0; padding: 3px 14px;\n"
"    background: var(--bg-surface); border-top: 1px solid var(--border-color);\n"
"    font-size: 11px; color: var(--text-muted); white-space: nowrap; overflow: hidden;\n"
"  }\n"
"  #statusbar b { color: var(--text-secondary); font-weight: 600; }\n"
"  #statusbar .sb-right { margin-left: auto; }\n"
"  #toast {\n"
"    position: fixed; bottom: 36px; left: 50%%; transform: translate(-50%%, 12px);\n"
"    background: var(--bg-active); color: var(--text-primary); border: 1px solid var(--border-color);\n"
"    border-radius: 6px; padding: 7px 14px; font-size: 12px; box-shadow: var(--shadow);\n"
"    opacity: 0; pointer-events: none; transition: opacity 0.18s, transform 0.18s; z-index: 2000;\n"
"  }\n"
"  #toast.show { opacity: 1; transform: translate(-50%%, 0); }\n"
"  #help {\n"
"    position: fixed; inset: 0; display: none; align-items: center; justify-content: center;\n"
"    background: rgba(0,0,0,0.5); z-index: 3000;\n"
"  }\n"
"  #help.open { display: flex; }\n"
"  #help .card {\n"
"    background: var(--bg-surface); border: 1px solid var(--border-color); border-radius: 8px;\n"
"    padding: 18px 24px; min-width: 320px; max-width: 92vw; box-shadow: var(--shadow);\n"
"  }\n"
"  #help h3 { color: var(--accent-cyan); font-size: 14px; margin-bottom: 10px; }\n"
"  #help table { border-collapse: collapse; width: 100%%; }\n"
"  #help td { padding: 3px 0; font-size: 12px; color: var(--text-secondary); }\n"
"  #help td:first-child { padding-right: 20px; white-space: nowrap; }\n"
"  kbd {\n"
"    font-family: inherit; font-size: 11px; color: var(--text-primary); background: var(--bg-panel);\n"
"    border: 1px solid var(--border-color); border-bottom-width: 2px; border-radius: 4px; padding: 0 6px;\n"
"  }\n"
"\n"
"  /* Narrow screens: stack the split view, trim the header */\n"
"  @media (max-width: 860px) {\n"
"    header .badge:not(.badge-highlight) { display: none; }\n"
"    .header-controls { margin-left: 0; width: 100%%; }\n"
"    .search-wrap { flex: 1; }\n"
"    .search-wrap input, .search-wrap input:focus { width: 100%%; }\n"
"    #view-split.active { flex-direction: column; }\n"
"    #src-pane { width: 100%% !important; flex: 0 0 45%%; min-height: 140px; height: auto; }\n"
"    #ir-pane { flex: 1; min-height: 140px; height: auto; }\n"
"    .resizer { display: none; }\n"
"    .op-badge { min-width: 0; }\n"
"    #statusbar .sb-right { display: none; }\n"
"  }\n"
"  @media (prefers-reduced-motion: reduce) {\n"
"    * { animation: none !important; transition: none !important; scroll-behavior: auto !important; }\n"
"  }\n"
"  @media print {\n"
"    header, #statusbar, #toast, #help { display: none !important; }\n"
"    body { height: auto; overflow: visible; }\n"
"    .tab-content.active { display: block; overflow: visible; height: auto; }\n"
"  }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"\n"
"<!-- Tooltip element -->\n"
"<div id=\"tooltip\">\n"
"  <div class=\"tt-title\" id=\"tt-title\"></div>\n"
"  <div class=\"tt-cat\" id=\"tt-cat\"></div>\n"
"  <div class=\"tt-stack\" id=\"tt-stack\"></div>\n"
"  <div class=\"tt-desc\" id=\"tt-desc\"></div>\n"
"</div>\n"
"\n"
"<!-- Header -->\n"
"<header>\n"
"  <div class=\"logo\">\n"
"    <span>&#x1F9EE;</span> xlang IR Explorer\n"
"  </div>\n"
"  <span class=\"badge badge-highlight\">%s</span>\n"
"  <span class=\"badge\">%d Lines</span>\n"
"  <span class=\"badge\">%d Instructions</span>\n"
"  <span class=\"badge\">%d Bytecode Bytes</span>\n"
"  <span class=\"badge\">%d Functions</span>\n",
	src_basename_esc, src_basename_esc,
	src_line_count, total_instructions, total_bytes, cat.count);

	if (prog) {
		fprintf(out, "  <span class=\"badge\" style=\"color:var(--accent-purple);border-color:var(--accent-purple);\">%d AST Nodes</span>\n",
		        ast_total_stmts + ast_total_exprs);
	}

	if (trace && trace->count > 0) {
		fprintf(out, "  <span class=\"badge\" style=\"color:var(--accent-green);border-color:var(--accent-green);\">%d VM Steps</span>\n",
		        trace->count);
	}

	fprintf(out,
"\n"
"  <!-- Tab Navigation -->\n"
"  <div class=\"nav-tabs\">\n"
"    <button class=\"tab-btn active\" onclick=\"switchTab('split', this)\">&#x25EB; Split View <span style=\"opacity:0.6\">(1)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('linear', this)\">&#x2261; Linear Bytecode <span style=\"opacity:0.6\">(2)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('ast', this)\">&#x1F333; AST Tree <span style=\"opacity:0.6\">(3)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('llvm', this)\">&#x2699; LLVM IR <span style=\"opacity:0.6\">(4)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('tables', this)\">&#x26C1; Constants &amp; Symbols <span style=\"opacity:0.6\">(5)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('analytics', this)\">&#x1F4CA; Analytics <span style=\"opacity:0.6\">(6)</span></button>\n"
"    <button class=\"tab-btn\" onclick=\"switchTab('trace', this)\">&#x25B6; VM Trace <span style=\"opacity:0.6\">(7)</span></button>\n"
"  </div>\n"
"\n"
"  <!-- Header Controls -->\n"
"  <div class=\"header-controls\">\n"
"    <div class=\"search-wrap\">\n"
"      <input type=\"text\" id=\"search-input\" placeholder=\"Search IR / Source (Ctrl+F)\" oninput=\"handleSearch()\" onkeydown=\"searchKey(event)\">\n"
"      <button class=\"search-nav-btn\" onclick=\"prevMatch()\" title=\"Previous match\">&#x25B2;</button>\n"
"      <button class=\"search-nav-btn\" onclick=\"nextMatch()\" title=\"Next match\">&#x25BC;</button>\n"
"      <span id=\"search-count\" style=\"font-size:10px;color:var(--text-muted);margin-left:2px;\"></span>\n"
"    </div>\n"
"    <button class=\"font-ctrl\" onclick=\"changeFontSize(-1)\" title=\"Decrease font size\">A-</button>\n"
"    <button class=\"font-ctrl\" onclick=\"changeFontSize(1)\" title=\"Increase font size\">A+</button>\n"
"    <select class=\"theme-select\" id=\"theme-select\" onchange=\"changeTheme(this.value)\">\n"
"      <option value=\"mocha\">Catppuccin Mocha</option>\n"
"      <option value=\"tokyo\">Tokyo Night</option>\n"
"      <option value=\"light\">Catppuccin Latte</option>\n"
"    </select>\n"
"  </div>\n"
"</header>\n");

	/* ----------------------------------------------------------------------
	 * Tab 1: Split View (Source & Correlated Bytecode IR)
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content active\" id=\"view-split\">\n"
"  <!-- Left: Source Code Pane -->\n"
"  <div class=\"pane\" id=\"src-pane\">\n"
"    <div class=\"pane-header\">\n"
"      <span>&#x1F4C4; Source: %s</span>\n"
"      <span style=\"font-size:10px;\">Click line to inspect IR</span>\n"
"    </div>\n",
	src_basename_esc);

	bool in_block_comment = false;
	for (int ln = 1; ln <= src_line_count; ln++) {
		const char* raw = src_lines[ln-1] ? src_lines[ln-1] : "";
		/* Worst case is ~35 output bytes per input byte (one-char operator in a span) */
		size_t hl_sz = strlen(raw) * 40 + 64;
		char* highlighted = (char*)malloc(hl_sz);
		if (highlighted) highlight_xlang_line(raw, highlighted, (int)hl_sz, &in_block_comment);
		int num_ops = by_line[ln].count;

		fprintf(out,
"    <div class=\"src-row%s%s\" data-line=\"%d\" onclick=\"selectLine(%d, true)\">\n"
"      <span class=\"line-num\">%d</span>\n",
		num_ops > 0 ? " has-code" : "",
		ln == 1 ? " active" : "",
		ln, ln, ln);

		if (num_ops > 0) {
			fprintf(out, "      <span class=\"line-ops-badge\">%d op%s</span>\n",
			        num_ops, num_ops > 1 ? "s" : "");
		}
		fprintf(out,
"      <span class=\"src-code\">%s</span>\n"
"    </div>\n",
		highlighted ? highlighted : "");
		free(highlighted);
	}

	fprintf(out,
"  </div>\n"
"\n"
"  <!-- Draggable Divider Resizer -->\n"
"  <div class=\"resizer\" id=\"split-resizer\"></div>\n"
"\n"
"  <!-- Right: Bytecode IR Pane -->\n"
"  <div class=\"pane\" id=\"ir-pane\">\n"
"    <div class=\"pane-header\">\n"
"      <span>&#x26A1; Correlated Bytecode IR</span>\n"
"      <span style=\"font-size:10px;\">Hover opcode for stack effect &bull; Click &rarr; to follow jump</span>\n"
"    </div>\n");

	for (int ln = 1; ln <= src_line_count; ln++) {
		InstrList* il = &by_line[ln];
		fprintf(out,
"    <div class=\"ir-section%s\" id=\"ir-sec-%d\">\n"
"      <div class=\"ir-sec-hdr\">\n"
"        <span>&#x25B6; Line %d</span>\n"
"        <span style=\"color:var(--text-muted);font-size:10px;\">(%d instruction%s)</span>\n"
"      </div>\n",
		ln == 1 ? " active" : "", ln, ln, il->count, il->count == 1 ? "" : "s");

		if (il->count == 0) {
			fprintf(out, "      <div style=\"color:var(--text-muted);font-size:11px;padding:3px 8px;\">&mdash; No bytecode emitted &mdash;</div>\n");
		} else {
			for (int k = 0; k < il->count; k++) {
				DecodedInstr* d = &il->instrs[k];
				fprintf(out,
"      <div class=\"ir-instr-row\" id=\"instr-split-%s-%04d\" data-line=\"%d\" onclick=\"selectLine(%d, false)\">\n"
"        <span class=\"offset-tag\">%04d</span>\n"
"        <span class=\"op-badge op-%s\" onmouseenter=\"showTooltip(event, '%s', '%s', '%s', '%s')\" onmouseleave=\"hideTooltip()\">%s</span>\n"
"        <span class=\"operand-cell\">%s</span>\n"
"      </div>\n",
				d->func_id, d->offset, ln, ln,
				d->offset,
				d->info->category,
				d->info->name, d->info->category_label, d->info->stack_eff, d->info->desc,
				d->info->name,
				d->operand_html[0] ? d->operand_html : "");
			}
		}
		fprintf(out, "    </div>\n");
	}

	fprintf(out,
"  </div>\n"
"</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 2: Linear Bytecode Stream
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content\" id=\"view-linear\">\n"
"  <table class=\"linear-table\">\n"
"    <thead>\n"
"      <tr>\n"
"        <th style=\"width:60px;\">Offset</th>\n"
"        <th style=\"width:130px;\">Hex Bytes</th>\n"
"        <th style=\"width:60px;\">Line</th>\n"
"        <th style=\"width:190px;\">Opcode</th>\n"
"        <th>Operands &amp; Jump Targets</th>\n"
"      </tr>\n"
"    </thead>\n"
"    <tbody>\n");

	for (int f = 0; f < cat.count; f++) {
		const FuncEntry* fe = &cat.entries[f];
		char esc_fn[128];
		html_escape(fe->name, esc_fn, sizeof(esc_fn));
		fprintf(out,
"      <tr class=\"linear-func-hdr\">\n"
"        <td colspan=\"5\">&#x1F4E6; Function: %s (Arity: %d, Upvalues: %d, Bytecode: %d bytes)</td>\n"
"      </tr>\n",
		esc_fn, fe->arity, fe->upvalues, fe->chunk->count);

		int offset = 0;
		while (offset < fe->chunk->count) {
			DecodedInstr d;
			int next = decode_instruction_rich(fe->chunk, offset, fe->id, &d);
			if (next <= offset) break;

			fprintf(out,
"      <tr id=\"instr-%s-%04d\" data-line=\"%d\">\n"
"        <td class=\"offset-tag\" style=\"font-weight:bold;\">%04d</td>\n"
"        <td class=\"hex-cell\">%s</td>\n"
"        <td><a href=\"javascript:void(0)\" class=\"src-link-badge\" onclick=\"goToSourceLine(%d)\">L%d</a></td>\n"
"        <td><span class=\"op-badge op-%s\" onmouseenter=\"showTooltip(event, '%s', '%s', '%s', '%s')\" onmouseleave=\"hideTooltip()\">%s</span></td>\n"
"        <td>%s</td>\n"
"      </tr>\n",
			fe->id, d.offset, d.line,
			d.offset,
			d.hex_bytes,
			d.line, d.line,
			d.info->category,
			d.info->name, d.info->category_label, d.info->stack_eff, d.info->desc,
			d.info->name,
			d.operand_html[0] ? d.operand_html : "<span style=\"color:var(--text-muted)\">&mdash;</span>");

			offset = next;
		}
	}

	fprintf(out,
"    </tbody>\n"
"  </table>\n"
"</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 3: AST Tree & Inspector
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content\" id=\"view-ast\">\n");

	if (prog) {
		fprintf(out,
"  <!-- AST Toolbar & Metrics -->\n"
"  <div class=\"ast-toolbar\">\n"
"    <div class=\"section-title\" style=\"margin-bottom:0;\">&#x1F333; Abstract Syntax Tree</div>\n"
"    <div style=\"display:flex;gap:6px;\">\n"
"      <button class=\"ast-mode-btn active\" onclick=\"switchAstView('tree', this)\">&#x1F333; Tree View</button>\n"
"      <button class=\"ast-mode-btn\" onclick=\"switchAstView('text', this)\">&#x1F4C4; Formatted Dump</button>\n"
"    </div>\n"
"    <div style=\"display:flex;gap:6px;margin-left:auto;\">\n"
"      <button class=\"ast-mode-btn\" onclick=\"setAllAstOpen(true)\">&#x229E; Expand All</button>\n"
"      <button class=\"ast-mode-btn\" onclick=\"setAllAstOpen(false)\">&#x229F; Collapse All</button>\n"
"      <button class=\"ast-mode-btn\" onclick=\"copyAstText()\">&#x1F4CB; Copy AST</button>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <!-- Metric Stats Cards -->\n"
"  <div class=\"metric-cards\" style=\"margin-bottom:8px;\">\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Statements</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Expression Nodes</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Functions Declared</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Classes Declared</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Max Tree Depth</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <!-- Tree Root View -->\n"
"  <div class=\"ast-tree-container\" id=\"ast-tree-root\">\n",
		ast_total_stmts, ast_total_exprs, ast_total_funcs, ast_total_classes, ast_max_depth);

		for (int i = 0; i < prog->statement_count; i++) {
			render_ast_stmt_tree(out, prog->statements[i]);
		}

		fprintf(out,
"  </div>\n"
"\n"
"  <!-- Textual Dump View -->\n"
"  <div class=\"code-viewer\" id=\"ast-text-root\" style=\"display:none;\">\n"
"    <pre id=\"ast-text-pre\" style=\"font-family:inherit;font-size:12px;color:var(--text-primary);line-height:20px;\">\n");

		for (int i = 0; i < prog->statement_count; i++) {
			render_ast_stmt_text(out, prog->statements[i], 0);
		}

		fprintf(out,
"    </pre>\n"
"  </div>\n");
	} else {
		fprintf(out,
"  <div style=\"text-align:center;padding:60px 20px;color:var(--text-muted);\">\n"
"    <div style=\"font-size:32px;margin-bottom:12px;\">&#x1F333;</div>\n"
"    <div style=\"font-size:15px;color:var(--text-secondary);margin-bottom:6px;\">AST Not Available</div>\n"
"    <div style=\"font-size:12px;\">Compile using <code>xlang --view &lt;script.xb&gt;</code> to inspect the full Abstract Syntax Tree.</div>\n"
"  </div>\n");
	}

	fprintf(out, "</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 4: LLVM IR
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content\" id=\"view-llvm\">\n");
	if (llvm_ir && llvm_ir[0]) {
		fprintf(out,
"  <div style=\"display:flex;align-items:center;justify-content:space-between;margin-bottom:10px;\">\n"
"    <div class=\"section-title\">&#x2699; Emitted LLVM Intermediate Representation (.ll)</div>\n"
"    <button class=\"font-ctrl\" onclick=\"copyLlvmCode()\">&#x1F4CB; Copy LLVM IR</button>\n"
"  </div>\n"
"  <div class=\"code-viewer\" id=\"llvm-viewer\">\n");

		/* Parse LLVM IR line by line */
		const char* cur = llvm_ir;
		int llvm_ln = 1;
		while (*cur) {
			const char* nl = strchr(cur, '\n');
			size_t llen = nl ? (size_t)(nl - cur) : strlen(cur);
			size_t esc_sz = llen * 40 + 64;
			char* linebuf = (char*)malloc(llen + 1);
			char* esc_llvm = (char*)malloc(esc_sz);
			if (linebuf && esc_llvm) {
				memcpy(linebuf, cur, llen);
				linebuf[llen] = '\0';
				highlight_llvm_line(linebuf, esc_llvm, (int)esc_sz);
			} else if (esc_llvm) {
				esc_llvm[0] = '\0';
			}

			fprintf(out,
"    <div class=\"llvm-line\">\n"
"      <span class=\"llvm-num\">%d</span>\n"
"      <span class=\"llvm-content\">%s</span>\n"
"    </div>\n",
			llvm_ln++, esc_llvm ? esc_llvm : "");
			free(linebuf);
			free(esc_llvm);

			if (!nl) break;
			cur = nl + 1;
		}
		fprintf(out, "  </div>\n");
	} else {
		fprintf(out,
"  <div style=\"text-align:center;padding:60px 20px;color:var(--text-muted);\">\n"
"    <div style=\"font-size:32px;margin-bottom:12px;\">&#x2699;</div>\n"
"    <div style=\"font-size:15px;color:var(--text-secondary);margin-bottom:6px;\">LLVM IR Not Generated</div>\n"
"    <div style=\"font-size:12px;\">Compile using <code>xlang --view &lt;script.xb&gt;</code> to generate both bytecode and native LLVM IR explorer tabs.</div>\n"
"  </div>\n");
	}
	fprintf(out, "</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 5: Constants & Symbols Inspector
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content\" id=\"view-tables\">\n"
"  <!-- Constant Pool -->\n"
"  <div>\n"
"    <div class=\"section-title\">&#x26C1; Constant Pool Inspector</div>\n"
"    <table class=\"data-grid\">\n"
"      <thead>\n"
"        <tr>\n"
"          <th style=\"width:100px;\">Index</th>\n"
"          <th style=\"width:140px;\">Scope / Function</th>\n"
"          <th style=\"width:130px;\">Type</th>\n"
"          <th>Resolved Value &amp; Representation</th>\n"
"        </tr>\n"
"      </thead>\n"
"      <tbody>\n");

	for (int f = 0; f < cat.count; f++) {
		const FuncEntry* fe = &cat.entries[f];
		const XIrValuePool* pool = &fe->chunk->constants;
		for (int i = 0; i < pool->count; i++) {
			XValue v = pool->values[i];
			const char* type_str = "UNKNOWN";
			char val_desc[512] = "";

			switch (v.type) {
			case VAL_NULL:   type_str = "VAL_NULL"; snprintf(val_desc, sizeof(val_desc), "<span class=\"tok-const\">null</span>"); break;
			case VAL_BOOL:   type_str = "VAL_BOOL"; snprintf(val_desc, sizeof(val_desc), "<span class=\"tok-const\">%s</span>", v.as.bval ? "true" : "false"); break;
			case VAL_INT:    type_str = "VAL_INT";  snprintf(val_desc, sizeof(val_desc), "<span class=\"val-num\">%" PRId64 "</span>", v.as.ival); break;
			case VAL_FLOAT:  type_str = "VAL_FLOAT"; snprintf(val_desc, sizeof(val_desc), "<span class=\"val-num\">%g</span>", v.as.fval); break;
			case VAL_STRING: {
				type_str = "VAL_STRING";
				char esc_val[256];
				html_escape(v.as.sval ? v.as.sval : "", esc_val, sizeof(esc_val));
				snprintf(val_desc, sizeof(val_desc), "<span class=\"val-str\">&quot;%s&quot;</span> <span class=\"val-meta\">(len: %zu)</span>",
				         esc_val, v.as.sval ? strlen(v.as.sval) : 0);
				break;
			}
			case VAL_FUNCTION: {
				type_str = "VAL_FUNCTION";
				XFunction* fn = v.as.fnval;
				if (fn) {
					char esc_fn_name[128];
					html_escape(fn->name ? fn->name : "fn", esc_fn_name, sizeof(esc_fn_name));
					snprintf(val_desc, sizeof(val_desc), "<span class=\"val-sym\">%s</span> (arity: %d, upvalues: %d, chunk: %d bytes)",
					         esc_fn_name, fn->arity, fn->upvalue_count, fn->chunk.count);
				} else {
					snprintf(val_desc, sizeof(val_desc), "Function (null)");
				}
				break;
			}
			case VAL_CLOSURE: type_str = "VAL_CLOSURE"; snprintf(val_desc, sizeof(val_desc), "Closure"); break;
			case VAL_OBJECT:  type_str = "VAL_OBJECT";  snprintf(val_desc, sizeof(val_desc), "Object Reference"); break;
			}

			char esc_fn[128];
			html_escape(fe->name, esc_fn, sizeof(esc_fn));
			fprintf(out,
"        <tr>\n"
"          <td style=\"font-weight:bold;color:var(--accent-blue);\">#%d</td>\n"
"          <td><span class=\"badge\">%s</span></td>\n"
"          <td><span class=\"badge\" style=\"color:var(--accent-purple)\">%s</span></td>\n"
"          <td>%s</td>\n"
"        </tr>\n",
			i, esc_fn, type_str, val_desc);
		}
	}

	fprintf(out,
"      </tbody>\n"
"    </table>\n"
"  </div>\n"
"\n"
"  <!-- Symbol Table -->\n"
"  <div>\n"
"    <div class=\"section-title\">&#x2728; Symbol Table Inspector</div>\n"
"    <table class=\"data-grid\">\n"
"      <thead>\n"
"        <tr>\n"
"          <th style=\"width:100px;\">Index</th>\n"
"          <th style=\"width:140px;\">Scope / Function</th>\n"
"          <th>Symbol Identifier Name</th>\n"
"        </tr>\n"
"      </thead>\n"
"      <tbody>\n");

	for (int f = 0; f < cat.count; f++) {
		const FuncEntry* fe = &cat.entries[f];
		const XIrSymbolTable* syms = &fe->chunk->symbols;
		for (int i = 0; i < syms->count; i++) {
			char esc_sym[128], esc_fn[128];
			html_escape(syms->symbols[i] ? syms->symbols[i] : "?", esc_sym, sizeof(esc_sym));
			html_escape(fe->name, esc_fn, sizeof(esc_fn));
			fprintf(out,
"        <tr>\n"
"          <td style=\"font-weight:bold;color:var(--accent-yellow);\">sym #%d</td>\n"
"          <td><span class=\"badge\">%s</span></td>\n"
"          <td><span class=\"val-sym\">%s</span></td>\n"
"        </tr>\n",
			i, esc_fn, esc_sym);
		}
	}

	fprintf(out,
"      </tbody>\n"
"    </table>\n"
"  </div>\n"
"</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 6: Opcode Analytics
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<div class=\"tab-content\" id=\"view-analytics\">\n"
"  <!-- Summary Metric Cards -->\n"
"  <div class=\"metric-cards\">\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Bytecode Size</div>\n"
"      <div class=\"metric-val\">%d <span style=\"font-size:12px;color:var(--text-muted)\">bytes</span></div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Instructions</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Unique Opcodes</div>\n"
"      <div class=\"metric-val\">%d <span style=\"font-size:12px;color:var(--text-muted)\">/ %zu</span></div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Functions / Closures</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"    <div class=\"metric-card\">\n"
"      <div class=\"metric-label\">Source Lines</div>\n"
"      <div class=\"metric-val\">%d</div>\n"
"    </div>\n"
"  </div>\n"
"\n"
"  <!-- Category Breakdown Distribution Bar -->\n"
"  <div>\n"
"    <div class=\"section-title\">&#x1F4CA; Opcode Category Distribution</div>\n"
"    <div class=\"dist-bar\">\n",
	total_bytes, total_instructions, unique_opcodes, sizeof(OP_METAS)/sizeof(OP_METAS[0]), cat.count, src_line_count);

	static const char* const cat_names[] = {
		"Control Flow", "Arithmetic", "Bitwise", "Logic", "Variables",
		"Objects", "Collections", "Calls", "Constants", "Stack", "System"
	};
	static const char* const cat_colors[] = {
		"#cba6f7", "#89dceb", "#94e2d5", "#a6e3a1", "#fab387",
		"#f9e2af", "#eba0ac", "#f38ba8", "#b4befe", "#9399b2", "#f5c2e7"
	};

	for (int i = 0; i < 11; i++) {
		if (cat_counts[i] > 0 && total_instructions > 0) {
			float pct = (float)cat_counts[i] * 100.0f / (float)total_instructions;
			fprintf(out, "      <div class=\"dist-seg\" style=\"width:%.2f%%;background:%s;\" title=\"%s: %d (%.1f%%)\"></div>\n",
			        pct, cat_colors[i], cat_names[i], cat_counts[i], pct);
		}
	}

	fprintf(out,
"    </div>\n"
"    <div class=\"legend\">\n");

	for (int i = 0; i < 11; i++) {
		if (cat_counts[i] > 0 && total_instructions > 0) {
			float pct = (float)cat_counts[i] * 100.0f / (float)total_instructions;
			fprintf(out,
"      <div class=\"legend-item\">\n"
"        <div class=\"legend-color\" style=\"background:%s;\"></div>\n"
"        <span>%s: <b>%d</b> (%.1f%%)</span>\n"
"      </div>\n",
			cat_colors[i], cat_names[i], cat_counts[i], pct);
		}
	}

	fprintf(out,
"    </div>\n"
"  </div>\n"
"\n"
"  <!-- Frequency Table -->\n"
"  <div>\n"
"    <div class=\"section-title\">&#x26A1; Opcode Frequency Breakdown</div>\n"
"    <table class=\"data-grid\">\n"
"      <thead>\n"
"        <tr>\n"
"          <th style=\"width:60px;\">Rank</th>\n"
"          <th style=\"width:220px;\">Opcode</th>\n"
"          <th style=\"width:150px;\">Category</th>\n"
"          <th style=\"width:90px;\">Count</th>\n"
"          <th style=\"width:90px;\">Share</th>\n"
"          <th>Distribution Bar</th>\n"
"        </tr>\n"
"      </thead>\n"
"      <tbody>\n");

	/* Sort opcodes by frequency descending */
	typedef struct OpSortItem { uint8_t op; int count; } OpSortItem;
	OpSortItem sort_items[256];
	int sort_count = 0;
	for (int i = 0; i < 256; i++) {
		if (op_counts[i] > 0) {
			sort_items[sort_count].op = (uint8_t)i;
			sort_items[sort_count].count = op_counts[i];
			sort_count++;
		}
	}
	for (int i = 0; i < sort_count - 1; i++) {
		for (int j = i + 1; j < sort_count; j++) {
			if (sort_items[j].count > sort_items[i].count) {
				OpSortItem tmp = sort_items[i];
				sort_items[i] = sort_items[j];
				sort_items[j] = tmp;
			}
		}
	}

	for (int rank = 0; rank < sort_count; rank++) {
		uint8_t op = sort_items[rank].op;
		int cnt = sort_items[rank].count;
		const OpcodeInfo* info = get_opcode_info(op);
		float pct = total_instructions > 0 ? (float)cnt * 100.0f / (float)total_instructions : 0.0f;

		fprintf(out,
"        <tr>\n"
"          <td style=\"color:var(--text-muted);font-weight:bold;\">#%d</td>\n"
"          <td><span class=\"op-badge op-%s\">%s</span></td>\n"
"          <td><span class=\"badge\">%s</span></td>\n"
"          <td style=\"font-weight:bold;color:var(--accent-cyan);\">%d</td>\n"
"          <td style=\"color:var(--text-secondary);\">%.1f%%</td>\n"
"          <td>\n"
"            <div style=\"background:var(--bg-surface);height:8px;border-radius:4px;overflow:hidden;\">\n"
"              <div style=\"width:%.2f%%;height:100%%;background:var(--accent-blue);\"></div>\n"
"            </div>\n"
"          </td>\n"
"        </tr>\n",
		rank + 1, info->category, info->name, info->category_label, cnt, pct, pct);
	}

	fprintf(out,
"      </tbody>\n"
"    </table>\n"
"  </div>\n"
"</div>\n");

	/* ----------------------------------------------------------------------
	 * Tab 7: VM Execution Trace
	 * ---------------------------------------------------------------------- */
	write_trace_view(out, trace, src_line_count);

	/* ----------------------------------------------------------------------
	 * Client-Side JavaScript
	 * ---------------------------------------------------------------------- */
	fprintf(out,
"<footer id=\"statusbar\" role=\"status\">\n"
"  <span id=\"sb-pos\"></span>\n"
"  <span id=\"sb-info\"></span>\n"
"  <span class=\"sb-right\">Press <kbd>?</kbd> for shortcuts</span>\n"
"</footer>\n"
"<div id=\"toast\" role=\"status\" aria-live=\"polite\"></div>\n"
"<div id=\"help\" onclick=\"toggleHelp(false)\">\n"
"  <div class=\"card\" onclick=\"event.stopPropagation()\">\n"
"    <h3>Keyboard shortcuts</h3>\n"
"    <table>\n"
"      <tr><td><kbd>1</kbd>&ndash;<kbd>7</kbd></td><td>Switch tab</td></tr>\n"
"      <tr><td><kbd>[</kbd> <kbd>]</kbd> / <kbd>&larr;</kbd> <kbd>&rarr;</kbd></td><td>Step back / forward (VM Trace)</td></tr>\n"
"      <tr><td><kbd>Space</kbd></td><td>Play / Pause (VM Trace)</td></tr>\n"
"      <tr><td><kbd>&uarr;</kbd> <kbd>&darr;</kbd> / <kbd>k</kbd> <kbd>j</kbd></td><td>Previous / next source line (Split view)</td></tr>\n"
"      <tr><td><kbd>/</kbd> or <kbd>Ctrl</kbd>+<kbd>F</kbd></td><td>Search the current tab</td></tr>\n"
"      <tr><td><kbd>Enter</kbd> / <kbd>Shift</kbd>+<kbd>Enter</kbd></td><td>Next / previous match (in search box)</td></tr>\n"
"      <tr><td><kbd>n</kbd> / <kbd>N</kbd></td><td>Next / previous match</td></tr>\n"
"      <tr><td><kbd>Esc</kbd></td><td>Clear search / close this panel</td></tr>\n"
"      <tr><td><kbd>t</kbd></td><td>Cycle theme</td></tr>\n"
"      <tr><td><kbd>+</kbd> <kbd>-</kbd></td><td>Font size</td></tr>\n"
"      <tr><td><kbd>?</kbd></td><td>Show / hide this panel</td></tr>\n"
"    </table>\n"
"  </div>\n"
"</div>\n"
"<script>\n"
"const TOTAL_LINES = %d;\n"
"const TABS = ['split', 'linear', 'ast', 'llvm', 'tables', 'analytics', 'trace'];\n"
"const TAB_LABELS = { split: 'Split View', linear: 'Linear Bytecode', ast: 'AST Tree', llvm: 'LLVM IR', tables: 'Constants & Symbols', analytics: 'Analytics', trace: 'VM Execution Trace' };\n"
"const THEMES = ['mocha', 'tokyo', 'light'];\n"
"const REDUCED = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;\n"
"const SCROLL = REDUCED ? 'auto' : 'smooth';\n"
"let activeLine = 1;\n"
"let activeTab = 'split';\n"
"let searchMatches = [];\n"
"let currentMatchIdx = -1;\n"
"let currentFontSize = 13;\n"
"\n"
"/* localStorage can throw (private mode, some file:// setups) - never let it break the UI */\n"
"const store = {\n"
"  get(k) { try { return localStorage.getItem(k); } catch (e) { return null; } },\n"
"  set(k, v) { try { localStorage.setItem(k, v); } catch (e) {} }\n"
"};\n"
"\n"
"/* ---------- Toast ---------- */\n"
"let toastTimer = null;\n"
"function toast(msg) {\n"
"  const t = document.getElementById('toast');\n"
"  t.textContent = msg;\n"
"  t.classList.add('show');\n"
"  clearTimeout(toastTimer);\n"
"  toastTimer = setTimeout(() => t.classList.remove('show'), 1800);\n"
"}\n"
"\n"
"/* ---------- Status bar & URL hash ---------- */\n"
"function updateStatus() {\n"
"  const pos = document.getElementById('sb-pos');\n"
"  const info = document.getElementById('sb-info');\n"
"  if (activeTab === 'split') {\n"
"    const rows = document.querySelectorAll('#ir-sec-' + activeLine + ' .ir-instr-row');\n"
"    pos.innerHTML = 'Ln <b>' + activeLine + '</b> / ' + TOTAL_LINES;\n"
"    if (rows.length) {\n"
"      const a = rows[0].querySelector('.offset-tag').textContent;\n"
"      const b = rows[rows.length - 1].querySelector('.offset-tag').textContent;\n"
"      info.textContent = rows.length + (rows.length === 1 ? ' instruction' : ' instructions') + '  \\u00b7  offsets ' + a + (a === b ? '' : '\\u2013' + b);\n"
"    } else {\n"
"      info.textContent = 'no bytecode on this line';\n"
"    }\n"
"  } else {\n"
"    pos.textContent = TAB_LABELS[activeTab] || activeTab;\n"
"    info.textContent = '';\n"
"  }\n"
"}\n"
"function updateHash() {\n"
"  const h = '#' + activeTab + (activeTab === 'split' ? ':' + activeLine : '');\n"
"  try { history.replaceState(null, '', h); } catch (e) {}\n"
"}\n"
"\n"
"/* ---------- Tabs ---------- */\n"
"function switchTab(tabName, btn) {\n"
"  if (TABS.indexOf(tabName) < 0) return;\n"
"  activeTab = tabName;\n"
"  document.querySelectorAll('.tab-btn').forEach(b => {\n"
"    const on = b.dataset.tab === tabName;\n"
"    b.classList.toggle('active', on);\n"
"    b.setAttribute('aria-selected', on ? 'true' : 'false');\n"
"    b.tabIndex = on ? 0 : -1;\n"
"  });\n"
"  document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));\n"
"  const target = document.getElementById('view-' + tabName);\n"
"  if (target) target.classList.add('active');\n"
"  store.set('xlang_ir_tab', tabName);\n"
"  updateStatus();\n"
"  updateHash();\n"
"  const q = document.getElementById('search-input');\n"
"  if (q && q.value.trim()) handleSearch(); else clearMarks();\n"
"}\n"
"\n"
"/* ---------- Source <-> IR correlation ---------- */\n"
"function selectLine(n, scrollIr) {\n"
"  n = Math.max(1, Math.min(TOTAL_LINES, n | 0));\n"
"  activeLine = n;\n"
"  document.querySelectorAll('.src-row.active').forEach(e => e.classList.remove('active'));\n"
"  document.querySelectorAll('.ir-section.active').forEach(e => e.classList.remove('active'));\n"
"  const sr = document.querySelector('.src-row[data-line=\"' + n + '\"]');\n"
"  const ir = document.getElementById('ir-sec-' + n);\n"
"  if (sr) { sr.classList.add('active'); if (activeTab === 'split') sr.scrollIntoView({ behavior: SCROLL, block: 'nearest' }); }\n"
"  if (ir) { ir.classList.add('active'); if (activeTab === 'split') ir.scrollIntoView({ behavior: SCROLL, block: 'nearest' }); }\n"
"  updateStatus();\n"
"  updateHash();\n"
"}\n"
"\n"
"function goToSourceLine(ln) {\n"
"  switchTab('split');\n"
"  selectLine(ln, true);\n"
"}\n"
"\n"
"/* Jump targets live in two places (split and linear view); only the one in the visible tab can be scrolled to */\n"
"function jumpToOffset(fnId, targetOffset, event) {\n"
"  if (event) event.stopPropagation();\n"
"  const pad = String(targetOffset).padStart(4, '0');\n"
"  const ids = ['instr-split-' + fnId + '-' + pad, 'instr-' + fnId + '-' + pad];\n"
"  if (activeTab === 'linear') ids.reverse();\n"
"  let el = null;\n"
"  for (const id of ids) {\n"
"    const c = document.getElementById(id);\n"
"    if (c && c.offsetParent !== null) { el = c; break; }\n"
"  }\n"
"  if (!el) {\n"
"    /* Not visible in the current tab: fall back to the linear view, which lists every instruction */\n"
"    const lin = document.getElementById('instr-' + fnId + '-' + pad);\n"
"    if (lin) { switchTab('linear'); el = lin; }\n"
"  }\n"
"  if (el) {\n"
"    el.scrollIntoView({ behavior: SCROLL, block: 'center' });\n"
"    el.classList.remove('target-highlight');\n"
"    void el.offsetWidth;\n"
"    el.classList.add('target-highlight');\n"
"    const ln = parseInt(el.getAttribute('data-line'), 10);\n"
"    if (activeTab === 'split' && ln > 0) {\n"
"      activeLine = ln;\n"
"      document.querySelectorAll('.src-row.active').forEach(e => e.classList.remove('active'));\n"
"      document.querySelectorAll('.ir-section.active').forEach(e => e.classList.remove('active'));\n"
"      const sr = document.querySelector('.src-row[data-line=\"' + ln + '\"]');\n"
"      const sec = document.getElementById('ir-sec-' + ln);\n"
"      if (sr) { sr.classList.add('active'); sr.scrollIntoView({ behavior: SCROLL, block: 'nearest' }); }\n"
"      if (sec) sec.classList.add('active');\n"
"      updateStatus();\n"
"      updateHash();\n"
"    }\n"
"  }\n"
"}\n"
"\n"
"/* ---------- Tooltip ---------- */\n"
"const tooltip = document.getElementById('tooltip');\n"
"function showTooltip(e, name, cat, stack, desc) {\n"
"  document.getElementById('tt-title').textContent = name;\n"
"  document.getElementById('tt-cat').textContent = cat;\n"
"  document.getElementById('tt-stack').textContent = 'Stack: ' + stack;\n"
"  document.getElementById('tt-desc').textContent = desc;\n"
"  tooltip.style.display = 'block';\n"
"  const w = tooltip.offsetWidth, h = tooltip.offsetHeight;\n"
"  let x = e.clientX + 14, y = e.clientY + 14;\n"
"  if (x + w > window.innerWidth - 8) x = Math.max(8, e.clientX - w - 14);\n"
"  if (y + h > window.innerHeight - 8) y = Math.max(8, e.clientY - h - 14);\n"
"  tooltip.style.left = x + 'px';\n"
"  tooltip.style.top = y + 'px';\n"
"}\n"
"function hideTooltip() { tooltip.style.display = 'none'; }\n"
"window.addEventListener('scroll', hideTooltip, true);\n"
"\n"
"/* ---------- Draggable divider (pointer events: mouse, touch and pen) ---------- */\n"
"const resizer = document.getElementById('split-resizer');\n"
"const srcPane = document.getElementById('src-pane');\n"
"const splitEl = document.getElementById('view-split');\n"
"let isDragging = false;\n"
"if (resizer) {\n"
"  resizer.addEventListener('pointerdown', e => {\n"
"    isDragging = true;\n"
"    resizer.setPointerCapture(e.pointerId);\n"
"    resizer.classList.add('dragging');\n"
"    document.body.style.userSelect = 'none';\n"
"  });\n"
"  resizer.addEventListener('pointermove', e => {\n"
"    if (!isDragging) return;\n"
"    const r = splitEl.getBoundingClientRect();\n"
"    const pct = ((e.clientX - r.left) / r.width) * 100;\n"
"    if (pct > 20 && pct < 80) srcPane.style.width = pct + '%%';\n"
"  });\n"
"  const endDrag = () => {\n"
"    if (!isDragging) return;\n"
"    isDragging = false;\n"
"    resizer.classList.remove('dragging');\n"
"    document.body.style.userSelect = '';\n"
"    store.set('xlang_ir_split_ratio', parseFloat(srcPane.style.width) || 50);\n"
"  };\n"
"  resizer.addEventListener('pointerup', endDrag);\n"
"  resizer.addEventListener('pointercancel', endDrag);\n"
"  resizer.addEventListener('dblclick', () => { srcPane.style.width = '50%%'; store.set('xlang_ir_split_ratio', 50); });\n"
"}\n"
"\n"
"/* ---------- Theme & font ---------- */\n"
"function changeTheme(th) {\n"
"  if (THEMES.indexOf(th) < 0) th = 'mocha';\n"
"  document.documentElement.setAttribute('data-theme', th);\n"
"  const sel = document.getElementById('theme-select');\n"
"  if (sel) sel.value = th;\n"
"  store.set('xlang_ir_theme', th);\n"
"}\n"
"function cycleTheme() {\n"
"  const cur = document.documentElement.getAttribute('data-theme');\n"
"  changeTheme(THEMES[(THEMES.indexOf(cur) + 1) %% THEMES.length]);\n"
"}\n"
"function changeFontSize(delta) {\n"
"  currentFontSize = Math.max(10, Math.min(20, currentFontSize + delta));\n"
"  document.documentElement.style.setProperty('--base-font-size', currentFontSize + 'px');\n"
"  store.set('xlang_ir_font_size', currentFontSize);\n"
"}\n"
"\n"
"/* ---------- Search (DOM based: matched text is never re-parsed as HTML) ---------- */\n"
"const MAX_MARKS = 2000;\n"
"function clearMarks() {\n"
"  document.querySelectorAll('mark.search-match').forEach(m => {\n"
"    const p = m.parentNode;\n"
"    if (!p) return;\n"
"    p.replaceChild(document.createTextNode(m.textContent), m);\n"
"    p.normalize();\n"
"  });\n"
"  searchMatches = [];\n"
"  currentMatchIdx = -1;\n"
"}\n"
"\n"
"function handleSearch() {\n"
"  clearMarks();\n"
"  const countEl = document.getElementById('search-count');\n"
"  const q = document.getElementById('search-input').value.trim().toLowerCase();\n"
"  if (!q) { countEl.textContent = ''; return; }\n"
"  const container = document.querySelector('.tab-content.active');\n"
"  if (!container) return;\n"
"\n"
"  const walker = document.createTreeWalker(container, NodeFilter.SHOW_TEXT, {\n"
"    acceptNode(n) {\n"
"      const p = n.parentElement;\n"
"      if (!p || !n.nodeValue.trim()) return NodeFilter.FILTER_REJECT;\n"
"      if (p.closest('script, style, option, select, button, textarea, .lin-toolbar, [hidden]')) return NodeFilter.FILTER_REJECT;\n"
"      return n.nodeValue.toLowerCase().indexOf(q) >= 0 ? NodeFilter.FILTER_ACCEPT : NodeFilter.FILTER_REJECT;\n"
"    }\n"
"  });\n"
"  const nodes = [];\n"
"  while (walker.nextNode()) nodes.push(walker.currentNode);\n"
"\n"
"  let total = 0, capped = false;\n"
"  for (const n of nodes) {\n"
"    if (total >= MAX_MARKS) { capped = true; break; }\n"
"    const text = n.nodeValue, low = text.toLowerCase();\n"
"    if (low.length !== text.length) continue;\n"
"    const frag = document.createDocumentFragment();\n"
"    let pos = 0, i;\n"
"    while ((i = low.indexOf(q, pos)) !== -1 && total < MAX_MARKS) {\n"
"      if (i > pos) frag.appendChild(document.createTextNode(text.slice(pos, i)));\n"
"      const m = document.createElement('mark');\n"
"      m.className = 'search-match';\n"
"      m.textContent = text.slice(i, i + q.length);\n"
"      frag.appendChild(m);\n"
"      pos = i + q.length;\n"
"      total++;\n"
"    }\n"
"    if (pos < text.length) frag.appendChild(document.createTextNode(text.slice(pos)));\n"
"    n.parentNode.replaceChild(frag, n);\n"
"  }\n"
"  searchMatches = Array.from(container.querySelectorAll('mark.search-match'));\n"
"  countEl.textContent = searchMatches.length ? (searchMatches.length + (capped ? '+' : '') + ' found') : 'No matches';\n"
"  if (searchMatches.length) stepMatch(1);\n"
"}\n"
"\n"
"function stepMatch(dir) {\n"
"  const n = searchMatches.length;\n"
"  if (!n) return;\n"
"  if (currentMatchIdx >= 0 && searchMatches[currentMatchIdx]) searchMatches[currentMatchIdx].classList.remove('current');\n"
"  currentMatchIdx = (currentMatchIdx + dir + n) %% n;\n"
"  const m = searchMatches[currentMatchIdx];\n"
"  m.classList.add('current');\n"
"  let d = m.closest('details');\n"
"  while (d) { d.open = true; d = d.parentElement ? d.parentElement.closest('details') : null; }\n"
"  m.scrollIntoView({ behavior: SCROLL, block: 'center' });\n"
"  document.getElementById('search-count').textContent = (currentMatchIdx + 1) + '/' + n;\n"
"}\n"
"function nextMatch() { stepMatch(1); }\n"
"function prevMatch() { stepMatch(-1); }\n"
"\n"
"function searchKey(e) {\n"
"  if (e.key === 'Enter') { e.preventDefault(); stepMatch(e.shiftKey ? -1 : 1); }\n"
"  else if (e.key === 'Escape') { e.target.value = ''; handleSearch(); e.target.blur(); }\n"
"}\n"
"function focusSearch() {\n"
"  const el = document.getElementById('search-input');\n"
"  el.focus();\n"
"  el.select();\n"
"}\n"
"\n"
"/* ---------- AST controls ---------- */\n"
"function setAllAstOpen(open) {\n"
"  document.querySelectorAll('#ast-tree-root details').forEach(d => d.open = open);\n"
"}\n"
"function switchAstView(mode, btn) {\n"
"  document.querySelectorAll('.ast-mode-btn').forEach(b => {\n"
"    if (b.textContent.includes('Tree') || b.textContent.includes('Dump')) b.classList.remove('active');\n"
"  });\n"
"  if (btn) btn.classList.add('active');\n"
"  document.getElementById('ast-tree-root').style.display = (mode === 'tree') ? 'block' : 'none';\n"
"  document.getElementById('ast-text-root').style.display = (mode === 'tree') ? 'none' : 'block';\n"
"}\n"
"\n"
"/* ---------- Clipboard (with fallback for contexts where navigator.clipboard is unavailable) ---------- */\n"
"function copyText(text, okMsg) {\n"
"  const fallback = () => {\n"
"    const ta = document.createElement('textarea');\n"
"    ta.value = text;\n"
"    ta.style.position = 'fixed';\n"
"    ta.style.opacity = '0';\n"
"    document.body.appendChild(ta);\n"
"    ta.select();\n"
"    let ok = false;\n"
"    try { ok = document.execCommand('copy'); } catch (e) {}\n"
"    document.body.removeChild(ta);\n"
"    toast(ok ? okMsg : 'Copy failed - select the text manually');\n"
"  };\n"
"  if (navigator.clipboard && navigator.clipboard.writeText) {\n"
"    navigator.clipboard.writeText(text).then(() => toast(okMsg), fallback);\n"
"  } else {\n"
"    fallback();\n"
"  }\n"
"}\n"
"function copyAstText() {\n"
"  copyText(document.getElementById('ast-text-pre').textContent, 'AST dump copied');\n"
"}\n"
"function copyLlvmCode() {\n"
"  const text = Array.from(document.querySelectorAll('#llvm-viewer .llvm-content'))\n"
"    .map(el => el.textContent).join('\\n');\n"
"  copyText(text, 'LLVM IR copied');\n"
"}\n"
"\n"
"/* ---------- Linear view toolbar: function jump + opcode category filter ---------- */\n"
"function buildLinearToolbar() {\n"
"  const view = document.getElementById('view-linear');\n"
"  const table = view && view.querySelector('.linear-table');\n"
"  if (!table) return;\n"
"  const headers = Array.from(table.querySelectorAll('.linear-func-hdr'));\n"
"  const rows = Array.from(table.querySelectorAll('tbody tr[id]'));\n"
"  const cats = {};\n"
"  rows.forEach(r => {\n"
"    const b = r.querySelector('.op-badge');\n"
"    /* class list is \"op-badge op-<category>\": skip the generic op-badge class */\n"
"    const cls = b ? Array.from(b.classList).find(c => c.indexOf('op-') === 0 && c !== 'op-badge') : null;\n"
"    const cat = cls ? cls.slice(3) : '';\n"
"    r.dataset.cat = cat;\n"
"    if (cat) cats[cat] = (cats[cat] || 0) + 1;\n"
"  });\n"
"\n"
"  const bar = document.createElement('div');\n"
"  bar.className = 'lin-toolbar';\n"
"\n"
"  if (headers.length > 1) {\n"
"    const lbl = document.createElement('span');\n"
"    lbl.className = 'lbl';\n"
"    lbl.textContent = 'Function';\n"
"    const sel = document.createElement('select');\n"
"    sel.setAttribute('aria-label', 'Jump to function');\n"
"    headers.forEach((h, i) => {\n"
"      const m = /Function:\\s*(.*?)\\s*\\(Arity/.exec(h.textContent);\n"
"      const o = document.createElement('option');\n"
"      o.value = i;\n"
"      o.textContent = m ? m[1] : h.textContent.trim();\n"
"      sel.appendChild(o);\n"
"    });\n"
"    sel.addEventListener('change', () => headers[+sel.value].scrollIntoView({ behavior: SCROLL, block: 'start' }));\n"
"    bar.appendChild(lbl);\n"
"    bar.appendChild(sel);\n"
"  }\n"
"\n"
"  const active = new Set();\n"
"  const apply = () => {\n"
"    let hdr = null, any = false;\n"
"    const flush = () => { if (hdr) hdr.hidden = active.size > 0 && !any; };\n"
"    table.querySelectorAll('tbody tr').forEach(r => {\n"
"      if (r.classList.contains('linear-func-hdr')) { flush(); hdr = r; any = false; return; }\n"
"      const show = active.size === 0 || active.has(r.dataset.cat);\n"
"      r.hidden = !show;\n"
"      if (show) any = true;\n"
"    });\n"
"    flush();\n"
"    const q = document.getElementById('search-input');\n"
"    if (activeTab === 'linear' && q && q.value.trim()) handleSearch();\n"
"  };\n"
"  const keys = Object.keys(cats).sort((a, b) => cats[b] - cats[a]);\n"
"  if (keys.length > 1) {\n"
"    const lbl = document.createElement('span');\n"
"    lbl.className = 'lbl';\n"
"    lbl.textContent = 'Filter';\n"
"    bar.appendChild(lbl);\n"
"    keys.forEach(k => {\n"
"      const c = document.createElement('button');\n"
"      c.className = 'chip';\n"
"      c.dataset.cat = k;\n"
"      c.textContent = k + ' ' + cats[k];\n"
"      c.setAttribute('aria-pressed', 'false');\n"
"      c.addEventListener('click', () => {\n"
"        const on = !active.has(k);\n"
"        if (on) active.add(k); else active.delete(k);\n"
"        c.classList.toggle('on', on);\n"
"        c.setAttribute('aria-pressed', on ? 'true' : 'false');\n"
"        apply();\n"
"      });\n"
"      bar.appendChild(c);\n"
"    });\n"
"    const clr = document.createElement('button');\n"
"    clr.className = 'chip';\n"
"    clr.textContent = 'clear';\n"
"    clr.addEventListener('click', () => {\n"
"      active.clear();\n"
"      bar.querySelectorAll('.chip.on').forEach(x => { x.classList.remove('on'); x.setAttribute('aria-pressed', 'false'); });\n"
"      apply();\n"
"    });\n"
"    bar.appendChild(clr);\n"
"  }\n"
"  if (bar.children.length) view.insertBefore(bar, table);\n"
"}\n"
"\n"
"/* ---------- VM Execution Trace Stepper & Player ---------- */\n"
"let traceCurStep = 0;\n"
"let tracePlayTimer = null;\n"
"let tracePlaySpeed = 200;\n"
"const traceTotal = (typeof TRACE_DATA !== 'undefined') ? TRACE_DATA.length : 0;\n"
"\n"
"function traceJump(idx) {\n"
"  if (traceTotal === 0) return;\n"
"  idx = Math.max(0, Math.min(traceTotal - 1, idx | 0));\n"
"  traceCurStep = idx;\n"
"  const item = TRACE_DATA[idx];\n"
"  if (!item) return;\n"
"  const slider = document.getElementById('trace-slider');\n"
"  if (slider) slider.value = idx;\n"
"  const counter = document.getElementById('trace-counter');\n"
"  if (counter) counter.textContent = 'Step ' + (idx + 1) + ' / ' + traceTotal;\n"
"  document.querySelectorAll('.active-trace-row').forEach(r => r.classList.remove('active-trace-row'));\n"
"  const row = document.getElementById('tr-step-' + idx);\n"
"  if (row) {\n"
"    row.classList.add('active-trace-row');\n"
"    row.scrollIntoView({ behavior: 'auto', block: 'nearest' });\n"
"  }\n"
"  const stepNum = document.getElementById('insp-step-num');\n"
"  if (stepNum) stepNum.textContent = '#' + (idx + 1);\n"
"  const funcEl = document.getElementById('insp-func');\n"
"  if (funcEl) funcEl.textContent = item.fn;\n"
"  const locEl = document.getElementById('insp-loc');\n"
"  if (locEl) locEl.textContent = 'Line ' + item.line + ' (offset ' + String(item.off).padStart(4, '0') + ')';\n"
"  const opEl = document.getElementById('insp-op');\n"
"  if (opEl) opEl.textContent = item.op + (item.opnd ? ' ' + item.opnd : '');\n"
"  const stackCount = document.getElementById('insp-stack-count');\n"
"  if (stackCount) stackCount.textContent = item.sdepth;\n"
"  const stackSlots = document.getElementById('insp-stack-slots');\n"
"  if (stackSlots) {\n"
"    if (item.sdepth === 0 || !item.stack || item.stack.trim() === '') {\n"
"      stackSlots.innerHTML = '<div style=\"color:var(--text-muted);font-size:12px;padding:8px;\">(Stack is empty)</div>';\n"
"    } else {\n"
"      const matches = item.stack.match(/\\[\\s*([^\\]]+?)\\s*\\]/g);\n"
"      if (matches && matches.length > 0) {\n"
"        let html = '';\n"
"        for (let s = matches.length - 1; s >= 0; s--) {\n"
"          const val = matches[s].replace(/^\\[\\s*/, '').replace(/\\s*\\]$/, '');\n"
"          const isTop = s === matches.length - 1;\n"
"          html += '<div class=\"stack-slot' + (isTop ? ' stack-top-slot' : '') + '\">';\n"
"          html += '<span style=\"color:var(--text-muted);font-size:10px;\">slot ' + s + (isTop ? ' (TOP)' : '') + '</span>';\n"
"          html += '<span style=\"color:var(--accent-cyan);font-weight:bold;\">' + val + '</span>';\n"
"          html += '</div>';\n"
"        }\n"
"        stackSlots.innerHTML = html;\n"
"      } else {\n"
"        stackSlots.innerHTML = '<div style=\"font-family:var(--font-mono);font-size:12px;\">' + item.stack + '</div>';\n"
"      }\n"
"    }\n"
"  }\n"
"  if (item.line > 0 && typeof selectLine === 'function') {\n"
"    selectLine(item.line, false);\n"
"  }\n"
"}\n"
"\n"
"function traceStep(delta) {\n"
"  traceJump(traceCurStep + delta);\n"
"}\n"
"\n"
"function traceTogglePlay() {\n"
"  const btn = document.getElementById('btn-trace-play');\n"
"  if (tracePlayTimer) {\n"
"    clearInterval(tracePlayTimer);\n"
"    tracePlayTimer = null;\n"
"    if (btn) btn.innerHTML = '&#x25B6; Play';\n"
"  } else {\n"
"    if (traceCurStep >= traceTotal - 1) traceJump(0);\n"
"    tracePlayTimer = setInterval(() => {\n"
"      if (traceCurStep >= traceTotal - 1) {\n"
"        traceTogglePlay();\n"
"      } else {\n"
"        traceStep(1);\n"
"      }\n"
"    }, tracePlaySpeed);\n"
"    if (btn) btn.innerHTML = '&#x23F8; Pause';\n"
"  }\n"
"}\n"
"\n"
"function traceSetSpeed(ms) {\n"
"  tracePlaySpeed = ms || 200;\n"
"  if (tracePlayTimer) {\n"
"    clearInterval(tracePlayTimer);\n"
"    tracePlayTimer = setInterval(() => {\n"
"      if (traceCurStep >= traceTotal - 1) {\n"
"        traceTogglePlay();\n"
"      } else {\n"
"        traceStep(1);\n"
"      }\n"
"    }, tracePlaySpeed);\n"
"  }\n"
"}\n"
"\n"
"function filterTraceTable(q) {\n"
"  q = (q || '').toLowerCase().trim();\n"
"  const rows = document.querySelectorAll('#trace-tbody tr');\n"
"  rows.forEach(r => {\n"
"    if (!q) {\n"
"      r.style.display = '';\n"
"    } else {\n"
"      const txt = r.textContent.toLowerCase();\n"
"      r.style.display = txt.indexOf(q) >= 0 ? '' : 'none';\n"
"    }\n"
"  });\n"
"}\n"
"\n"
"/* ---------- Help overlay ---------- */\n"
"function toggleHelp(force) {\n"
"  const h = document.getElementById('help');\n"
"  const open = (typeof force === 'boolean') ? force : !h.classList.contains('open');\n"
"  h.classList.toggle('open', open);\n"
"}\n"
"\n"
"/* ---------- Keyboard ---------- */\n"
"document.addEventListener('keydown', e => {\n"
"  const t = e.target, tag = t && t.tagName;\n"
"  const typing = tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || (t && t.isContentEditable);\n"
"  if ((e.ctrlKey || e.metaKey) && !e.shiftKey && !e.altKey && e.key.toLowerCase() === 'f') {\n"
"    e.preventDefault();\n"
"    focusSearch();\n"
"    return;\n"
"  }\n"
"  if (e.key === 'Escape') {\n"
"    const h = document.getElementById('help');\n"
"    if (h.classList.contains('open')) { toggleHelp(false); return; }\n"
"    if (!typing) {\n"
"      const q = document.getElementById('search-input');\n"
"      if (q.value) { q.value = ''; handleSearch(); }\n"
"    }\n"
"    return;\n"
"  }\n"
"  if (typing || e.ctrlKey || e.metaKey || e.altKey) return;\n"
"  switch (e.key) {\n"
"    case '/': e.preventDefault(); focusSearch(); break;\n"
"    case '?': toggleHelp(); break;\n"
"    case 'n': stepMatch(1); break;\n"
"    case 'N': stepMatch(-1); break;\n"
"    case 't': cycleTheme(); break;\n"
"    case '+': case '=': changeFontSize(1); break;\n"
"    case '-': changeFontSize(-1); break;\n"
"    case 'ArrowDown': case 'j':\n"
"      if (activeTab === 'split') { selectLine(activeLine + 1, true); e.preventDefault(); }\n"
"      else if (activeTab === 'trace') { traceStep(1); e.preventDefault(); }\n"
"      break;\n"
"    case 'ArrowUp': case 'k':\n"
"      if (activeTab === 'split') { selectLine(activeLine - 1, true); e.preventDefault(); }\n"
"      else if (activeTab === 'trace') { traceStep(-1); e.preventDefault(); }\n"
"      break;\n"
"    case 'ArrowLeft': case '[':\n"
"      if (activeTab === 'trace') { traceStep(-1); e.preventDefault(); }\n"
"      break;\n"
"    case 'ArrowRight': case ']':\n"
"      if (activeTab === 'trace') { traceStep(1); e.preventDefault(); }\n"
"      break;\n"
"    case ' ':\n"
"      if (activeTab === 'trace') { traceTogglePlay(); e.preventDefault(); }\n"
"      break;\n"
"    default:\n"
"      if (/^[1-7]$/.test(e.key)) switchTab(TABS[+e.key - 1]);\n"
"  }\n"
"});\n"
"\n"
"/* ---------- Boot: restore saved state (runs immediately; the script sits at the end of <body>) ---------- */\n"
"(function boot() {\n"
"  let theme = store.get('xlang_ir_theme');\n"
"  if (!theme && window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches) theme = 'light';\n"
"  if (theme) changeTheme(theme);\n"
"\n"
"  const savedFont = parseInt(store.get('xlang_ir_font_size'), 10);\n"
"  if (savedFont) {\n"
"    currentFontSize = Math.max(10, Math.min(20, savedFont));\n"
"    document.documentElement.style.setProperty('--base-font-size', currentFontSize + 'px');\n"
"  }\n"
"  const ratio = parseFloat(store.get('xlang_ir_split_ratio'));\n"
"  if (srcPane && ratio > 20 && ratio < 80) srcPane.style.width = ratio + '%%';\n"
"\n"
"  /* Tab buttons: tag each with its tab name and add ARIA roles */\n"
"  const bar = document.querySelector('.nav-tabs');\n"
"  if (bar) bar.setAttribute('role', 'tablist');\n"
"  document.querySelectorAll('.tab-btn').forEach(b => {\n"
"    const m = /switchTab\\('([a-z]+)'/.exec(b.getAttribute('onclick') || '');\n"
"    if (m) b.dataset.tab = m[1];\n"
"    b.setAttribute('role', 'tab');\n"
"  });\n"
"\n"
"  buildLinearToolbar();\n"
"\n"
"  /* Initial view: URL hash (#split:12, #llvm, #trace) wins over the last-used tab */\n"
"  const applyHash = () => {\n"
"    const m = /^#([a-z]+)(?::(\\d+))?$/.exec(location.hash);\n"
"    if (!m || TABS.indexOf(m[1]) < 0) return false;\n"
"    switchTab(m[1]);\n"
"    if (m[1] === 'split' && m[2]) selectLine(parseInt(m[2], 10), true);\n"
"    if (m[1] === 'trace' && m[2]) traceJump(parseInt(m[2], 10));\n"
"    return true;\n"
"  };\n"
"  if (!applyHash()) {\n"
"    const saved = store.get('xlang_ir_tab');\n"
"    switchTab(TABS.indexOf(saved) >= 0 ? saved : 'split');\n"
"    selectLine(1, false);\n"
"  }\n"
"  if (traceTotal > 0) {\n"
"    traceJump(0);\n"
"  }\n"
"  window.addEventListener('hashchange', applyHash);\n"
"})();\n"
"</script>\n"
"</body>\n"
"</html>\n",
	src_line_count);

	fclose(out);

	/* Cleanup */
	catalog_free(&cat);
	if (src_lines) {
		for (int i = 0; i < src_line_count; i++) free(src_lines[i]);
		free(src_lines);
	}
	if (by_line) {
		for (int i = 0; i <= src_line_count; i++) {
			if (by_line[i].instrs) free(by_line[i].instrs);
		}
		free(by_line);
	}
	if (llvm_ir) free(llvm_ir);

	return true;
}

/* --------------------------------------------------------------------------
 * Convenience wrapper matching original signature
 * -------------------------------------------------------------------------- */
bool xdis_html_write(const XIrChunk* chunk, const char* source_path,
                     const char* out_html)
{
	return xdis_html_write_full(chunk, NULL, source_path, out_html, NULL);
}