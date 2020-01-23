#include "functions.h"
#include <string.h>
#include <time.h>
#include "http.h"

//#define F

#ifdef F
//#define next next_debug(__FUNCTION__)

#endif
/*
 * for (int i = 1; i < 2; i++) //base function for base types
	{
		func* t = new_func_on_stack(&(SIMPLE_TYPE+ i)->functions);
		t->func_name = "len";
		t->func_return.var_type = T_INT;
		t->func_code = &len;
		///////////////////////////////
		t = new_func_on_stack(&(SIMPLE_TYPE+ i)->functions);
		t->func_name = "index";
		var* u = new_var_on_stack(&t->fun_p, "a",T_INT);

		t->func_return.var_type = T_CHAR;
		t->func_code = &index_;
	}
	func* top;
	int size;
	func* root;


 */
void len(func* y);
void index_(func* y);
var t = {.var_type=T_INT, .name="a"};
func xd;
func x = {
	.fun_p={0}, .func_code= &len, .func_name="len", .function_type=f_main, .func_return={.var_type=T_INT}, .ref=0,
	.context=0, .stack_next=&xd
};
func xd = {
	.fun_p={.root=&t, .top=&t, .size=1}, .func_code= &index_, .func_name="index", .function_type=f_main,
	.func_return={.var_type=T_INT}, .ref=0, .context=0,/* func* STACK_NEXT*/0
};
type_stack akak = {.top=T_NEW, .root=T_LONG, .size=8};
type SIMPLE_TYPE[8] = {
	{.id=0,.name="long", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_STRING},
	{.id=1,.name="string", .propertys={0}, .functions={.root=&x, .top=&xd, 2}, .base=0, .context=0, .stack_next=T_CHAR},
	{.id=2,.name="char", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_INT},
	{.id=3,.name="int", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_BOOL},
	{.id=4,.name="bool", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_FLOAT},
	{.id=5,.name="float", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_ARRAY},
	{.id=6,.name="_array", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=T_NEW},
	{.id=7,.name="new", .propertys={0}, .functions={0, 0, 0}, .base=0, .context=0, .stack_next=0},


};


extern fl staic_flag2[] = {
	{0,0}, {0,0}, {0,0}, {0,0}, {0,0},
	{0,0}, {0,0}, {0,0}, {0,0},
	{0,0}
};


extern int fi = 0;
#define INT_TYPE types->root

node* getRoot(node* j)
{
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

node* getFirstType_backword_from(node* in, node_type b)
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
	if (t & itype) S(y,"itype,");
	if (t & keyword)S(y,"keyword,");
	if (t & var_name)S(y,"var_name,");
	if (t & value)S(y,"value,");
	if (t & operators_n)S(y,"operators_n,");
	if (t & equles)S(y,"equles,");
	if (t & endl)S(y,"endl,");
	if (t & s_index)S(y,"s_index,");
	if (t & parentheses1)S(y,"parentheses1,");
	if (t & parentheses1c)S(y,"parentheses1c,");
	if (t & comma)S(y,"comma,");
	if (t & s_index_c)S(y,"s_index_c,");
	if (t & parentheses4)S(y,"parentheses4,");
	if (t & parentheses4c)S(y,"parentheses4c,");
	return y;
}


node* static_flag_op2(node_type mtype, node* w_node, bool added)
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
	short* ma = (short*)m;
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
	bool cont = false;
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

node* calculate4(var* ms, node* m, node_type z, func* km)
{
	//calculate(const node* m, func* funct, var* contxt, parse_obj stop, node* end, var* out)
	return calculate(m, km, 0, z, NULL, ms);
}

node* calculate3(var* ms, node* m, func* km)
{
	return calculate(m, km, NULL, (node_type)0, NULL, ms);
}


node* getFirstType_with_value(node* in, node_type b, void* value)
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
				if ((int)value == (int)temp->value)
					return temp;
			}

			else if (strcmp((char*)temp->value, (char*)value) == 0)
				return temp;
		}
		len++;
		temp = temp->next;
	}
	return NULL;
}

node* getFirstType(node* in, node_type b)
{
	return getFirstType_with_value(in, b, NULL);
}

node* getLastType(node* in, const node_type b)
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


void do_work1(var* calc, var* me, int index)
{
	if (strcmp(me->var_type->name, "int") == 0)
	{
		me->value_int[index] = *calc->value_int;
	}
	else if (strcmp(me->var_type->name, "float") == 0)
	{
		me->value_float[index] = *calc->value_float;
	}
	else if (strcmp(me->var_type->name, "long") == 0)
	{
		me->value_long[index] = *calc->value_long;
	}
	else if (strcmp(me->var_type->name, "char") == 0)
	{
		me->value_char_ptr[index] = *calc->value_char_ptr;
	}
	else if (strcmp(me->var_type->name, "bool") == 0)
	{
		me->value_int[index] = *calc->value_int;
	}
	else if (strcmp(me->var_type->name, "string") == 0)
	{
		me->val_str_ptr[index] = *calc->val_str_ptr;
	}
	else
	{
		me->value_type[index] = *calc->value_type;
	}
}

