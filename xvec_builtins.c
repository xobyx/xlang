#include "xvec_builtins.h"
#include "functions.h"
#include "xast_parser.h"
#include "xdiag.h"
#include "xir_compiler.h"
#include "xgc.h"
#include "xcollection.h"
#include "ximport.h"
#include "xjson.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <math.h>
#ifndef _WIN32
#include <sys/time.h>
#include <dirent.h>
#endif

/* -------------------------------------------------------------------------
 * Runtime C API Declarations (from xllvm_rt)
 * ------------------------------------------------------------------------- */

char* _str_concat(const char* s1, const char* s2);
char* _str_from_int(int val);
char* _str_from_float(double val);
int _len(const char* s);
char* _trim(const char* s);
char* _lower(const char* s);
char* _upper(const char* s);
char* substr(const char* s, int start, int len);
int index_of(const char* s, const char* needle);
int str_eq(const char* s1, const char* s2);
int starts_with(const char* s, const char* prefix);
int ends_with(const char* s, const char* suffix);
int regex_match(const char* s, const char* pattern);
char* regex_find(const char* s, const char* pattern);
char* regex_replace(const char* s, const char* pattern, const char* repl);
int str_split(const char* s, const char* delim);

double math_sqrt(double x);
double math_pow(double base, double exp);
double math_abs(double x);
double math_min(double a, double b);
double math_max(double a, double b);
double math_floor(double x);
double math_ceil(double x);
double math_round(double x);
double math_sin(double x);
double math_cos(double x);
double math_tan(double x);
double math_log(double x);

char* file_read_all(const char* path);
int file_write_all(const char* path, const char* content);
int file_append(const char* path, const char* content);
int file_exists(const char* path);
int file_size(const char* path);
int file_remove(const char* path);
int file_open(const char* path, const char* mode);
char* file_read(int fd, int bytes);
int file_write(int fd, const char* data);
int file_close(int fd);

int dir_create(const char* path);
int dir_exists(const char* path);
int dir_remove(const char* path);

int socket_create(const char* type);
int socket_connect(int fd, const char* host, int port);
int socket_bind(int fd, const char* host, int port);
int socket_listen(int fd, int backlog);
int socket_accept(int fd);
int socket_send(int fd, const char* data);
char* socket_recv(int fd, int max_bytes);
int socket_close(int fd);
int socket_set_timeout(int fd, int sec);
int socket_set_reuseaddr(int fd, int enable);
int socket_sendto(int fd, const char* data, const char* host, int port);
char* socket_recvfrom(int fd, int max_bytes);
char* http_get(const char* url);

int get_argc(void);
char* get_arg(int index);
int system_exec(const char* cmd);
char* system_getenv(const char* name);
int system_setenv(const char* name, const char* val);
char* proc_capture(const char* cmd);
void* proc_run(const char* cmd);

typedef struct {
	char* stdout_str;
	int exit_code;
} RtProcResult;

int datetime_now(void);
char* datetime_format(int ts, const char* fmt);
int datetime_year(int ts);
int datetime_month(int ts);
int datetime_day(int ts);
int datetime_hour(int ts);
int datetime_minute(int ts);
int datetime_second(int ts);
int datetime_clock_ms(void);

int json_is_valid(const char* str);
void* json_parse(const char* str);
char* json_stringify(void* m);

/* -------------------------------------------------------------------------
 * Argument Unpacking Helpers
 * ------------------------------------------------------------------------- */

static inline double get_float_arg(const XValue* args, int argc, int idx, double def)
{
	if (idx >= argc) return def;
	if (args[idx].type == VAL_FLOAT) return args[idx].as.fval;
	if (args[idx].type == VAL_INT) return (double)args[idx].as.ival;
	return def;
}

static inline int64_t get_int_arg(const XValue* args, int argc, int idx, int64_t def)
{
	if (idx >= argc) return def;
	if (args[idx].type == VAL_INT) return args[idx].as.ival;
	if (args[idx].type == VAL_FLOAT) return (int64_t)args[idx].as.fval;
	return def;
}

static inline const char* get_str_arg(const XValue* args, int argc, int idx, const char* def)
{
	if (idx >= argc) return def;
	if (args[idx].type == VAL_STRING && args[idx].as.sval != NULL) return args[idx].as.sval;
	return def;
}

/* -------------------------------------------------------------------------
 * Core VM-Aware Builtins
 * ------------------------------------------------------------------------- */

XValue vec_assert(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (!is_assert_enabled())
	{
		return xval_int(1);
	}

	if (argc < 1)
	{
		fprintf(stderr, "Assertion failed: assert requires at least 1 argument\n");
		exit(1);
	}

	bool condition = xval_is_truthy(args[0]);
	if (!condition)
	{
		const char* msg = (argc >= 2 && args[1].type == VAL_STRING && args[1].as.sval)
			? args[1].as.sval
			: NULL;

		if (msg != NULL)
		{
			fprintf(stderr, "Assertion failed: %s\n", msg);
		}
		else
		{
			fprintf(stderr, "Assertion failed\n");
		}
		exit(1);
	}

	return xval_int(1);
}

XValue vec_print_f(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc < 1 || args[0].type != VAL_STRING || args[0].as.sval == NULL)
	{
		return xval_int(0);
	}

	const char* fmt = args[0].as.sval;
	int arg_idx = 1;
	for (const char* p = fmt; *p != '\0'; p++)
	{
		if (*p == '%' && *(p + 1) != '\0')
		{
			p++;
			if (*p == '%')
			{
				putchar('%');
				continue;
			}
			if (arg_idx < argc)
			{
				XValue arg = args[arg_idx++];
				if (arg.type == VAL_INT) printf("%ld", (long)arg.as.ival);
				else if (arg.type == VAL_FLOAT) printf("%f", arg.as.fval);
				else if (arg.type == VAL_STRING) printf("%s", arg.as.sval ? arg.as.sval : "null");
				else if (arg.type == VAL_BOOL) printf("%s", arg.as.bval ? "True" : "False");
				else if (arg.type == VAL_NULL) printf("null");
				else xval_print(arg);
			}
		}
		else
		{
			putchar(*p);
		}
	}
	size_t flen = strlen(fmt);
	if (flen == 0 || fmt[flen - 1] != '\n')
	{
		putchar('\n');
	}
	return xval_int(0);
}

XValue vec_print(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc == 0)
	{
		putchar('\n');
		return xval_int(0);
	}

	if (argc > 1 && args[0].type == VAL_STRING && args[0].as.sval && strchr(args[0].as.sval, '%') != NULL)
	{
		return vec_print_f(vm, receiver, argc, args);
	}

	for (int i = 0; i < argc; i++)
	{
		xval_print(args[i]);
		if (i + 1 < argc)
		{
			putchar(' ');
		}
	}
	putchar('\n');
	return xval_int(0);
}

