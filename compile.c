#include "compile.h"


void _cons_(func_deftion* f)
{
}
void define_new_class_prop( char* name,type_def* contern_class,type_def* new_var_type ,var** out_var)
{

	if (contern_class != NULL)
		{
			var* by_name;
			for(int i=0; i<contern_class->d_propertys_size;i++)
			{
				if(contern_class->d_propertys[i]->name == name )
				{
					by_name= contern_class->d_propertys[i];
				}
			}
			if (by_name != NULL)
			{
				//TODO: error if var already defined
				*out_var = by_name;
			}

			else
			{
				contern_class->d_propertys[contern_class->d_propertys_size]->type_define=new_var_type;
				var *ivar= contern_class->d_propertys[contern_class->d_propertys_size]  ;
				contern_class->d_propertys[contern_class->d_propertys_size]->name=name;
				contern_class->d_propertys_size++;
				*out_var = ivar;

			}
		}


}
void define_new_var_onfunction(char* name,fcall * mfun, type_def* new_var_type, var** out_var)
{
	
		var* svar = fget_var_by_name_fc(name,mfun);
		if (svar != NULL )//&& svar->type_define != NULL && new_var_type != NULL && new_var_type == svar->type_define)
		{
			printf("\ndefine_new_var_onfunction: multi definiton var [ %s ]\n",name);
			exit(-1);
			//*out_var = svar;
		}


		else
		{
			
			mfun->func_parmeters[mfun->parm_count_c].name=name;
			mfun->func_parmeters[mfun->parm_count_c].type_define=new_var_type;			
			*out_var = &mfun->func_parmeters[mfun->parm_count_c];
			mfun->parm_count_c++;
		}
	
}
void define_new_var_globle(var** out_var, type_def* new_var_type, char* name)
{
	//incde function
	

		
			var* var = new_var(name, new_var_type);
			*out_var = var;
		
	
}

