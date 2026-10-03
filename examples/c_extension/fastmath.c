#include "xlang.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Direct C functions (callable via C ABI from LLVM IR or other C code) */
XLANG_EXPORT double fastmath_hypot(double x, double y)
{
	return hypot(x, y);
}

XLANG_EXPORT int fastmath_gcd(int a, int b)
{
	while (b != 0)
	{
		int t = b;
		b = a % b;
		a = t;
	}
	return a < 0 ? -a : a;
}

XLANG_EXPORT char* fastmath_reverse(const char* s)
{
	if (!s) return strdup("");
	size_t len = strlen(s);
	char* rev = (char*)malloc(len + 1);
	if (!rev) return strdup("");
	for (size_t i = 0; i < len; i++)
	{
		rev[i] = s[len - 1 - i];
	}
	rev[len] = '\0';
	return rev;
}

/* Vectorcall wrappers (for Bytecode VM and dynamic dispatch) */
static XValue vec_fast_hypot(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	double x = (argc > 0 && args[0].type == XLANG_VAL_FLOAT) ? args[0].as.fval :
	           (argc > 0 && args[0].type == XLANG_VAL_INT) ? (double)args[0].as.ival : 0.0;
	double y = (argc > 1 && args[1].type == XLANG_VAL_FLOAT) ? args[1].as.fval :
	           (argc > 1 && args[1].type == XLANG_VAL_INT) ? (double)args[1].as.ival : 0.0;
	return xval_float(fastmath_hypot(x, y));
}

static XValue vec_fast_gcd(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int a = (argc > 0 && args[0].type == XLANG_VAL_INT) ? (int)args[0].as.ival : 0;
	int b = (argc > 1 && args[1].type == XLANG_VAL_INT) ? (int)args[1].as.ival : 0;
	return xval_int(fastmath_gcd(a, b));
}

static XValue vec_fast_reverse(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	const char* s = (argc > 0 && args[0].type == XLANG_VAL_STRING && args[0].as.sval) ? args[0].as.sval : "";
	char* rev = fastmath_reverse(s);
	return xval_str(rev);
}

XLANG_EXPORT char* fastmath_format_stat(const char* name, int count, double avg)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s: count=%d, avg=%.2f", name ? name : "item", count, avg);
	return strdup(buf);
}

XLANG_EXPORT char* fastmath_inspect_point(void* obj)
{
	(void)obj;
	return strdup("Point(inspected)");
}

static XExtensionContext* s_ctx = NULL;

static XValue vec_fast_format_stat(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	const char* name = (argc > 0 && args[0].type == XLANG_VAL_STRING) ? args[0].as.sval : "item";
	int count = (argc > 1 && args[1].type == XLANG_VAL_INT) ? (int)args[1].as.ival : 0;
	double avg = (argc > 2 && args[2].type == XLANG_VAL_FLOAT) ? args[2].as.fval :
	             (argc > 2 && args[2].type == XLANG_VAL_INT) ? (double)args[2].as.ival : 0.0;
	if (s_ctx && s_ctx->str_format)
	{
		char* res = s_ctx->str_format("%s: count=%d, avg=%.2f", name, count, avg);
		return xval_str(res);
	}
	return xval_str(fastmath_format_stat(name, count, avg));
}

static XValue vec_fast_inspect_point(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (!s_ctx || argc < 1 || !s_ctx->instance_is(args[0], "Point"))
	{
		return xval_str("not a Point");
	}
	XValue x = s_ctx->instance_get_field(args[0], "x");
	XValue y = s_ctx->instance_get_field(args[0], "y");
	int64_t ix = (x.type == XLANG_VAL_INT) ? x.as.ival : 0;
	int64_t iy = (y.type == XLANG_VAL_INT) ? y.as.ival : 0;
	char* msg = s_ctx->str_format("Point(x=%ld, y=%ld)", (long)ix, (long)iy);
	return xval_str(msg);
}

/* Function descriptors table */
static const XExtensionFunc s_fastmath_funcs[] = {
	{ "hypot",         vec_fast_hypot,         2, XLANG_TYPE_FLOAT,  { XLANG_TYPE_FLOAT, XLANG_TYPE_FLOAT } },
	{ "gcd",           vec_fast_gcd,           2, XLANG_TYPE_INT,    { XLANG_TYPE_INT,   XLANG_TYPE_INT   } },
	{ "reverse",       vec_fast_reverse,       1, XLANG_TYPE_STRING, { XLANG_TYPE_STRING }                  },
	{ "format_stat",   vec_fast_format_stat,   3, XLANG_TYPE_STRING, { XLANG_TYPE_STRING, XLANG_TYPE_INT, XLANG_TYPE_FLOAT } },
	{ "inspect_point", vec_fast_inspect_point, 1, XLANG_TYPE_STRING, { XLANG_TYPE_OBJECT }                 },
	{ NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

/* Module entrypoint */
XLANG_EXPORT XLANG_EXTENSION_ENTRY
{
	s_ctx = ctx;
	return ctx->register_module(ctx, "fastmath", s_fastmath_funcs);
}
