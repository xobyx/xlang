#include "functions.h"
#include <string.h>
#if defined(__GNUC__)|| defined(__MINGW64__)
#include <stdarg.h>
#endif
#include <time.h>
#include "http.h"
#include "echo.h"
//#define F

void step(node** nod)
{
	if ((*nod)->type_ != endl)
		*nod = (*nod)->next;
}

bool eat(node** nod, enum node_type_enum next,bool must)
{
	node* n = (*nod)->next;

	if (must && n->type_ != next)
	{
		printf("Error : missing in line %d - c-%s:%s:%d", (*nod)->line,__FILE__, __FUNCTION__,__LINE__);
		exit(0);
	}

	*nod = n->type_ == next ? n : *nod;
	return (*nod)->type_ == next;
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
	switch (fcall->func_parmeters->type_define->type_id)
	{
	case 0:

	default:
		break;
	}
}

const func_deftion int_function[] = {
	{
		.start_func_parmeters = {T_ANY}, .func_code = &int_add, .func_name = "add", .function_type = f_main,
		.access = PUBLIC, .start_parm_count = 1, .return_type = T_INT, .ref = 0, .stack_next = 0
	}
};
void xeql(fcall* fcall);
void xreplace(fcall* fcall);
type_def SIMPLE_TYPE[] = {
	{
		.type_id = 0, .type_name = "long", .d_propertys = {0}, .d_functions = {0}, .base = 0,
		.stack_next = T_STRING
	},
	{
		.type_id = 1, .type_name = "string", .d_propertys = {0},
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
				.return_type = T_STRING, .ref = 0, .stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 3, .base = 0,
		.stack_next = T_CHAR
	},
	{.type_id = 2, .type_name = "char", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_INT},
	{
		.type_id = 3, .type_name = "int", .d_propertys = {0}, .d_functions = int_function, .base = 0,
		.stack_next = T_BOOL
	},
	{.type_id = 4, .type_name = "bool", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_FLOAT},
	{.type_id = 5, .type_name = "float", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_OBJECT},
	{.type_id = 6, .type_name = "object", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_ARRAY},
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
	{.type_id = 8, .type_name = "T", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_FUNC},
	{.type_id = 9, .type_name = "func", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_TYPE_INFO},
	{.type_id = 10, .type_name = "type", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = 0}

};


fl staic_flag2[] = {
	{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0},
	{0, 0}
};


int fi = 0;


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
	node* p = NULL;
	if (added)
	{
		for (int i = 0; i < 10; i++)
			if (staic_flag2[i].wait_type == 0)
			{
				staic_flag2[i].wait_type = mtype;
				staic_flag2[i].waiting_node = w_node;
				break;
			}
	}
	else
	{
		for (int i = 9; i >= 0; i--)
			if (staic_flag2[i].wait_type == mtype)
			{
				if (staic_flag2[i].waiting_node != NULL)
				{
					staic_flag2[i].waiting_node->ref_node = w_node;
					p = staic_flag2[i].waiting_node;
				}

				staic_flag2[i].wait_type = (node_type)0;
				staic_flag2[i].waiting_node = NULL;
				break;
			}
	}
	return p;
}


bool static_flag_check2x(node_type* m)
{
	int* ma = (int*)m;
	bool cont = false;
	for (int i = 0; i < 10; i++)
		if (staic_flag2[i].wait_type != 0)
		{
			cont = true;
			*ma |= staic_flag2[i].wait_type;
		}

	return cont;
}

fl* static_flag_check2()
{
	for (int i = 0; i < 10; i++)
		if (staic_flag2[i].wait_type != 0)
		{
			return &staic_flag2[i];
		}

	return NULL;
}