void set_value(var* context, func* temp, node** cx)
{
	bool array_copy = false;
	if ((*cx)->next->type_ == equles ||
		(*cx)->next->type_ == s_index && getFirstType(*cx, s_index_c)->next->type_ == equles)
	{
		var* calc = new_temp_var(NULL);
		var* me = NULL;

		int index = 0;

		if (context != NULL)
		{
			me = fget_var_by_name(&((type*)context->value)->propertys,(*cx)->value_char_ptr);
		}
		else if (temp != NULL)
		{
			me = fget_var_by_name(&temp->fun_p, (*cx)->value_char_ptr);
		}

		if (me == NULL)
		{
			me = get_var_by_name((*cx)->value_char_ptr);
		}


		if ((*cx)->next->tp.s_index)
		{
			var* get_index = new_temp_var(T_INT);


			node* close = get_close_part((*cx)->next);
			calculate((*cx)->next->next, temp, context, (node_type)0, close, get_index);

			index = *(int*)get_index->value;
			free_temp_var(get_index);
			*cx = close;
		}
		else if (me->size > 1)
		{
			calc->size = me->size;
			array_copy = true;
		}
		
		if ((*cx)->next->type_ == equles)
		{
			calc->var_type = me->var_type;


			//c = calculate(res, c->next->next, temp);
			*cx = calculate((*cx)->next->next, temp, context, endl, NULL, calc);

			if (*cx == NULL)
			{
				return;
			}
		}
		if (array_copy)
		{
			free(me->value);
			me->value = calc->value;
			return;
		}
		do_work1(calc, me, index);
		if ((*cx)->type_ != endl)
		{
			*cx = getFirstType(*cx, endl);
		}
	}
}


bool var_bool_value(var* m)
{
	return *(bool*)m->value;
}

#define  START(index) if((index)==0) printf("["); else printf(",") ;


void type_print(type* u, int dep)
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
	printf("%stype id = %d \n", m, (int)u);
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
		for (func* yb = u->functions.root; yb != NULL; yb = yb->stack_next)
		{
			printf("%s  %s\n", m, yb->func_name);
		}
	}
	printf("%s}\n", m);
	printf("%s}\n", m);
	if (*m != '\0')
		free(m);
}

