#include "functions.h"
#include "func_stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(__GNUC__)|| defined(__MINGW64__)
#include <stdarg.h>
#endif
#include <time.h>
#include "http.h"
#include "echo.h"
#include "xsocket.h"
#include "xfile.h"
#include "xsys.h"
#include "xstring.h"
#include "xmath.h"
#include "xcollection.h"
#include "xdatetime.h"
#include "xjson.h"
#include "ximport.h"
#include "xgc.h"
#include "xir_compiler.h"
#include "xvm.h"

void len(fcall* y);
type_stack simple_type_stack = {.top = T_FUNC, .root = T_LONG, .size = 10};

void int_add(fcall* fcall)
{
	if (fcall == NULL || fcall->context == NULL || fcall->context->value_int == NULL) return;
	int val = 0;
	if (fcall->parm_count_c >= 1 && fcall->func_parmeters[0].value_int != NULL)
		val = *fcall->func_parmeters[0].value_int;
	fcall->_return.value_int = new_int(1, *fcall->context->value_int + val);
	fcall->_return.values = fcall->_return.value_int;
	fcall->_return.type_define = T_INT;
	fcall->_return.size = 1;
}

void xassert(fcall* fcall)
{
	if (fcall == NULL || fcall->parm_count_c < 1)
	{
		fprintf(stderr, "Assertion failed: assert requires at least 1 argument\n");
		exit(1);
	}

	bool condition = false;
	var* arg0 = &fcall->func_parmeters[0];

	if (arg0->type_define == T_BOOL)
	{
		if (arg0->value_bool != NULL)
			condition = *arg0->value_bool;
		else if (arg0->values != NULL)
			condition = *(bool*)arg0->values;
	}
	else if (arg0->type_define == T_INT)
	{
		if (arg0->value_int != NULL)
			condition = (*arg0->value_int != 0);
		else if (arg0->values != NULL)
			condition = (*(int*)arg0->values != 0);
	}
	else if (arg0->type_define == T_FLOAT)
	{
		if (arg0->value_float != NULL)
			condition = (*arg0->value_float != 0.0f);
		else if (arg0->values != NULL)
			condition = (*(float*)arg0->values != 0.0f);
	}
	else if (arg0->type_define == T_STRING)
	{
		char* str = NULL;
		if (arg0->value_str_ptr != NULL)
			str = *arg0->value_str_ptr;
		else if (arg0->values != NULL)
			str = (char*)arg0->values;
		condition = (str != NULL && str[0] != '\0');
	}
	else
	{
		condition = (arg0->values != NULL || arg0->value_type_instsance != NULL);
	}

	if (!condition)
	{
		const char* msg = NULL;
		if (fcall->parm_count_c >= 2)
		{
			var* arg1 = &fcall->func_parmeters[1];
			if (arg1->type_define == T_STRING)
			{
				if (arg1->value_str_ptr != NULL && *arg1->value_str_ptr != NULL)
					msg = *arg1->value_str_ptr;
				else if (arg1->values != NULL)
					msg = (char*)arg1->values;
			}
		}

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

	fcall->_return.type_define = T_INT;
	fcall->_return.value_int = new_int(1, 1);
	fcall->_return.values = fcall->_return.value_int;
}

void xeql(fcall* fcall);
void xreplace(fcall* fcall);
type_def SIMPLE_TYPE[] = {
	{
		.type_id = 0, .type_name = "long", .base = 0,
		.stack_next = T_STRING
	},
	{
		.type_id = 1, .type_name = "string",
		.d_functions =
		{
			{
				.start_func_parmeters = {0}, .func_code = &len, .func_name = "len", .function_type = f_main,
				.access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 1, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &xeql, .func_name = "eql", .function_type = f_main,
				.access = PUBLIC,
				.return_type = T_BOOL, .stack_next = T_STRING->d_functions + 2, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_ANY,T_ANY}, .func_code = &xreplace, .func_name = "replace",
				.function_type = f_main,
				.access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 3, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_substr, .func_name = "substr",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 4, .start_parm_count = 2
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_index_of, .func_name = "index_of",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 5, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_index_of, .func_name = "find",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 6, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_trim, .func_name = "trim",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 7, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_lower, .func_name = "to_lower",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 8, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_lower, .func_name = "lower",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 9, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_upper, .func_name = "to_upper",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 10, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_upper, .func_name = "upper",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 11, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_starts_with, .func_name = "starts_with",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 12, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_ends_with, .func_name = "ends_with",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 13, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_match, .func_name = "regex_match",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 14, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_match, .func_name = "match",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = T_STRING->d_functions + 15, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_find, .func_name = "regex_find",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 16, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_replace, .func_name = "regex_replace",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .stack_next = T_STRING->d_functions + 17, .start_parm_count = 2
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_string_split, .func_name = "split",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 18, .base = 0,
		.stack_next = T_CHAR
	},
	{.type_id = 2, .type_name = "char", .base = 0, .stack_next = T_INT},
	{
		.type_id = 3, .type_name = "int",
		.d_functions =
		{
			{
				.start_func_parmeters = {T_ANY}, .func_code = &int_add, .func_name = "add",
				.function_type = f_main, .access = PUBLIC, .start_parm_count = 1,
				.return_type = T_INT, .stack_next = 0
			}
		},
		.d_function_size = 1, .base = 0,
		.stack_next = T_BOOL
	},
	{.type_id = 4, .type_name = "bool", .base = 0, .stack_next = T_FLOAT},
	{.type_id = 5, .type_name = "float", .base = 0, .stack_next = T_OBJECT},
	{.type_id = 6, .type_name = "object", .base = 0, .stack_next = T_ARRAY},
	{
		.type_id = 7, .type_name = "array", .d_propertys = {{.name = "type", .type_define = T_TYPE_INFO}},
		.d_functions = {
			{
				.start_func_parmeters = {0}, .func_code = &len, .func_name = "len",
				.function_type = f_main, .access = PUBLIC, .return_type = T_INT, .stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 1, .base = 0,
		.stack_next = T_ANY
	},
	{.type_id = 8, .type_name = "T", .base = 0, .stack_next = T_FUNC},
	{.type_id = 9, .type_name = "func", .base = 0, .stack_next = T_TYPE_INFO},
	{.type_id = 10, .type_name = "type", .base = 0, .stack_next = 0}
};






// check if text starts with keyword at a word boundary
int starts_with_keyword(const char* text, const char* keyword)
{
	if (text == NULL || keyword == NULL)
		return 0;

	size_t len_kw = strlen(keyword);
	if (strncmp(text, keyword, len_kw) != 0)
		return 0;

	char next = text[len_kw];
	if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
		return 0;

	return 1;
}

int eql(const char* n, const char* x)
{
	return starts_with_keyword(n, x);
}




void assign_array_index(var* nvalue, var* marray, int index)
{
	if (marray == NULL || nvalue == NULL || index < 0) return;
	if (marray->type_define == T_INT)
	{
		marray->value_int[index] = *nvalue->value_int;
	}
	else if (marray->type_define == T_FLOAT)
	{
		marray->value_float[index] = *nvalue->value_float;
	}
	else if (marray->type_define == T_LONG)
	{
		marray->value_long[index] = *nvalue->value_long;
	}
	else if (marray->type_define == T_CHAR)
	{
		marray->value_char_ptr[index] = *nvalue->value_char_ptr;
	}
	else if (marray->type_define == T_BOOL)
	{
		marray->value_bool[index] = *nvalue->value_bool;
	}
	else if (marray->type_define == T_STRING && nvalue->type_define == T_STRING)
	{
		marray->value_str_ptr[index] = *nvalue->value_str_ptr;
	}
	else if (marray->type_define == T_STRING && nvalue->type_define == T_CHAR)
	{
		char* dst = marray->value_str_ptr[0];
		dst[index] = *nvalue->value_char_ptr;
	}
	else if (marray->value_type_instsance != NULL && nvalue->value_type_instsance != NULL)
	{
		marray->value_type_instsance[index] = *nvalue->value_type_instsance;
	}
}


void unescape_string(char* str)
{
	if (str == NULL) return;
	char* dst = str;
	for (char* p = str; *p; p++)
	{
		if (*p == '\\' && *(p + 1) == 'n')
		{
			*dst++ = '\n';
			p++;
		}
		else if (*p == '\\' && *(p + 1) == 't')
		{
			*dst++ = '\t';
			p++;
		}
		else if (*p == '\\' && *(p + 1) == 'r')
		{
			*dst++ = '\r';
			p++;
		}
		else if (*p == '\\' && *(p + 1) == '\\')
		{
			*dst++ = '\\';
			p++;
		}
		else if (*p == '\\' && *(p + 1) == '"')
		{
			*dst++ = '"';
			p++;
		}
		else
		{
			*dst++ = *p;
		}
	}
	*dst = '\0';
}

void scap_string(char* m)
{
	unescape_string(m);
}

void print_f(fcall* temp)
{
	if (temp == NULL || temp->parm_count_c < 1)
		return;

	var* fparms = temp->func_parmeters;
	if (fparms == NULL || fparms->type_define != T_STRING || fparms->value_str_ptr == NULL || *fparms->value_str_ptr == NULL)
		return;

	const char* fmt = *fparms->value_str_ptr;
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
			if (arg_idx < temp->parm_count_c)
			{
				var* arg = &temp->func_parmeters[arg_idx++];
				if (arg->values == NULL)
				{
					printf("null");
				}
				else if (arg->type_define == T_INT)
				{
					printf("%d", *arg->value_int);
				}
				else if (arg->type_define == T_STRING)
				{
					printf("%s", *arg->value_str_ptr ? *arg->value_str_ptr : "null");
				}
				else if (arg->type_define == T_FLOAT)
				{
					printf("%f", *arg->value_float);
				}
				else if (arg->type_define == T_LONG)
				{
					printf("%ld", *arg->value_long);
				}
				else if (arg->type_define == T_CHAR)
				{
					printf("%c", *arg->value_char_ptr);
				}
				else if (arg->type_define == T_BOOL)
				{
					printf("%s", *arg->value_bool ? "True" : "False");
				}
			}
		}
		else
		{
			putchar(*p);
		}
	}
	putchar('\n');
}

void eval(fcall* temp)
{
	if (temp->parm_count_c == 0) return;
	if (temp->parm_count_c == 1 && temp->func_parmeters->type_define == T_STRING)
	{
		const char* code = *temp->func_parmeters->value_str_ptr;
		if (code && *code)
		{
			AstProgram* prog = xast_parse_source(code, "<eval>");
			if (prog != NULL && xdiag_get_error_count() == 0)
			{
				XIrChunk chunk;
				xir_chunk_init(&chunk);
				xir_compile_program(prog, &chunk);
				XVm vm;
				xvm_init(&vm);
				xvm_run(&vm, &chunk);
				xvm_free(&vm);
				xir_chunk_free(&chunk);
				ast_program_destroy(prog);
			}
			else if (prog != NULL)
			{
				ast_program_destroy(prog);
			}
		}
	}
}

void print(fcall* temp)
{
	if (temp == NULL || temp->parm_count_c == 0)
	{
		printf("\n");
		if (temp != NULL)
			temp->_return.value_int = new_int(1, 0);
		return;
	}
	if (temp->parm_count_c > 1)
	{
		print_f(temp);
		return;
	}
	var* m = temp->func_parmeters;
	if (m == NULL || m->type_define == NULL || m->values == NULL)
	{
		printf("null\n");
		temp->_return.value_int = new_int(1, 0);
		return;
	}
	int* r = (int*)malloc(sizeof(int));
	*r = -1;
	if (m->type_define == T_INT)
	{
		for (int ms = 0; ms < m->size; ms++)
		{
			*r = printf("%d \n", m->value_int[ms]);
		}
	}
	else if (m->type_define == T_STRING)
	{
		for (int ms = 0; ms < m->size; ms++)
		{
			*r = printf("%s \n", m->value_str_ptr[ms] ? m->value_str_ptr[ms] : "null");
		}
	}
	else if (m->type_define == T_LONG)
	{
		for (int i = 0; i < m->size; i++)
		{
			*r = printf("%ld\n", m->value_long[i]);
		}
	}
	else if (m->type_define == T_CHAR)
	{
		for (int i = 0; i < m->size; i++)
		{
			*r = printf("%c\n", m->value_char_ptr[i]);
		}
	}
	else if (m->type_define == T_FLOAT)
	{
		for (int i = 0; i < m->size; i++)
		{
			*r = printf("%f\n", m->value_float[i]);
		}
	}
	else if (m->type_define == T_BOOL)
	{
		for (int i = 0; i < m->size; i++)
		{
			*r = printf("%s\n", m->value_bool[i] ? "True" : "False");
		}
	}
	else
	{
		printf("[object]\n");
	}
	temp->_return.value_int = r;
}

var* new_var(char* name, type_def* vtype)
{
	return new_var_on_stack(varss, name, vtype);
}

var* new_temp_var(type_def* typ)
{
	var* y = new_var_on_stack(t_varss, NULL, typ);

	return y;
}

type_def* new_type()
{
	type_def* local = new_type_stack(types);
	local->type_id = types->size - 1;
	return local;
}


char** get_pptr_string(char* t)
{
	char** y = (char**)install_memory_with_type(T_STRING, 1);
	if (y != NULL)
	{
		*y = t;
		return y;
	}
	printf("unknown state");
	return NULL;
}


void import(fcall* d)
{
	if (d->func_parmeters->type_define == T_STRING && d->func_parmeters->value_str_ptr != NULL && *d->func_parmeters->value_str_ptr != NULL)
	{
		x_import_module(*d->func_parmeters->value_str_ptr);
	}
	else
	{
		printf("Error: input is not string\n");
	}
}

void time_x(fcall* d)
{
	time_t a;
	time(&a);

	char* buff = (char*)malloc(sizeof(char) * 20);
	memset(buff, 0, 20);

	struct tm* tm_info = localtime(&a);
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", tm_info);

	d->_return.value_str_ptr = get_pptr_string(buff);
}

void _exit_(fcall* d)
{
	if (d->func_parmeters->type_define == T_INT && d->func_parmeters->value_int != NULL)
	{
		exit(*d->func_parmeters->value_int);
	}
	else
	{
		printf("too few arguments to function exit");
	}
}

bool rxx = false;

void random_(fcall* inc)
{
	if (!rxx)
	{
		srand(time(0));
		rxx = true;
	}

	int* x = (int*)malloc(sizeof(int));
	*x = 0;
	const int in = *inc->func_parmeters->value_int;
	if (inc->func_parmeters->type_define->type_name == T_INT->type_name)
	{
		*x = (rand() % in) + 1;
	}
	else
	{
		printf("Error: input is not a int");
	}
	inc->_return.value_int = x;
}

void sin__(fcall* d)
{
	int* x = (int*)malloc(sizeof(int));
	if (d->func_parmeters->type_define->type_name == T_INT->type_name)
	{
		//	*x = sinf(*(float*)d->fun_p.root->value);
	}
	else
	{
		printf("Error: input is not string");
	}
	d->_return.value_int = x;
}

void scan(fcall* d) ///xscan(var out,"%s");
{
	if (d->func_parmeters->type_define->type_name == T_STRING->type_name)
	{
		var* out = get_globle_var_by_name(*d->func_parmeters->value_str_ptr);

		scanf("%d", out->value_int);

		//b++;
	}
	else
	{
		printf("Error: input is not string");
	}
}


void str(fcall* d)
{
	char* buff = NULL;
	if (d->func_parmeters->type_define == T_FLOAT)
	{
		float* u = d->func_parmeters->value_float;
		buff = (char*)malloc(sizeof(float) * 4 + 1);
		sprintf(buff, "%f", *u);
		char* t = (char*)malloc(strlen(buff) + 1);
		strcpy(t, buff);

		d->_return.value_str_ptr = get_pptr_string(t);
	}
	else if (d->func_parmeters->type_define == T_INT)
	{
		int* u = d->func_parmeters->value_int;
		buff = (char*)malloc(sizeof(int) * 4 + 1);
		sprintf(buff, "%d", *u);
		char* t = (char*)malloc(strlen(buff) + 1);
		strcpy(t, buff);

		d->_return.value_str_ptr = get_pptr_string(t);
	}
	else if (d->func_parmeters->type_define == T_CHAR)
	{
		int size = (sizeof(char) * d->func_parmeters->size) + 1;
		char* ubuff = (char*)calloc(1, size);

		strcpy(ubuff, d->func_parmeters->value_char_ptr);

		d->_return.value_str_ptr = get_pptr_string(ubuff);
	}
	else
	{
		printf("Error: input is not string");
	}
	if (buff)free(buff);
}


void install_default_functions()
{
	funcs = &base_function;
}

void add_type(char* name)
{
	type_def* mt = new_type();
	mt->type_name = name;
}

int* new_int(int count, int value)
{
	int* re = (int*)gc_calloc(count, sizeof(int), GC_KIND_RAW);
	*re = value;
	return re;
}

void xeql(fcall* y)
{
	var *a, *b = NULL;

	if (y->context != NULL)
	{
		b = y->func_parmeters;
		a = y->context;
	}
	else
	{
		b = y->func_parmeters + 1;
		a = y->func_parmeters;
	}

	if (a->type_define != b->type_define)
	{
		y->_return.value_int = new_int(1, 0);
	}
	else
	{
		if (a->type_define == T_STRING)
		{
			y->_return.value_int = new_int(1, strcmp(*a->value_str_ptr, *b->value_str_ptr) == 0);
		}
		else
		{
			y->_return.value_int = new_int(1, *a->value_int == *b->value_int);
		}
	}
}

void len(fcall* y)
{
	if (y->context != NULL)
	{
		if (y->context->type_define == T_STRING)
		{
			if (y->context->size == 1)
				y->_return.value_int = new_int(1, strlen(*y->context->value_str_ptr));
			else
				y->_return.value_int = new_int(1, y->context->size);
		}
		if (y->context->type_define->base == T_ARRAY)
		{
			y->_return.value_int = (int*)new_int(1, y->context->value_type_instsance->size);
		}
	}
	else
	{
		if (y->parm_count_c == 1)
		{
			if (y->func_parmeters->size > 1)
			{
				y->_return.value_int = new_int(1, y->func_parmeters->size);
			}
			else if (y->func_parmeters->type_define == T_STRING)
			{
				y->_return.value_int = new_int(1, strlen(*y->func_parmeters->value_str_ptr));
			}
		}
	}
}


void xreplace(fcall* y)
{
	var* p1;
	char *orig = NULL, *rep = NULL, *with = NULL;
	if (y->context != NULL)
	{
		p1 = y->context;
		orig = *y->context->value_str_ptr;
		if (y->func_parmeters->type_define == T_STRING)
		{
			rep = *y->func_parmeters->value_str_ptr;
		}
		else if (y->func_parmeters->type_define == T_CHAR)
		{
			rep = y->func_parmeters->value_char_ptr;
		}

		if ((y->func_parmeters + 1)->type_define == T_STRING)
		{
			with = *(y->func_parmeters + 1)->value_str_ptr;
		}
		else if ((y->func_parmeters + 1)->type_define == T_CHAR)
		{
			with = (y->func_parmeters + 1)->value_char_ptr;
		}
	}
	else
	{
		p1 = y->func_parmeters + 0;
		orig = *y->func_parmeters->value_str_ptr;
		if ((y->func_parmeters + 1)->type_define == T_STRING)
		{
			rep = *(y->func_parmeters + 1)->value_str_ptr;
		}
		else if ((y->func_parmeters + 1)->type_define == T_CHAR)
		{
			rep = (y->func_parmeters + 1)->value_char_ptr;
		}

		if ((y->func_parmeters + 2)->type_define == T_STRING)
		{
			with = *(y->func_parmeters + 2)->value_str_ptr;
		}
		else if ((y->func_parmeters + 2)->type_define == T_CHAR)
		{
			with = (y->func_parmeters + 2)->value_char_ptr;
		}
	}

	char* result; // the return string
	char* ins; // the next insert point
	char* tmp; // varies
	int len_rep; // length of rep (the string to remove)
	int len_with; // length of with (the string to replace rep with)
	int len_front; // distance between rep and end of last rep
	int count; // number of replacements

	// sanity checks and initialization
	if (!orig || !rep)
	{
		y->_return.value_str_ptr = p1->value_str_ptr;
		return;
	}

	len_rep = strlen(rep);
	if (len_rep == 0)
	{
		y->_return.value_str_ptr = p1->value_str_ptr; // empty rep causes infinite loop during count
		return;
	}

	if (!with)
		with = "";
	len_with = strlen(with);

	// count the number of replacements needed
	ins = orig;
	for (count = 0; (tmp = strstr(ins, rep)); ++count)
	{
		ins = tmp + len_rep;
	}

	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
	{
		y->_return.value_str_ptr = p1->value_str_ptr;
		return;
	}

	// first time through the loop, all the variable are set correctly
	// from here on,
	//    tmp points to the end of the result string
	//    ins points to the next occurrence of rep in orig
	//    orig points to the remainder of orig after "end of rep"
	while (count--)
	{
		ins = strstr(orig, rep);
		len_front = ins - orig;
		tmp = strncpy(tmp, orig, len_front) + len_front;
		tmp = strcpy(tmp, with) + len_with;
		orig += len_front + len_rep; // move to next "end of rep"
	}
	strcpy(tmp, orig);
	y->_return.value_str_ptr = get_pptr_string(result);
	y->_return.size=1;
	return;
}


void install_default_types()
{
	types = &simple_type_stack;
}

func_deftion* new_func()
{
	return new_func_on_stack(funcs);
}

type_def* get_type_by_name(char* name)
{
	if (name == NULL || types == NULL) return NULL;
	type_stack* vs = types;

	for (type_def* i = vs->root; i != NULL; i = i->stack_next)
	{
		if (i->type_name != NULL && strcmp(name, i->type_name) == 0)
			return i;
	}
	return NULL;
}

func_deftion* get_func_by_name(char* name)
{
	for (func_deftion* i = funcs->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->func_name) == 0)
		{
			return i;
		}
	}
	return NULL;
}


