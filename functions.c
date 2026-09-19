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
#include "compile.h"
#include "parse.h"

//#define F

void step(node** nod)
{
	if (nod != NULL && *nod != NULL && (*nod)->type_ != endl)
		*nod = (*nod)->next;
}

bool eat(node** nod, enum node_type_enum next, bool must)
{
	if (nod == NULL || *nod == NULL || (*nod)->next == NULL)
	{
		if (must)
		{
			printf("Error : missing in line %d - c-%s:%s:%d\n", *nod ? (*nod)->line : 0, __FILE__, __func__, __LINE__);
			exit(1);
		}
		return false;
	}
	node* n = (*nod)->next;

	if (must && n->type_ != next)
	{
		printf("Error : missing in line %d - c-%s:%s:%d\n", (*nod)->line, __FILE__, __func__, __LINE__);
		exit(1);
	}

	if (n->type_ == next)
	{
		*nod = n;
		return true;
	}
	return false;
}

void len(fcall* y);
//void index_(func_deftion* y);
//var t = {.type_define = T_INT, .name = "a"};
///func_deftion xd;

//func_deftion xd = {
//	.start_func_parmeters = {T_INT},.func_code = &index_,.func_name = "index",.function_type = f_main,
//	.return_type = T_INT,.ref = 0,/* func* STACK_NEXT*/0
//};
type_stack simple_type_stack = {.top = T_FUNC, .root = T_LONG, .size = 10};

void int_add(fcall* fcall)
{
	if (fcall->func_parmeters[0].type_define == T_INT)
	{
		fcall->_return.value_int = new_int(1, *fcall->context->value_int + *fcall->func_parmeters[0].value_int);
	}
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
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 1, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &xeql, .func_name = "eql", .function_type = f_main,
				.access = PUBLIC,
				.return_type = T_BOOL, .ref = 0, .stack_next = T_STRING->d_functions + 2, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_ANY,T_ANY}, .func_code = &xreplace, .func_name = "replace",
				.function_type = f_main,
				.access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 3, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_substr, .func_name = "substr",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 4, .start_parm_count = 2
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_index_of, .func_name = "index_of",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 5, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_index_of, .func_name = "find",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 6, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_trim, .func_name = "trim",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 7, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_lower, .func_name = "to_lower",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 8, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_lower, .func_name = "lower",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 9, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_upper, .func_name = "to_upper",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 10, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {0}, .func_code = &x_to_upper, .func_name = "upper",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 11, .start_parm_count = 0
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_starts_with, .func_name = "starts_with",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 12, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_ends_with, .func_name = "ends_with",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 13, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_match, .func_name = "regex_match",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 14, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_match, .func_name = "match",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = T_STRING->d_functions + 15, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_regex_find, .func_name = "regex_find",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 16, .start_parm_count = 1
			},
			{
				.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_replace, .func_name = "regex_replace",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_STRING, .ref = 0, .stack_next = T_STRING->d_functions + 17, .start_parm_count = 2
			},
			{
				.start_func_parmeters = {T_STRING}, .func_code = &x_string_split, .func_name = "split",
				.function_type = f_main, .access = PUBLIC,
				.return_type = T_INT, .ref = 0, .stack_next = 0, .start_parm_count = 1
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
				.return_type = T_INT, .ref = 0, .stack_next = 0
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
				.function_type = f_main, .access = PUBLIC, .return_type = T_INT, .ref = 0,
				.stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 1, .base = 0,
		.stack_next = T_ANY
	},
	{.type_id = 8, .type_name = "T", .base = 0, .stack_next = T_FUNC},
	{.type_id = 9, .type_name = "func", .base = 0, .stack_next = T_TYPE_INFO},
	{.type_id = 10, .type_name = "type", .base = 0, .stack_next = 0}
};





node* get_root(node* j)
{
	//printf("%s", __FUNCTION__);
	if (j->type_ != endl)
	{
		//printf("\nerror line %d \nfile: %s",__LINE__,__FILE__);
		//return NULL;
	}

	node* temp = (node*)j;

	while (temp->parent != NULL)
	{
		temp = temp->parent;
	}
	return temp;
}

node* get_first_type_backword_from(node* in, node_type b)
{
	node* save = (node*)in;
	while (save != NULL && save->type_ != b)
	{
		save = save->parent;
	}
	return save;
}