void vprint(func* temp)
{
	var* m = temp->fun_p.root;

	type_print(m->var_type, 0);

	return;
	printf("var_type->%s array: %s type id:%d \n", m->var_type->name, m->size > 1 ? "Yes" : "No", (int)m->var_type);
	type_print(m->var_type, 0);

	for (int ms = 0; ms < m->size; ms++)
	{
		START(ms)
		if (!is_base_type(m->var_type))
		{
			type* u = ((type*)m->value) + ms; //  m[ms]
			printf("type id:%d\n", (int)m->var_type);
			type_print(u, 1);
		}
		else
		{
			printf("base_type\n");
		}
	}
	printf("]\n");
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

void print_f(func* temp)
{
	//va_list a;
	void** y = (void**)malloc(sizeof(char**) * (temp->fun_p.size - 1));
	var* m = temp->fun_p.root;
	var* x = m;
	if (m->var_type->name == T_STRING->name)
	{
		for (int i = 0; i < temp->fun_p.size - 1; i++)
		{
			x = x->stack_next;
			*(y + i) = x->var_type->name == T_STRING->name ? *(char**)x->value : (void*)*(int*)x->value;
		}

		vprintf(*(char**)m->value, (va_list)y);


		free(y);
	}
}

void print(func* temp)
{
	if (temp->fun_p.size > 1)
	{
		print_f(temp);
		return;
	}
	var* m = temp->fun_p.root;
	int* r = (int*)malloc(sizeof(int));
	*r = -1;
	char* k;
	if (strcmp(m->var_type->name, "int") == 0)
	{
		char* k2 = "%d]\n";
		k = "%d\n";
		if (m->size > 1)
		{
			k = "%d,";
			*r = printf("%s(%d)=[", m->var_type->name, m->size);
			for (int ms = 0; ms < m->size - 1; ms++)
			{
				*r = printf(k, ((int*)m->value)[ms]);
			}
			*r = printf(k2, ((int*)m->value)[m->size - 1]);
		}
		else
		{
			*r = printf(k, ((int*)m->value)[0]);
		}
	}
	else if (strcmp(m->var_type->name, "string") == 0)
	{
		char* k2 = "%s]\n";
		k = "%s\n";
		if (m->size > 1)
		{
			k = "%s,";
			*r = printf("%s(%d)=[", m->var_type->name, m->size);
			for (int ms = 0; ms < m->size - 1; ms++)
			{
				*r = printf(k, ((char**)m->value)[ms]);
			}
			*r = printf(k2, ((char**)m->value)[m->size - 1]);
		}
		else
		{
			*r = printf(k, *(char**)m->value);
		}
	}
	else if (strcmp(m->var_type->name, "long") == 0)
	{
		k = "%lu\n";
		*r = printf(k, ((long*)m->value)[0]);
	}
	else if (strcmp(m->var_type->name, "char") == 0)
	{
		k = "%c\n";
		*r = printf(k, *(char*)m->value);
	}
	else if (strcmp(m->var_type->name, "float") == 0)
	{
		k = "%f\n";
		*r = printf(k, *(float*)m->value);
	}
	else
	{
		//TODO : print var to string 
		vprint(temp);
	}
	temp->func_return.value = r;
}

var* new_var(char* name, type* vtype)
{
	return new_var_on_stack(varss, name, vtype);
}

var* new_temp_var(type* typ)
{
	var* y = new_var_on_stack(t_varss,NULL, typ);

	return y;
}

type* new_type()
{
	type* local= new_type_stack(types);
	local->id = types->size -1 ;
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

void call_func_in(func* mfunc)
{
	//d->ref must be parth
	node* stop = get_close_part(mfunc->ref);
	//c = (node*)temp->func_code;
	compile(mfunc->context, mfunc->ref->parent, mfunc, stop);
	if (mfunc->function_type == constr)
	{
		mfunc->func_return.value = mfunc->context->value;
	}
}

void import(func* d)
{
	if (d->fun_p.root->var_type->name == T_STRING->name)
	{
		char* y = *(char**)d->fun_p.root->value;
		FILE* sf;
		errno_t se = fopen_s(&sf, y, "r");
		if (se == 0)
		{
			char* buf = get_filebuff(sf);
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

void time_(func* d)
{
	time_t a;
	time(&a);

	char* buff = (char*)malloc(sizeof(char) * 20);
	memset(buff, 0, 20);

	struct tm* tm_info = localtime(&a);;
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", tm_info);


	d->func_return.value = get_pptr_string(buff);
}

bool rxx = false;

void random(func* inc)
{
	if (!rxx)

	{
		srand(time(0));
		rxx = true;
	}

	int* x = (int*)malloc(sizeof(int));
	*x = 0;
	const int in = *(int*)inc->fun_p.root->value;
	if (inc->fun_p.root->var_type->name == T_INT->name)
	{
		*x = (rand() % in) + 1;
	}
	else
	{
		printf("Error: input is not a int");
	}
	inc->func_return.value = x;
}

void sin__(func* d)
{
	int* x = (int*)malloc(sizeof(int));
	if (d->fun_p.root->var_type->name == T_INT->name)
	{
		//	*x = sinf(*(float*)d->fun_p.root->value);
	}
	else
	{
		printf("Error: input is not string");
	}
	d->func_return.value = x;
}

void scan(func* d) ///xscan(var out,"%s");
{
	if (d->fun_p.root->var_type->name == T_STRING->name)
	{
		var* out = get_var_by_name(*(char**)d->fun_p.root->value);

		scanf_s("%d", (int*)out->value);


		//b++;
	}
	else
	{
		printf("Error: input is not string");
	}
}


void str(func* d)
{
	char* buff = NULL;
	if (d->fun_p.root->var_type->name == T_FLOAT->name)
	{
		float* u = (float*)d->fun_p.root->value;
		buff = (char*)malloc((sizeof(char)) * sizeof(float) * 4 + 1);
		sprintf(buff, "%f", *u);
		char* t = (char *)malloc(strlen(buff) + 1);
		strcpy(t, buff);

		d->func_return.value = get_pptr_string(t);
	}
	else if (d->fun_p.root->var_type->name == T_INT->name)
	{
		int* u = (int*)d->fun_p.root->value;
		buff = (char*)malloc((sizeof(char)) * sizeof(int) * 4 + 1);
		sprintf(buff, "%d", *u);
		char* t = (char *)malloc(strlen(buff) + 1);
		strcpy(t, buff);


		d->func_return.value = get_pptr_string(t);
	}
	else if (d->fun_p.root->var_type->name == T_CHAR->name)
	{
		char* u = (char*)d->fun_p.root->value;
		char* ubuff = (char*)malloc(sizeof(char) + 1);
		memset(ubuff, 0, 2);
		*ubuff = *u;


		d->func_return.value = get_pptr_string(ubuff);
	}
	else
	{
		printf("Error: input is not string");
	}
	if (buff)free(buff);
}


func* add_function_gloable(char* name, type* return_type, int pcount, const def_function fe, char** par_name,
                           type** par_type)
{
	return add_function(funcs, name, return_type, pcount, fe, par_name, par_type);
}

func* add_function(func_stack* s, char* name, type* return_type, int pcount, const def_function fe, char** par_name,
                   type** par_type)
{
	func* nx2 = new_func_on_stack(s);

	nx2->func_code = fe;
	nx2->func_return.var_type = return_type;
	nx2->func_name = name;
	var_stack* y = &nx2->fun_p;

	int u = 0;
	type** a = par_type;
	for (char** c = par_name; u < pcount; c++, a++)
	{
		var* v = new_var_on_stack(y, *c, *a);


		u++;
	}

	return nx2;
}

static char* parm_name[1] = {"a"};
static type* parm_type[1] = {T_STRING};
static type* parm_type_num[1] = {T_INT};

void install_default_functions()
{

	funcs=&base_function;
	return;
	add_function_gloable("print", T_INT, 0, &print, NULL, NULL);
	add_function_gloable("vprint", T_INT, 0, &vprint, NULL, NULL);
	add_function_gloable("xprint", T_INT, 0, &print, NULL, NULL);
	add_function_gloable("time_", T_STRING, 1, &time_, parm_name, parm_type);
	add_function_gloable("xtime", T_STRING, 1, &time_, parm_name, parm_type);
	add_function_gloable("scan", T_INT, 1, &scan, parm_name, parm_type);
	add_function_gloable("xscan", T_INT, 1, &scan, parm_name, parm_type);
	add_function_gloable("sin", T_INT, 1, &sin__, parm_name, parm_type_num);
	add_function_gloable("xsin", T_INT, 1, &sin__, parm_name, parm_type_num);
	add_function_gloable("random", T_INT, 1, &random, parm_name, parm_type_num);
	add_function_gloable("xrandom", T_INT, 1, &random, parm_name, parm_type_num);
	add_function_gloable("str", T_STRING, 1, &str, parm_name, parm_type_num);
	add_function_gloable("xstr", T_STRING, 1, &str, parm_name, parm_type_num);
	///add_function_gloable("http", T_STRING, 4, &http, s_parm_name, s_parm_type);
	///add_function_gloable("xhttp", T_STRING, 4, &http, s_parm_name, s_parm_type);
	add_function_gloable("import", T_INT, 1, &import, s_parm_name, parm_type);
	add_function_gloable("ximport", T_INT, 1, &import, s_parm_name, parm_type);
}

void add_type(char* name)
{
	type* mt = new_type();
	mt->name = name;
}

void len(func* y)
{
	if (y->context->var_type->name == T_STRING->name)
	{
		char** ys = (char**)y->context->value;
		int* n = (int*)malloc(strlen(*ys));
		y->func_return.value = n;
	}
	if (y->context->var_type->name == T_ARRAY->name)
	{
		y->func_return.value = (int*)malloc(y->context->base_type->size);
	}

	else
	{
		printf("exp:::");
		///TODO:
	}
}


//for string only now char
void index_(func* y)
{
	bool iarray = false;
	const int index = *(int*)(y->context == NULL ? y->fun_p.root->stack_next : y->fun_p.root)->value;
	var* context = y->context != NULL ? y->context : (y->fun_p.size == 2) ? y->fun_p.root : NULL;
	if (context->var_type->name == T_ARRAY->name)
	{
		context = context->base_type;
		iarray = true;
	}
	if (context->var_type->name == get_type_by_name((char*)"string")->name)
	{
		if (iarray)
		{
			char** ys = (char**)context->value;
			y->func_return.value = ys + index;
		}
		else
		{
			char* ys = *(char**)context->value;
			int n = strlen(ys);

			if (index < n)
				y->func_return.value = ys + index;
		}
	}
	else if (context->var_type->name == T_INT->name)
	{
		int* ys = (int*)context->value;


		if (index < context->size)
			y->func_return.value = ys + index;
	}
	else
	{
		type* ys = (type*)context->value;


		if (index < context->size)
			y->func_return.value = ys + index;
	}
}

void xreplace(func* y)
{
	//	str
	//char**ys= (char**)y->context->value;
	//	int* n =new int(strlen(*ys));
	//y->func_return.value=n;
}

typedef type* (ty)(type*[8]);

void install_default_types()
{
	types = &akak;
	//for (int i = 0; i < 8; i++)
	//	{

	//	add_type(base_types_name[i]);
	//SIMPLE_TYPE[i] = types->top;
	//	}
	/*
	for (int i = 1; i < 2; i++) //base function for base types
	{
		func* t = new_func_on_stack(&(SIMPLE_TYPE + i)->functions);
		t->func_name = "len";
		t->func_return.var_type = T_INT;
		t->func_code = &len;
		///////////////////////////////
		t = new_func_on_stack(&(SIMPLE_TYPE + i)->functions);
		t->func_name = "index";
		var* u = new_var_on_stack(&t->fun_p, "a",T_INT);

		t->func_return.var_type = T_CHAR;
		t->func_code = &index_;
	}
	*/
	HMODULE load_library_a = LoadLibraryA("httplib_dy.dll");
	if (load_library_a != NULL)
	{
		FARPROC proc_address = GetProcAddress(load_library_a, "get_type");
		type* nt = ((ty*)proc_address)(SIMPLE_TYPE);

		func* xz = add_function_gloable(nt->functions.root->func_name, nt, 0, nt->functions.root->func_code,NULL, NULL);
		//func* temp = copy_func(nt->functions.root);
		//temp->func_name = nt->functions.root->func_name;
		//func_stack_push(funcs, temp);
		xz->function_type = constr;

		type_stack_push(types, nt);
		type_print(nt, 0);
	}
}


func* new_func()
{
	return new_func_on_stack(funcs);
}

type* get_type_by_name(char* name)
{
	type_stack* vs = types;
	//if (name == NULL) return NULL;

	//if (vs->size > 0)
	for (type* i = vs->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->name) == 0)
			return i;
	}
	return NULL;
}

func* get_func_by_name(char* name)
{
	for (func* i = funcs->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->func_name) == 0)
		{
			return copy_func(i);
		}
	}
	return NULL;
}

func* copy_func(func* i)
{
	func* m = new_func_on_stack(t_funcs);
	var_stack* malloc1 = (var_stack*)malloc(sizeof(var_stack));
	memset(malloc1, 0, sizeof(var_stack));

	m->fun_p = *malloc1;
	for (var* v = i->fun_p.root; v != NULL; v = v->stack_next)
	{
		var* vb = new_var_on_stack(&m->fun_p, v->name, v->var_type);
	}
	m->func_return = *new_temp_var(i->func_return.var_type);
	m->ref = i->ref;
	m->context = i->context;
	m->function_type = i->function_type;
	m->func_code = i->func_code;


	return m;
}

func* get_func_by_name_with_var(var* a, char* name)
{
	if (NULL == a) return get_func_by_name(name);

	///if (name == NULL) return NULL;


	for (func* i = a->var_type->functions.root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->func_name) == 0)
		{
			return copy_func(i);
		}
	}
	return get_func_by_name(name);
}