func_deftion* get_obj_function(var* object_var, char* name)
{
	if (NULL == object_var || NULL == name || NULL == object_var->type_define)
		return NULL;

	type_def* t = object_var->type_define;
	if (is_base_type(t))
	{
		for (int i = 0; i < t->d_function_size; i++)
		{
			if (t->d_functions[i].func_name != NULL && strcmp(name, t->d_functions[i].func_name) == 0)
				return &t->d_functions[i];
		}
		for (func_deftion* i = t->d_functions; i != NULL; i = i->stack_next)
		{
			if (i->func_name != NULL && strcmp(name, i->func_name) == 0)
				return i;
		}
		return NULL;
	}

	for (type_def* curr = t; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_function_size; i++)
		{
			if (curr->d_functions[i].func_name != NULL && strcmp(name, curr->d_functions[i].func_name) == 0)
				return &curr->d_functions[i];
		}
	}
	if (object_var->value_type_instsance != NULL)
	{
		for (func_deftion* i = object_var->value_type_instsance->functions.root; i != NULL; i = i->stack_next)
		{
			if (i->func_name != NULL && strcmp(name, i->func_name) == 0)
				return i;
		}
	}
	return NULL;
}

func_deftion* get_class_function(type_def* t, const char* name)
{
	if (t == NULL || name == NULL) return NULL;
	for (type_def* curr = t; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_function_size; i++)
		{
			if (curr->d_functions[i].func_name != NULL && strcmp(name, curr->d_functions[i].func_name) == 0)
				return &curr->d_functions[i];
		}
	}
	return NULL;
}