//ignor strings
int eql(const char* n, const char* x)
{
	char *mk, *tf;

	if (strlen(n) >= strlen(x))
	{
		mk = (char*)x;
		tf = (char*)n;
	}
	else
	{
		mk = (char*)n;
		tf = (char*)x;
	}

	for (; *mk != 0; mk++, tf++)
	{
		if (*mk != *tf)
			return 0;
	}
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
			if (value == NULL)
				return temp;
			if (b == keyword)
			{
				if (((int)value) == temp->value_keyword)
					return temp;
			}

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


// ReSharper disable CppParameterMayBeConstPtrOrRef
void assign_array_index(var* nvalue, var* marray, int index)

{
	//if(index >= me->size ){printf("error index out range ..");exit(0);}
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
		marray->value_int[index] = *nvalue->value_int;
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
	else
	{
		marray->value_type_instsance[index] = *nvalue->value_type_instsance;
	}
}


void scap_string(char* m)
{
	for (char* y = m; *y; y++)
	{
		if (*y == '\\' && *(y + 1) == 'n')
		{
			*y = 0x0a;
			*(y + 1) = 0x0d;
			y++;
		}
	}
}

void print_f(fcall* temp)
{
	//va_list a;
	void** margs = (void**)malloc(sizeof(char**) * (temp->parm_count_c - 1)); //arg list
	var* fparms = temp->func_parmeters;
	var* x = fparms;
	if (fparms->type_define == T_STRING)
	{
		for (int i = 1; i < temp->parm_count_c; i++)
		{
			x = x + i;
			*(margs + (i - 1)) = x->type_define == T_STRING ? *x->value_str_ptr : x->value_int;
		}
#if !defined(__GNUC__)&& !defined(__MINGW64__)


		vprintf(*fparms->value_str_ptr, margs);
#endif

		free(margs);
	}
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
	if (temp->parm_count_c > 1)
	{
		print_f(temp);
		return;
	}
	var* m = temp->func_parmeters;
	int* r = (int*)malloc(sizeof(int));
	*r = -1;
	char* k;
	if (m->type_define == T_INT)
	{
		for (int ms = 0; ms < m->size; ms++)
		{
			*r = printf("%d \n", m->value_int[ms]);
		}
	}
	else if (m->type_define == T_STRING)
	{
		//*r = printf("%s(%d)=[", m->type_define->type_name, m->size);
		for (int ms = 0; ms < m->size; ms++)
		{
			*r = printf("%s \n", m->value_str_ptr[ms]);
		}
	}
	else if (m->type_define == T_LONG)
	{
		for (int i = 0; i < m->size; i++)
		{
			*r = printf("%lu\n", m->value_long[i]);
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
		//TODO : print var to string
		//vprint(temp);
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
		//TODO:2022
		mfunc->_return.value_type_instsance = mfunc->context->value_type_instsance;
	}
}

void import(fcall* d)
{
	if (d->func_parmeters->type_define == T_STRING)
	{
		char* y = *d->func_parmeters->value_str_ptr;
		FILE* sf;

		sf = fopen(y, "r");


		if (sf != NULL)
		{
			char* buf = get_file_buffer(sf);
			//TODO: check for parsed file
			start_parse_lines(buf, false);
			free(buf);
			fclose(sf);
		}
		else
		{
			printf("\nCan't import File %s not found", y);
		}

		//b++;
	}
	else
	{
		printf("Error: input is not string");
	}
}

void time_x(fcall* d)
{
	time_t a;
	time(&a);

	char* buff = (char*)malloc(sizeof(char) * 20);
	memset(buff, 0, 20);

	struct tm* tm_info = localtime(&a);;
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
	int* re = calloc(count, sizeof(int));
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
			y->_return.value_int = new_int(1, *a->value_int == *a->value_int);
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
	char *orig, *rep, *with;
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

		if ((y->func_parmeters + 1)->type_define == T_STRING)
		{
			with = *(y->func_parmeters + 2)->value_str_ptr;
		}
		else if ((y->func_parmeters + 1)->type_define == T_CHAR)
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
	for (count = 0; tmp = strstr(ins, rep); ++count)
	{
		ins = tmp + len_rep;
	}

	tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

	if (!result)
	{
		y->_return.value_str_ptr = p1->value_str_ptr;
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

int calls = 0;
char* temp = "";
var* tempv;

func_deftion* new_func()
{
	return new_func_on_stack(funcs);
}

type_def* get_type_by_name(char* name)
{
	type_stack* vs = types;
	//if (name == NULL) return NULL;

	//if (vs->size > 0)
	for (type_def* i = vs->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->type_name) == 0)
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
	if (NULL == object_var)
	{
		printf("error : get_func_by_name_with_var | %s - C:%s:%d", name, __FILE__, __LINE__);
		exit(-1);
	}
	func_deftion* funcs = NULL;
	if (is_base_type(object_var->type_define))
	{
		funcs = object_var->type_define->d_functions;
	}
	else
	{
		funcs = object_var->value_type_instsance->functions.root;
	}

	for (func_deftion* i = funcs; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->func_name) == 0)
		{
			return i;
		}
	}
	printf("error : get_func_by_name_with_var | %s", name);
	exit(-1);
	return NULL;
}

var* get_globle_var_by_name(char* name)
{
	if (name == temp)
		return tempv;
	for (var* i = varss->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->name) == 0)
		{
			tempv = i;
			temp = name;
			return i;
		}
	}
	return NULL;
}