var* get_var_by_name(char* name)
{
	for (var* i = varss->root; i != NULL; i = i->stack_next)
	{
		if (strcmp(name, i->name) == 0)
			return i;
	}
	return NULL;
}

var* fget_var_by_name(var_stack* y, char* name)
{
	var* v = get_var_by_name(name);
	if (NULL == y) return get_var_by_name(name);

	if (v == NULL)
		for (var* i = y->root; i != NULL; i = i->stack_next)
		{
			if (strcmp(name, i->name) == 0)
				return i;
		}
	//if(y->m_class!=NULL)
	//printf("error: class %s has no mamber name %s \n",y->m_class->name,name);
	return get_var_by_name(name);
}

bool is_base_type1(type* t)
{
	type* u = types->root;
	int x = 0;

	while (x < 5)
	{
		if (u == t)
			return true;
		u = u->stack_next;
		x++;
	}

	return false;
}

bool is_base_type(type* t)
{
	type* u = types->root;
	int x = 0;

	while (x < 7)
	{
		if (u->name == t->name)
			return true;
		u = u->stack_next;
		x++;
	}

	return false;
}

void* install_memory(var* n)
{
	type* y = n->var_type;
	if (n->size > 1)
	{
		void* yyy = install_memory_with_type(y, n->size);
		type* ntype = copy_type_a(n->var_type, true);
		type* array_type = copy_type_a(T_ARRAY, true);
		var* array_obj = new_var_on_stack(&ntype->propertys, "array", array_type);
		func* type_func = new_func_on_stack(&ntype->functions);
		func* array_func = new_func_on_stack(&array_type->functions);

		type_func->func_name = "aindex";
		array_func->func_name = "index";

		new_var_on_stack(&type_func->fun_p, "a",T_INT);
		new_var_on_stack(&array_func->fun_p, "a",T_INT);

		type_func->func_return.var_type = array_func->func_return.var_type = y;
		type_func->func_code = array_func->func_code = &index_;


		add_function(&ntype->functions, "alen", T_INT, 0, &len, NULL, NULL);
		add_function(&array_type->functions, "len", T_INT, 0, &len, NULL, NULL);
		array_obj->base_type = n; //u


		n->var_type = ntype;


		return yyy;
	}
	return install_memory_with_type(y, n->size);
}