var* get_class_property(type_def* t, const char* name)
{
	if (t == NULL || name == NULL) return NULL;
	for (type_def* curr = t; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_propertys_size; i++)
		{
			if (curr->d_propertys[i].name != NULL && strcmp(name, curr->d_propertys[i].name) == 0)
				return &curr->d_propertys[i];
		}
	}
	return NULL;
}

type_def* get_class_of_function(func_deftion* fd)
{
	if (fd == NULL || types == NULL) return NULL;
	for (type_def* t = types->root; t != NULL; t = t->stack_next)
	{
		for (int i = 0; i < t->d_function_size; i++)
		{
			if (&t->d_functions[i] == fd)
				return t;
		}
	}
	return NULL;
}

var* get_global_var_by_name(char* name)
{
	if (name == NULL || varss == NULL)
		return NULL;
	for (var* i = varss->root; i != NULL; i = i->stack_next)
	{
		if (i->name != NULL && strcmp(name, i->name) == 0)
			return i;
	}
	return NULL;
}

var* get_globle_var_by_name(char* name)
{
	return get_global_var_by_name(name);
}

var* get_function_var_by_name(char* name, fcall* func_call)
{
	if (name == NULL || func_call == NULL)
		return NULL;
	for (int i = 0; i < func_call->parm_count_c; i++)
	{
		if (func_call->func_parmeters[i].name != NULL && strcmp(name, func_call->func_parmeters[i].name) == 0)
			return &func_call->func_parmeters[i];
	}
	return NULL;
}