char* parse_obj_to_str(const node_type t)
{
	char* y = (char*)malloc(1024);
	memset(y, 0, 1024);
	if (t & itype)
		S(y, "itype,");
	if (t & keyword)
		S(y, "keyword,");
	if (t & var_name)
		S(y, "var_name,");
	if (t & value)
		S(y, "value,");
	if (t & operators_n)
		S(y, "operators_n,");
	if (t & equles)
		S(y, "equles,");
	if (t & endl)
		S(y, "endl,");
	if (t & s_index)
		S(y, "s_index,");
	if (t & parentheses1)
		S(y, "parentheses1,");
	if (t & parentheses1_c)
		S(y, "parentheses1c,");
	if (t & comma)
		S(y, "comma,");
	if (t & s_index_c)
		S(y, "s_index_c,");
	if (t & parentheses4)
		S(y, "parentheses4,");
	if (t & parentheses4_c)
		S(y, "parentheses4c,");
	return y;
}


node* static_flag_op2(const node_type mtype, node* w_node, bool added)
{
	return parser_delim_op(current_parser_ctx, mtype, w_node, added);
}


bool static_flag_check2x(node_type* m)
{
	return parser_delim_check_unclosed(current_parser_ctx, m);
}

fl* static_flag_check2()
{
	return parser_delim_get_first_unclosed(current_parser_ctx);
}