void copy_type(type* src, type* dstn, bool func_cpy)
{
	memcpy(dstn, src, sizeof(type));

	type* n_type = dstn;

	var_stack* xp = (var_stack*)malloc(sizeof(var_stack));
	var_stack_init(xp);

	//printf("\ncopy %s %s\n",src->w==child?"inc type":"super type",src->name);
	if (src->propertys.size > 0)
	{
		for (var* x = src->propertys.root; x != NULL && x->name != NULL; x = x->stack_next)
		{
			if (strcmp(x->name, "this") != 0)
			{
				var* y = new_var_on_stack(xp, x->name, x->var_type);

				y->size = x->size;


				if (x->access == STATIC)
				{
					y->value = x->value;
				}
				else
				{
					if (x->value != NULL && is_base_type(x->var_type))
					{
						y->value = install_memory_with_type(x->var_type, x->size);

						set_value_copy_var(y, x);
					}
					else
					{
						y->value = install_memory_with_type(x->var_type, x->size);
					}
				}
			}
		}
		if (func_cpy)
		{
			func_stack* fn = (func_stack*)malloc(sizeof(var_stack));
			func_stack_init(fn);
			for (func* x = src->functions.root; x != NULL && x->func_name != NULL; x = x->stack_next)
			{
				func* fv = new_func_on_stack(fn);
				memcpy(fv, x, sizeof(func) - 4);
			}

			n_type->functions = *fn;
		}
	}
	xp->m_class = n_type;
	n_type->propertys = *xp;
	n_type->w = child;
	var* y = new_var_on_stack(&n_type->propertys, "this", n_type);

	y->value = n_type;
}

type* copy_type_a(type* src, bool func_cpy)
{
	type* dstn = (type*)malloc(sizeof(type));
	copy_type(src, dstn, func_cpy);
	return dstn;
}

void* install_memory_with_type(type* tc, const int s)
{
	void* r = NULL;
	if (tc == NULL) return r;
	if (T_INT == tc)
	{
		r = (int*)malloc(sizeof(int) * s);
		memset(r, 0, sizeof(int) * s);
	}


	else if (T_LONG == tc)
		r = (long*)malloc(sizeof(long) * s);

	else if (T_CHAR == tc)
		r = (char*)malloc(sizeof(char) * s);
	else if (T_FLOAT == tc)
		r = (float*)malloc(sizeof(float) * s);
	else if (T_STRING == tc)
		r = (char**)malloc(sizeof(char*) * s);
	else if (T_BOOL == tc)
		r = (bool*)malloc(sizeof(int) * s);

	else
	{
		type* n_copy = (type*)malloc(sizeof(type) * s);
		memset(n_copy, 0, sizeof(type));
		n_copy->w = child;
		type* hadow = n_copy;
		for (int j = 0; j < s; j++)
		{
			copy_type(tc, hadow++, false);
		}

		r = n_copy;
	}

	return r;
}

int get_index_value(func* funct, node* k)
{
	if (k->next->tp.s_index)
	{
		var* atx = new_temp_var(T_INT);
		calculate4(atx, k, s_index_c, funct);
		return *(int*)atx->value;
	}
	return 0;
}