var* fget_var_by_name_fc(char* name, fcall* y)
{
	return get_function_var_by_name(name, y);
}

int type_def_compute_field_offsets(type_def* td)
{
	if (td == NULL) return 0;
	int base_count = 0;
	if (td->base != NULL && td->base != td)
	{
		base_count = type_def_compute_field_offsets(td->base);
	}
	for (int i = 0; i < td->d_propertys_size; i++)
	{
		td->d_propertys[i].slot_idx = base_count + i;
		td->field_descriptors[i].slot_idx = base_count + i;
		td->field_descriptors[i].name = td->d_propertys[i].name;
		td->field_descriptors[i].type_define = td->d_propertys[i].type_define;
		td->field_descriptors[i].access = td->d_propertys[i].access;
	}
	td->total_field_count = base_count + td->d_propertys_size;
	return td->total_field_count;
}

int type_def_find_field_slot(const type_def* td, const char* name)
{
	if (td == NULL || name == NULL) return -1;
	for (const type_def* curr = td; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_propertys_size; i++)
		{
			if (curr->field_descriptors[i].name != NULL && strcmp(curr->field_descriptors[i].name, name) == 0)
			{
				return curr->field_descriptors[i].slot_idx;
			}
			if (curr->d_propertys[i].name != NULL && strcmp(curr->d_propertys[i].name, name) == 0)
			{
				return curr->d_propertys[i].slot_idx;
			}
		}
		if (curr->base == curr) break;
	}
	return -1;
}

void define_class_property(const char* name, type_def* container_class, type_def* prop_type, var** out_var)
{
	if (container_class != NULL)
	{
		for (int i = 0; i < container_class->d_propertys_size; i++)
		{
			if (container_class->d_propertys[i].name != NULL && strcmp(container_class->d_propertys[i].name, name) == 0)
			{
				if (out_var) *out_var = &container_class->d_propertys[i];
				return;
			}
		}

		int psize = container_class->d_propertys_size;
		var* ivar = &container_class->d_propertys[psize];
		ivar->type_define = prop_type;
		ivar->name = (char*)name;
		ivar->slot_idx = psize;
		ivar->access = PUBLIC;

		FieldDescriptor* fd = &container_class->field_descriptors[psize];
		fd->name = (char*)name;
		fd->type_define = prop_type;
		fd->slot_idx = psize;
		fd->access = PUBLIC;

		container_class->d_propertys_size++;
		if (out_var) *out_var = ivar;
	}
}

