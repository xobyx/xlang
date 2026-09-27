#include "xvec_builtins.h"
#include "functions.h"
#include "xast_parser.h"
#include "xdiag.h"
#include "xir_compiler.h"
#include "xgc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#endif

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

	/* If first arg has '%' and there are multiple arguments, format print */
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

	/* If code is a single expression, compile as expression to preserve stack return value */
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

		/* Copy globals in */
		for (int i = 0; i < target_vm->global_count && i < VM_GLOBALS_MAX; i++)
		{
			eval_vm.globals[i].name = target_vm->globals[i].name ? strdup(target_vm->globals[i].name) : NULL;
			eval_vm.globals[i].value = target_vm->globals[i].value;
		}
		eval_vm.global_count = target_vm->global_count;
	}

	XVmResult res = xvm_run(&eval_vm, &chunk);
	XValue ret_val = (res == VM_OK && eval_vm.stack_top > eval_vm.stack) ? xvm_pop(&eval_vm) : xval_null();

	if (target_vm != NULL)
	{
		/* Transfer newly created instances to target_vm */
		if (eval_vm.all_instances != NULL)
		{
			XInstance* last = eval_vm.all_instances;
			while (last->next != NULL) last = last->next;
			last->next = target_vm->all_instances;
			target_vm->all_instances = eval_vm.all_instances;
			eval_vm.all_instances = NULL;
		}

		/* Transfer newly created closures to target_vm */
		if (eval_vm.all_closures != NULL)
		{
			XClosure* last = eval_vm.all_closures;
			while (last->next != NULL) last = last->next;
			last->next = target_vm->all_closures;
			target_vm->all_closures = eval_vm.all_closures;
			eval_vm.all_closures = NULL;
		}

		/* Detach shared classes so eval_vm teardown doesn't free them */
		eval_vm.all_classes = NULL;
		eval_vm.class_object = NULL;
		eval_vm.class_list = NULL;
		eval_vm.class_map = NULL;
		eval_vm.class_datetime = NULL;

		/* Propagate new/updated globals back to caller VM */
		for (int i = 0; i < eval_vm.global_count; i++)
		{
			if (eval_vm.globals[i].name != NULL)
			{
				xvm_set_global(target_vm, eval_vm.globals[i].name, eval_vm.globals[i].value);
			}
		}
	}

	xvm_free(&eval_vm);
	xir_chunk_free(&chunk);
	ast_program_destroy(prog);

	return ret_val;
}

/* -------------------------------------------------------------------------
 * Reflection Builtins
 * ------------------------------------------------------------------------- */

XValue vec_typeof(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc < 1) return xval_str("null");

	switch (args[0].type)
	{
	case VAL_NULL:     return xval_str("null");
	case VAL_BOOL:     return xval_str("bool");
	case VAL_INT:      return xval_str("int");
	case VAL_FLOAT:    return xval_str("float");
	case VAL_STRING:   return xval_str("string");
	case VAL_FUNCTION:
	case VAL_CLOSURE:  return xval_str("function");
	case VAL_OBJECT:
		{
			XInstance* inst = (XInstance*)args[0].as.oval;
			if (inst != NULL && inst->klass != NULL && inst->klass->name != NULL)
			{
				return xval_str(inst->klass->name);
			}
			return xval_str("object");
		}
	default:
		return xval_str("unknown");
	}
}

XValue vec_type_name(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	return vec_typeof(vm, receiver, argc, args);
}

XValue vec_has_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc < 2 || args[0].type != VAL_OBJECT || args[1].type != VAL_STRING)
	{
		return xval_bool(false);
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (inst == NULL || inst->klass == NULL || args[1].as.sval == NULL)
	{
		return xval_bool(false);
	}

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	return xval_bool(slot >= 0);
}

XValue vec_get_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc < 2 || args[0].type != VAL_OBJECT || args[1].type != VAL_STRING)
	{
		return xval_null();
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (inst == NULL || inst->klass == NULL || args[1].as.sval == NULL)
	{
		return xval_null();
	}

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		return inst->fields[slot];
	}
	return xval_null();
}

XValue vec_set_field(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm;
	(void)receiver;
	if (argc < 3 || args[0].type != VAL_OBJECT || args[1].type != VAL_STRING)
	{
		return xval_bool(false);
	}

	XInstance* inst = (XInstance*)args[0].as.oval;
	if (inst == NULL || inst->klass == NULL || args[1].as.sval == NULL)
	{
		return xval_bool(false);
	}

	int slot = xclass_find_field_slot(inst->klass, args[1].as.sval);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		inst->fields[slot] = args[2];
		return xval_bool(true);
	}
	return xval_bool(false);
}

/* -------------------------------------------------------------------------
 * System & GC Builtins
 * ------------------------------------------------------------------------- */

XValue vec_clock_ms(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
#ifdef _WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	return xval_int((int64_t)(t / 10000));
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t ms = (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
	return xval_int(ms);
#endif
}

XValue vec_gc_collect(XVm* vm, XValue receiver, int argc, const XValue* args)
{
	(void)vm; (void)receiver; (void)argc; (void)args;
	size_t collected = gc_collect();
	return xval_int((int64_t)collected);
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
	xvm_register_vector_func("println", vec_println, 1);
	xvm_register_vector_func("print_f", vec_print_f, 1);
	xvm_register_vector_func("eval", vec_eval, 1);

	/* Reflection Functions */
	xvm_register_vector_func("typeof", vec_typeof, 1);
	xvm_register_vector_func("type_name", vec_type_name, 1);
	xvm_register_vector_func("has_field", vec_has_field, 2);
	xvm_register_vector_func("get_field", vec_get_field, 2);
	xvm_register_vector_func("set_field", vec_set_field, 3);

	/* System & GC Functions */
	xvm_register_vector_func("clock_ms", vec_clock_ms, 0);
	xvm_register_vector_func("gc_collect", vec_gc_collect, 0);
}