XValue vec_println(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	return vec_print(vm, receiver, argc, args);
}

XValue vec_eval(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)receiver;
	if (argc < 1 || args[0].type != VAL_STRING || args[0].as.sval == NULL || args[0].as.sval[0] == '\0')
	{
		return xval_null();
	}

	const char* code = args[0].as.sval;
	AstProgram* prog = xast_parse_source(code, "<eval>");
	if (prog == NULL || xdiag_get_error_count() > 0)
	{
		if (prog != NULL) ast_program_destroy(prog);
		return xval_null();
	}

	XIrChunk chunk;
	xir_chunk_init(&chunk);

	if (prog->statement_count == 1 && prog->statements[0]->type == AST_STMT_EXPR)
	{
		xir_compile_expr(prog->statements[0]->as.expr, &chunk);
	}
	else
	{
		xir_compile_program(prog, &chunk);
	}

	XVm* target_vm = vm ? vm : g_current_vm;
	XVm eval_vm;
	xvm_init(&eval_vm);

	if (target_vm != NULL)
	{
		eval_vm.class_object = target_vm->class_object;
		eval_vm.class_list = target_vm->class_list;
		eval_vm.class_map = target_vm->class_map;
		eval_vm.class_datetime = target_vm->class_datetime;
		eval_vm.all_classes = target_vm->all_classes;

		for (int i = 0; i < target_vm->global_count && i < VM_GLOBALS_MAX; i++)
		{
			eval_vm.globals[i].name = target_vm->globals[i].name ? strdup(target_vm->globals[i].name) : NULL;
			eval_vm.globals[i].value = target_vm->globals[i].value;
		}
		eval_vm.global_count = target_vm->global_count;
	}

	eval_vm.print_trace = false;
	XVmResult res = xvm_run(&eval_vm, &chunk);

	XValue result = xval_null();
	if (res == VM_OK && eval_vm.stack_top > eval_vm.stack)
	{
		result = *(eval_vm.stack_top - 1);
	}

	if (target_vm != NULL)
	{
		for (int i = 0; i < eval_vm.global_count; i++)
		{
			if (eval_vm.globals[i].name != NULL)
			{
				xvm_set_global(target_vm, eval_vm.globals[i].name, eval_vm.globals[i].value);
			}
		}

		if (eval_vm.all_instances != NULL)
		{
			XInstance* last = eval_vm.all_instances;
			while (last->next != NULL) last = last->next;
			last->next = target_vm->all_instances;
			target_vm->all_instances = eval_vm.all_instances;
			eval_vm.all_instances = NULL;
		}

		if (eval_vm.all_closures != NULL)
		{
			XClosure* last_c = eval_vm.all_closures;
			while (last_c->next != NULL) last_c = last_c->next;
			last_c->next = target_vm->all_closures;
			target_vm->all_closures = eval_vm.all_closures;
			eval_vm.all_closures = NULL;
		}
	}

	eval_vm.all_classes = NULL;
	xvm_free(&eval_vm);
	xir_chunk_free(&chunk);
	ast_program_destroy(prog);

	return result;
}

XValue vec_scan(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)receiver;
	int val = 0;
	if (scanf("%d", &val) == 1 && vm != NULL && argc > 0 && args[0].type == VAL_STRING && args[0].as.sval != NULL)
	{
		xvm_set_global(vm, args[0].as.sval, xval_int(val));
	}
	return xval_int(val);
}

XValue vec_readline(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)receiver;
	char buf[4096];
	if (fgets(buf, sizeof(buf), stdin) != NULL)
	{
		size_t len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
		if (len > 1 && buf[len - 2] == '\r') buf[len - 2] = '\0';
		if (argc > 0 && args[0].type == VAL_STRING && args[0].as.sval != NULL && vm != NULL)
		{
			xvm_set_global(vm, args[0].as.sval, xval_str(buf));
		}
		return xval_str(buf);
	}
	return xval_str("");
}

XValue vec_random(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	static bool s_seeded = false;
	if (!s_seeded) { srand((unsigned int)time(NULL)); s_seeded = true; }
	int max = (argc > 0) ? (int)get_int_arg(args, argc, 0, 0) : 0;
	if (max > 0) return xval_int((rand() % max) + 1);
	return xval_int(rand());
}

XValue vec_time(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	time_t a; time(&a);
	char buff[64];
	struct tm* tm_info = localtime(&a);
	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
	return xval_str(buff);
}

XValue vec_exit(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	int code = (argc > 0) ? (int)get_int_arg(args, argc, 0, 0) : 0;
	exit(code);
	return xval_int(0);
}

XValue vec_echo(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	printf("{\n");
	if (argc > 0)
	{
		printf("  ");
		xval_print(args[0]);
		printf("\n");
	}
	printf("}\n");
	return xval_int(0);
}

XValue vec_import(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc > 0 && args[0].type == VAL_STRING && args[0].as.sval != NULL)
	{
		x_import_module(args[0].as.sval);
		return xval_int(1);
	}
	return xval_int(0);
}

/* -------------------------------------------------------------------------
 * Reflection Builtins
 * ------------------------------------------------------------------------- */

XValue vec_typeof(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc < 1) return xval_str("unknown");
	switch (args[0].type)
	{
	case VAL_NULL: return xval_str("null");
	case VAL_BOOL: return xval_str("bool");
	case VAL_INT: return xval_str("int");
	case VAL_FLOAT: return xval_str("float");
	case VAL_STRING: return xval_str("string");
	case VAL_OBJECT:
		if (args[0].as.oval != NULL)
		{
			XInstance* inst = (XInstance*)args[0].as.oval;
			if (inst->klass && inst->klass->name)
				return xval_str(inst->klass->name);
			return xval_str("instance");
		}
		return xval_str("object");
	case VAL_FUNCTION: return xval_str("function");
	case VAL_CLOSURE: return xval_str("closure");
	default: return xval_str("unknown");
	}
}

XValue vec_type_name(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	return vec_typeof(vm, receiver, argc, args);
}

XValue vec_has_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc < 2 || args[0].type != VAL_OBJECT || args[0].as.oval == NULL ||
	    args[1].type != VAL_STRING || args[1].as.sval == NULL)
	{
		return xval_bool(false);
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (!inst->klass) return xval_bool(false);

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	return xval_bool(slot >= 0);
}

XValue vec_get_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc < 2 || args[0].type != VAL_OBJECT || args[0].as.oval == NULL ||
	    args[1].type != VAL_STRING || args[1].as.sval == NULL)
	{
		return xval_null();
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (!inst->klass) return xval_null();

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		return inst->fields[slot];
	}
	return xval_null();
}