void define_new_class_prop(char* name, type_def* contern_class, type_def* new_var_type, var** out_var)
{
	define_class_property(name, contern_class, new_var_type, out_var);
}

FieldDescriptor* type_def_get_field_descriptor(const type_def* td, const char* name)
{
	if (td == NULL || name == NULL) return NULL;
	for (const type_def* curr = td; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_propertys_size; i++)
		{
			if (curr->field_descriptors[i].name != NULL && strcmp(curr->field_descriptors[i].name, name) == 0)
			{
				return (FieldDescriptor*)&curr->field_descriptors[i];
			}
			if (curr->d_propertys[i].name != NULL && strcmp(curr->d_propertys[i].name, name) == 0)
			{
				return (FieldDescriptor*)&curr->field_descriptors[i];
			}
		}
		if (curr->base == curr) break;
	}
	return NULL;
}

FieldDescriptor* type_def_get_field_descriptor_by_slot(const type_def* td, int slot)
{
	if (td == NULL || slot < 0) return NULL;
	for (const type_def* curr = td; curr != NULL; curr = curr->base)
	{
		for (int i = 0; i < curr->d_propertys_size; i++)
		{
			if (curr->field_descriptors[i].slot_idx == slot)
			{
				return (FieldDescriptor*)&curr->field_descriptors[i];
			}
		}
		if (curr->base == curr) break;
	}
	return NULL;
}

static void init_instance_prop_obj(var* inctance_prop);

type_instance* type_instance_create(type_def* td)
{
	if (td == NULL) return NULL;
	if (td->total_field_count == 0 && td->d_propertys_size > 0)
	{
		type_def_compute_field_offsets(td);
	}
	uint32_t fcount = (uint32_t)td->total_field_count;
	size_t alloc_size = sizeof(type_instance) + fcount * sizeof(var);
	type_instance* inst = (type_instance*)gc_calloc(1, alloc_size, GC_KIND_INSTANCE);
	inst->type = td;
	inst->field_count = fcount;
	inst->size = 1;
	inst->id = 0;

	for (type_def* cur = td; cur != NULL; cur = cur->base)
	{
		for (int i = 0; i < cur->d_propertys_size; i++)
		{
			var* proto = &cur->d_propertys[i];
			int slot = proto->slot_idx;
			if (slot >= 0 && (uint32_t)slot < fcount)
			{
				var* f = &inst->fields[slot];
				f->name = proto->name;
				f->type_define = proto->type_define;
				f->access = proto->access;
				f->holder = inst;
				f->slot_idx = slot;
				f->size = 1;
				if (proto->access != STATIC)
				{
					if (proto->type_define != NULL)
					{
						f->values = install_memory(f);
						if (is_base_type(proto->type_define))
						{
							if (proto->values != NULL)
							{
								set_value_copy_var(f, proto);
							}
						}
						else
						{
							f->value_type_instsance = (type_instance*)f->values;
							init_instance_prop_obj(f);
						}
					}
				}
				else
				{
					f->values = proto->values;
				}
			}
		}
		if (cur->base == cur) break;
	}

	return inst;
}

extern type_instance xlnag_object;

var* type_instance_get_field(type_instance* inst, const char* name)
{
	if (inst == NULL || (!gc_is_managed(inst) && inst != &xlnag_object) || inst->type == NULL || name == NULL) return NULL;
	int slot = type_def_find_field_slot(inst->type, name);
	if (slot >= 0 && (uint32_t)slot < inst->field_count)
	{
		return &inst->fields[slot];
	}
	return NULL;
}

var* type_instance_get_field_by_slot(type_instance* inst, int slot)
{
	if (inst == NULL || (!gc_is_managed(inst) && inst != &xlnag_object) || slot < 0 || (uint32_t)slot >= inst->field_count) return NULL;
	return &inst->fields[slot];
}

int type_instance_get_id(type_instance* inst)
{
	if (inst == NULL || (!gc_is_managed(inst) && inst != &xlnag_object)) return -1;
	if (inst->id > 0) return inst->id;
	var* f = type_instance_get_field(inst, "id");
	if (f != NULL && f->value_int != NULL) return *f->value_int;
	return -1;
}

var* find_var_in_scope(char* name, fcall* called_function, var* called_var)
{
	var* ret;
	if (called_function != NULL)
	{
		ret = get_function_var_by_name(name, called_function);
		if (ret != NULL) return ret;
	}

	if (called_var != NULL)
	{
		if (strcmp(name, "this") == 0) return called_var;
		if (called_var->value_type_instsance != NULL)
		{
			ret = type_instance_get_field(called_var->value_type_instsance, name);
			if (ret != NULL) return ret;
		}
	}

	return get_global_var_by_name(name);
}

var* all_get_var_by_name(char* name, fcall* called_function, var* called_var)
{
	return find_var_in_scope(name, called_function, called_var);
}


bool is_base_type(type_def* t)
{
	if (t == NULL)
		return false;
	return t >= SIMPLE_TYPE && t < (SIMPLE_TYPE + 11);
}

void* install_memory(var* target_var)
{
	if (target_var == NULL) return NULL;
	int count = target_var->size > 0 ? target_var->size : 1;
	target_var->size = count;
	if (count == 1)
	{
		if (target_var->type_define == T_INT ||
		    target_var->type_define == T_LONG ||
		    target_var->type_define == T_FLOAT ||
		    target_var->type_define == T_BOOL)
		{
			target_var->inline_val.raw_primitive = 0;
			target_var->values = &target_var->inline_val;
			return target_var->values;
		}
	}
	target_var->values = install_memory_with_type(target_var->type_define, target_var->size);
	return target_var->values;
}

static bool call_function(fcall* mfunc, var** context)
{
	if (mfunc == NULL || mfunc->deftion == NULL) return false;
	if (mfunc->deftion->function_type == constr && (*context == NULL || (*context)->values == NULL))
	{
		mfunc->_return.size = 1;
		mfunc->_return.values = install_memory(&mfunc->_return);
		*context = &mfunc->_return;
	}
	if (*context != NULL && (*context)->value_type_instsance == NULL && (*context)->values != NULL && !is_base_type((*context)->type_define))
	{
		(*context)->value_type_instsance = (type_instance*)(*context)->values;
	}
	mfunc->context = *context;
	mfunc->has_returned = false;

	if (mfunc->deftion->func_code != NULL)
		mfunc->deftion->func_code(mfunc);
	gc_pop_frame();

	return false;
}

