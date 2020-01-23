#include "compile.h"


void _cons_(func* f)
{
}

void define_new_var_g_f(type* contern_class, func* mfun, var** out_var, type* var_type, char* name)
{
	//incde function
	if (mfun != NULL)
	{
		var* svar = fget_var_by_name(&mfun->fun_p, name);
		if (svar != NULL && svar->var_type != NULL && var_type != NULL && var_type  == svar->var_type)
		{
			*out_var = svar;
		}


		else
		{
			var* ivar = new_var_on_stack(&mfun->fun_p, name, var_type);
			*out_var = ivar;
		}
	}
	else
	{
		if (contern_class != NULL)
		{
			var* by_name = fget_var_by_name(&contern_class->propertys, name);
			if (by_name != NULL)
			{
				//TODO: error if var already defined
				*out_var = by_name;
			}

			else
			{
				var* ivar = new_var_on_stack(&contern_class->propertys, name, var_type);
				*out_var = ivar;
			}
		}

		else
		{
			var* var = new_var(name, var_type);
			*out_var = var;
		}
	}
}

node* add_new_func_code(node* c, type* contner_class)
{
	//TODO: check if already found

	const bool cons = c->value_type == T_NEW;
	func* m = contner_class == NULL || cons ? new_func() : new_func_on_stack(&contner_class->functions);

	memset(m, 0, sizeof(func));
	m->func_return.var_type = cons ? contner_class : c->value_type;
	m->function_type = cons ? constr : f_main ;
	c = c->next;
	if (cons)
	{
		const size_t size = sizeof(char) * (strlen(c->value_char_ptr) + 2);
		char* y = (char*)malloc(size);
		memset(y, 0, size);
		//strcat_s(y, ":");
		*y=':';

		strcat(y, c->value_char_ptr);
		m->func_name = y;
	}
	else
	{
		m->func_name = c->value_char_ptr;
	}

	//parse paramater
	c = c->next;  //(
	node* close = get_close_part(c);
	if (c->type_ != parentheses4 || close == NULL)
	{
		printf("ERROR:");
		exit(-1);
	}
	
	var_stack* y = &m->fun_p;
	/* TODO: check end */
	while (c != close)
	{
		if (c->type_ == itype)
		{
			///if(c->next->type_==var_name)
			new_var_on_stack(y, (char*)c->next->value, (type*)c->value);

			c = c->next;
		}

		c = c->next;
	}


	node* func_decl = getFirstType(c, parentheses1);
	node* end = get_close_part(func_decl);
	if (end == NULL)
	{
		printf("error { not closed");
		exit(-1);
	}
	m->func_code = &call_func_in;
	m->ref = func_decl;

	return end->next;
}

void step_forwrod(node ** nod)
{
	*nod = (*nod)->next; 
}

node* setup_function_parms(node** nod, func* function, var* context, func* in_function)
{
	*nod = getFirstType(*nod, parentheses4);
	node* close = get_close_part(*nod);
	var_stack* y = & function->fun_p;

	step_forwrod(nod);
	var* ro = y->root;
	while (*nod != close && (*nod)->type_ != endl)
	{
		//var* pv = new_temp_var(NULL);
		// allow to get parameters form current function old_call
		if (ro == NULL)
		{
			ro = new_var_on_stack(y, "U",NULL);
		}

		
		*nod = calculate(*nod, in_function, context, comma, close, ro);
		if (*nod == NULL)
			return close;


		ro = ro->stack_next;


		if ((*nod)->type_ == comma)
		{
			*nod = (*nod)->next;
		}
	}
	return close;
}

bool call_function(func* temp, var** context)
{
	if (temp->function_type == constr)
	{
		var* cp = new_temp_var(temp->func_return.var_type);

		cp->value = install_memory_with_type(temp->func_return.var_type, 1);
		*context = cp;
	}
	temp->context = *context;

	temp->func_code(temp);


	return false;
}