// check if n starts with keyword x at a word boundary
int eql(const char* n, const char* x)
{
	if (n == NULL || x == NULL)
		return 0;

	size_t len_x = strlen(x);
	if (strncmp(n, x, len_x) != 0)
		return 0;

	char next = n[len_x];
	if ((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
		return 0;

	return 1;
}


node* get_first_type_with_value(node* in, node_type b, void* value)
{
	//node* temp=getRoot(in);
	int len = 0;
	node* temp = (node*)in;
	while (temp != NULL)
	{
		if (temp->type_ == b)
		{
			if (b == keyword)
			{
				if (((intptr_t)value) == temp->value_keyword)
					return temp;
			}
			else if (value == NULL)
				return temp;
			else if (strcmp((char*)temp->value_raw, (char*)value) == 0)
				return temp;
		}
		len++;
		temp = temp->next;
	}
	return NULL;
}

node* get_first_type(node* in, node_type b)
{
	return get_first_type_with_value(in, b, NULL);
}

node* get_last_type(node* in, const node_type b)
{
	node* temp = (node*)in;
	node* m = NULL;
	while (temp != NULL)
	{
		if (temp->type_ == b)
		{
			m = temp;
		}

		temp = temp->next;
	}
	return m;
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


void scap_string(char* m)
{
	if (m == NULL) return;
	char* dst = m;
	for (char* y = m; *y; y++)
	{
		if (*y == '\\' && *(y + 1) == 'n')
		{
			*dst++ = '\n';
			y++;
		}
		else if (*y == '\\' && *(y + 1) == 't')
		{
			*dst++ = '\t';
			y++;
		}
		else if (*y == '\\' && *(y + 1) == 'r')
		{
			*dst++ = '\r';
			y++;
		}
		else if (*y == '\\' && *(y + 1) == '\\')
		{
			*dst++ = '\\';
			y++;
		}
		else if (*y == '\\' && *(y + 1) == '"')
		{
			*dst++ = '"';
			y++;
		}
		else
		{
			*dst++ = *y;
		}
	}
	*dst = '\0';
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
	if (temp->parm_count_c == 0)return;
	if (temp->parm_count_c == 1 && temp->func_parmeters->type_define == T_STRING)
	{
		pre_parse_line(*temp->func_parmeters->value_str_ptr, 0);
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

void call_func_in(fcall* mfunc)
{
	//d->ref must be parth
	node* stop = get_close_part(mfunc->deftion->ref);
	//c = (node*)temp->func_code;
	compile(mfunc->context, mfunc->deftion->ref->parent, mfunc, stop, NULL);
	if (mfunc->deftion->function_type == constr)
	{
		if (mfunc->context != NULL)
		{
			mfunc->_return.value_type_instsance = mfunc->context->value_type_instsance;
			mfunc->_return.values = mfunc->context->values;
			mfunc->_return.type_define = mfunc->context->type_define;
		}
	}
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

	/*
		HMODULE load_library_a = LoadLibraryA("httplib_dy.dll");
		if (load_library_a != NULL)
		{
			ty* get_type_fnction = (ty*)GetProcAddress(load_library_a, "get_type");
			type_def* nt = get_type_fnction(SIMPLE_TYPE);

			func_deftion* xz = add_function_gloable(nt->d_functions[0]->func_name, nt, nt->d_functions[0]->start_parm_count,
													nt->d_functions[0]->func_code,
													nt->d_functions[0]->start_func_parmeters_name,
													nt->d_functions[0]->start_func_parmeters);
			//func* temp = copy_func(nt->functions.root);
			//temp->func_name = nt->functions.root->func_name;
			//func_stack_push(funcs, temp);
			xz->function_type = constr;

			//type_stack_push(types, nt);
			//type_print(nt, 0);
		}
		*/
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

var* get_globle_var_by_name(char* name)
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


var* fget_var_by_name_fc(char* name, fcall* y)
{
	if (name == NULL || y == NULL)
		return NULL;
	for (int i = 0; i < y->parm_count_c; i++)
	{
		if (y->func_parmeters[i].name != NULL && strcmp(name, y->func_parmeters[i].name) == 0)
			return &y->func_parmeters[i];
	}
	return NULL;
}

var* get_var_by_name_on_stack(char* name, var_stack* y)
{
	if (name == NULL || y == NULL)
		return NULL;
	for (var* i = y->root; i != NULL; i = i->stack_next)
	{
		if (i->name != NULL && strcmp(name, i->name) == 0)
			return i;
	}
	return NULL;
}


var* all_get_var_by_name(char* name, fcall* called_function, var* called_var)
{
	var* ret;
	if (called_function != NULL)
	{
		ret = fget_var_by_name_fc(name, called_function);
		if (ret != NULL)return ret;
	}

	if (called_var != NULL && called_var->value_type_instsance != NULL)
	{
		ret = get_var_by_name_on_stack(name, &called_var->value_type_instsance->propertys);
		if (ret != NULL)return ret;
	}


	return get_globle_var_by_name(name);
}


bool is_base_type(type_def* t)
{
	if (t == NULL)
		return false;
	return t >= SIMPLE_TYPE && t < (SIMPLE_TYPE + 11);
}

void* install_memory(var* n)
{
	return install_memory_with_type(n->type_define, n->size);
}

static void init_instance_prop_obj(var* inctance_prop)
{
	if (inctance_prop == NULL || inctance_prop->type_define == NULL || is_base_type(inctance_prop->type_define))
		return;

	inctance_prop->value_type_instsance = (type_instance*)inctance_prop->values;

	type_def* pt = inctance_prop->type_define;
	for (int i = 0; i < pt->d_function_size; i++)
	{
		if (pt->d_functions[i].func_name != NULL &&
			(strcmp(pt->d_functions[i].func_name, pt->type_name) == 0 ||
			 pt->d_functions[i].function_type == constr))
		{
			if (pt->d_functions[i].start_parm_count == 0)
			{
				fcall* cfc = create_fcall(&pt->d_functions[i]);
				call_function(cfc, &inctance_prop);
				gc_free_any(cfc);
				break;
			}
		}
	}
}

void instance_type(type_def* type_protype, void* dstn_array, int size)
{
	for (int i = 0; i < size; i++)
	{
		type_instance* type_new_instance = ((type_instance*)dstn_array) + i;
		type_new_instance->type = type_protype;
		type_new_instance->size = size;

		var_stack* props = (var_stack*)malloc(sizeof(var_stack));
		var_stack_init(props);

		type_def* cur = type_protype->base;
		while (cur != NULL && cur != cur->base)
		{
			for (int m = 0; m < cur->d_propertys_size; m++)
			{
				var* protype_prop = cur->d_propertys + m;
				if (protype_prop->name != NULL && strcmp(protype_prop->name, "this") != 0)
				{
					if (get_var_by_name_on_stack(protype_prop->name, props) == NULL)
					{
						var* inctance_prop = new_var_on_stack(props, protype_prop->name, protype_prop->type_define);
						inctance_prop->size = 1;
						inctance_prop->holder = type_new_instance;
						inctance_prop->access = protype_prop->access;
						if (protype_prop->access == STATIC)
						{
							inctance_prop->values = protype_prop->values;
						}
						else
						{
							inctance_prop->values = install_memory_with_type(protype_prop->type_define, 1);
							if (protype_prop->values != NULL && is_base_type(protype_prop->type_define))
								set_value_copy_var(inctance_prop, protype_prop);
							init_instance_prop_obj(inctance_prop);
						}
					}
				}
			}
			cur = cur->base;
		}

		if (type_protype->d_propertys_size > 0)
		{
			for (int m = 0; m < type_protype->d_propertys_size; m++)
			{
				var* protype_prop = type_protype->d_propertys + m;

				if (protype_prop->name != NULL && strcmp(protype_prop->name, "this") != 0)
				{
					var* inctance_prop = new_var_on_stack(props, protype_prop->name, protype_prop->type_define);

					inctance_prop->size = 1;
					inctance_prop->holder = type_new_instance;
					inctance_prop->access = protype_prop->access;
					if (protype_prop->access == STATIC)
					{
						inctance_prop->values = protype_prop->values;
					}
					else
					{
						inctance_prop->values = install_memory_with_type(protype_prop->type_define, 1);
						if (protype_prop->values != NULL && is_base_type(protype_prop->type_define))
						{
							set_value_copy_var(inctance_prop, protype_prop);
						}
						init_instance_prop_obj(inctance_prop);
					}
				}
			}
		}
		props->stack_holder = type_new_instance;
		type_new_instance->propertys = *props;
		free(props);

		var* y = new_var_on_stack(&type_new_instance->propertys, "this", type_protype);
		y->value_type_instsance = type_new_instance;
	}
}

void copy_object(type_instance* src, void* dstn_array, int size)
{
	for (int i = 0; i < size; i++)
	{
		type_instance* dstn = (type_instance*)dstn_array + i;
		dstn->type = src->type;

		dstn->size = src->size;
		var_stack* props = (var_stack*)malloc(sizeof(var_stack));
		var_stack_init(props);

		//printf("\ncopy %s %s\n",src->w==child?"inc type":"super type",src->name);
		if (src->propertys.size > 0)
		{
			for (var* prop = src->propertys.root; prop != NULL; prop = prop->stack_next)
			{
				if (strcmp(prop->name, "this") != 0)
				{
					var* inctance_prop = new_var_on_stack(props, prop->name, prop->type_define);

					inctance_prop->size = size;

					inctance_prop->holder = dstn;

					inctance_prop->access = prop->access;
					if ((prop)->access == STATIC)
					{
						//TODO:2022
						//(*y)->values = x->values;
					}
					else
					{
						if (is_base_type(prop->type_define))
						{
							inctance_prop->values = install_memory_with_type(prop->type_define, size);

							set_value_copy_var(inctance_prop, prop);
						}
						else //inctance_prop
						{
							inctance_prop->values = install_memory_with_type(prop->type_define, size);
							copy_object(prop->value_type_instsance, inctance_prop->values, size);
							//set_value_copy_var(inctance_prop, prop);
						}
					}
				}
			}
		}
		props->stack_holder = src;
		dstn->propertys = *props;

		var* y = new_var_on_stack(&dstn->propertys, "this", dstn->type);

		y->value_type_instsance = dstn;
	}
}


void* install_memory_with_type(type_def* mtype, const int count)
{
	void* r = NULL;
	if (mtype == NULL) return r;
	const int ncount = count > 0 ? count : 1;
	if (T_INT == mtype)
	{
		r = (int*)gc_calloc(ncount, sizeof(int), GC_KIND_RAW);
	}
	else if (T_LONG == mtype)
		r = (long*)gc_calloc(ncount, sizeof(long), GC_KIND_RAW);
	else if (T_CHAR == mtype)
	{
		r = (char*)gc_calloc(ncount + 1, sizeof(char), GC_KIND_RAW);
	}
	else if (T_FLOAT == mtype)
		r = (float*)gc_calloc(ncount, sizeof(float), GC_KIND_RAW);
	else if (T_STRING == mtype)
		r = (char**)gc_calloc(ncount, sizeof(char*), GC_KIND_RAW);
	else if (T_BOOL == mtype)
		r = (bool*)gc_calloc(ncount, sizeof(bool), GC_KIND_RAW);

	else if (!is_base_type(mtype))
	{
		type_instance* n_copy_array = (type_instance*)gc_calloc(ncount, sizeof(type_instance), GC_KIND_INSTANCE);

		instance_type(mtype, n_copy_array, ncount);

		r = n_copy_array;
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
		//int* g = (int*)mvar->value;
		//memcpy(memory, g, sizeof(int) * mvar->size);

		copy_object(src->value_type_instsance, (type_instance*)out_memory, src->size);
	}
	return false;
}

bool is_double_equle(node* mnode)
{
	return mnode->type_ == equles && mnode->next->type_ == equles;
}

bool is_double_oprater(node* mnode)
{
	if (mnode == NULL || mnode->next == NULL || mnode->value_char_ptr == NULL || mnode->next->value_char_ptr == NULL)
		return false;

	char c1 = *mnode->value_char_ptr;
	char c2 = *mnode->next->value_char_ptr;

	if (c1 == '=' && c2 == '=') return true; /* == */
	if (c1 == '!' && c2 == '=') return true; /* != */
	if (c1 == '<' && c2 == '=') return true; /* <= */
	if (c1 == '>' && c2 == '=') return true; /* >= */
	if (c1 == '<' && c2 == '<') return true; /* << */
	if (c1 == '>' && c2 == '>') return true; /* >> */
	if (c1 == '&' && c2 == '&') return true; /* && */
	if (c1 == '|' && c2 == '|') return true; /* || */

	return false;
}


node* get_close_part(node* t)
{
	if (t->ref_node != NULL &&
		((t->btype.name == s_index && t->ref_node->type_ == s_index_c) ||
			(t->btype.name == parentheses1 && t->ref_node->type_ == parentheses1_c) ||
			(t->btype.name == parentheses4 && t->ref_node->type_ == parentheses4_c)))
	{
		return t->ref_node;
	}

	printf("ERROR: Line:%d BLOCK %s", t->line, __FUNCTION__);
	exit(0);
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

//memory must allocted before call 
void set_value_copy_var(var* dstn, var* scr)
{
	if (dstn == NULL || scr == NULL) return;
	if (T_INT == dstn->type_define)
		*dstn->value_int = *scr->value_int;
	else if (T_STRING == dstn->type_define)
	{
		const char* s = (scr->value_str_ptr != NULL && *scr->value_str_ptr != NULL) ? *scr->value_str_ptr : "";
		*dstn->value_str_ptr = (char*)malloc(strlen(s) + 1);
		strcpy(*dstn->value_str_ptr, s);
	}
	else if (T_CHAR == dstn->type_define)
	{
		*dstn->value_char_ptr = *scr->value_char_ptr;
	}
	else if (T_LONG == dstn->type_define)
	{
		*dstn->value_long = *scr->value_long;
	}
	else if (T_FLOAT == dstn->type_define)
	{
		*dstn->value_float = *scr->value_float;
	}
	else if (T_BOOL == dstn->type_define)
	{
		*dstn->value_bool = *scr->value_bool;
	}
	else
	{
		if (dstn->value_type_instsance != NULL && scr->value_type_instsance != NULL)
			*dstn->value_type_instsance = *scr->value_type_instsance;
	}
}

//var type and memory must already alocated  //for value  //  only parse int
void set_value_copy_node(var* dstn, node* scr)
{
	if (T_INT == dstn->type_define)
		*dstn->value_int = atoi(scr->value_char_ptr);
	else if (T_STRING == dstn->type_define)
	{
		*dstn->value_str_ptr = (char*)gc_calloc(1, strlen(scr->value_char_ptr) + 1, GC_KIND_STRING);
		strcpy(*dstn->value_str_ptr, scr->value_char_ptr);
	}
	else if (T_FLOAT == dstn->type_define)
	{
		*dstn->value_float = atof(scr->value_char_ptr);
	}
	else if (T_LONG == dstn->type_define)
	{
		*dstn->value_long = atol(scr->value_char_ptr);
	}
	else if (T_BOOL == dstn->type_define)
	{
		*dstn->value_bool = strcmp(scr->value_char_ptr, "false") != 0;
	}
	else if (T_CHAR == dstn->type_define)
	{
		*dstn->value_char_ptr = *scr->value_char_ptr;
	}
}


//type_def parms[] = {T_STRING, T_INT};

func_deftion simple_function_array[] = {
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "print", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0, .stack_next = simple_function_array + 1
	},
	{
		//vprint
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "vprint", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 2
	},
	{
		.start_func_parmeters = {0}, .func_code = &time_x, .func_name = "time", .function_type = f_main,
		.return_type = T_STRING,
		.ref = 0,
		.stack_next = simple_function_array + 3
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &scan, .func_name = "scan", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 4
	},
	{
		.start_func_parmeters = {T_INT}, .start_parm_count = 1, .func_code = &random_, .func_name = "random",
		.function_type = f_main, .return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 5
	},
	{
		.start_func_parmeters = {T_INT}, .start_parm_count = 1, .func_code = &str, .func_name = "str",
		.function_type = f_main,
		.return_type = T_STRING,
		.ref = 0,
		.stack_next = simple_function_array + 6
	},
	{
		.start_func_parmeters = {T_STRING}, .start_parm_count = 1, .func_code = &import, .func_name = "import",
		.function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 7
	},
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "xxxx", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 8
	},
	{
		.start_func_parmeters = {0}, .func_code = &print, .func_name = "pxcrint", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 9
	},
	{
		.start_func_parmeters = {0}, .func_code = &_exit_, .func_name = "exit", .function_type = f_main,
		.return_type = T_INT,
		.ref = 0,
		.stack_next = simple_function_array + 10
	},
	{
		.start_func_parmeters = {0}, .func_code = &len, .func_name = "len", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.ref = 0,
		.stack_next = simple_function_array + 11
	},
	{
		.start_func_parmeters = {0}, .func_code = &_echo, .func_name = "echo", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.ref = 0,
		.stack_next = simple_function_array + 12
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &eval, .func_name = "eval", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 1,
		.ref = 0,
		.stack_next = simple_function_array + 13
	},
	{
		.start_func_parmeters = {T_ANY,T_ANY}, .func_code = &xeql, .func_name = "eql", .function_type = f_main,
		.return_type = T_INT,
		.start_parm_count = 2,
		.ref = 0,
		.stack_next = simple_function_array + 14
	},
	{
		.start_func_parmeters = {T_ANY,T_ANY, T_ANY}, .func_code = &xreplace, .func_name = "replace",
		.function_type = f_main,
		.return_type = T_STRING,
		.start_parm_count = 3,
		.ref = 0,
		.stack_next = simple_function_array + 15
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_socket_create, .func_name = "socket_create",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 16
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_socket_create, .func_name = "socket",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 17
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_connect, .func_name = "socket_connect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 18
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_connect, .func_name = "connect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 19
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_bind, .func_name = "socket_bind",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 20
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_socket_bind, .func_name = "bind",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 21
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_listen, .func_name = "socket_listen",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 22
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_listen, .func_name = "listen",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 23
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_accept, .func_name = "socket_accept",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 24
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_accept, .func_name = "accept",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 25
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_socket_send, .func_name = "socket_send",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 26
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_socket_send, .func_name = "send",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 27
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recv, .func_name = "socket_recv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 28
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recv, .func_name = "recv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 29
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_close, .func_name = "socket_close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 30
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_socket_close, .func_name = "close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 31
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_set_timeout, .func_name = "socket_set_timeout",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 32
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_set_reuseaddr, .func_name = "socket_set_reuseaddr",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 33
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_STRING, T_INT}, .func_code = &x_socket_sendto, .func_name = "socket_sendto",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 4, .ref = 0,
		.stack_next = simple_function_array + 34
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_socket_recvfrom, .func_name = "socket_recvfrom",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 35
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_http_get, .func_name = "http_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 36
	},
	/* File I/O (36 - 48) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_read_all, .func_name = "file_read_all",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 37
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_read_all, .func_name = "read_file",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 38
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_write_all, .func_name = "file_write_all",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 39
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_write_all, .func_name = "write_file",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 40
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_append, .func_name = "file_append",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 41
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_exists, .func_name = "file_exists",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 42
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_remove, .func_name = "file_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 43
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_remove, .func_name = "file_delete",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 44
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_file_size, .func_name = "file_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 45
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_file_open, .func_name = "file_open",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 46
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_file_read, .func_name = "file_read",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 47
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_file_write, .func_name = "file_write",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 48
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_file_close, .func_name = "file_close",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 49
	},
	/* CLI & System (49 - 56) */
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_get_arg, .func_name = "get_arg",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 50
	},
	{
		.start_func_parmeters = {0}, .func_code = &x_get_argc, .func_name = "get_argc",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 51
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_exec, .func_name = "system_exec",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 52
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_exec, .func_name = "exec",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 53
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_getenv, .func_name = "system_getenv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 54
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_system_getenv, .func_name = "getenv",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 55
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_system_setenv, .func_name = "system_setenv",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 56
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_system_setenv, .func_name = "setenv",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 57
	},
	/* Regex & String Utilities (57 - 67) */
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_match, .func_name = "regex_match",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 58
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_regex_find, .func_name = "regex_find",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 59
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING, T_STRING}, .func_code = &x_regex_replace, .func_name = "regex_replace",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 60
	},
	{
		.start_func_parmeters = {T_STRING, T_INT, T_INT}, .func_code = &x_substr, .func_name = "substr",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 61
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_index_of, .func_name = "index_of",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 62
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_index_of, .func_name = "find",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 63
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_trim, .func_name = "trim",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 64
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_to_lower, .func_name = "to_lower",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 65
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_to_upper, .func_name = "to_upper",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 66
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_starts_with, .func_name = "starts_with",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 67
	},
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_ends_with, .func_name = "ends_with",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 68
	},
	/* Math Library (68 - 91) */
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sqrt, .func_name = "math_sqrt",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 69
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sqrt, .func_name = "sqrt",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 70
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_pow, .func_name = "math_pow",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 71
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_pow, .func_name = "pow",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 72
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_abs, .func_name = "math_abs",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 73
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_abs, .func_name = "abs",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 74
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_min, .func_name = "math_min",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 75
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_min, .func_name = "min",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 76
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_max, .func_name = "math_max",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 77
	},
	{
		.start_func_parmeters = {T_ANY, T_ANY}, .func_code = &x_math_max, .func_name = "max",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 78
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_floor, .func_name = "math_floor",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 79
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_floor, .func_name = "floor",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 80
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_ceil, .func_name = "math_ceil",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 81
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_ceil, .func_name = "ceil",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 82
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_round, .func_name = "math_round",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 83
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_round, .func_name = "round",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 84
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sin, .func_name = "math_sin",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 85
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_sin, .func_name = "sin",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 86
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_cos, .func_name = "math_cos",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 87
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_cos, .func_name = "cos",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 88
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_tan, .func_name = "math_tan",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 89
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_tan, .func_name = "tan",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 90
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_log, .func_name = "math_log",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 91
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_math_log, .func_name = "log",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 92
	},
	/* Dynamic List (92 - 112) */
	{
		.start_func_parmeters = {0}, .func_code = &x_list_create, .func_name = "list_new",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 93
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_free, .func_name = "list_free",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 94
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_size, .func_name = "list_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 95
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_add, .func_name = "list_add",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 96
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_add_int, .func_name = "list_add_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 97
	},
	{
		.start_func_parmeters = {T_INT, T_ANY}, .func_code = &x_list_add_float, .func_name = "list_add_float",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 98
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get, .func_name = "list_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 99
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get_int, .func_name = "list_get_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 100
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_get_float, .func_name = "list_get_float",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 101
	},
	{
		.start_func_parmeters = {T_INT, T_INT, T_STRING}, .func_code = &x_list_set, .func_name = "list_set",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 102
	},
	{
		.start_func_parmeters = {T_INT, T_INT, T_INT}, .func_code = &x_list_set_int, .func_name = "list_set_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 103
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_remove_at, .func_name = "list_remove_at",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 104
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_clear, .func_name = "list_clear",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 105
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_contains, .func_name = "list_contains",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 106
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_contains_int, .func_name = "list_contains_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 107
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_index_of, .func_name = "list_index_of",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 108
	},
	{
		.start_func_parmeters = {T_INT, T_INT}, .func_code = &x_list_index_of_int, .func_name = "list_index_of_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 109
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_pop, .func_name = "list_pop",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 110
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_pop_int, .func_name = "list_pop_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 111
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_list_join, .func_name = "list_join",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 112
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_list_to_string, .func_name = "list_to_string",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 113
	},
	/* Hash Map (113 - 128) */
	{
		.start_func_parmeters = {0}, .func_code = &x_map_create, .func_name = "map_new",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 114
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_free, .func_name = "map_free",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 115
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_size, .func_name = "map_size",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 116
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_STRING}, .func_code = &x_map_put, .func_name = "map_put",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 117
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_INT}, .func_code = &x_map_put_int, .func_name = "map_put_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 118
	},
	{
		.start_func_parmeters = {T_INT, T_STRING, T_ANY}, .func_code = &x_map_put_float, .func_name = "map_put_float",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 3, .ref = 0,
		.stack_next = simple_function_array + 119
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get, .func_name = "map_get",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 120
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get_int, .func_name = "map_get_int",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 121
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_get_float, .func_name = "map_get_float",
		.function_type = f_main, .return_type = T_FLOAT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 122
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_has, .func_name = "map_has",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 123
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_map_remove, .func_name = "map_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 124
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_clear, .func_name = "map_clear",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 125
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_keys, .func_name = "map_keys",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 126
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_values, .func_name = "map_values",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 127
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_to_string, .func_name = "map_to_string",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 128
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_map_keys_list, .func_name = "map_keys_list",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 129
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_collect, .func_name = "gc_collect",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 130
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_allocated_bytes, .func_name = "gc_allocated_bytes",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 131
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_total_objects, .func_name = "gc_total_objects",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 132
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_enable, .func_name = "gc_enable",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 133
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_disable, .func_name = "gc_disable",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 134
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_gc_set_threshold, .func_name = "gc_set_threshold",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 135
	},
	{
		.start_func_parmeters = {}, .func_code = &x_gc_dump, .func_name = "gc_dump",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 136
	},
	{
		.start_func_parmeters = {}, .func_code = &x_clock_ms, .func_name = "clock_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 137
	},
	{
		.start_func_parmeters = {}, .func_code = &x_clock_ms, .func_name = "time_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 138
	},
	/* String Ergonomics (138) */
	{
		.start_func_parmeters = {T_STRING, T_STRING}, .func_code = &x_string_split, .func_name = "str_split",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 139
	},
	/* JSON Module (139 - 141) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_json_parse, .func_name = "json_parse",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 140
	},
	{
		.start_func_parmeters = {T_ANY}, .func_code = &x_json_stringify, .func_name = "json_stringify",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 141
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_json_is_valid, .func_name = "json_is_valid",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 142
	},
	/* DateTime Module (142 - 150) */
	{
		.start_func_parmeters = {}, .func_code = &x_datetime_now, .func_name = "datetime_now",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 143
	},
	{
		.start_func_parmeters = {T_INT, T_STRING}, .func_code = &x_datetime_format, .func_name = "datetime_format",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 2, .ref = 0,
		.stack_next = simple_function_array + 144
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_year, .func_name = "datetime_year",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 145
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_month, .func_name = "datetime_month",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 146
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_day, .func_name = "datetime_day",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 147
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_hour, .func_name = "datetime_hour",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 148
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_minute, .func_name = "datetime_minute",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 149
	},
	{
		.start_func_parmeters = {T_INT}, .func_code = &x_datetime_second, .func_name = "datetime_second",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 150
	},
	{
		.start_func_parmeters = {}, .func_code = &x_datetime_clock_ms, .func_name = "datetime_clock_ms",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 0, .ref = 0,
		.stack_next = simple_function_array + 151
	},
	/* Directory Operations (151 - 154) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_list, .func_name = "dir_list",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 152
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_create, .func_name = "dir_create",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 153
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_exists, .func_name = "dir_exists",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 154
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_dir_remove, .func_name = "dir_remove",
		.function_type = f_main, .return_type = T_INT, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 155
	},
	/* Process Operations (155 - 156) */
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_proc_capture, .func_name = "proc_capture",
		.function_type = f_main, .return_type = T_STRING, .start_parm_count = 1, .ref = 0,
		.stack_next = simple_function_array + 156
	},
	{
		.start_func_parmeters = {T_STRING}, .func_code = &x_proc_run, .func_name = "proc_run",
		.function_type = f_main, .return_type = T_ANY, .start_parm_count = 1, .ref = 0,
		.stack_next = 0
	}
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
	//.propertys = {.size = 6, .top = tinfo + 5, .root = tinfo + 0, .stack_holder = 0}
};
var start_var = {
	.name = "xlang", .type_define = T_OBJECT, .value_type_instsance = &xlnag_object,
	.size = 1, .holder = NULL, .base_type = 0, .ref = 0, .stack_next = tinfo, .access = PUBLIC
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
		var* id_prop = NULL;
		if (name->value_type_instsance != NULL)
			id_prop = get_var_by_name_on_stack("id", &name->value_type_instsance->propertys);
		if (id_prop != NULL && id_prop->value_int != NULL)
		{
			int list_id = *id_prop->value_int;
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


bool stop_here(node_type stop_in_type, node* stop_in_node, node* mnode)
{
	if ((stop_in_type != none && (mnode->type_ & stop_in_type)) || (stop_in_node != NULL && (mnode == stop_in_node ||
		mnode->parent == stop_in_node)))
		return true;
	return false;
}