XValue vec_set_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc < 3 || args[0].type != VAL_OBJECT || args[0].as.oval == NULL ||
	    args[1].type != VAL_STRING || args[1].as.sval == NULL)
	{
		return xval_null();
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (!inst->klass) return xval_null();

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		inst->fields[slot] = args[2];
		return args[2];
	}
	return xval_null();
}

/* -------------------------------------------------------------------------
 * String & Regex Operations
 * ------------------------------------------------------------------------- */

XValue vec_len(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	if (receiver.type == VAL_STRING)
	{
		return xval_int(receiver.as.sval ? (int64_t)strlen(receiver.as.sval) : 0);
	}
	if (argc > 0)
	{
		if (args[0].type == VAL_STRING)
			return xval_int(args[0].as.sval ? (int64_t)strlen(args[0].as.sval) : 0);
		if (args[0].type == VAL_OBJECT && args[0].as.oval != NULL)
		{
			XInstance* inst = (XInstance*)args[0].as.oval;
			if (inst->klass && strcmp(inst->klass->name, "List") == 0)
				return xval_int(x_list_count(inst->id));
			if (inst->klass && strcmp(inst->klass->name, "Map") == 0)
				return xval_int(x_map_count(inst->id));
		}
	}
	return xval_int(0);
}

XValue vec_str(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver;
	if (argc == 0) return xval_str("");
	if (args[0].type == VAL_STRING) return args[0];
	if (args[0].type == VAL_INT)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "%" PRId64, args[0].as.ival);
		return xval_str(buf);
	}
	if (args[0].type == VAL_FLOAT)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%f", args[0].as.fval);
		return xval_str(buf);
	}
	if (args[0].type == VAL_BOOL) return xval_str(args[0].as.bval ? "true" : "false");
	return xval_str("");
}