void compile_var_name_start(node** cx, func** tempx, var* context)
{
	if ((*cx)->next->type_ == dot)
	{
		var* m = NULL;
		if (strcmp((*cx)->value_char_ptr, "this") == 0)
			m = context;
		else
			m = fget_var_by_name(context != NULL ? &(context->value_type->propertys) : NULL, (*cx)->value_char_ptr);

		
		step_forwrod(cx); // .
		step_forwrod(cx); // V.(V)
		compile_var_name_start(cx, tempx, m);
		return;
		//TODO : continios after line end 
	}
	if ((*cx)->next->type_ == equles || (*cx)->next->type_ == s_index)
	{
		set_value(context, *tempx, cx);
		if (!(*cx) || (*cx)->next == NULL)
			return;
	}
	else if ((*cx)->next->type_ == operators_n)
	{
		var* m = fget_var_by_name(context != NULL ? &(context->value_type->propertys) : NULL, (char*)(*cx)->value);
		if (m != NULL)
		{
			calculate((*cx), *tempx, context, endl, NULL, m);
		}
	}
	else if ((*cx)->next->type_ == parentheses4) ///else added after [66e2ce6179febc9f335dd6886b5a8c226a4d4183]
	{
		func* tempxc = get_func_by_name_with_var(context, (char*)(*cx)->value);
		if (tempxc == NULL)
		{
			printf("function %s isn'node defined", (char*)(*cx)->value);
			return;
		}
		setup_function_parms(cx, tempxc, context, *tempx);
		call_function(tempxc, &context);
	}
	else
	{
		printf("ERROR: var : %s in line %d not defined in %s %s line %d\n", (char*)(*cx)->value, (*cx)->line,
		       __FUNCTION__,
		       __FILE__,
		       __LINE__);
	}
}

typedef struct if_block
{
	byte setted;
	byte value;
} if_block;

bool scape_block(node** cx)
{
	if ((*cx)->ref_node != NULL && (*cx)->ref_node->opt != NULL)
	{
		if_block* m = (if_block*)(*cx)->ref_node->opt;
		return m->setted == 1 && m->value > 0;
	}
	return false;
}

void if_eif_function(func* temp, node** cx)
{
	var* m = new_temp_var(T_BOOL);
	node* save = *cx; ///{if-eif}
	if_block* heif = (if_block*)malloc(sizeof(if_block));

	if (scape_block(cx)) //check if pre branch takin
	{
		heif->value = 2;
		heif->setted = 1;
		save->opt = heif;
		*cx = getFirstType((*cx), parentheses1)->ref_node; //TODO: 
		return;
	}
	int bt = (int)(*cx)->value;
	if (bt != _else_)
	{
		node* el = get_close_part((*cx)->next);
		node* last = calculate((*cx)->next->next, temp, NULL, (node_type)0, el, m);
		(*cx) = last;

		(*cx) = getFirstType((*cx), parentheses1);
		node* close = get_close_part((*cx));
		(*cx) = (*cx)->next;
		if (*(bool*)m->value) /// true
		{
			heif->value = 1;
			heif->setted = 1;
			save->opt = heif;
		}
		else
		{
			(*cx) = close;

			heif->value = 0;
			heif->setted = 1;
			save->opt = heif;
		}
	}
	else
	{
	}
}


void while_function(func* temp, node** c)
{
	var* m = new_temp_var(T_BOOL);

	node* el = get_close_part((*c)->next);
	node* cond = (*c)->next->next;
	*c = calculate(cond, temp, NULL, 0, el, m);


	*c = getFirstType(*c, parentheses1);
	node* close = get_close_part(*c);
	*c = (*c)->next;


	while (var_bool_value(m))
	{
		//node* last=calculate(new_var(),cond,parse_obj::parentheses4c,temp);
		//as.print_line_debuge(c,1);
		compile(NULL, *c, temp, close);
		//check the condition again
		calculate(cond, temp, NULL, (node_type)0, el, m);
	}

	*c = close; //getFirstType(c, parse_obj::parentheses1c);
}

