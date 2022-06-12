#include "types.h"
#include <stdio.h>

#define p printf
#define l(i) printf("%*s",i*4," ");
void echo_type_instance(type_instance* c, int level);
void echo_type_def(type_def c, int level);
void echo_func_def(func_deftion m, int le);
void echo_var(var p, int level);
void _echo(fcall* v)
{
	p("{\n");
	echo_var(v->func_parmeters[0], 1);
	p("}\n");
}
void echo_var(var v, int le)
{

	l(le)p("name:%s\n", v.name);
	l(le)p("name:%d\n", v.size);


	switch (v.type_define->type_id)
	{
	case 0:///long
		l(le)p("value long : %ld", *v.value_long);
		break;
	case 1://string
		l(le)p("value string : %s", *v.value_str_ptr);
		break;
	case 2://char
		l(le)p("value char : %c", *v.value_char_ptr);
		break;
	case 3://int
		l(le)p("value_int : %d", *v.value_int);
		break;
	case 4://bool
		l(le)p("value_bool : %s", *v.value_bool ? "true" : "false");
		break;
	case 5://float
		l(le)p("value float : %f", *v.value_float);
		break;
	case 9: //T_FUNC
	{
		l(le)p("value function :\n");
		l(le)p("{\n");
		
		echo_func_def( *v.value_func,le+1);
		l(le)p("}\n");
		break;
	}
	default:
		p("unimpl %d %s", __LINE__, __FILE__);
		break;
	}
	l(le) p(", addr = {{%#x}} , {{size = %d}} \n", (int)v.values, v.size);

	//struct type_instance * holder;
	l(le)p("type define : \n");
	l(le)p("{\n");
	echo_type_def(*v.type_define, le + 1);
	l(le)p("}\n");
	//struct type_def* base_type;

	//struct node* ref;
	//struct var* stack_next;
	///enum var_access access;


}
void echo_func_def(func_deftion m, int le)
{
	l(le)p("function name : %s \n", m.func_name);
	l(le)p("parms count : %d \n", m.start_parm_count);
	l(le)p("parms: \n");
	l(le)p("{\n");
	int nl =le+1;
	int nnl =le+2;
	for (int i = 0; i < m.start_parm_count; i++)
	{
		l(nl)p("{\n");
		l(nnl)p("parm index :%d , name : %s\n", i, m.start_func_parmeters_name[i]);
		if (*(m.start_func_parmeters+i) != NULL)
		{
			l(nnl)p("parm type :\n");
			l(nnl)p("{\n");
			echo_type_def(*m.start_func_parmeters[i], le + 3);
			l(nnl)p("}\n");
		}
		else
		{
			l(nnl)p("typedef: ANY TYPE\n");
		}
		l(nl)p("}\n");
	
	}
	l(le)p("}\n");
	l(le)p("function code: %p \n", m.func_code);

	//function_type function_type;
	l(le)p("return type : \n");
	l(le)p("{\n");
	echo_type_def(*m.return_type, le + 1);
	l(le)p("}\n");
	node* ref;
	//enum var_access access;
	//unused
	//struct func_deftion* stack_next;	
}

void echo_type_def(type_def c, int le)
{
	l(le)p("type name : %s \n", c.type_name);
	l(le)p("type id : %d \n", c.type_id);
	l(le)p("props count : %d\n", c.d_propertys_size);
	l(le)p("props :\n");
	l(le)p("{\n");
	for (int i = 0; i < c.d_propertys_size; i++)
	{
		echo_var(c.d_propertys[i], le + 1);

	}
	l(le)p("}\n");

	l(le)p("function count : %d\n", c.d_function_size);
	l(le)p("functions : \n");
	l(le)p("{\n");
	for (int i = 0; i < c.d_function_size; i++)
	{
		echo_func_def(c.d_functions[i], le + 1);

	}
	l(le)p("}\n");




	///struct type_def* base;
	///struct type_def* stack_next;
	///enum var_access access;
	//enum w_type w;

}
void echo_type_instance(type_instance* c, int le)
{
	l(le)p("props count : %d", c->propertys.size);
	l(le)p("prop : \n");
	l(le)p("{\n");
	for (var* i = c->propertys.root; i != NULL; i = i->stack_next)
	{
		echo_var(*i, le + 1);

	}
	l(le)p("}\n");

	l(le)p("function count : %d", c->functions.size);
	l(le)p("functions :\n");
	l(le)p("{\n");
	for (func_deftion* i = c->functions.root; i != NULL; i = i->stack_next)
	{
		echo_func_def(*i, le + 1);

	}
	l(le)p("}\n");

	l(le)p("type define : \n");
	l(le)p("{\n");
	echo_type_def(*c->type, le + 1);
	l(le)p("}\n");
	l(le)p("base :\n");
	l(le)p("{\n");
	echo_type_instance(c->base, le + 1);
	l(le)p("}\n");
	//struct type_def* stack_next;
	//enum var_access access;
	///enum w_type w;

}