static void init_instance_prop_obj(var* instance_prop)
{
	if (instance_prop == NULL || instance_prop->type_define == NULL || is_base_type(instance_prop->type_define))
		return;

	instance_prop->value_type_instsance = (type_instance*)instance_prop->values;

	type_def* pt = instance_prop->type_define;
	for (int i = 0; i < pt->d_function_size; i++)
	{
		if (pt->d_functions[i].func_name != NULL &&
			(strcmp(pt->d_functions[i].func_name, pt->type_name) == 0 ||
			 pt->d_functions[i].function_type == constr))
		{
			if (pt->d_functions[i].start_parm_count == 0)
			{
				fcall* cfc = create_fcall(&pt->d_functions[i]);
				call_function(cfc, &instance_prop);
				gc_free_any(cfc);
				break;
			}
		}
	}
}

void instance_type(type_def* type_prototype, void* dest_array, int size)
{
	if (type_prototype == NULL || dest_array == NULL) return;
	for (int i = 0; i < size; i++)
	{
		type_instance* inst = type_instance_create(type_prototype);
		if (inst != NULL)
		{
			((type_instance**)dest_array)[i] = inst;
		}
	}
}


void* install_memory_with_type(type_def* var_type, const int count)
{
	void* r = NULL;
	if (var_type == NULL) return r;
	const int element_count = count > 0 ? count : 1;
	if (T_INT == var_type)
	{
		r = (int*)gc_calloc(element_count, sizeof(int), GC_KIND_RAW);
	}
	else if (T_LONG == var_type)
		r = (long*)gc_calloc(element_count, sizeof(long), GC_KIND_RAW);
	else if (T_CHAR == var_type)
	{
		r = (char*)gc_calloc(element_count + 1, sizeof(char), GC_KIND_RAW);
	}
	else if (T_FLOAT == var_type)
		r = (float*)gc_calloc(element_count, sizeof(float), GC_KIND_RAW);
	else if (T_STRING == var_type)
		r = (char**)gc_calloc(element_count, sizeof(char*), GC_KIND_RAW);
	else if (T_BOOL == var_type)
		r = (bool*)gc_calloc(element_count, sizeof(bool), GC_KIND_RAW);

	else if (!is_base_type(var_type))
	{
		r = type_instance_create(var_type);
	}

	return r;
}


void setup_t(var* out, type_def** saved_return_type, bool indexx, var* mvar)
{
	if (out->type_define == NULL && mvar->type_define != NULL)
	{
		out->type_define = mvar->type_define;
		if (!indexx) //TEST
		{
			out->size = mvar->size;
		}
	}
	else if (out->type_define != NULL)
	{
		if (mvar->type_define != out->type_define)
		{
			*saved_return_type = out->type_define;
			out->type_define = mvar->type_define;
		}
	}
	if (indexx && mvar->size == 1 && mvar->type_define == T_STRING)
	{
		out->type_define = T_CHAR;
	}
}

bool copy_array(var* out, void* out_memory, var* src)
{
	if (out->size != src->size)
	{
		printf("ERROR: size mismatch  %s[%d]\n", src->name, src->size);
		return true;
	}
	else if (out->type_define == T_CHAR)
	{
		for (int i = 0; i < src->size; i++)
		{
			((char*)out_memory)[i] = src->value_char_ptr[i];
		}
	}
	else if (out->type_define == T_STRING)
	{
		int u = 0;
		char** g = src->value_str_ptr;
		while (u < src->size)
		{
			((char**)out_memory)[u] = (char*)malloc(strlen(*g) + 1);
			strcpy(((char**)out_memory)[u], *g);
			u++;
			g++;
		}
	}
	else if (out->type_define == T_INT)
	{
		int* g = src->value_int;
		memcpy(out_memory, g, sizeof(int) * src->size);
	}
	else if (!is_base_type(out->type_define))
	{
		out->value_type_instsance = src->value_type_instsance;
		out->values = src->values;
	}
	return false;
}




char* get_file_buffer(FILE* sf)
{
	fseek(sf, 0, SEEK_SET);
	fseek(sf, 0, SEEK_END);
	long size = ftell(sf);
	fseek(sf, 0, SEEK_SET);
	char* buff = (char*)malloc(sizeof(char) * (size + 1));
	memset(buff, 0, size + 1);
	fread(buff, size, 1, sf);
	return buff;
}

// memory must be allocated before call
void set_value_copy_var(var* dest, var* src)
{
	if (dest == NULL || src == NULL) return;
	if (T_INT == dest->type_define)
		*dest->value_int = *src->value_int;
	else if (T_STRING == dest->type_define)
	{
		const char* s = (src->value_str_ptr != NULL && *src->value_str_ptr != NULL) ? *src->value_str_ptr : "";
		*dest->value_str_ptr = (char*)malloc(strlen(s) + 1);
		strcpy(*dest->value_str_ptr, s);
	}
	else if (T_CHAR == dest->type_define)
	{
		*dest->value_char_ptr = *src->value_char_ptr;
	}
	else if (T_LONG == dest->type_define)
	{
		*dest->value_long = *src->value_long;
	}
	else if (T_FLOAT == dest->type_define)
	{
		*dest->value_float = *src->value_float;
	}
	else if (T_BOOL == dest->type_define)
	{
		*dest->value_bool = *src->value_bool;
	}
	else
	{
		if (dest->value_type_instsance != NULL && src->value_type_instsance != NULL)
			*dest->value_type_instsance = *src->value_type_instsance;
	}
}



//type_def parms[] = {T_STRING, T_INT};