//for VAR_NAME (EXP,EXP,EXP)
void for_function(func* temp, node** c)
{
	*c = (*c)->next;
	var* kk = fget_var_by_name(&temp->fun_p, (char*)(*c)->value);
	*c = (*c)->next; //(
	node* el = get_close_part(*c); //)
	*c = (*c)->next;
	*c = calculate(*c, temp, NULL, comma, NULL, kk);
	*c = (*c)->next;

	node* cond = *c;
	var* bool_var = new_temp_var(T_BOOL);

	func* y = new_func();

	*c = calculate(*c, temp, NULL, comma, NULL, bool_var);
	node* exp = *c = (*c)->next;

	*c = getFirstType(*c, parentheses1);
	node* close = get_close_part(*c);
	*c = (*c)->next;
	var_stack* a = (var_stack*)malloc(sizeof(var_stack));
	memcpy(a, &temp->fun_p, sizeof(var_stack));
	//var_stack(temp->fun_p);
	if (temp != NULL) y->fun_p = *a;

	while (var_bool_value(bool_var))
	{
		//node* last=calculate(new_var(),cond,parse_obj::parentheses4c,temp);
		//as.print_line_debuge(c,1);
		compile(NULL, *c, y, close);
		//check the condition again
		calculate(exp, temp, NULL, (node_type)0, el, kk);
		calculate(cond, temp, NULL, comma, NULL, bool_var);
	}

	*c = close; //getFirstType(c, parse_obj::parentheses1c);	
}


void compile_type(node* out, func* temp, node* stop, type* b);


void install_class(node** n)
{
	type* t = new_type();
	char* str = (*n)->next->value_char_ptr;

	size_t len = strlen(str);
	char* name = (char*)malloc(len + 1);
	memset(name, 0, len + 1);
	strcpy(name, str);
	t->name = name;
	node* r = (*n)->next->next->next; //class tt(-);
	if (r->type_ == var_name)
		t->base = get_type_by_name(r->value_char_ptr);
	node* start = getFirstType(r, parentheses1);
	node* tm = get_close_part(start);


	compile_type(start->next,NULL, tm, t);
	*n = tm;
}

void compile(var* parent, node* out, func* temp, node* stop)
{
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	if (stop == NULL)
		in = getRoot(out);
	else
		in = out;

	for (node* c = in; c != NULL; c = c->next)
	{
		if (stop != NULL && (c == stop || (c->parent != NULL && c->parent == stop)))
		{
			break;
		}
		switch (c->type_)
		{
		case var_name:
			{
				if (c->opt != NULL && *(char*)c->opt == 'a')
				{
					func* tempx = get_func_by_name_with_var(parent, c->value_char_ptr);
					if (tempx == NULL)
					{
						printf("function %s isn'node defined", c->value_char_ptr);
						return;
					}
					setup_function_parms(&c, tempx, parent, temp);
					call_function(tempx, &parent);
				}
				else
					compile_var_name_start(&c, &temp, parent);
			}
			break;
		case itype:
			{
				///int function()
				if (c->next->opt != NULL && *(char*)c->next->opt == 'f')
				{
					c = add_new_func_code(c, NULL);
				}
				else
				{
					var* m;
					node* name = getFirstType(c, var_name);
					define_new_var_g_f(NULL, temp, &m, c->value_type, name->value_char_ptr);
					if (c->next->tp.s_index && *(char*)c->next->opt == 's')
					{
						var* t = new_temp_var(T_INT);

						calculate4(t, c->next->next, s_index_c, temp);
						m->size = *t->value_int;
						c = name;
					}
					else
					{
						m->size = 1;
						c = name;
					}


					if (c->next->type_ == equles)
					{
						if (temp != NULL)
						{
							c = calculate3(m, c->next->next, temp);
						}
						else
						{
							if (c->next->next->type_ == itype && get_type_by_name((char*)c->next->next->value))
							{
								//c->next->next->type_=var_name;
							}
							//c = calculate(m, c->next->next, temp);
							//m->value = install_memory(m->var_type, m->size);
							c = calculate(c->next->next, temp, NULL, endl, NULL, m);
						}
					}
					else if (c->next->type_ == endl)
					{
						m->value = install_memory_with_type(m->var_type, m->size);
					}
				}
				break;
			}
		case keyword:
			{
				switch (c->_ptr_)
				{
				case _if_:
					if_eif_function(temp, &c);
					break;
				case _for_:
					for_function(temp, &c);
					break;
				case _while_:
					while_function(temp, &c);
					break;
				case _do_:
					break;
				case _else_:
					if_eif_function(temp, &c);
					break;
				case _eif_:
					if_eif_function(temp, &c);
					break;
				case _return_:
					if (temp != NULL)
					{
						var* re = &temp->func_return;

						node* k = calculate(c->next, temp, parent, (node_type)0, NULL, re);
						c = stop;
						return;
					}
					break;
				case _break_:
					c = stop;
					return;

				case _class_:
					install_class(&c);
					break;
				case _static_:
					break;
				default:
					break;
				}
			}

		default: ;
		}
		//return from function code


		if (c == NULL)
			break;
	}
}