node* add_new_func_code(node* c, type_def* container_class)
{
	//TODO: check if already found

	const bool cons = c->value_type == T_NEW_INC;
	func_deftion* m = container_class == NULL || cons ? new_func() : container_class->d_functions[container_class->d_function_size++];
	
	memset(m, 0, sizeof(func_deftion));
	m->return_type = cons ? container_class : c->value_type;
	m->function_type = cons ? constr : f_main;
	c = c->next;
	if (cons)
	{
		const size_t size = sizeof(char) * (strlen(c->value_char_ptr) + 2);
		char* y = (char*)malloc(size);
		memset(y, 0, size);
		//strcat_s(y, ":");
		*y = ':';

		strcat(y, c->value_char_ptr);
		m->func_name = y;
	}
	else
	{
		m->func_name = c->value_char_ptr;
	}

	//parse paramater
	c = c->next; //(
	node* close = get_close_part(c);
	if (c->type_ != parentheses4 || close == NULL)
	{
		printf("ERROR: function def new function");
		exit(-1);
	}

	type_def ** function_protype_parms = m->start_func_parmeters;
	int i=0;
	/* TODO: check end */
	while (c != close)
	{
		if (c->type_ == itype)
		{
			///if(c->next->type_==var_name)
		///	new_var_on_stack(function_protype_parms++, (char*)c->next->value_raw, c->value_type);
			m->start_func_parmeters[i]=  c->value_type;
			m->start_func_parmeters_name[i]=(char*)c->next->value_raw;
		
			
			i++;
			
			c = c->next;
		}
		m->start_parm_count=i;
		c = c->next;
	}


	node* func_decl = get_first_type(c, parentheses1);
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

void step_forwrod(node** nod)
{
	*nod = (*nod)->next;
}

node* setup_function_parms(node** nod, fcall* function, var* context, fcall* in_function)
{
	*nod = get_first_type(*nod, parentheses4);
	node* close = get_close_part(*nod);
	//var* y =  function->func_parmeters;
	//int m = function->deftion->start_parm_count;
	int i =0;
	step_forwrod(nod);
	
	while (*nod != close && (*nod)->type_ != endl )//&& i<m)
	{
		//var* pv = new_temp_var(NULL);
		// allow to get parameters form current function old_call

		
		
		var * n =&function->func_parmeters[i++];
		n->values=NULL;
		n->type_define=NULL;
		

		*nod = calculate(*nod, in_function, context, comma, close,n);
		if (*nod == NULL)
			return close;


		


		if ((*nod)->type_ == comma)
		{
			*nod = (*nod)->next;
		}
	}
	function->parm_count_c=i;

	return close;
}

bool call_function(fcall* mfunc, var** context)
{
	if (mfunc->deftion->function_type == constr)
	{
		

		mfunc->_return.values = install_memory_with_type(mfunc->_return.type_define, 1);
		*context = &mfunc->_return;
	}
	mfunc->context = *context;

	mfunc->deftion->func_code(mfunc);


	return false;
}

//[type_inctance.[prop]]
void compile_var_name_start(node ** pnode, fcall * function_c, var * calling_object)
{
	if ((*pnode)->next->type_ == dot)
	{
		var* m;
		if (strcmp((*pnode)->value_char_ptr, "this") == 0)
		{
			m = calling_object;
		}
		else
		{
			m= all_get_var_by_name((*pnode)->value_char_ptr,function_c,calling_object);
			
		}

		step_forwrod(pnode); // .
		step_forwrod(pnode); // V.(V)
		compile_var_name_start(pnode, function_c, m);
		return;
		//TODO : continios after line end 
	}
	if ((*pnode)->next->type_ == equles || (*pnode)->next->type_ == s_index)
	{
		set_value(calling_object, function_c, pnode);
		if (!(*pnode) || (*pnode)->next == NULL)
			return;
	}
	else if ((*pnode)->next->type_ == operators_n)
	{
		var* m = all_get_var_by_name((char*)(*pnode)->value_raw,
			function_c,
			calling_object);
		if (m != NULL)
		{
			calculate((*pnode), function_c, calling_object, endl, NULL, m);
		}
	}
	else if ((*pnode)->next->type_ == parentheses4) ///else added after [66e2ce6179febc9f335dd6886b5a8c226a4d4183]
	{
		func_deftion* tempxc = get_obj_function(calling_object, (*pnode)->value_char_ptr);
		if (tempxc == NULL)
		{
			printf("function %s isn'node defined", (*pnode)->value_char_ptr);
			return;
		}
		fcall *new_function= create_fcall(tempxc);
		
		setup_function_parms(pnode, new_function, calling_object, function_c);
		call_function(new_function, &calling_object);
	}
	else
	{
		printf("ERROR: var : %s in line %d not defined in %s %s line %d\n", (char*)(*pnode)->value_raw, (*pnode)->line,
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
	if ((*cx)->ref_node != NULL && (*cx)->ref_node->opt_raw != NULL)
	{
		if_block* m = (if_block*)(*cx)->ref_node->opt_raw;
		return m->setted == 1 && m->value > 0;
	}
	return false;
}

void if_eif_function(node** cx, fcall * temp, var* calling_obj)
{
	var* m = new_temp_var(T_BOOL);
	node* save = *cx; ///{if-eif}
	if_block* heif = (if_block*)malloc(sizeof(if_block));

	if (scape_block(cx)) //check if pre branch takin
	{
		heif->value = 2;
		heif->setted = 1;
		save->opt_raw = heif;
		*cx = get_first_type((*cx), parentheses1)->ref_node; //TODO: 
		return;
	}
	int bt = (int)(*cx)->value_raw;
	if (bt != _else_)
	{
		node* el = get_close_part((*cx)->next);
		node* last = calculate((*cx)->next->next, temp, NULL, (node_type)0, el, m);
		(*cx) = last;

		(*cx) = get_first_type((*cx), parentheses1);
		node* close = get_close_part((*cx));
		(*cx) = (*cx)->next;
		if (*(bool*)m->values) /// true
		{
			heif->value = 1;
			heif->setted = 1;
			save->opt_raw = heif;
		}
		else
		{
			(*cx) = close;

			heif->value = 0;
			heif->setted = 1;
			save->opt_raw = heif;
		}
	}
	else
	{
	}
}


void while_function(node** c, fcall * temp, var* calling_obj)
{
	var* m = new_temp_var(T_BOOL);

	node* el = get_close_part((*c)->next);
	node* cond = (*c)->next->next;
	*c = calculate(cond, temp, NULL, 0, el, m);


	*c = get_first_type(*c, parentheses1);
	node* close = get_close_part(*c);
	*c = (*c)->next;


	while (*m->value_int)
	{
		//node* last=calculate(new_var(),cond,parse_obj::parentheses4c,temp);
		//as.print_line_debuge(c,1);
		compile(NULL, *c, temp, close);
		//check the condition again
		calculate(cond, temp, NULL, (node_type)0, el, m);
	}

	*c = close; //getFirstType(c, parse_obj::parentheses1c);
}

//for VAR_NAME ((start)EXP,(end)EXP[cond])
void for_function(node** c, fcall * funcall, var * calling_object)
{
	*c = (*c)->next; // for_var
	var* for_v = all_get_var_by_name((*c)->value_char_ptr,funcall,calling_object);//var name
	*c = (*c)->next; //(
	node* el = get_close_part(*c); //)
	*c = (*c)->next; //start_exp
	node* start_exp = *c;
	*c = calculate(start_exp, funcall, calling_object, comma, NULL, for_v);
	*c = (*c)->next; //step_exp
    node* step_exp= *c;

 *c = get_first_type(*c, comma)->next;

	node* cond = *c;//cond_exp
	var* bool_var = new_temp_var(T_BOOL);

	//func_deftion* y = new_func();

	//*c = calculate(*c, funcall,calling_object, comma,*c, bool_var);
		*c = calculate(cond, funcall,calling_object, (node_type)0,el, bool_var);
	

	*c = get_first_type(*c, parentheses1);
	node* for_close = get_close_part(*c);
	*c = (*c)->next;
	//var_stack* a = (var_stack*)malloc(sizeof(var_stack));
	//memcpy(a, &funcall-.start_func_parmeters, sizeof(var_stack));
	//var_stack(temp->fun_p);
	//if (funcall != NULL)
	//{

	//	y-.start_func_parmeters[0] = *a;
	//}
//for for_v(start_exp,step_exp,cond)
	while (*bool_var->value_int)
	{
		//node* last=calculate(new_var(),cond,parse_obj::parentheses4c,temp);
		//as.print_line_debuge(c,1);
		compile(calling_object, *c,funcall , for_close);
		//check the condition again
		calculate(step_exp, funcall, calling_object, comma, NULL, for_v);
	
		calculate(cond, funcall, calling_object, (node_type)0, el, bool_var);
	}

	*c = for_close; //getFirstType(c, parse_obj::parentheses1c);	
}


void compile_type(node* out, fcall * function_call, node* stop, type_def* b);


void install_class(node** n)
{
	type_def* mtype = new_type();
	step_forwrod(n); //class->
	char* str = (*n)->value_char_ptr;

	size_t len = strlen(str);
	char* name = (char*)malloc(len + 1);
	memset(name, 0, len + 1);
	strcpy(name, str);
	mtype->type_name = name;
	step_forwrod(n);
	if ((*n)->type_ == parentheses4)
	{
		step_forwrod(n);
		if ((*n)->type_ == var_name)
		{
			mtype->base = get_type_by_name((*n)->value_char_ptr);
		}
	}


	node* start = get_first_type(*n, parentheses1);
	node* tm = get_close_part(start);


	compile_type(start->next,NULL, tm, mtype);
	*n = tm;
}

void compile(var* parent, node* out, fcall* c_function, node* stop)
{
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	if (stop == NULL)
		in = get_root(out);
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
				if (c->opt_name_type == function_call)
				{
					func_deftion* tempx;
					if (parent == NULL)
					{
						tempx = get_func_by_name(c->value_char_ptr);
					}
					else
					{
						tempx = get_obj_function(parent, c->value_char_ptr);
					}

					if (tempx == NULL)
					{
						printf("function %s isn'node defined", c->value_char_ptr);
						return;
					}
					fcall* fcall = create_fcall(tempx);
					setup_function_parms(&c, fcall, parent, c_function);
					call_function(fcall, &parent);
				}
				else
					compile_var_name_start(&c, c_function, parent);
			}
			break;
		case itype:
			{
				///int function()
				if (c->next->type_ == var_name && c->next->opt_name_type == function_def)
				{
					c = add_new_func_code(c, NULL);
				}
				else
				{
					var* n_var;
					node* name = get_first_type(c, var_name);
					if(c_function!=NULL)
					{
						define_new_var_onfunction(name->value_char_ptr,c_function,c->value_type,&n_var);
					}
					else
					{
					define_new_var_globle(&n_var, c->value_type,name->value_char_ptr);
					}

					if (c->next->btype.node_type_bit.s_index && *(char*)c->next->opt_raw == 's')
					{
						var* t = new_temp_var(T_INT);

						calculate4(t, c->next->next, s_index_c, c_function);
						n_var->size = *(t->value_int);
						c = name;
					}
					else
					{
						n_var->size = 1;
						c = name;
					}


					if (c->next->type_ == equles)
					{
						if (c_function != NULL)
						{
							c = calculate3(n_var, c->next->next, c_function);
						}
						else
						{
							if (c->next->next->type_ == itype && get_type_by_name((char*)c->next->next->value_raw))
							{
								//c->next->next->type_=var_name;
							}
							//c = calculate(m, c->next->next, temp);
							//m->value = install_memory(m->var_type, m->size);
							c = calculate(c->next->next, c_function, NULL, endl, NULL, n_var);
						}
					}
					else if (c->next->type_ == endl)
					{
						n_var->values = install_memory_with_type(n_var->type_define, n_var->size);
					}
				}
				break;
			}
		case keyword:
			{
				switch (c->_ptr_)
				{
				case _if_:
					if_eif_function(&c, c_function,parent );
					break;
				case _for_:
					for_function(&c, c_function,parent );
					break;
				case _while_:
					while_function(&c, c_function,parent );
					break;
				case _do_:
					break;
				case _else_:
					if_eif_function(&c, c_function, parent );
					break;
				case _eif_:
					if_eif_function(&c, c_function, parent );
					break;
				case _return_:
					if (c_function != NULL)
					{
						var* re = &c_function->_return;

						node* k = calculate(c->next, c_function, parent, (node_type)0, NULL, re);
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


void compile_type(node* out, fcall * function_c, node* stop, type_def* continar_class)
{
	int u = 0;
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	if (stop == NULL)
		in = get_root(out);
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
				compile_var_name_start(&c, function_c, NULL);
			}
			break;
		case itype:
			{
				///int function()
				if (c->next->_opt_ptr_ == function_def)
				{
					c = add_new_func_code(c, continar_class);
				}
				else
				{
					var* m;
					node* name = get_first_type(c, var_name);
					define_new_class_prop(name->value_char_ptr,continar_class,c->value_type, &m);
					if (c->next->btype.node_type_bit.s_index && *(char*)c->next->opt_raw == 's')
					{
						var* t = new_temp_var(T_INT);

						calculate4(t, c->next->next, s_index_c, function_c);
						m->size = *t->value_int;
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
						if (function_c != NULL)
						{
							c = calculate3(m, c->next->next, function_c);
						}
						else
						{
							c = calculate3(m, c->next->next, function_c);
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
				if_eif_function(&c, function_c,NULL );
				break;
			case _for_:
				for_function(&c, function_c,NULL );
				break;
			case _while_:
				while_function(&c, function_c, NULL);
				break;
			case _do_:
				break;
			case _else_:
				if_eif_function(&c, function_c,NULL );
				break;
			case _eif_:
				if_eif_function(&c, function_c,NULL );
				break;
			case _return_:
				if (function_c != NULL)
				{
					var* re = &function_c->_return;
					calculate3(re, c->next, function_c);
					c = get_last_type(c, endl);
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
		if (function_c != NULL && c != NULL && (c->next == NULL) && function_c->deftion->ref != NULL)
		{
#ifdef DE
			printf("debug function %s return %d",temp->func_name,*(int*)temp->func_return.value);
#endif

			c = function_c->deftion->ref->parent;
			function_c = NULL;
		}
		else
		{
			//printf("error FILE:%s\nLINE:%d",__FILE__,__LINE__);
		}

		if (c == NULL)
			break;
	}
}
