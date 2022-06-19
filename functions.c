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
	node* n =(*nod)->next;

	if (must && n->type_!= next )
	{
		printf("Error : missing in line %d - c-%s:%s:%d", (*nod)->line,__FILE__, __FUNCTION__,__LINE__);
		exit(0);
	}
	
	*nod = n->type_== next ? n : *nod;
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
				.return_type = T_INT, .ref = 0, .stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 1, .base = 0,
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
			.function_type = f_main, .access = PUBLIC,.return_type = T_INT, .ref = 0,
			.stack_next = 0, .start_parm_count = 1
			}
		},
		.d_function_size = 1, .base = 0,
		.stack_next = T_ANY
	},
	{.type_id = 8, .type_name = "T", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_FUNC},
	{.type_id = 9, .type_name = "func", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = T_TYPE_INFO},
	{.type_id = 10, .type_name = "itype", .d_propertys = {0}, .d_functions = {0}, .base = 0, .stack_next = 0}

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

node* calculate4(var* ms, node* m, node_type z, fcall* km)
{
	//calculate(const node* m, func* funct, var* contxt, parse_obj stop, node* end, var* out)
	return calculate(m, km, 0, z, NULL, ms);
}

node* calculate3(var* ms, node* m, fcall* km)
{
	return calculate(m, km, NULL, (node_type)0, NULL, ms);
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

void set_value(var* context, fcall* temp, node** cx)
{
	bool array_copy = false;
	if (((*cx)->next->type_ == equles) ||
		((*cx)->next->type_ == s_index && get_first_type(*cx, s_index_c)->next->type_ == equles))
	{
		var* calc_result = new_temp_var(NULL);
		var* me = NULL;

		int index = 0;

		if (context != NULL)
		{
			me = get_var_by_name_on_stack((*cx)->value_char_ptr, &context->value_type_instsance->propertys);
		}
		else if (temp != NULL)
		{
			me = fget_var_by_name_fc((*cx)->value_char_ptr, temp);
		}

		if (me == NULL)
		{
			me = get_globle_var_by_name((*cx)->value_char_ptr);
		}
		bool index_assian = false;


		if ((*cx)->next->btype.node_type_bit.s_index)
		{
			var* get_index = new_temp_var(T_INT);


			node* close = get_close_part((*cx)->next);
			calculate((*cx)->next->next, temp, context, (node_type)0, close, get_index);

			index = *get_index->value_int;
			free_temp_var(get_index);
			*cx = close;
			index_assian = true;
		}
		else if (me->size > 1)
		{
			calc_result->size = me->size;
			array_copy = true;
		}

		if ((*cx)->next->type_ == equles)
		{
			calc_result->type_define = ((me->type_define == T_STRING) && index_assian) ? T_CHAR : me->type_define;


			//c = calculate(res, c->next->next, temp);
			*cx = calculate((*cx)->next->next, temp, context, endl, NULL, calc_result);

			if (*cx == NULL)
			{
				return;
			}
		}
		if (array_copy)
		{
			free(me->values);
			me->values = calc_result->values;
			return;
		}
		assign_array_index(calc_result, me, index);
		if ((*cx)->type_ != endl)
		{
			*cx = get_first_type(*cx, endl);
		}
	}
}


#define  START(index) if((index)==0) printf("["); else printf(",") ;

/*
void type_print(type_def* u, int dep)
{
	char* m = "";
	if (dep > 0)
	{
		m = (char*)malloc(sizeof(char) * (dep * 4) + 1);
		memset(m, 0, sizeof(char) * (dep * 4) + 1);
		memset(m, ' ', (dep * 4));
	}
	///p.cprintf(2, "%stype_print:\n", m);
	printf("%s{\n", m);

	printf("%stype name = %s \n", m, u->name);
	printf("%stype.type_id = %d \n", m, (int)u);
	//p.cprintf(3, "%s%s propertys = \n", m, u->name);
	printf("%s{\n", m);
	if (u->propertys.size > 0)
	{
		for (var* yb = u->propertys.root; yb != NULL; yb = yb->stack_next)
		{
			if (strcmp(yb->name, "this") != 0)
			{
				//p.cprintf(6, " property name : %s%s\n", m, yb->name);
				type_print(yb->var_type, dep + 4);
			}
		}
		printf("%s}\n", m);
	}
	//p.cprintf(4, "%s%s functions = \n", m, u->name);
	printf("%s{\n", m);
	if (u->functions.size > 0)
	{
		for (func_deftion* yb = u->functions.root; yb != NULL; yb = yb->stack_next)
		{
			printf("%s  %s\n", m, yb->func_name);
		}
	}
	printf("%s}\n", m);
	printf("%s}\n", m);
	if (*m != '\0')
		free(m);
}

void vprint(func_deftion* temp)
{
	var* m = temp->func_parmeters.root;

	type_print(m->var_type, 0);

	return;
	printf("var_type->%s array: %s type.type_id:%d \n", m->var_type->name, m->size > 1 ? "Yes" : "No", (int)m->var_type);
	type_print(m->var_type, 0);

	for (int ms = 0; ms < m->size; ms++)
	{
		START(ms)
		if (!is_base_type(m->var_type))
		{
			type_def* u = ((type_def*)m->values) + ms; //  m[ms]
			printf("type.type_id:%d\n", (int)m->var_type);
			type_print(u, 1);
		}
		else
		{
			printf("base_type\n");
		}
	}
	printf("]\n");
}
*/
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
		char* k2 = "%d]\n";
		k = "%d\n";
		if (m->size > 1)
		{
			k = "%d,";
			*r = printf("%s(%d)=[", m->type_define->type_name, m->size);
			for (int ms = 0; ms < m->size - 1; ms++)
			{
				*r = printf(k, (m->value_int)[ms]);
			}
			*r = printf(k2, (m->value_int)[m->size - 1]);
		}
		else
		{
			*r = printf(k, (m->value_int)[0]);
		}
	}
	else if (m->type_define == T_STRING)
	{
		char* k2 = "%s]\n";
		k = "%s\n";
		if (m->size > 1)
		{
			k = "%s,";
			*r = printf("%s(%d)=[", m->type_define->type_name, m->size);
			for (int ms = 0; ms < m->size - 1; ms++)
			{
				*r = printf(k, (m->value_str_ptr)[ms]);
			}
			*r = printf(k2, (m->value_str_ptr)[m->size - 1]);
		}
		else
		{
			*r = printf(k, *m->value_str_ptr);
		}
	}
	else if (m->type_define == T_LONG)
	{
		for (int i = 0; i < m->size; i++)
		{
			k = "%lu\n";
			*r = printf(k, (m->value_long)[i]);
		}
	}
	else if (m->type_define == T_CHAR)
	{
		for (int i = 0; i < m->size; i++)
		{
			k = "%c\n";
			*r = printf(k, m->value_char_ptr[i]);
		}
	}
	else if (m->type_define == T_FLOAT)
	{
		for (int i = 0; i < m->size; i++)
		{
			k = "%f\n";
			*r = printf(k, *m->value_float);
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


//for string only now char
/*
void index_(fcall* y)
{
	bool iarray = false;
	const int index = *(y->context == NULL ? y->func_parmeters.root->stack_next : y->func_parmeters.root)->value_int;
	var* context = y->context != NULL ? y->context : (y->func_parmeters.size == 2) ? y->func_parmeters.root : NULL;
	if (context->var_type->name == T_ARRAY->name)
	{
		context = context->base_type;
		iarray = true;
	}
	if (context->var_type-.type_id == T_STRING-.type_id)
	{
		if (iarray)
		{
			char** ys = (char**)context->val_str_ptr;
			y->func_return.values = ys + index;
		}
		else
		{
			char* ys = *(char**)context->val_str_ptr;
			int n = strlen(ys);

			if (index < n)
				y->func_return.values = ys + index;
		}
	}
	else if (context->var_type-.type_id == T_INT-.type_id)
	{
		int* ys = (int*)context->value_int;


		if (index < context->size)
			y->func_return.values = ys + index;
	}
	else
	{
		type_def* ys = (type_def*)context->value_type;


		if (index < context->size)
			y->func_return.values = ys + index;
	}
}
*/
void xreplace(func_deftion* y)
{
	//	str
	//char**ys= (char**)y->context->value;
	//	int* n =new int(strlen(*ys));
	//y->func_return.value=n;
}

typedef type_def* (ty)(type_def*);

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

int get_index_value(fcall* funct, node* k)
{
	if (k->next->btype.node_type_bit.s_index)
	{
		var* atx = new_temp_var(T_INT);
		calculate4(atx, k, s_index_c, funct);
		return *atx->value_int;
	}
	return 0;
}

var* get_type_inc_obj_var(node** mnode, var* called_object, func_deftion** outp)
{
	var* fvar = NULL;
	if (called_object != NULL)
	{
		fvar = get_var_by_name_on_stack((*mnode)->value_char_ptr, &called_object->value_type_instsance->propertys);
	}
	else
	{
		fvar = get_globle_var_by_name((*mnode)->value_char_ptr);
	}

	if (fvar == NULL)
	{
		printf("error");
		exit(-1);
	}
	while ((*mnode)->next != NULL && (*mnode)->next->type_ == dot)
	{
		*mnode = (*mnode)->next->next;
		var* base = fvar;
		if ((*mnode)->opt_name_type == var_call)
		{
			fvar = get_var_by_name_on_stack((*mnode)->value_char_ptr, &fvar->value_type_instsance->propertys);
		}

		if ((*mnode)->opt_name_type == function_call)
		{
			func_deftion* func1 = get_obj_function(base, (*mnode)->value_char_ptr);
			if (func1 != NULL)
			{
				*outp = func1;
				return base;
			}
		}
	}

	return fvar;
}

var* get_type_inc_obj_var2(node** mnode, var* called_object, fcall* cfanc, func_deftion** outp)
{
	var* fvar = NULL;
	if (cfanc != NULL)
	{
		fvar = fget_var_by_name_fc((*mnode)->value_char_ptr, cfanc);
	}
	else if (called_object != NULL) // inc class
	{
		fvar = get_var_by_name_on_stack((*mnode)->value_char_ptr, &called_object->value_type_instsance->propertys);
	}
	else
	{
		fvar = get_globle_var_by_name((*mnode)->value_char_ptr);
	}

	if (fvar == NULL)
	{
		printf("error: var %s not defined on line %d  on\nC: %s:%d\n", (*mnode)->value_char_ptr, (*mnode)->line,
		       __FILE__, __LINE__);
		exit(-1);
	}
	while ((*mnode)->next != NULL && (*mnode)->next->type_ == dot)
	{
		*mnode = (*mnode)->next->next; //after .
		var* base = fvar;
		if ((*mnode)->opt_name_type == var_call)
		{
			fvar = get_var_by_name_on_stack((*mnode)->value_char_ptr, &fvar->value_type_instsance->propertys);
		}

		if ((*mnode)->opt_name_type == function_call)
		{
			func_deftion* func1 = get_obj_function(base, (*mnode)->value_char_ptr);
			if (func1 != NULL)
			{
				//fcall* s= create_fcall(func1);
				//node * m =setup_function_parms(mnode,s,called_object,cfanc);
				//call_function(s,)
				*outp = func1;
				return base;
			}
		}
	}

	return fvar;
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

node* calculate(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
                var* calc_result)
{
	//bool is_mem_set = false;
	var* last_var_name = NULL;
	int f = 0;
	node* mnode = (node*)cnode;
	type_def* saved_return_type = calc_result->type_define;

	int in = 0;
	char* op = NULL;

	if (calc_result->size == 0)
	{
		calc_result->size = 1;
	}
	bool indexx = false;
	void* memory = NULL;

	int i = 0;

	while ((mnode->btype.value & (node_type)0xc043) == 0)
	{
		struct var* name_var = NULL;
		if ((stop_in_type != 0 && mnode->type_ == stop_in_type) || (stop_in_node != NULL && mnode == stop_in_node))
			break
				;
		else if (mnode->btype.value & (value | var_name | parentheses4))
		{
			if (mnode->type_ == parentheses4) // (      expr      )
			{
				name_var = new_temp_var(NULL);
				mnode = calculate(mnode->next, calling_function, calling_object, (node_type)0, get_close_part(mnode),
				                  name_var);
				//continue;
			}
			else if (mnode->type_ == var_name)
			{
				func_deftion* name_function = NULL;
				if (mnode->opt_name_type == function_call)
				{
					name_function = get_func_by_name(mnode->value_char_ptr);
				}
				else if (mnode->opt_name_type == var_call && calling_function != NULL)
				{
					name_var = get_type_inc_obj_var2(&mnode, calling_object, calling_function, &name_function);
				}
				else if (mnode->opt_name_type == var_call)
				{
					name_var = get_globle_var_by_name(mnode->value_char_ptr);
					if (mnode->next->type_ == dot)
					{
						name_var = get_type_inc_obj_var(&mnode, name_var, &name_function);
					}
				}
				//get value from function
				if (name_var != NULL && name_function == NULL)
				{
					if (mnode->next->btype.node_type_bit.s_index)
					{
						node* close = get_close_part(mnode->next);
						var* ind = new_temp_var(T_INT);

						calculate(mnode->next->next, calling_function, calling_object, (node_type)0, close, ind);
						mnode = close;
						in = *ind->value_int;
						free_temp_var(ind);
						calc_result->size = 1;
						indexx = true;
					}
					if (f == 1)
					{
						++name_var->value_int[in];
						f = 0;
					}
				}
				//get value from var
				else if (name_function != NULL)
				{
					//node* y = get_close_part(get_first_type(mnode, parentheses4)); ///1
					fcall* fcall = create_fcall(name_function);
					//compile_var_name_start(k, funct);
					//compile(mvar, k, funct, y);
					setup_function_parms(&mnode, fcall, calling_object == NULL ? name_var : calling_object,
					                     calling_function);
					call_function(fcall, calling_object == NULL ? &name_var : &calling_object);


					var* rx = malloc(sizeof(var));
					memcpy(rx, &fcall->_return, sizeof(var));
					//mvar->values= fun->func_return.values;
					name_var = rx;
				}
				else
				{
					printf("ERROR: var %s on line %d is not defined\n", (char*)mnode->value_raw, mnode->line);
					exit(-1);
					return NULL;
				}

				last_var_name = name_var;
			}
			else if (mnode->type_ == value)
			{
				name_var = new_temp_var((type_def*)mnode->opt_type_ptr);

				name_var->values = install_memory_with_type(mnode->opt_type_ptr, 1);
				set_value_copy_node(name_var, mnode);
			}

			if (op == NULL) //
			{
				setup_t(calc_result, &saved_return_type, indexx, name_var);
				if (memory == NULL)
					memory = calc_result->values == NULL ? install_memory(calc_result) : calc_result->values;
				if (memory == NULL) //// T_FUNC
				{
					memory = name_var->values;
				}
				//copy array [int string]
				if (!indexx && calc_result->size > 1 && name_var->size > 1)
				{
					copy_array(calc_result, memory, name_var);
				}

				else if (calc_result->type_define == T_INT)
					((int*)memory)[i++] = name_var->value_int[in];

				else if (calc_result->type_define == T_FLOAT)
					((float*)memory)[i++] = name_var->value_float[in];

				else if (calc_result->type_define == T_LONG)
					((long*)memory)[i++] = name_var->value_long[in];

				else if (calc_result->type_define == T_CHAR)
					((char*)memory)[i++] = name_var->type_define != T_STRING
						                       ? name_var->value_char_ptr[in]
						                       : name_var->value_str_ptr[0][in];
				else if (calc_result->type_define == T_STRING)
				{
					char* val = name_var->value_str_ptr[in];
					((char**)memory)[i] = (char*)malloc(strlen(val) + 1);
					strcpy(((char**)memory)[i], val);
					i++;
				}
				else if (calc_result->type_define == T_BOOL)
					((bool*)memory)[i++] = name_var->value_bool[in];

				else if (!is_base_type(calc_result->type_define))
				{
					///FIXME:
					((type_instance*)memory)[i++] = name_var->value_type_instsance[in];
				}
				in = 0; //reset index
			}
			else //v2 op != NULL
			{
				if ((*op == '+' && *(op + 1) == '+') || (*op == '-' && *(op + 1) == '-'))
				{
				}
				if ((*op == '&' && *(op + 1) == '&') || (*op == '|' && *(op + 1) == '|'))
				{
					struct var* atx = new_temp_var(NULL);
					//node* end= calculate4(atx, k->parent->parent->parent, parse_obj::parentheses4c, funct);
					node* endm = calculate4(atx, get_first_type_backword_from(mnode, operators_n)->next, parentheses4_c,
					                        calling_function);
					name_var = atx;
					calc_result->type_define = T_BOOL;
					//k = getFirstType(k, parse_obj::endl);
					mnode = endm;
				}
				if (calc_result->type_define == T_INT)
				{
					//TODO:no need allready done by in
					//int y = get_index_value(funct, k);
					int* mx = (int*)memory + (i - 1);
					const int to = name_var->value_int[in];
					///math_opration(op,mx,to);
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_FLOAT)
				{
					const int y = get_index_value(calling_function, mnode);
					float* mx = (float*)memory + (i - 1);
					float to = name_var->type_define != NULL && name_var->type_define->type_name == T_INT->type_name
						           ? (float)name_var->value_int[y]
						           : name_var->value_float[y];
					//MATH_OPERATORS
				}
				else if (calc_result->type_define == T_LONG)
				{
					int y = get_index_value(calling_function, mnode);
					long* mx = (long*)memory + (i - 1);
					long to = (name_var->value_long)[y];
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_CHAR)
				{
					char* mx = &((char*)memory)[i - 1];
					char to = name_var->value_char_ptr[0];
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_BOOL)
				{
					//bool* mx = &((bool*)memory)[i - 1];
					bool* mx = ((bool*)memory) + (i - 1);
					bool to = *(bool*)name_var->values;
					BOOL_OPERATORS
				}
				else if (calc_result->type_define == T_STRING)
				{
					int y = in;
					char** mx = &((char**)memory)[i - 1];
					char* to = name_var->value_str_ptr[y];
					//char* mtype = mb == NULL ? (char*)k->opt : (char*)mb->var_type->name;

					if (*op == '+')
					{
						//strcat(mx,to);
						long mxlen = strlen(*mx);
						long tolen = strlen(to);
						char* exp = (char*)realloc(*mx, mxlen + tolen + 1);


						strcat_s(exp, mxlen + tolen + 1, to);
						*mx = exp;
						if (exp)
						{
						}
						else
						{
							// deal with realloc failing because memory could not be allocated.
						}
					}
					in = 0;
				}
				else // type_inctance
				{
					//int y = get_index_value(funct, k);
					/*
					type_instance* typ_ = (type_instance*)memory + (i - 1);
					int to = name_var->value_int[in];
					var* x = new_temp_var(T_INT);
					func_deftion* uy = get_obj_function(x, "add");
					fcall* fcall1 = create_fcall(uy);
					fcall1->context = calling_object;
					fcall1->func_parmeters[0].value_int = &to;
					//node* ty = NULL;
					call_function(fcall1, &calling_object);
					//char* mtype = mb == NULL ? (char*)k->opt : (char*)mb->var_type->name;
					*/
				}
				op = NULL;

				///TO DO :why
				//free(mvar->value);
			}
		}
		else if (mnode->type_ == operators_n || is_double_equle(mnode))
		{
			if (is_double_oprater(mnode))
			{
				op = (char*)malloc(3);
				op[0] = *mnode->value_char_ptr;
				op[1] = *mnode->next->value_char_ptr;
				op[3] = '\0';
				mnode = mnode->next;
			}
			else
			{
				op = mnode->value_char_ptr;
			}
		}
		else ///error no unacceable
		{
			//printf("error cal");
			//exit(-1);
		}
		mnode = mnode->next;
	}

	if (saved_return_type != NULL)
	{
		calc_result->type_define = saved_return_type;
	}

	if (calc_result->values == NULL)
	{
		calc_result->values = memory;
	}
	else
	{
		calc_result->values = memory;
	}

	return mnode;
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
		.stack_next = NULL
	}

};
func_stack base_function = {.top = simple_function_array + 12, .size = 13, .root = simple_function_array + 0};

var tinfo[6] = {
	{.name = "_int", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 1},
	{.name = "_string", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 2},
	{.name = "_char", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 3},
	{.name = "_bool", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 4},
	{.name = "_long", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = tinfo + 5},
	{.name = "_object", .type_define = T_TYPE_INFO, .values = T_INT, .size = 1, .stack_next = 0}
};


type_instance tnc = {
	.functions = {
		.top = simple_function_array + 12, .size = 13, .root = simple_function_array + 0
	},
	.size = 1, .type = T_OBJECT,
	.propertys = {.size = 6, .top = tinfo + 5, .root = tinfo + 0, .stack_holder = 0}
};
var start_var[] = {
	{
		.name = "xlang", .type_define = T_OBJECT, .value_type_instsance = &tnc,
		.size = 1, .holder = NULL, .base_type = 0, .ref = 0, .stack_next = 0, .access = PUBLIC
	}
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

		ret = new_var(NULL, T_LONG);
		ret->value_long = name->value_long + index;
		break;
	case t_string:
		{
			if (name->size > 1)
			{
				ret = new_var(NULL, T_STRING);
				ret->value_str_ptr = name->value_str_ptr + index;
			}
			else
			{
				ret = new_var(NULL, T_CHAR);
				ret->value_char_ptr = *name->value_str_ptr + index;
			}
			break;
		}
	case t_char:
		ret = new_var(NULL, T_CHAR);
		ret->value_str_ptr = name->value_str_ptr + index;
		break;
	case t_int:
		ret = new_var(NULL, T_INT);
		ret->value_int = name->value_int + index;
		break;
	case t_bool:
		ret = new_var(NULL, T_BOOL);
		ret->value_bool = name->value_bool + index;
		break;
	case t_float:
		ret = new_var(NULL, T_FLOAT);
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
		mnode->parent == stop_in_node)))return true;
	return false;
}