func_deftion simple_function_array[] = {
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "print", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 1
	},
	{
		//vprint
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "vprint", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 2
	},
	{
		.start_func_parmeters = {0}, .func_code = &time_x, .func_name = "time", .function_type = f_main,
		.return_type = T_STRING,
		.stack_next = simple_function_array + 3
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &scan, .func_name = "scan", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 4
	},
	{
		.start_func_parmeters = {T_INT}, .start_parm_count = 1, .func_code = &random_, .func_name = "random",
		.function_type = f_main, .return_type = T_INT,
		.stack_next = simple_function_array + 5
	},
	{
		.start_func_parmeters = {T_INT}, .start_parm_count = 1, .func_code = &str, .func_name = "str",
		.function_type = f_main,
		.return_type = T_STRING,
		.stack_next = simple_function_array + 6
	},
	{
		.start_func_parmeters = {T_STRING}, .start_parm_count = 1, .func_code = &import, .func_name = "import",
		.function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 7
	},
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "xxxx", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 8
	},
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "pxcrint", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 9
	},
	{
		.start_func_parmeters = {0}, .func_code = &_exit_, .func_name = "exit", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = simple_function_array + 10
	},
	{
		.start_func_parmeters = {0}, .func_code = &len, .func_name = "len", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.stack_next = simple_function_array + 11
	},
	{
		.start_func_parmeters = {0}, .func_code = &_echo, .func_name = "echo", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.stack_next = simple_function_array + 12
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &eval, .func_name = "eval", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.stack_next = simple_function_array + 13
	},
	{
		.start_func_parmeters = {T_ANY,T_ANY}, .func_code = &xeql, .func_name = "eql", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 2,
		.stack_next = simple_function_array + 14
	},
	{
		.start_func_parmeters = {T_ANY,T_ANY, T_ANY}, .func_code = &xreplace, .func_name = "replace",
		.function_type = f_main,
		.return_type = T_STRING,
		.start_parm_count = 3,
		.stack_next = simple_function_array + 15
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_socket_create, .func_name = "socket_create",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 16
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_socket_create, .func_name = "socket",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 17
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_connect, .func_name = "socket_connect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 18
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_connect, .func_name = "connect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 19
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_bind, .func_name = "socket_bind",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 20
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_bind, .func_name = "bind",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 21
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_listen, .func_name = "socket_listen",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 22
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_listen, .func_name = "listen",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 23
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_accept, .func_name = "socket_accept",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 24
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_accept, .func_name = "accept",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 25
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_socket_send, .func_name = "socket_send",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 26
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_socket_send, .func_name = "send",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 27
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recv, .func_name = "socket_recv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 28
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recv, .func_name = "recv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 29
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_close, .func_name = "socket_close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 30
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_close, .func_name = "close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 31
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_set_timeout, .func_name = "socket_set_timeout",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 32
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_set_reuseaddr, .func_name = "socket_set_reuseaddr",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 33
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_STRING, T_INT}, .func_code = &x_socket_sendto, .func_name = "socket_sendto",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 4, .stack_next = simple_function_array + 34
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recvfrom, .func_name = "socket_recvfrom",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 35
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_http_get, .func_name = "http_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 36
	},
	/* File I/O (36 - 48) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_read_all, .func_name = "file_read_all",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 37
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_read_all, .func_name = "read_file",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 38
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_write_all, .func_name = "file_write_all",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 39
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_write_all, .func_name = "write_file",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 40
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_append, .func_name = "file_append",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 41
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_exists, .func_name = "file_exists",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 42
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_remove, .func_name = "file_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 43
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_remove, .func_name = "file_delete",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 44
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_size, .func_name = "file_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 45
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_open, .func_name = "file_open",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 46
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_file_read, .func_name = "file_read",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 47
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_file_write, .func_name = "file_write",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 48
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_file_close, .func_name = "file_close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 49
	},
	/* CLI & System (49 - 56) */
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_get_arg, .func_name = "get_arg",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 50
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_get_argc, .func_name = "get_argc",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 51
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_exec, .func_name = "system_exec",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 52
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_exec, .func_name = "exec",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 53
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_getenv, .func_name = "system_getenv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 54
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_getenv, .func_name = "getenv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 55
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_system_setenv, .func_name = "system_setenv",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 56
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_system_setenv, .func_name = "setenv",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 57
	},
	/* Regex & String Utilities (57 - 67) */
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_match, .func_name = "regex_match",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 58
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_find, .func_name = "regex_find",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 59
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING, T_STRING}, .func_code = &x_regex_replace, .func_name = "regex_replace",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 3, .stack_next = simple_function_array + 60
	},
	{
		.start_func_parmeters = {T_STRING, T_INT, T_INT}, .func_code = &x_substr, .func_name = "substr",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 3, .stack_next = simple_function_array + 61
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_index_of, .func_name = "index_of",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 62
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_index_of, .func_name = "find",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 63
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_trim, .func_name = "trim",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 64
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_to_lower, .func_name = "to_lower",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 65
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_to_upper, .func_name = "to_upper",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 66
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_starts_with, .func_name = "starts_with",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 67
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_ends_with, .func_name = "ends_with",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 68
	},
	/* Math Library (68 - 91) */
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sqrt, .func_name = "math_sqrt",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 69
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sqrt, .func_name = "sqrt",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 70
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_pow, .func_name = "math_pow",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .stack_next = simple_function_array + 71
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_pow, .func_name = "pow",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .stack_next = simple_function_array + 72
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_abs, .func_name = "math_abs",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 73
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_abs, .func_name = "abs",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 74
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_min, .func_name = "math_min",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 75
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_min, .func_name = "min",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 76
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_max, .func_name = "math_max",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 77
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_max, .func_name = "max",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 78
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_floor, .func_name = "math_floor",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 79
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_floor, .func_name = "floor",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 80
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_ceil, .func_name = "math_ceil",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 81
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_ceil, .func_name = "ceil",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 82
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_round, .func_name = "math_round",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 83
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_round, .func_name = "round",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 84
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sin, .func_name = "math_sin",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 85
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sin, .func_name = "sin",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 86
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_cos, .func_name = "math_cos",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 87
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_cos, .func_name = "cos",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 88
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_tan, .func_name = "math_tan",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 89
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_tan, .func_name = "tan",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 90
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_log, .func_name = "math_log",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 91
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_log, .func_name = "log",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .stack_next = simple_function_array + 92
	},
	/* Dynamic List (92 - 112) */
	{
		.start_func_parmeters = {0}, .func_code = &x_list_create, .func_name = "list_new",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 93
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_free, .func_name = "list_free",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 94
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_size, .func_name = "list_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 95
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_add, .func_name = "list_add",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 96
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_add_int, .func_name = "list_add_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 97
	},
	{
		.start_func_parmeters = {T_INT, T_ANY}, .func_code = &x_list_add_float, .func_name = "list_add_float",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 98
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get, .func_name = "list_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 99
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get_int, .func_name = "list_get_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 100
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get_float, .func_name = "list_get_float",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .stack_next = simple_function_array + 101
	},
	{
		.start_func_parmeters = {T_INT, T_INT, T_STRING}, .func_code = &x_list_set, .func_name = "list_set",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 102
	},
	{
		.start_func_parmeters = {T_INT, T_INT, T_INT}, .func_code = &x_list_set_int, .func_name = "list_set_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 103
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_remove_at, .func_name = "list_remove_at",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 104
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_clear, .func_name = "list_clear",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 105
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_contains, .func_name = "list_contains",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 106
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_contains_int, .func_name = "list_contains_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 107
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_index_of, .func_name = "list_index_of",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 108
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_index_of_int, .func_name = "list_index_of_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 109
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_pop, .func_name = "list_pop",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 110
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_pop_int, .func_name = "list_pop_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 111
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_join, .func_name = "list_join",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 112
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_to_string, .func_name = "list_to_string",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 113
	},
	/* Hash Map (113 - 128) */
	{
		.start_func_parmeters = {0}, .func_code = &x_map_create, .func_name = "map_new",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 114
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_free, .func_name = "map_free",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 115
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_size, .func_name = "map_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 116
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_STRING}, .func_code = &x_map_put, .func_name = "map_put",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 117
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_map_put_int, .func_name = "map_put_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 118
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_ANY}, .func_code = &x_map_put_float, .func_name = "map_put_float",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .stack_next = simple_function_array + 119
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get, .func_name = "map_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 120
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get_int, .func_name = "map_get_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 121
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get_float, .func_name = "map_get_float",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .stack_next = simple_function_array + 122
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_has, .func_name = "map_has",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 123
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_remove, .func_name = "map_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 124
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_clear, .func_name = "map_clear",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 125
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_keys, .func_name = "map_keys",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 126
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_values, .func_name = "map_values",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 127
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_to_string, .func_name = "map_to_string",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 128
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_keys_list, .func_name = "map_keys_list",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 129
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_collect, .func_name = "gc_collect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 130
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_allocated_bytes, .func_name = "gc_allocated_bytes",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 131
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_total_objects, .func_name = "gc_total_objects",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 132
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_enable, .func_name = "gc_enable",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 133
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_disable, .func_name = "gc_disable",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 134
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_gc_set_threshold, .func_name = "gc_set_threshold",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 135
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_dump, .func_name = "gc_dump",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 136
	},
	{
		.start_func_parmeters = {}, .func_code = &x_clock_ms, .func_name = "clock_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 137
	},
	{
		.start_func_parmeters = {}, .func_code = &x_clock_ms, .func_name = "time_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 138
	},
	/* String Ergonomics (138) */
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_string_split, .func_name = "str_split",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .stack_next = simple_function_array + 139
	},
	/* JSON Module (139 - 141) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_json_parse, .func_name = "json_parse",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .stack_next = simple_function_array + 140
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_json_stringify, .func_name = "json_stringify",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 141
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_json_is_valid, .func_name = "json_is_valid",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 142
	},
	/* DateTime Module (142 - 150) */
	{
		.start_func_parmeters = {}, .func_code = &x_datetime_now, .func_name = "datetime_now",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 143
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_datetime_format, .func_name = "datetime_format",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .stack_next = simple_function_array + 144
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_year, .func_name = "datetime_year",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 145
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_month, .func_name = "datetime_month",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 146
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_day, .func_name = "datetime_day",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 147
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_hour, .func_name = "datetime_hour",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 148
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_minute, .func_name = "datetime_minute",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 149
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_second, .func_name = "datetime_second",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 150
	},
	{
		.start_func_parmeters = {}, .func_code = &x_datetime_clock_ms, .func_name = "datetime_clock_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .stack_next = simple_function_array + 151
	},
	/* Directory Operations (151 - 154) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_list, .func_name = "dir_list",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .stack_next = simple_function_array + 152
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_create, .func_name = "dir_create",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 153
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_exists, .func_name = "dir_exists",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 154
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_remove, .func_name = "dir_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .stack_next = simple_function_array + 155
	},
	/* Process Operations (155 - 156) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_proc_capture, .func_name = "proc_capture",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .stack_next = simple_function_array + 156
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_proc_run, .func_name = "proc_run",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .stack_next = simple_function_array + 157
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .start_parm_count = 1, .func_code = &xassert, .func_name = "assert", .function_type = f_main,
		.return_type = T_INT,
		.stack_next = 0
	},
};
func_stack base_function = {.top = simple_function_array + (SIMPLE_FUNC_COUNT - 1), .size = SIMPLE_FUNC_COUNT, .root = simple_function_array + 0};

var tinfo[6] = {
	{.name = "_int", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 1},
	{.name = "_string", .type_define = T_TYPE_INFO, .values = T_STRING, .size = 1, .stack_next = tinfo + 2},
	{.name = "_char", .type_define = T_TYPE_INFO, .values = T_CHAR, .size = 1, .stack_next = tinfo + 3},
	{.name = "_bool", .type_define = T_TYPE_INFO, .values = T_BOOL, .size = 1, .stack_next = tinfo + 4},
	{.name = "_long", .type_define = T_TYPE_INFO, .values = T_LONG, .size = 1, .stack_next = tinfo + 5},
	{.name = "_object", .type_define = T_TYPE_INFO, .values = T_OBJECT, .size = 1, .stack_next = 0}
};


type_instance xlnag_object = {
	.functions = {
		.top = simple_function_array + 12, .size = 13, .root = simple_function_array + 0
	},
	.size = 1, .type = T_OBJECT,
	.id = 0,
	.field_count = 0,
};
var start_var = {
	.name = "xlang", .type_define = T_OBJECT, .value_type_instsance = &xlnag_object,
	.size = 1, .holder = NULL, .base_type = 0, .stack_next = tinfo, .access = PUBLIC
};
var_stack var_start_stack = {
	.top = tinfo + 5, .root = &start_var, .size = 7
};

fcall* create_fcall(func_deftion* fd)
{
	if (fd == NULL) return NULL;
	fcall* function_c = (fcall*)gc_calloc(1, sizeof(fcall), GC_KIND_FCALL);
	function_c->deftion = fd;
	for (int i = 0; i < fd->start_parm_count; i++)
	{
		function_c->func_parmeters[i].name = fd->start_func_parmeters_name[i];
		function_c->func_parmeters[i].type_define = fd->start_func_parmeters[i];
		function_c->func_parmeters[i].size = 1;
	}
	function_c->parm_count_c = fd->start_parm_count;
	function_c->_return.type_define = fd->return_type;
	function_c->_return.size = 1;
	gc_push_frame(function_c);
	return function_c;
}


var* get_array_item(var* name, int index)
{
	if (name == NULL || name->type_define == NULL || index < 0)
		return NULL;

	if (name->type_define->type_name != NULL && strcmp(name->type_define->type_name, "List") == 0)
	{
		int list_id = type_instance_get_id(name->value_type_instsance);
		if (list_id > 0)
		{
			return x_list_get_var(list_id, index);
		}
		return NULL;
	}

	if (name->values == NULL)
		return NULL;

	var* ret = NULL;
	switch (name->type_define->type_id)
	{
	case t_long:
		if (index >= name->size) return NULL;
		ret = new_temp_var(T_LONG);
		ret->value_long = name->value_long + index;
		break;
	case t_string:
		{
			if (name->size > 1)
			{
				if (index >= name->size) return NULL;
				ret = new_temp_var(T_STRING);
				ret->value_str_ptr = name->value_str_ptr + index;
			}
			else
			{
				if (*name->value_str_ptr == NULL || (size_t)index >= strlen(*name->value_str_ptr)) return NULL;
				ret = new_temp_var(T_CHAR);
				ret->value_char_ptr = *name->value_str_ptr + index;
			}
			break;
		}
	case t_char:
		if (index >= name->size) return NULL;
		ret = new_temp_var(T_CHAR);
		ret->value_char_ptr = name->value_char_ptr + index;
		break;
	case t_int:
		if (index >= name->size) return NULL;
		ret = new_temp_var(T_INT);
		ret->value_int = name->value_int + index;
		break;
	case t_bool:
		if (index >= name->size) return NULL;
		ret = new_temp_var(T_BOOL);
		ret->value_bool = name->value_bool + index;
		break;
	case t_float:
		if (index >= name->size) return NULL;
		ret = new_temp_var(T_FLOAT);
		ret->value_float = name->value_float + index;
		break;
	case t_array:
		break;
	default:
		printf("error: unknown type in get_array_item\n");
	}

	return ret;
}


func_deftion* get_obj_function2(var* object_var, char* name)
{
	return get_obj_function(object_var, name);
}