void compile_type(node* out, func* temp, node* stop, type* b)
{
	int u = 0;
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	if (stop == NULL)
		in = getRoot(out);
	else
		in = out;

	for (node* c = in; c != NULL; c = c->next)
	{
		if (stop != NULL && (c == stop || (c->parent != NULL && c->parent == stop))) break;
		switch (c->type_)
		{
		case var_name:
			{
				//TODO: error
				compile_var_name_start(&c, &temp, NULL);
			}
			break;
		case itype:
			{
				///int function()
				if (c->next->opt != NULL && *(char*)c->next->opt == 'f')
				{
					c = add_new_func_code(c, b);
				}
				else
				{
					var* m;
					node* name = getFirstType(c, var_name);
					define_new_var_g_f(b, temp, &m, (type*)c->value, (char*)name->value);
					if (c->next->tp.s_index && *(char*)c->next->opt == 's')
					{
						var* t = new_temp_var(T_INT);

						calculate4(t, c->next->next, s_index_c, temp);
						m->size = *(int*)t->value;
						c = name;
					}
					else
					{
						m->size = 1;
						c = name;
					}
					if (u == 1)
					{
						u = 0;
						m->access = STATIC;
					}

					if (c->next->type_ == equles)
					{
						if (temp != NULL)
						{
							c = calculate3(m, c->next->next, temp);
						}
						else
						{
							c = calculate3(m, c->next->next, temp);
						}
					}
					else if (c->next->type_ == endl)
					{
						//	m->value = install_memory(m->var_type, m->size);
					}
				}
				break;
			}
		case keyword:
			switch ((int)c->_ptr_)
			{
			case _if_:
				if_eif_function(temp, &c);
				break;
			case _for_:
				for_function(temp, &c);
				break;
			case _while_:
				while_function(temp, &c);
				break;
			case _do_:
				break;
			case _else_:
				if_eif_function(temp, &c);
				break;
			case _eif_:
				if_eif_function(temp, &c);
				break;
			case _return_:
				if (temp != NULL)
				{
					var* re = &temp->func_return;
					calculate3(re, c->next, temp);
					c = getLastType(c, endl);
				}

				break;
			case _break_:
				c = stop;
				return;

			case _class_:
				install_class(&c);
				break;
			case _static_:
				u = 1;
				break;
			default:
				break;
			}

		default: ;
		}
		//return from function code
		if (temp != NULL && c != NULL && (c->next == NULL) && temp->ref != NULL)
		{
#ifdef DE
			printf("debug function %s return %d",temp->func_name,*(int*)temp->func_return.value);
#endif

			c = temp->ref->parent;
			temp = NULL;
		}
		else
		{
			//printf("error FILE:%s\nLINE:%d",__FILE__,__LINE__);
		}

		if (c == NULL)
			break;
	}
}