var* fget_var_by_name_fc(char* name, fcall* y)
{
	if (temp == name)
	{
		return tempv;
	}
	for (int i = 0; i < y->parm_count_c; i++)
	{
		if (strcmp(name, y->func_parmeters[i].name) == 0)
		{
			tempv = &y->func_parmeters[i];
			temp = name;
			return tempv;
		}
	}
	return NULL;
}

var* get_var_by_name_on_stack(char* name, var_stack* y)
{
	//printf("call %s  ,%d address: %d  \n", name , ++calls,(int)name);
	if (temp == name)
	{
		return tempv;
	}
	for (var* i = y->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->name) == 0)
		{
			tempv = i;
			temp = name;
			return tempv;
		}
	}

	//tempv = get_globle_var_by_name(name);
	temp = tempv != NULL ? name : 0;
	return tempv;
}


var* all_get_var_by_name(char* name, fcall* called_function, var* called_var)
{
	var* ret;
	if (called_function != NULL)
	{
		ret = fget_var_by_name_fc(name, called_function);
		if (ret != NULL)return ret;
	}

	if (called_var != NULL)
	{
		ret = get_var_by_name_on_stack(name, &called_var->value_type_instsance->propertys);
		if (ret != NULL)return ret;
	}


	return get_globle_var_by_name(name);
}


bool is_base_type(type_def* t)
{
	return t->type_id < simple_type_stack.size;
}

void* install_memory(var* n)
{
	return install_memory_with_type(n->type_define, n->size);
}