XValue vec_substr(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s;
	int start, len;
	if (receiver.type == VAL_STRING)
	{
		s = receiver.as.sval ? receiver.as.sval : "";
		start = (int)get_int_arg(args, argc, 0, 0);
		len = (int)get_int_arg(args, argc, 1, (int)strlen(s));
	}
	else
	{
		s = get_str_arg(args, argc, 0, "");
		start = (int)get_int_arg(args, argc, 1, 0);
		len = (int)get_int_arg(args, argc, 2, (int)strlen(s));
	}
	char* res = substr(s, start, len);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_index_of(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s;
	const char* needle;
	if (receiver.type == VAL_STRING)
	{
		s = receiver.as.sval ? receiver.as.sval : "";
		needle = get_str_arg(args, argc, 0, "");
	}
	else
	{
		s = get_str_arg(args, argc, 0, "");
		needle = get_str_arg(args, argc, 1, "");
	}
	return xval_int(index_of(s, needle));
}

XValue vec_trim(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	char* res = _trim(s);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_to_lower(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	char* res = _lower(s);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_to_upper(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	char* res = _upper(s);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_starts_with(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	const char* p = (receiver.type == VAL_STRING) ? get_str_arg(args, argc, 0, "") : get_str_arg(args, argc, 1, "");
	return xval_bool(starts_with(s, p) != 0);
}

XValue vec_ends_with(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	const char* suf = (receiver.type == VAL_STRING) ? get_str_arg(args, argc, 0, "") : get_str_arg(args, argc, 1, "");
	return xval_bool(ends_with(s, suf) != 0);
}

XValue vec_regex_match(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	const char* pat = (receiver.type == VAL_STRING) ? get_str_arg(args, argc, 0, "") : get_str_arg(args, argc, 1, "");
	return xval_bool(regex_match(s, pat) != 0);
}

XValue vec_regex_find(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	const char* pat = (receiver.type == VAL_STRING) ? get_str_arg(args, argc, 0, "") : get_str_arg(args, argc, 1, "");
	char* res = regex_find(s, pat);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_regex_replace(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	const char* s;
	const char* pat;
	const char* repl;
	if (receiver.type == VAL_STRING)
	{
		s = receiver.as.sval ? receiver.as.sval : "";
		pat = get_str_arg(args, argc, 0, "");
		repl = get_str_arg(args, argc, 1, "");
	}
	else
	{
		s = get_str_arg(args, argc, 0, "");
		pat = get_str_arg(args, argc, 1, "");
		repl = get_str_arg(args, argc, 2, "");
	}
	char* res = regex_replace(s, pat, repl);
	XValue v = xval_str(res ? res : "");
	if (res) free(res);
	return v;
}

XValue vec_replace(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	return vec_regex_replace(vm, receiver, argc, args);
}

XValue vec_str_split(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	const char* s = (receiver.type == VAL_STRING) ? (receiver.as.sval ? receiver.as.sval : "") : get_str_arg(args, argc, 0, "");
	const char* delim = (receiver.type == VAL_STRING) ? get_str_arg(args, argc, 0, "") : get_str_arg(args, argc, 1, "");

	int list_id = x_list_alloc();
	if (list_id != -1 && s != NULL)
	{
		size_t dlen = delim ? strlen(delim) : 0;
		if (dlen == 0)
		{
			char single[2] = {0, 0};
			for (const char* p = s; *p != '\0'; p++)
			{
				single[0] = *p;
				x_list_append_str(list_id, single);
			}
		}
		else
		{
			const char* cur = s;
			const char* found = strstr(cur, delim);
			while (found != NULL)
			{
				size_t part_len = (size_t)(found - cur);
				char* part = (char*)malloc(part_len + 1);
				if (part != NULL)
				{
					memcpy(part, cur, part_len);
					part[part_len] = '\0';
					x_list_append_str(list_id, part);
					free(part);
				}
				cur = found + dlen;
				found = strstr(cur, delim);
			}
			x_list_append_str(list_id, cur);
		}
	}

	if (receiver.type == VAL_STRING)
	{
		XInstance* inst = xinstance_create_with_id(vm, "List", list_id);
		return xval_obj(inst);
	}
	return xval_int(list_id);
}

XValue vec_eql(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	if (receiver.type != VAL_NULL && argc >= 1)
	{
		return xval_bool(xval_equal(receiver, args[0]));
	}
	if (argc >= 2)
	{
		return xval_bool(xval_equal(args[0], args[1]));
	}
	return xval_bool(false);
}

/* -------------------------------------------------------------------------
 * Math Operations
 * ------------------------------------------------------------------------- */

XValue vec_math_sqrt(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_sqrt(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_pow(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_pow(get_float_arg(a, argc, 0, 0.0), get_float_arg(a, argc, 1, 0.0))); }
XValue vec_math_abs(XVm* vm, XValue r, int argc, const XValue* a) {
	(void)vm; (void)r;
	if (argc > 0 && a[0].type == VAL_INT) { int64_t v = a[0].as.ival; return xval_int(v < 0 ? -v : v); }
	return xval_float(math_abs(get_float_arg(a, argc, 0, 0.0)));
}
XValue vec_math_min(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_min(get_float_arg(a, argc, 0, 0.0), get_float_arg(a, argc, 1, 0.0))); }
XValue vec_math_max(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_max(get_float_arg(a, argc, 0, 0.0), get_float_arg(a, argc, 1, 0.0))); }
XValue vec_math_floor(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_floor(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_ceil(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_ceil(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_round(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_round(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_sin(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_sin(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_cos(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_cos(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_tan(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_tan(get_float_arg(a, argc, 0, 0.0))); }
XValue vec_math_log(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_float(math_log(get_float_arg(a, argc, 0, 0.0))); }

/* -------------------------------------------------------------------------
 * File Operations
 * ------------------------------------------------------------------------- */

XValue vec_file_read_all(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = file_read_all(get_str_arg(a, argc, 0, ""));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_file_write_all(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_write_all(get_str_arg(a, argc, 0, ""), get_str_arg(a, argc, 1, "")));
}

XValue vec_file_append(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_append(get_str_arg(a, argc, 0, ""), get_str_arg(a, argc, 1, "")));
}

XValue vec_file_exists(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_bool(file_exists(get_str_arg(a, argc, 0, "")) != 0);
}

XValue vec_file_size(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_size(get_str_arg(a, argc, 0, "")));
}

XValue vec_file_remove(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_remove(get_str_arg(a, argc, 0, "")));
}

XValue vec_file_open(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_open(get_str_arg(a, argc, 0, ""), (argc > 1) ? get_str_arg(a, argc, 1, "r") : "r"));
}

XValue vec_file_read(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = file_read((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 1024));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_file_write(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_write((int)get_int_arg(a, argc, 0, -1), get_str_arg(a, argc, 1, "")));
}

XValue vec_file_close(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(file_close((int)get_int_arg(a, argc, 0, -1)));
}

/* -------------------------------------------------------------------------
 * Directory Operations
 * ------------------------------------------------------------------------- */

XValue vec_dir_create(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(dir_create(get_str_arg(a, argc, 0, "")));
}

XValue vec_dir_exists(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_bool(dir_exists(get_str_arg(a, argc, 0, "")) != 0);
}

XValue vec_dir_remove(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(dir_remove(get_str_arg(a, argc, 0, "")));
}

XValue vec_dir_list(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)r;
	const char* path = get_str_arg(a, argc, 0, ".");
	int list_id = x_list_alloc();
#ifndef _WIN32
	DIR* d = opendir((path && *path) ? path : ".");
	if (d)
	{
		struct dirent* entry;
		while ((entry = readdir(d)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			x_list_append_str(list_id, entry->d_name);
		}
		closedir(d);
	}
#else
	(void)path;
#endif
	XInstance* inst = xinstance_create_with_id(vm, "List", list_id);
	return xval_obj(inst);
}

/* -------------------------------------------------------------------------
 * Socket Operations
 * ------------------------------------------------------------------------- */

XValue vec_socket_create(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_create(get_str_arg(a, argc, 0, "tcp")));
}

XValue vec_socket_connect(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_connect((int)get_int_arg(a, argc, 0, -1), get_str_arg(a, argc, 1, "127.0.0.1"), (int)get_int_arg(a, argc, 2, 80)));
}

XValue vec_socket_bind(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_bind((int)get_int_arg(a, argc, 0, -1), get_str_arg(a, argc, 1, "0.0.0.0"), (int)get_int_arg(a, argc, 2, 80)));
}

XValue vec_socket_listen(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_listen((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 128)));
}

XValue vec_socket_accept(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_accept((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_socket_send(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_send((int)get_int_arg(a, argc, 0, -1), get_str_arg(a, argc, 1, "")));
}

XValue vec_socket_recv(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = socket_recv((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 4096));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_socket_close(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_close((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_socket_set_timeout(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_set_timeout((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 5)));
}

XValue vec_socket_set_reuseaddr(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_set_reuseaddr((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 1)));
}

XValue vec_socket_sendto(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(socket_sendto((int)get_int_arg(a, argc, 0, -1), get_str_arg(a, argc, 1, ""), get_str_arg(a, argc, 2, "127.0.0.1"), (int)get_int_arg(a, argc, 3, 80)));
}

XValue vec_socket_recvfrom(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = socket_recvfrom((int)get_int_arg(a, argc, 0, -1), (int)get_int_arg(a, argc, 1, 4096));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_http_get(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = http_get(get_str_arg(a, argc, 0, ""));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

/* -------------------------------------------------------------------------
 * List Operations
 * ------------------------------------------------------------------------- */

XValue vec_list_new(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r; (void)argc; (void)a;
	return xval_int(x_list_alloc());
}

XValue vec_list_free(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(x_list_free_id((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_list_size(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(x_list_count((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_list_add(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* val = get_str_arg(a, argc, 1, "");
	return xval_int(x_list_append_str(id, val));
}

XValue vec_list_add_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int val = (int)get_int_arg(a, argc, 1, 0);
	return xval_int(x_list_append_int(id, val));
}

XValue vec_list_add_float(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	float val = (float)get_float_arg(a, argc, 1, 0.0);
	return xval_int(x_list_append_float(id, val));
}

XValue vec_list_get(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	const char* s = x_list_item_str(id, idx);
	return xval_str(s ? s : "");
}

XValue vec_list_get_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	return xval_int(x_list_item_int(id, idx));
}

XValue vec_list_get_float(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	return xval_float((double)x_list_item_float(id, idx));
}

XValue vec_list_set(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	const char* val = get_str_arg(a, argc, 2, "");
	return xval_int(x_list_set_item_str(id, idx, val));
}

XValue vec_list_set_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	int val = (int)get_int_arg(a, argc, 2, 0);
	return xval_int(x_list_set_item_int(id, idx, val));
}

XValue vec_list_remove_at(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int idx = (int)get_int_arg(a, argc, 1, 0);
	return xval_int(x_list_remove_item(id, idx));
}

XValue vec_list_clear(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	return xval_int(x_list_clear_items(id));
}

XValue vec_list_contains(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* val = get_str_arg(a, argc, 1, "");
	int count = x_list_count(id);
	for (int i = 0; i < count; i++)
	{
		const char* item = x_list_item_str(id, i);
		if (item && strcmp(item, val) == 0) return xval_int(1);
	}
	return xval_int(0);
}

XValue vec_list_contains_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int val = (int)get_int_arg(a, argc, 1, 0);
	int count = x_list_count(id);
	for (int i = 0; i < count; i++)
	{
		if (x_list_item_int(id, i) == val) return xval_int(1);
	}
	return xval_int(0);
}

XValue vec_list_index_of(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* val = get_str_arg(a, argc, 1, "");
	int count = x_list_count(id);
	for (int i = 0; i < count; i++)
	{
		const char* item = x_list_item_str(id, i);
		if (item && strcmp(item, val) == 0) return xval_int(i);
	}
	return xval_int(-1);
}

XValue vec_list_index_of_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int val = (int)get_int_arg(a, argc, 1, 0);
	int count = x_list_count(id);
	for (int i = 0; i < count; i++)
	{
		if (x_list_item_int(id, i) == val) return xval_int(i);
	}
	return xval_int(-1);
}

XValue vec_list_pop(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int count = x_list_count(id);
	if (count > 0)
	{
		const char* last = x_list_item_str(id, count - 1);
		XValue v = xval_str(last ? last : "");
		x_list_remove_item(id, count - 1);
		return v;
	}
	return xval_str("");
}

XValue vec_list_pop_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	int count = x_list_count(id);
	if (count > 0)
	{
		int last = x_list_item_int(id, count - 1);
		x_list_remove_item(id, count - 1);
		return xval_int(last);
	}
	return xval_int(0);
}

XValue vec_list_join(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* sep = get_str_arg(a, argc, 1, "");
	int count = x_list_count(id);
	if (count == 0) return xval_str("");

	size_t cap = 256;
	char* buf = (char*)malloc(cap);
	buf[0] = '\0';
	size_t len = 0;
	size_t sep_len = strlen(sep);

	for (int i = 0; i < count; i++)
	{
		const char* item = x_list_item_str(id, i);
		if (!item) item = "";
		size_t ilen = strlen(item);
		while (len + ilen + sep_len + 1 >= cap)
		{
			cap *= 2;
			buf = (char*)realloc(buf, cap);
		}
		if (i > 0 && sep_len > 0)
		{
			memcpy(buf + len, sep, sep_len);
			len += sep_len;
		}
		memcpy(buf + len, item, ilen);
		len += ilen;
		buf[len] = '\0';
	}
	XValue v = xval_str(buf);
	free(buf);
	return v;
}

XValue vec_list_to_string(XVm* vm, XValue r, int argc, const XValue* a)
{
	return vec_list_join(vm, r, argc, a);
}

/* -------------------------------------------------------------------------
 * Map Operations
 * ------------------------------------------------------------------------- */

XValue vec_map_new(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r; (void)argc; (void)a;
	return xval_int(x_map_alloc());
}

XValue vec_map_free(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(x_map_free_id((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_map_size(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(x_map_count((int)get_int_arg(a, argc, 0, -1)));
}

XValue vec_map_put(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	const char* v = get_str_arg(a, argc, 2, "");
	return xval_int(x_map_insert_str(id, k, v));
}

XValue vec_map_put_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	int v = (int)get_int_arg(a, argc, 2, 0);
	return xval_int(x_map_insert_int(id, k, v));
}

XValue vec_map_put_float(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	float v = (float)get_float_arg(a, argc, 2, 0.0);
	return xval_int(x_map_insert_float(id, k, v));
}

XValue vec_map_get(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	const char* s = x_map_fetch_str(id, k);
	return xval_str(s ? s : "");
}

XValue vec_map_get_int(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	return xval_int(x_map_fetch_int(id, k));
}

XValue vec_map_get_float(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	return xval_float((double)x_map_fetch_float(id, k));
}

XValue vec_map_has(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	return xval_bool(x_map_contains_key(id, k));
}

XValue vec_map_remove(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	const char* k = get_str_arg(a, argc, 1, "");
	return xval_int(x_map_remove_key(id, k));
}

XValue vec_map_clear(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	return xval_int(x_map_free_id(id));
}

XValue vec_map_keys(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	char** keys = NULL;
	int count = x_map_get_all_keys(id, &keys);
	if (count <= 0 || !keys) return xval_str("[]");

	size_t cap = 256;
	char* buf = (char*)malloc(cap);
	buf[0] = '[';
	buf[1] = '\0';
	size_t len = 1;
	for (int i = 0; i < count; i++)
	{
		size_t klen = strlen(keys[i]);
		while (len + klen + 4 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
		if (i > 0) { buf[len++] = ','; buf[len++] = ' '; }
		buf[len++] = '"';
		memcpy(buf + len, keys[i], klen);
		len += klen;
		buf[len++] = '"';
		buf[len] = '\0';
	}
	free(keys);
	if (len + 2 >= cap) buf = (char*)realloc(buf, len + 2);
	buf[len++] = ']';
	buf[len] = '\0';
	XValue v = xval_str(buf);
	free(buf);
	return v;
}

XValue vec_map_values(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r; (void)argc; (void)a;
	return xval_str("[]");
}

XValue vec_map_to_string(XVm* vm, XValue r, int argc, const XValue* a)
{
	return vec_map_keys(vm, r, argc, a);
}

XValue vec_map_keys_list(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)r;
	int id = (int)get_int_arg(a, argc, 0, -1);
	char** keys = NULL;
	int count = x_map_get_all_keys(id, &keys);
	int list_id = x_list_alloc();
	if (keys != NULL && count > 0)
	{
		for (int i = 0; i < count; i++)
		{
			x_list_append_str(list_id, keys[i]);
		}
		free(keys);
	}
	XInstance* inst = xinstance_create_with_id(vm, "List", list_id);
	return xval_obj(inst);
}

/* -------------------------------------------------------------------------
 * System & Process Operations
 * ------------------------------------------------------------------------- */

XValue vec_get_argc(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; return xval_int(get_argc()); }

XValue vec_get_arg(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = get_arg((int)get_int_arg(a, argc, 0, 0));
	return xval_str(s ? s : "");
}

XValue vec_system_exec(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(system_exec(get_str_arg(a, argc, 0, "")));
}

XValue vec_system_getenv(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = system_getenv(get_str_arg(a, argc, 0, ""));
	return xval_str(s ? s : "");
}

XValue vec_system_setenv(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_int(system_setenv(get_str_arg(a, argc, 0, ""), get_str_arg(a, argc, 1, "")));
}

XValue vec_proc_capture(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = proc_capture(get_str_arg(a, argc, 0, ""));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_proc_run(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)r;
	const char* cmd = get_str_arg(a, argc, 0, "");
	void* pr = proc_run(cmd);
	XInstance* inst = xinstance_create(vm, "ProcessResult");
	if (pr != NULL && inst != NULL)
	{
		RtProcResult* res = (RtProcResult*)pr;
		if (inst->field_count >= 2)
		{
			inst->fields[0] = xval_str(res->stdout_str ? res->stdout_str : "");
			inst->fields[1] = xval_int(res->exit_code);
		}
		if (res->stdout_str) free(res->stdout_str);
		free(res);
	}
	return xval_obj(inst);
}

XValue vec_clock_ms(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
#ifdef _WIN32
	clock_t c = clock();
	int64_t ms = (int64_t)(c * 1000 / CLOCKS_PER_SEC);
	return xval_int(ms);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t ms = (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
	return xval_int(ms);
#endif
}

/* -------------------------------------------------------------------------
 * DateTime Operations
 * ------------------------------------------------------------------------- */

XValue vec_datetime_now(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; return xval_int(datetime_now()); }

XValue vec_datetime_format(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	char* s = datetime_format((int)get_int_arg(a, argc, 0, 0), get_str_arg(a, argc, 1, "%Y-%m-%d %H:%M:%S"));
	XValue v = xval_str(s ? s : "");
	if (s) free(s);
	return v;
}

XValue vec_datetime_year(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_year((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_month(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_month((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_day(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_day((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_hour(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_hour((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_minute(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_minute((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_second(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; return xval_int(datetime_second((int)get_int_arg(a, argc, 0, 0))); }
XValue vec_datetime_clock_ms(XVm* vm, XValue r, int argc, const XValue* a) { return vec_clock_ms(vm, r, argc, a); }

/* -------------------------------------------------------------------------
 * JSON Operations
 * ------------------------------------------------------------------------- */

XValue vec_json_is_valid(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	return xval_bool(x_json_validate(get_str_arg(a, argc, 0, "")) != 0);
}

XValue vec_json_parse(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)r;
	const char* str = get_str_arg(a, argc, 0, "");
	int is_list = 0;
	int id = x_json_parse_to_id(str, &is_list);
	if (id != -1)
	{
		const char* cls = is_list ? "List" : "Map";
		XInstance* inst = xinstance_create_with_id(vm, cls, id);
		return xval_obj(inst);
	}
	return xval_null();
}

XValue vec_json_stringify(XVm* vm, XValue r, int argc, const XValue* a)
{
	(void)vm; (void)r;
	if (argc == 0) return xval_str("{}");
	int id = 0;
	int is_list = 0;
	if (a[0].type == VAL_OBJECT && a[0].as.oval != NULL)
	{
		XInstance* inst = (XInstance*)a[0].as.oval;
		id = inst->id;
		if (inst->klass && inst->klass->name && strcmp(inst->klass->name, "List") == 0)
			is_list = 1;
	}
	else if (a[0].type == VAL_INT)
	{
		id = (int)a[0].as.ival;
	}
	char* s = x_json_stringify_id(id, is_list);
	XValue v = xval_str(s ? s : "{}");
	if (s) free(s);
	return v;
}

/* -------------------------------------------------------------------------
 * GC Operations
 * ------------------------------------------------------------------------- */

XValue vec_gc_collect(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	size_t collected = gc_collect();
	return xval_int((int64_t)collected);
}

XValue vec_gc_allocated_bytes(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; return xval_int((int64_t)gc_allocated_bytes()); }
XValue vec_gc_total_objects(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; return xval_int((int64_t)gc_total_objects()); }
XValue vec_gc_enable(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; gc_enable(); return xval_int(1); }
XValue vec_gc_disable(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; gc_disable(); return xval_int(0); }
XValue vec_gc_set_threshold(XVm* vm, XValue r, int argc, const XValue* a) {
	(void)vm; (void)r;
	int bytes = (int)get_int_arg(a, argc, 0, 0);
	if (bytes > 0) gc_set_threshold((size_t)bytes);
	return xval_int(bytes);
}
XValue vec_gc_dump(XVm* vm, XValue r, int argc, const XValue* a) { (void)vm; (void)r; (void)argc; (void)a; gc_dump(); return xval_int(0); }

/* -------------------------------------------------------------------------
 * Primitive Type Methods
 * ------------------------------------------------------------------------- */

XValue vec_int_add(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	int64_t base = (receiver.type == VAL_INT) ? receiver.as.ival :
	               (receiver.type == VAL_FLOAT) ? (int64_t)receiver.as.fval : 0;
	int64_t val = (argc > 0) ? get_int_arg(args, argc, 0, 0) : 0;
	return xval_int(base + val);
}

/* -------------------------------------------------------------------------
 * Registration
 * ------------------------------------------------------------------------- */

void xvm_register_vector_func(const char* name, XVectorFn fn, int arity)
{
	if (!name || !fn) return;

	func_deftion* fd = get_func_by_name((char*)name);
	if (fd == NULL)
	{
		fd = new_func();
		fd->func_name = strdup(name);
		fd->start_parm_count = arity;
	}
	fd->native_kind = NATIVE_KIND_VECTORCALL;
	fd->vector_func = (void*)fn;
}

static bool s_vec_initialized = false;

void xvec_builtins_init(void)
{
	if (s_vec_initialized) return;
	s_vec_initialized = true;

	/* Core VM-Aware Functions */
	xvm_register_vector_func("assert", vec_assert, 1);
	xvm_register_vector_func("print", vec_print, 1);
	xvm_register_vector_func("vprint", vec_print, 1);
	xvm_register_vector_func("println", vec_println, 1);
	xvm_register_vector_func("print_f", vec_print_f, 1);
	xvm_register_vector_func("printf", vec_print_f, 1);
	xvm_register_vector_func("eval", vec_eval, 1);
	xvm_register_vector_func("scan", vec_scan, 1);
	xvm_register_vector_func("readline", vec_readline, 0);
	xvm_register_vector_func("read_line", vec_readline, 0);
	xvm_register_vector_func("gets", vec_readline, 0);
	xvm_register_vector_func("random", vec_random, 1);
	xvm_register_vector_func("time", vec_time, 0);
	xvm_register_vector_func("exit", vec_exit, 1);
	xvm_register_vector_func("echo", vec_echo, 1);
	xvm_register_vector_func("import", vec_import, 1);

	/* Reflection Functions */
	xvm_register_vector_func("typeof", vec_typeof, 1);
	xvm_register_vector_func("type_name", vec_type_name, 1);
	xvm_register_vector_func("has_field", vec_has_field, 2);
	xvm_register_vector_func("get_field", vec_get_field, 2);
	xvm_register_vector_func("set_field", vec_set_field, 3);

	/* String & Regex Functions */
	xvm_register_vector_func("len", vec_len, 1);
	xvm_register_vector_func("strlen", vec_len, 1);
	xvm_register_vector_func("str", vec_str, 1);
	xvm_register_vector_func("substr", vec_substr, 2);
	xvm_register_vector_func("index_of", vec_index_of, 2);
	xvm_register_vector_func("find", vec_index_of, 2);
	xvm_register_vector_func("trim", vec_trim, 1);
	xvm_register_vector_func("to_lower", vec_to_lower, 1);
	xvm_register_vector_func("lower", vec_to_lower, 1);
	xvm_register_vector_func("to_upper", vec_to_upper, 1);
	xvm_register_vector_func("upper", vec_to_upper, 1);
	xvm_register_vector_func("starts_with", vec_starts_with, 2);
	xvm_register_vector_func("ends_with", vec_ends_with, 2);
	xvm_register_vector_func("regex_match", vec_regex_match, 2);
	xvm_register_vector_func("match", vec_regex_match, 2);
	xvm_register_vector_func("regex_find", vec_regex_find, 2);
	xvm_register_vector_func("regex_replace", vec_regex_replace, 3);
	xvm_register_vector_func("replace", vec_replace, 3);
	xvm_register_vector_func("str_split", vec_str_split, 2);
	xvm_register_vector_func("eql", vec_eql, 2);

	/* Math Functions */
	xvm_register_vector_func("sqrt", vec_math_sqrt, 1);
	xvm_register_vector_func("math_sqrt", vec_math_sqrt, 1);
	xvm_register_vector_func("pow", vec_math_pow, 2);
	xvm_register_vector_func("math_pow", vec_math_pow, 2);
	xvm_register_vector_func("abs", vec_math_abs, 1);
	xvm_register_vector_func("math_abs", vec_math_abs, 1);
	xvm_register_vector_func("min", vec_math_min, 2);
	xvm_register_vector_func("math_min", vec_math_min, 2);
	xvm_register_vector_func("max", vec_math_max, 2);
	xvm_register_vector_func("math_max", vec_math_max, 2);
	xvm_register_vector_func("floor", vec_math_floor, 1);
	xvm_register_vector_func("math_floor", vec_math_floor, 1);
	xvm_register_vector_func("ceil", vec_math_ceil, 1);
	xvm_register_vector_func("math_ceil", vec_math_ceil, 1);
	xvm_register_vector_func("round", vec_math_round, 1);
	xvm_register_vector_func("math_round", vec_math_round, 1);
	xvm_register_vector_func("sin", vec_math_sin, 1);
	xvm_register_vector_func("math_sin", vec_math_sin, 1);
	xvm_register_vector_func("cos", vec_math_cos, 1);
	xvm_register_vector_func("math_cos", vec_math_cos, 1);
	xvm_register_vector_func("tan", vec_math_tan, 1);
	xvm_register_vector_func("math_tan", vec_math_tan, 1);
	xvm_register_vector_func("log", vec_math_log, 1);
	xvm_register_vector_func("math_log", vec_math_log, 1);

	/* File Functions */
	xvm_register_vector_func("read_file", vec_file_read_all, 1);
	xvm_register_vector_func("file_read_all", vec_file_read_all, 1);
	xvm_register_vector_func("write_file", vec_file_write_all, 2);
	xvm_register_vector_func("file_write_all", vec_file_write_all, 2);
	xvm_register_vector_func("file_append", vec_file_append, 2);
	xvm_register_vector_func("file_exists", vec_file_exists, 1);
	xvm_register_vector_func("file_size", vec_file_size, 1);
	xvm_register_vector_func("file_remove", vec_file_remove, 1);
	xvm_register_vector_func("file_delete", vec_file_remove, 1);
	xvm_register_vector_func("file_open", vec_file_open, 2);
	xvm_register_vector_func("file_read", vec_file_read, 2);
	xvm_register_vector_func("file_write", vec_file_write, 2);
	xvm_register_vector_func("file_close", vec_file_close, 1);

	/* Directory Functions */
	xvm_register_vector_func("dir_list", vec_dir_list, 1);
	xvm_register_vector_func("dir_create", vec_dir_create, 1);
	xvm_register_vector_func("dir_exists", vec_dir_exists, 1);
	xvm_register_vector_func("dir_remove", vec_dir_remove, 1);

	/* Socket Functions */
	xvm_register_vector_func("socket_create", vec_socket_create, 1);
	xvm_register_vector_func("socket", vec_socket_create, 1);
	xvm_register_vector_func("socket_connect", vec_socket_connect, 3);
	xvm_register_vector_func("connect", vec_socket_connect, 3);
	xvm_register_vector_func("socket_bind", vec_socket_bind, 3);
	xvm_register_vector_func("bind", vec_socket_bind, 3);
	xvm_register_vector_func("socket_listen", vec_socket_listen, 2);
	xvm_register_vector_func("listen", vec_socket_listen, 2);
	xvm_register_vector_func("socket_accept", vec_socket_accept, 1);
	xvm_register_vector_func("accept", vec_socket_accept, 1);
	xvm_register_vector_func("socket_send", vec_socket_send, 2);
	xvm_register_vector_func("send", vec_socket_send, 2);
	xvm_register_vector_func("socket_recv", vec_socket_recv, 2);
	xvm_register_vector_func("recv", vec_socket_recv, 2);
	xvm_register_vector_func("socket_close", vec_socket_close, 1);
	xvm_register_vector_func("close", vec_socket_close, 1);
	xvm_register_vector_func("socket_set_timeout", vec_socket_set_timeout, 2);
	xvm_register_vector_func("socket_set_reuseaddr", vec_socket_set_reuseaddr, 2);
	xvm_register_vector_func("socket_sendto", vec_socket_sendto, 4);
	xvm_register_vector_func("socket_recvfrom", vec_socket_recvfrom, 2);
	xvm_register_vector_func("http_get", vec_http_get, 1);

	/* List Functions */
	xvm_register_vector_func("list_new", vec_list_new, 0);
	xvm_register_vector_func("list_free", vec_list_free, 1);
	xvm_register_vector_func("list_size", vec_list_size, 1);
	xvm_register_vector_func("list_add", vec_list_add, 2);
	xvm_register_vector_func("list_add_int", vec_list_add_int, 2);
	xvm_register_vector_func("list_add_float", vec_list_add_float, 2);
	xvm_register_vector_func("list_get", vec_list_get, 2);
	xvm_register_vector_func("list_get_int", vec_list_get_int, 2);
	xvm_register_vector_func("list_get_float", vec_list_get_float, 2);
	xvm_register_vector_func("list_set", vec_list_set, 3);
	xvm_register_vector_func("list_set_int", vec_list_set_int, 3);
	xvm_register_vector_func("list_remove_at", vec_list_remove_at, 2);
	xvm_register_vector_func("list_clear", vec_list_clear, 1);
	xvm_register_vector_func("list_contains", vec_list_contains, 2);
	xvm_register_vector_func("list_contains_int", vec_list_contains_int, 2);
	xvm_register_vector_func("list_index_of", vec_list_index_of, 2);
	xvm_register_vector_func("list_index_of_int", vec_list_index_of_int, 2);
	xvm_register_vector_func("list_pop", vec_list_pop, 1);
	xvm_register_vector_func("list_pop_int", vec_list_pop_int, 1);
	xvm_register_vector_func("list_join", vec_list_join, 2);
	xvm_register_vector_func("list_to_string", vec_list_to_string, 1);

	/* Map Functions */
	xvm_register_vector_func("map_new", vec_map_new, 0);
	xvm_register_vector_func("map_free", vec_map_free, 1);
	xvm_register_vector_func("map_size", vec_map_size, 1);
	xvm_register_vector_func("map_put", vec_map_put, 3);
	xvm_register_vector_func("map_put_int", vec_map_put_int, 3);
	xvm_register_vector_func("map_put_float", vec_map_put_float, 3);
	xvm_register_vector_func("map_get", vec_map_get, 2);
	xvm_register_vector_func("map_get_int", vec_map_get_int, 2);
	xvm_register_vector_func("map_get_float", vec_map_get_float, 2);
	xvm_register_vector_func("map_has", vec_map_has, 2);
	xvm_register_vector_func("map_remove", vec_map_remove, 2);
	xvm_register_vector_func("map_clear", vec_map_clear, 1);
	xvm_register_vector_func("map_keys", vec_map_keys, 1);
	xvm_register_vector_func("map_values", vec_map_values, 1);
	xvm_register_vector_func("map_to_string", vec_map_to_string, 1);
	xvm_register_vector_func("map_keys_list", vec_map_keys_list, 1);

	/* System & Process Functions */
	xvm_register_vector_func("get_arg", vec_get_arg, 1);
	xvm_register_vector_func("get_argc", vec_get_argc, 0);
	xvm_register_vector_func("system_exec", vec_system_exec, 1);
	xvm_register_vector_func("exec", vec_system_exec, 1);
	xvm_register_vector_func("system_getenv", vec_system_getenv, 1);
	xvm_register_vector_func("getenv", vec_system_getenv, 1);
	xvm_register_vector_func("system_setenv", vec_system_setenv, 2);
	xvm_register_vector_func("setenv", vec_system_setenv, 2);
	xvm_register_vector_func("proc_capture", vec_proc_capture, 1);
	xvm_register_vector_func("proc_run", vec_proc_run, 1);
	xvm_register_vector_func("clock_ms", vec_clock_ms, 0);
	xvm_register_vector_func("time_ms", vec_clock_ms, 0);

	/* DateTime Functions */
	xvm_register_vector_func("datetime_now", vec_datetime_now, 0);
	xvm_register_vector_func("datetime_format", vec_datetime_format, 2);
	xvm_register_vector_func("datetime_year", vec_datetime_year, 1);
	xvm_register_vector_func("datetime_month", vec_datetime_month, 1);
	xvm_register_vector_func("datetime_day", vec_datetime_day, 1);
	xvm_register_vector_func("datetime_hour", vec_datetime_hour, 1);
	xvm_register_vector_func("datetime_minute", vec_datetime_minute, 1);
	xvm_register_vector_func("datetime_second", vec_datetime_second, 1);
	xvm_register_vector_func("datetime_clock_ms", vec_datetime_clock_ms, 0);

	/* JSON Functions */
	xvm_register_vector_func("json_parse", vec_json_parse, 1);
	xvm_register_vector_func("json_stringify", vec_json_stringify, 1);
	xvm_register_vector_func("json_is_valid", vec_json_is_valid, 1);

	/* GC Functions */
	xvm_register_vector_func("gc_collect", vec_gc_collect, 0);
	xvm_register_vector_func("gc_allocated_bytes", vec_gc_allocated_bytes, 0);
	xvm_register_vector_func("gc_total_objects", vec_gc_total_objects, 0);
	xvm_register_vector_func("gc_enable", vec_gc_enable, 0);
	xvm_register_vector_func("gc_disable", vec_gc_disable, 0);
	xvm_register_vector_func("gc_set_threshold", vec_gc_set_threshold, 1);
	xvm_register_vector_func("gc_dump", vec_gc_dump, 0);

	/* String Methods Vectorcall Registration */
	for (int i = 0; i < 30; i++)
	{
		func_deftion* fn = &T_STRING->d_functions[i];
		if (fn->func_name == NULL) continue;
		fn->native_kind = NATIVE_KIND_VECTORCALL;
		if (strcmp(fn->func_name, "len") == 0 || strcmp(fn->func_name, "length") == 0) fn->vector_func = (void*)vec_len;
		else if (strcmp(fn->func_name, "eql") == 0) fn->vector_func = (void*)vec_eql;
		else if (strcmp(fn->func_name, "replace") == 0) fn->vector_func = (void*)vec_replace;
		else if (strcmp(fn->func_name, "substr") == 0) fn->vector_func = (void*)vec_substr;
		else if (strcmp(fn->func_name, "index_of") == 0 || strcmp(fn->func_name, "find") == 0) fn->vector_func = (void*)vec_index_of;
		else if (strcmp(fn->func_name, "trim") == 0) fn->vector_func = (void*)vec_trim;
		else if (strcmp(fn->func_name, "to_lower") == 0 || strcmp(fn->func_name, "lower") == 0) fn->vector_func = (void*)vec_to_lower;
		else if (strcmp(fn->func_name, "to_upper") == 0 || strcmp(fn->func_name, "upper") == 0) fn->vector_func = (void*)vec_to_upper;
		else if (strcmp(fn->func_name, "starts_with") == 0) fn->vector_func = (void*)vec_starts_with;
		else if (strcmp(fn->func_name, "ends_with") == 0) fn->vector_func = (void*)vec_ends_with;
		else if (strcmp(fn->func_name, "regex_match") == 0 || strcmp(fn->func_name, "match") == 0) fn->vector_func = (void*)vec_regex_match;
		else if (strcmp(fn->func_name, "regex_find") == 0) fn->vector_func = (void*)vec_regex_find;
		else if (strcmp(fn->func_name, "regex_replace") == 0) fn->vector_func = (void*)vec_regex_replace;
		else if (strcmp(fn->func_name, "split") == 0) fn->vector_func = (void*)vec_str_split;
	}

	/* Int Methods Vectorcall Registration */
	for (int i = 0; i < T_INT->d_function_size; i++)
	{
		func_deftion* fn = &T_INT->d_functions[i];
		if (fn->func_name == NULL) continue;
		fn->native_kind = NATIVE_KIND_VECTORCALL;
		if (strcmp(fn->func_name, "add") == 0) fn->vector_func = (void*)vec_int_add;
	}

	/* Array Methods Vectorcall Registration */
	for (int i = 0; i < T_ARRAY->d_function_size; i++)
	{
		func_deftion* fn = &T_ARRAY->d_functions[i];
		if (fn->func_name == NULL) continue;
		fn->native_kind = NATIVE_KIND_VECTORCALL;
		if (strcmp(fn->func_name, "len") == 0) fn->vector_func = (void*)vec_len;
	}
}