var* get_obj_var(node** nop, var* context, func** outp)
{
	node* no = *nop;
	func* out = *outp;
	var* tt = NULL;
	//	if (context!=NULL&&eql((char*)no->value, "this") == 1)
	//		tt = context;
	tt = fget_var_by_name(context != NULL ? &context->value_type->propertys : NULL, (char*)no->value);
	// c.v
	var* save = tt;
	//if(no->next->next!=NULL&&no->next->next->next!=NULL&&no->next->next->next->type_==dot)
	while (no->next->type_ == dot)
	{
		no = no->next->next;
		save = tt;
		tt = fget_var_by_name(
			(tt != NULL)
				? (!(is_base_type(tt->var_type) || tt->size > 1) ? &(context->value_type->propertys) : &tt->var_type->propertys)
				: NULL,
			(char*)no->value);
		if (tt == NULL && save != NULL)
		{
			func* i = get_func_by_name_with_var(save, (char*)no->value);
			if (i != NULL)
			{
				*outp = i;
				return save;
			}
		}
	}

	return tt;
}


void setup_t(var* out, type** saved_return_type, bool indexx, var* mvar)
{
	if (out->var_type == NULL && mvar->var_type != NULL)
	{
		out->var_type = mvar->var_type;
		if (!indexx) //TEST
		{
			out->size = mvar->size;
		}
	}
	else if (out->var_type != NULL)
	{
		if (mvar->var_type != out->var_type)
		{
			*saved_return_type = out->var_type;
			out->var_type = mvar->var_type;
		}
	}
	if (indexx && mvar->size == 1 && mvar->var_type == T_STRING)
	{
		out->var_type = T_CHAR;
	}
}

bool copy_array(var* out, void* memory, var* mvar)
{
	if (out->size != mvar->size)
	{
		printf("ERROR: size mismatch  %s[%d]\n", mvar->name, mvar->size);
		return true;
	}

	if (out->var_type->name == T_STRING->name)
	{
		int u = 0;
		char** g = (char**)mvar->value;
		while (u < mvar->size)
		{
			((char**)memory)[u] = (char*)malloc(strlen(*g) + 1);
			strcpy(((char**)memory)[u], *g);
			u++;
			g++;
		}
	}
	else if (out->var_type->name == T_INT->name)
	{
		int* g = (int*)mvar->value;
		memcpy(memory, g, sizeof(int) * mvar->size);
	}
	else if (!is_base_type(out->var_type))
	{
		//int* g = (int*)mvar->value;
		//memcpy(memory, g, sizeof(int) * mvar->size);
		int u = 0;
		type* g = (type*)mvar->value;
		type* ct = (type*)memory;
		while (u < mvar->size)
		{
			copy_type(g++, ct++, false);
			u++;
		}
	}
	return false;
}