void instance_type(type_def* type_protype, void* dstn_array, int size)
{
	for (int i = 0; i < size; i++)
	{
		type_instance* type_new_instance = ((type_instance*)dstn_array) + i;

		var_stack* props = (var_stack*)malloc(sizeof(var_stack));
		var_stack_init(props);

		//printf("\ncopy %s %s\n",src->w==child?"inc type":"super type",src->name);
		if (type_protype->d_propertys_size > 0)
		{
			for (int m = 0; m < type_protype->d_propertys_size; m++)
			{
				var* protype_prop = type_protype->d_propertys + m;

				if (strcmp(protype_prop->name, "this") != 0)
				{
					var* inctance_prop = new_var_on_stack(props, protype_prop->name, protype_prop->type_define);

					inctance_prop->size = size;

					inctance_prop->holder = type_new_instance;

					inctance_prop->access = protype_prop->access;
					if ((protype_prop)->access == STATIC)
					{
						inctance_prop->values = protype_prop->values;
					}
					else
					{
						if (is_base_type(protype_prop->type_define))
						{
							inctance_prop->values = install_memory_with_type(protype_prop->type_define, size);

							set_value_copy_var(inctance_prop, protype_prop);
						}
						else
						{
							inctance_prop->values = install_memory_with_type(protype_prop->type_define, size);
							set_value_copy_var(inctance_prop, protype_prop);
						}
					}
				}
			}
		}
		props->stack_holder = type_new_instance;
		type_new_instance->propertys = *props;

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
	int size = count == 0 ? 1 : count;
	void* r = NULL;
	if (mtype == NULL) return r;
	if (T_INT == mtype)
	{
		r = (int*)calloc(count, sizeof(int));
	}
	else if (T_LONG == mtype)
		r = (long*)calloc(count, sizeof(long));
	else if (T_CHAR == mtype)
	{
		r = (char*)malloc(sizeof(char) * count + 1);
		memset(r, 0, sizeof(char) * count + 1);
	}
	else if (T_FLOAT == mtype)
		r = (float*)calloc(count, sizeof(float));
	else if (T_STRING == mtype)
		r = (char**)calloc(count, sizeof(char*));
	else if (T_BOOL == mtype)
		r = (bool*)calloc(count, sizeof(int));

	else if (!is_base_type(mtype))
	{
		type_instance* n_copy_array = (type_instance*)calloc(count, sizeof(type_instance));


		instance_type(mtype, n_copy_array, count);


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
	return (mnode->type_ == operators_n && (mnode->next->type_ == operators_n || mnode->next->type_ == equles))
		|| (mnode->type_ == equles && mnode->next->type_ == equles);
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
	if (T_INT == dstn->type_define)
		*dstn->value_int = *scr->value_int;
	else if (T_STRING == dstn->type_define)
	{
		*dstn->value_str_ptr = (char*)malloc(strlen(*scr->value_str_ptr) + 1);
		strcpy(*dstn->value_str_ptr, *scr->value_str_ptr);
	}
	else if (T_CHAR == dstn->type_define)
	{
		*dstn->value_char_ptr = *scr->value_char_ptr;
	}
	else
	{
		printf("un imp type...");
		exit;
	}
}

//var type and memory must already alocated  //for value  //  only parse int
void set_value_copy_node(var* dstn, node* scr)
{
	if (T_INT == dstn->type_define)
		*dstn->value_int = atoi(scr->value_char_ptr);
	else if (T_STRING == dstn->type_define)
	{
		*dstn->value_str_ptr = (char*)calloc(1, strlen(scr->value_char_ptr) + 1);
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
		.stack_next = 0
	}


};
func_stack base_function = {.top = simple_function_array + 14, .size = 15, .root = simple_function_array + 0};

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
	fcall* function_c = (fcall*)calloc(1, sizeof(fcall));
	function_c->deftion = fd;
	for (int i = 0; i < fd->start_parm_count; i++)
	{
		function_c->func_parmeters[i].name = fd->start_func_parmeters_name[i];
		function_c->func_parmeters[i].type_define = fd->start_func_parmeters[i];
	}
	function_c->parm_count_c = fd->start_parm_count;
	function_c->_return.type_define = fd->return_type;
	return function_c;
}


var* get_array_item(var* name, int index)
{
	var* ret = NULL;
	switch (name->type_define->type_id)
	{
	case t_long:

		ret = new_temp_var(T_LONG);
		ret->value_long = name->value_long + index;
		break;
	case t_string:
		{
			if (name->size > 1)
			{
				ret = new_temp_var(T_STRING);
				ret->value_str_ptr = name->value_str_ptr + index;
			}
			else
			{
				ret = new_temp_var(T_CHAR);
				ret->value_char_ptr = *name->value_str_ptr + index;
			}
			break;
		}
	case t_char:
		ret = new_temp_var(T_CHAR);
		ret->value_str_ptr = name->value_str_ptr + index;
		break;
	case t_int:
		ret = new_temp_var(T_INT);
		ret->value_int = name->value_int + index;
		break;
	case t_bool:
		ret = new_temp_var(T_BOOL);
		ret->value_bool = name->value_bool + index;
		break;
	case t_float:
		ret = new_temp_var(T_FLOAT);
		ret->value_float = name->value_float + index;
		break;
	case t_array:

		break;
	default:
		printf("error");
	}


	return ret;
}


func_deftion* get_obj_function2(var* object_var, char* name)
{
	if (NULL == object_var)
	{
		printf("error : get_func_by_name_with_var | %s - C:%s:%d", name, __FILE__, __LINE__);
		exit(-1);
	}
	func_deftion* funcs = NULL;
	if (is_base_type(object_var->type_define))
	{
		funcs = object_var->type_define->d_functions;
	}
	else
	{
		funcs = object_var->value_type_instsance->functions.root;
	}

	for (func_deftion* i = funcs; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->func_name) == 0)
		{
			return i;
		}
	}
	printf("error : get_func_by_name_with_var | %s", name);
	exit(-1);
	return NULL;
}


bool stop_here(node_type stop_in_type, node* stop_in_node, node* mnode)
{
	if ((stop_in_type != none && mnode->type_ == stop_in_type) || (stop_in_node != NULL && (mnode == stop_in_node ||
		mnode->parent == stop_in_node)))
		return true;
	return false;
}