node* calculate(node* m, func* funct, var* contxt, node_type stop, node* end, var* out)
{
	bool is_mem_set = false;
	var* uu = NULL;
	int f = 0;
	node* k = (node*)m;
	type* saved_return_type = NULL;
	int siz;
	int in = 0;
	char* op = NULL;

	if (out->size == 0)
	{
		out->size = 1;
	}
	bool indexx = false;
	void* memory = NULL;

	int i = 0;

	while ((k->btype.value & (node_type)0xc043) == 0)
	{
		struct var* mvar = NULL;
		if ((stop != 0 && k->type_ == stop) || (end != NULL && k == end))break;
		//..CAST..

		if (k->btype.value & (value | var_name | parentheses4))
		{
			/* ********CAST******	
				 if (k->type_ == parentheses4 && k->next->type_ == itype)
				{
					mvar = new_temp_var((type*)k->next->value);
					k = calculate(get_close_part(k)->next, funct, contxt, stop, end, mvar);
				}
				******************************
				*/
			if (k->type_ == parentheses4)
			{
				mvar = new_temp_var(NULL);
				k = calculate(k->next, funct, contxt, (node_type)0, get_close_part(k), mvar);
			}
			else if (k->type_ == var_name)
			{
				func* fun = NULL;
				if (k->opt != NULL && *(char*)k->opt == 'a')
				{
					fun = get_func_by_name((char*)k->value);
				}
				else
				{
					mvar = get_obj_var(&k, contxt, &fun);
					if (mvar == NULL && fun == NULL)
					{
						mvar = fget_var_by_name(&funct->fun_p, (char*)k->value);
					}
				}


				if (mvar != NULL && fun == NULL)
				{
					if (k->next->tp.s_index)
					{
						node* close = get_close_part(k->next);
						var* ind = new_temp_var(T_INT);

						calculate(k->next->next, funct, contxt, (node_type)0, close, ind);
						k = close;
						in = *(int*)ind->value;
						free_temp_var(ind);
						out->size = 1;
						indexx = true;
					}
					if (f == 1)
					{
						++*((int*)mvar->value + in);
						f = 0;
					}
				}
				else // (mb == NULL)
				{
					if (fun == NULL)
					{
						printf("ERROR: var %s on line %d is not defined\n", (char*)k->value, k->line);
						exit(-1);
						return NULL;
					}
					node* y = get_close_part(getFirstType(k, parentheses4)); ///1
					//compile_var_name_start(k, funct);
					//compile(mvar, k, funct, y);
					setup_function_parms(&k, fun, contxt == NULL ? mvar : contxt, funct);
					call_function(fun, contxt == NULL ? &mvar : &contxt);


					var* rx = malloc(sizeof(var));
					memcpy(rx, &fun->func_return, sizeof(var));

					mvar = rx;
					fun->func_return.value = NULL;
				}
				uu = mvar;
			}
			else if (k->type_ == value)
			{
				mvar = new_temp_var((type*)k->opt);

				mvar->value = install_memory_with_type((type*)k->opt, 1);
				set_value_copy_node(mvar, k);
			}

			if (op == NULL)
			{
				is_mem_set = true;

				setup_t(out, &saved_return_type, indexx, mvar);
				if (memory == NULL)
					memory = out->value == NULL ? install_memory(out) : out->value;
				//copy array [int string]
				if (!indexx && out->size > 1 && mvar->size > 1)
				{
					copy_array(out, memory, mvar);
				}

				else if (out->var_type->name == T_INT->name)
					((int*)memory)[i++] = *((int*)mvar->value + in);

				else if (out->var_type->name == T_FLOAT->name)
					((float*)memory)[i++] = *((float*)mvar->value + in);

				else if (out->var_type->name == T_LONG->name)
					((long*)memory)[i++] = *((long*)mvar->value + in);

				else if (out->var_type->name == T_CHAR->name)
					((char*)memory)[i++] = mvar->var_type != T_STRING
						                       ? *(char*)mvar->value + in
						                       : *(*(char**)mvar->value + in);
				else if (strcmp(out->var_type->name, "string") == 0)
				{
					char* val = *((char**)mvar->value + in);
					((char**)memory)[i] = (char*)malloc(strlen(val) + 1);
					strcpy(((char**)memory)[i], val);
					i++;
				}
				else if (out->var_type->name == T_BOOL->name)
					((bool*)memory)[i++] = *((bool*)mvar->value + in);

				else
				{
					///FIXME: 
					((type*)memory)[i++] = *(mvar->value_type + in);
				}
			}
			else //v2 op != NULL
			{
				if ((*op == '+' && *(op + 1) == '+') || (*op == '-' && *(op + 1) == '-'))
				{
				}
				if ((*op == '&' && *(op + 1) == '&') || (*op == '|' && *(op + 1) == '|'))
				{
					struct var* atx = new_temp_var(NULL);
					//node* end= calculate(atx, k->parent->parent->parent, parse_obj::parentheses4c, funct);
					node* endm = calculate4(atx, getFirstType_backword_from(k, operators_n)->next, parentheses4c,
					                        funct);
					mvar = atx;
					out->var_type = T_BOOL;
					//k = getFirstType(k, parse_obj::endl);
					k = endm;
				}
				if (out->var_type->name == T_INT->name)
				{
					//TODO:no need allready done by in
					//int y = get_index_value(funct, k);
					int* mx = (int*)memory + (i - 1);
					const int to = *((int*)(mvar->value) + in);
					///math_opration(op,mx,to);
					MATH_OPERATORS
				}
				else if (out->var_type->name == T_FLOAT->name)
				{
					const int y = get_index_value(funct, k);
					float* mx = (float*)memory + (i - 1);
					float to = mvar->var_type != NULL && mvar->var_type->name == T_INT->name
						           ? (float)*((int*)(mvar->value) + y)
						           : *((float*)(mvar->value) + y);
					MATH_OPERATORS
				}
				else if (out->var_type->name == T_LONG->name)
				{
					int y = get_index_value(funct, k);
					long* mx = (long*)memory + (i - 1);
					long to = *((long*)(mvar->value) + y);
					MATH_OPERATORS
				}
				else if (out->var_type->name == T_CHAR->name)
				{
					char* mx = &((char*)memory)[i - 1];
					char to = *(char*)mvar->value;
					MATH_OPERATORS
				}
				else if (out->var_type->name == T_BOOL->name)
				{
					bool* mx = &((bool*)memory)[i - 1];
					bool to = *(bool*)mvar->value;
					MATH_OPERATORS
				}
				else if (out->var_type->name == T_STRING->name)
				{
					int y = in;
					char** mx = &((char**)memory)[i - 1];
					char* to = *((char**)mvar->value + y);
					//char* mtype = mb == NULL ? (char*)k->opt : (char*)mb->var_type->name;

					if (*op == '+')
					{
						//strcat(mx,to);
						long mxlen = strlen(*mx);
						long tolen = strlen(to);
						char* exp = (char*)realloc(*mx, strlen(*mx) + strlen(to) + 1);


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
				else
				{
					//int y = get_index_value(funct, k);
					type* typ_ = (type*)memory + (i - 1);
					int to = *((int*)(mvar->value) + in);
					var* x = new_temp_var(typ_);
					func* uy = get_func_by_name_with_var(x, "add");
					uy->context = contxt;
					uy->fun_p.root->value = &to;
					node* ty = NULL;
					call_function(uy, &contxt);
					//char* mtype = mb == NULL ? (char*)k->opt : (char*)mb->var_type->name;
				}
				op = NULL;

				///TO DO :why
				//free(mvar->value);
			}
		}
		else if (k->type_ == operators_n || (k->type_ == equles && k->next->type_ ==
				equles)
		)
		{
			if (*(char*)k->value == '+' && *(char*)k->next->value == '+')
			{
				if (uu != NULL)
				{
					k = k->next;
					(*(int*)uu->value)++;
					uu = NULL;
				}
				else
				{
					k = k->next;
					f = 1;
				}
			}
			else if (k->type_ == equles)
			{
				op = "==";
				k = k->next;
			}
			else if (*(char*)k->value == '&' && *(char*)k->next->value == '&')
			{
				op = "&&";
				k = k->next;
			}
			else if (*(char*)k->value == '>' && *(char*)k->next->value == '=')
			{
				op = ">=";
				k = k->next;
			}
			else if (*(char*)k->value == '<' && *(char*)k->next->value == '=')
			{
				op = "<=";
				k = k->next;
			}
			else if (*(char*)k->value == '|' && *(char*)k->next->value == '|')
			{
				op = "||";
				k = k->next;
			}
			else

				op = (char*)k->value;
		}

		k = k->next;
	}

	if (saved_return_type != NULL)out->var_type = saved_return_type;
	if (out->value == NULL)
	{
		out->value = memory;
	}
	else
	{
		out->value = memory;
	}

	return k;
}


/*node* calculate2(node* m, func* funct, var* contxt, node_type stop, node* end, var* out)
{
	var* h1 = NULL;
	var* h2 = NULL;
	var* r = NULL;
	while (true)
	{
		var* t = g(h1, h2);
		switch (m->btype.name)
		{
		case itype: break;
		case keyword: break;
		case var_name:
			t = get_var_by_name((char*)m->value);
			if (t == NULL)
			{
				func* x = get_func_by_name(contxt, (char*)m->value);
				//call_function(m, x, contxt,NULL, false);
				t = &x->func_return;
			}
			break;
		case value:
			t = new_var("",T_INT);
			//void* mem = install_memory(T(m->opt), 1);
			t->value = m->value;
			break;
		case operators_n:
			do
			{
				switch (*(char*)m->value)
				{
				case '+':

					break;
				case '-':
					break;
				case '*':
					break;
				case '/':
					break;
				default: ;
				}
				m = m->next;
			}
			while (m->type_ == operators_n);
			break;
		case equles: break;
		case endl: break;
		case s_index: break;
		case parentheses1: break;
		case parentheses1c: break;
		case comma: break;
		case s_index_c: break;
		case parentheses4: break;
		case parentheses4c: break;
		case dot: break;
		case twodot: break;
		case PARS: break;
		case HAVE_value: break;
		case NON_ONE_CHAR: break;
		default: ;
		}
		m = m->next;
	}
}*/

node* get_close_part(node* t)
{
	if (t->ref_node != NULL && (t->btype.name == s_index && t->ref_node->type_ == s_index_c ||
		t->btype.name == parentheses1 && t->ref_node->type_ == parentheses1c ||
		t->btype.name == parentheses4 && t->ref_node->type_ == parentheses4c))
	{
		return t->ref_node;
	}

	printf("ERROR: Line:%d BLOCK %s", t->line, __FUNCTION__);
	exit(0);
}


char* get_filebuff(FILE* sf)
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

//memory must by installed string char **;
void set_value_copy_var(var* dstn, var* scr)
{
	if (T_INT == dstn->var_type)
		*Pint(dstn->value) = *Pint(scr->value);
	else if (T_STRING == dstn->var_type)
	{
		*Pstring(dstn->value) = (char*)malloc(strlen(*Pstring(scr->value)) + 1);
		strcpy(*Pstring(dstn->value), *Pstring(scr->value));
	}
}

///var type and memory must already setted
void set_value_copy_node(var* dstn, node* scr)
{
	if (T_INT == dstn->var_type)
		*dstn->value_int = atoi(scr->value_char_ptr);
	else if (T_STRING == dstn->var_type)
	{
		*dstn->val_str_ptr = (char*)malloc(strlen(scr->value_char_ptr) + 1);
		strcpy(*dstn->val_str_ptr, scr->value_char_ptr);
	}
	else if (T_FLOAT == dstn->var_type)
	{
		*dstn->value_float = atof(scr->value_char_ptr);
	}
	else if (T_LONG == dstn->var_type)
	{
		*dstn->value_long = atol(scr->value_char_ptr);
	}
	else if (T_BOOL == dstn->var_type)
	{
		*Pbool(dstn->value) = strcmp(scr->value_char_ptr, "false") != 0;
	}
}



var parms[]={{.name="a",.var_type=T_STRING},{.name="a",.var_type=T_INT}};

func m[] = {
	{
		.fun_p={0}, .func_code= &print, .func_name="print", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+1
	},
	{
		.fun_p={0}, .func_code= &vprint, .func_name="vprint", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+2
	},
	{
		.fun_p={.root=parms,.top=parms,.size=1}, .func_code= &time_, .func_name="time", .function_type=f_main, .func_return={.var_type=T_STRING},
		.ref=0,
		.context=0, .stack_next=m+3
	},
	{
		.fun_p={.root=parms,.top=parms,.size=1}, .func_code= &scan, .func_name="scan", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+4
	},
	{
		.fun_p={.root=parms+1,.top=parms+1,.size=1}, .func_code= &random, .func_name="random", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+5
	},
	{
		.fun_p={.root=parms+1,.top=parms+1,.size=1}, .func_code= &str, .func_name="str", .function_type=f_main, .func_return={.var_type=T_STRING},
		.ref=0,
		.context=0, .stack_next=m+6
	},
	{
		.fun_p={.root=parms,.top=parms,.size=1}, .func_code= &import, .func_name="import", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+7
	},
	{
		.fun_p={0}, .func_code= &print, .func_name="xxxx", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=m+8
	},
	{
		.fun_p={0}, .func_code= &print, .func_name="pxcrint", .function_type=f_main, .func_return={.var_type=T_INT},
		.ref=0,
		.context=0, .stack_next=NULL
	}
};
func_stack base_function = {.top=m+8, .size=9, .root=m};