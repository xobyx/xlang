#include "compile.h"
extern int get_index_value2(node** nod, fcall* calling_function, var* calling_object);
extern node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
                  var* calc_result);

void define_new_class_prop(char* name, type_def* contern_class, type_def* new_var_type, var** out_var)
{
	if (contern_class != NULL)
	{
		for (int i = 0; i < contern_class->d_propertys_size; i++)
		{
			if ((contern_class->d_propertys + i)->name == name)
			{
				*out_var = contern_class->d_propertys + i;

				return;
			}
		}

		int psize = contern_class->d_propertys_size;
		(contern_class->d_propertys + psize)->type_define = new_var_type;
		var* ivar = (contern_class->d_propertys + psize);
		(contern_class->d_propertys + psize)->name = name;
		contern_class->d_propertys_size++;
		*out_var = ivar;
	}
}

void define_new_var_on_function(char* name, fcall* mfun, type_def* new_var_type, var** out_var)
{
	var* svar = fget_var_by_name_fc(name, mfun);
	if (svar != NULL) //&& svar->type_define != NULL && new_var_type != NULL && new_var_type == svar->type_define)
	{
		printf("\ndefine_new_var_onfunction: multi definiton var [ %s ] on line %d\n", name, mfun->deftion->ref->line);
		exit(-1);
		//*out_var = svar;
	}
	//add to function stack
	int count = mfun->parm_count_c;
	var* parms = mfun->func_parmeters;
	parms[count].name = name;
	parms[count].type_define = new_var_type;
	*out_var = parms + count;
	mfun->parm_count_c++;
}

void define_new_var_globle(var** out_var, type_def* new_var_type, char* name)
{
	//incde function


	var* var = new_var(name, new_var_type);
	*out_var = var;
}

node* add_new_func_code(node* c, type_def* return_type, type_def* container_class)
{
	//TODO: check if already found

	const bool cons = return_type == T_ANY;
	func_deftion* m = container_class == NULL || cons
		                  ? new_func()
		                  : container_class->d_functions + (container_class->d_function_size++);

	memset(m, 0, sizeof(func_deftion));
	m->return_type = cons ? container_class : return_type;
	m->function_type = cons ? constr : f_main;

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
		var* funcvr = new_var(m->func_name,T_FUNC);
		funcvr->value_func = m;
	}

	//parse paramater
	c = c->next; //(
	node* close = get_close_part(c);
	if (c->type_ != parentheses4 || close == NULL)
	{
		printf("ERROR: missing \'()\' for function %s on line %d __ C:%s:%d ", m->func_name, c->line,__FILE__,__LINE__);
		exit(-1);
	}


	int i = 0;
	/* TODO: check end */
	while (c != close)
	{
		if (c->type_ == itype)
		{
			///if(c->next->type_==var_name)
		///	new_var_on_stack(function_protype_parms++, (char*)c->next->value_raw, c->value_type);
			m->start_func_parmeters[i] = c->value_type;
			m->start_func_parmeters_name[i] = (char*)c->next->value_raw;


			i++;

			c = c->next;
		}
		m->start_parm_count = i;
		c = c->next;
	}


	node* func_decl = get_first_type(c, parentheses1);
	node* end = get_close_part(func_decl);
	if (end == NULL)
	{
		printf("error { not closed");
		printf("ERROR: missing \'}\'  in function %s on line %d __ C:%s:%d ", m->func_name, func_decl->line, __FILE__,
		       __LINE__);
		exit(-1);
	}
	m->func_code = &call_func_in;
	m->ref = func_decl;
	m->return_size = 1;
	return end->next;
}

void step_forwrod(node** nod)
{
	*nod = (*nod)->next;
}

//change node pos and return close ) 
node* setup_function_parms(node** nod, fcall* function, var* context, fcall* in_function)
{
	*nod = get_first_type(*nod, parentheses4);
	node* close = get_close_part(*nod);
	//var* y =  function->func_parmeters;
	//int m = function->deftion->start_parm_count;
	int i = 0;
	step_forwrod(nod);

	while (*nod != close && (*nod)->type_ != endl) //&& i<m)
	{
		//var* pv = new_temp_var(NULL);
		// allow to get parameters form current function old_call


		var* n = function->func_parmeters + (i++);
		n->values = NULL;
		n->type_define = NULL;


		*nod = calc(*nod, in_function, context, comma, close, n);
		if (*nod == NULL)
			return close;


		if ((*nod)->type_ == comma)
		{
			*nod = (*nod)->next;
		}
	}
	function->parm_count_c = i;

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
/*void compile_var_name_start(node** pnode, fcall* function_c, var* calling_object)
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
			m = all_get_var_by_name((*pnode)->value_char_ptr, function_c, calling_object);
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
		fcall* new_function = create_fcall(tempxc);

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
}*/

typedef struct if_block
{
	byte setted;
	byte value;
} if_block;


node* gelastjump(node* b)
{
	node* i = b;
	do
	{
		i = i->next_jump;
	}
	while (i->next_jump != NULL);
	return i;
}

void if_eif_function(node** cx, fcall* cfunction, var* calling_obj)
{
	node* ifeif = *cx; ///{if-eif}	
	node* prev = ifeif->ref_node;
	node* close = NULL;
	bool skip = (*cx)->value_keyword != _if_ && (prev != NULL && prev->taked == true);

	if ((*cx)->value_keyword != _else_)
	{
		eat(cx, parentheses4,true); //(
		if (skip == false)
		{
			eat(cx, endl,false);
			var* bool_result = new_temp_var(T_BOOL);
			calc((*cx)->next, cfunction, calling_obj, none, (*cx)->ref_node, bool_result);
			skip = ! *bool_result->value_bool;
		}
		*cx =(*cx)->ref_node;
	}

	eat(cx, endl,false);
	eat(cx, parentheses1,true);
	close = get_close_part(*cx);


	if (skip)
	{
		ifeif->taked = prev != NULL ? prev->taked : false;
	}
	else
	{
		ifeif->taked = true;
		compile(calling_obj, *cx, cfunction, close, NULL);
	}
	*cx = close;
}

void if_eif_function2(node** cx, fcall* cfunction, var* calling_obj)
{
	node* close = NULL;
	node* ifeif = *cx; ///{if-eif}
	//if_block* heif = (if_block*)malloc(sizeof(if_block));
	node* lcond = ifeif->ref_node;

	if ((*cx)->value_keyword == _if_ || lcond != NULL && lcond->taked == false)
	{
		if ((*cx)->value_keyword != _else_)
		{
			var* bool_result = new_temp_var(T_BOOL);

			eat(cx, parentheses4,true);
			node* el = get_close_part(*cx);
			*cx = calc((*cx)->next, cfunction, NULL, none, el, bool_result);


			*cx = get_first_type(*cx, parentheses1);
			close = get_close_part((*cx));
			*cx = (*cx)->next;
			if (bool_result->value_bool) /// true
			{
				ifeif->taked = true;
				compile(calling_obj, *cx, cfunction, close, NULL);
				//  [{|(]    [}|)]
				//*cx = ifeif->next_jump != NULL ? 
				//	get_first_type(gelastjump(ifeif), parentheses1)->ref_node
				//: close;


				//*cx=close;
			}
			else
			{
				ifeif->taked = false;
				(*cx) = close;
			}
		}
		else
		{
			*cx = get_first_type(*cx, parentheses1);
			close = get_close_part((*cx));
			*cx = (*cx)->next;
			compile(calling_obj, *cx, cfunction, close, NULL);
		}
	}
	else
	{
		(*cx) = close;
	}
}


void while_function(node** c, fcall* temp, var* calling_obj)
{
	var* m = new_temp_var(T_BOOL);

	node* el = get_close_part((*c)->next);
	node* cond = (*c)->next->next;
	*c = calc(cond, temp, NULL, none, el, m);


	*c = get_first_type(*c, parentheses1);
	node* close = get_close_part(*c);
	*c = (*c)->next;


	while (*m->value_int)
	{
		//node* last=calculate(new_var(),cond,parse_obj::parentheses4c,temp);
		//as.print_line_debuge(c,1);
		compile(NULL, *c, temp, close, NULL);
		//check the condition again
		calc(cond, temp, NULL, (node_type)0, el, m);
	}

	*c = close; //getFirstType(c, parse_obj::parentheses1c);
}

//for VAR_NAME ((start)EXP,(end)EXP[cond])
void for_function(node** c, fcall* funcall, var* calling_object)
{
	*c = (*c)->next; // for_var
	var* for_v = all_get_var_by_name((*c)->value_char_ptr, funcall, calling_object); //var name
	*c = (*c)->next; //(
	node* el = get_close_part(*c); //)
	*c = (*c)->next; //start_exp
	node* start_exp = *c;
	*c = calc(start_exp, funcall, calling_object, comma, NULL, for_v);
	*c = (*c)->next; //step_exp
	node* step_exp = *c;

	*c = get_first_type(*c, comma)->next;

	node* cond = *c; //cond_exp
	var* bool_var = new_temp_var(T_BOOL);

	//func_deftion* y = new_func();

	//*c = calculate(*c, funcall,calling_object, comma,*c, bool_var);
	*c = calc(cond, funcall, calling_object, (node_type)0, el, bool_var);


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
		compile(calling_object, *c, funcall, for_close, NULL);
		//check the condition again
		calc(step_exp, funcall, calling_object, comma, NULL, for_v);

		calc(cond, funcall, calling_object, (node_type)0, el, bool_var);
	}

	*c = for_close; //getFirstType(c, parse_obj::parentheses1c);
}


void compile_type(node* out, fcall* function_call, node* stop, type_def* b);


void install_class(node** n)
{
	type_def* mtype = new_type();
	eat(n, var_name,true); //class->
	char* cname = (*n)->value_char_ptr;


	mtype->type_name = cname;

	if (eat(n, parentheses4,false))
	{
		if (eat(n, var_name,false))
		{
			mtype->base = get_type_by_name((*n)->value_char_ptr);
		}
		eat(n, parentheses4_c,true);
	}


	node* start = get_first_type(*n, parentheses1);
	node* tm = get_close_part(start);


	compile(NULL, start->next, NULL, tm, mtype);
	*n = tm;
}


var* name_exp_assign(node** nod, fcall* calling_function, var* calling_object)
{
	var* mvar = NULL;
	bool isdot = false;
	while ((*nod)->type_ & (var_name | dot | s_index))
	{
		switch ((*nod)->type_)
		{
		case var_name:
			if ((*nod)->opt_name_type == var_call)
			{
				if (mvar != NULL)
				{
					if (!isdot)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						isdot = false;
						mvar = get_var_by_name_on_stack((*nod)->value_char_ptr, &mvar->value_type_instsance->propertys);
					}
				}
				else
				{
					if (calling_function != NULL)
						mvar = fget_var_by_name_fc((*nod)->value_char_ptr, calling_function);
					if (!mvar && calling_object != NULL)
						mvar = get_var_by_name_on_stack((*nod)->value_char_ptr,
						                                &calling_object->value_type_instsance->propertys);
					if (mvar == NULL)
						mvar = get_globle_var_by_name((*nod)->value_char_ptr);
				}
				step(nod);
			}
			else if ((*nod)->opt_name_type == function_call)
			{
				if (mvar != NULL)
				{
					if (!isdot)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						///find var on mvar functions
						isdot = false;
						func_deftion* name_function = get_obj_function(mvar, (*nod)->value_char_ptr);
						fcall* fcall = create_fcall(name_function);
						//compile_var_name_start(k, funct);
						//compile(mvar, k, funct, y);
						setup_function_parms(nod, fcall, calling_object, calling_function);
						call_function(fcall, calling_object == NULL ? &mvar : &calling_object);
						mvar = &fcall->_return;
						step(nod);
					}
				}
				else
				{
					func_deftion* name_function;
					if (calling_object != NULL)
					{
						name_function = get_obj_function(calling_object, (*nod)->value_char_ptr);
					}
					else
					{
						name_function = get_func_by_name((*nod)->value_char_ptr);
					}

					fcall* fcall = create_fcall(name_function);
					setup_function_parms(nod, fcall, calling_object, calling_function);
					call_function(fcall, &calling_object);
					mvar = &fcall->_return;
					step(nod);
				}
			}

			break;
		case dot:
			isdot = true;
			step(nod);
			break;
		case s_index:
			{
				node* pak = (*nod)->ref_node;
				const int index_value = get_index_value2(nod, calling_function, calling_object);

				mvar = get_array_item(mvar, index_value);
				*nod = pak;
				step(nod);
				break;
			}
		case value:
			{
				mvar = new_temp_var((*nod)->opt_type_ptr);

				mvar->values = install_memory_with_type((*nod)->opt_type_ptr, 1);
				set_value_copy_node(mvar, *nod);
				step(nod);
				break;
			}
		case parentheses4:
			{
				node* pak = (*nod)->ref_node;
				mvar = new_temp_var(NULL);
				//change mvar->value
				*nod = calc(*nod, calling_function, calling_object, 0, (*nod)->ref_node, mvar);
				step(nod);
				break;
			}


		default:
			break;
		}
	}


	return mvar;
}

//return after var_name [X]
var* add_var_to(node** c, fcall* c_function, var* calling_object, type_def* ncalss, type_def* var_type, int vsize)
{
	var* out = NULL;
	char* vname = (*c)->value_char_ptr;
	if (c_function != NULL)
	{
		define_new_var_on_function(vname, c_function, var_type, &out);
	}
	else if (ncalss != NULL) //prop
	{
		define_new_class_prop(vname, ncalss, var_type, &out);
	}
	else
	{
		define_new_var_globle(&out, var_type, vname);
	}

	out->size = vsize;
	return out;
}

node* compile(var* parent, node* out, fcall* c_function, node* stop, type_def* ncalss)
{
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	node* c;
	node* ret = NULL;
	if (stop == NULL)
		in = get_root(out);
	else
		in = out;

	for (c = in; c != NULL; c = c->next)
	{
		ret = c;
		if (stop != NULL && (c == stop || (c->parent != NULL && c->parent == stop)))
		{
			break;
		}
		switch (c->type_)
		{
		case var_name:
			{
				var* x = name_exp_assign(&c, c_function, parent);
				if (c->type_ == equles)
				{
					c = calc(c->next, c_function, parent, none,NULL, x);
				}
			}
			break;
		case itype:
			{
				type_def* var_type = c->value_type;
				int asize = 1;
				if (eat(&c, s_index,false))
				{
					var* vsize = new_temp_var(T_INT);

					c = calc(c->next, c_function, parent, none, c->ref_node, vsize);
					asize = *vsize->value_int;
					free_temp_var(vsize);
				}
				if (eat(&c, var_name,true))
				{
					if (c->opt_name_type == function_def)
					{
						c = add_new_func_code(c, var_type, ncalss);
					}
					else if (c->opt_name_type == var_def)
					{
						var* n_var = add_var_to(&c, c_function, parent, ncalss, var_type, asize);

						if (eat(&c, equles,false))
						{
							c = calc(c->next, c_function, parent, none,NULL, n_var);
						}
						else //endl
						{
							n_var->values = install_memory_with_type(n_var->type_define, n_var->size);
						}
					}
				}

				ret = c;
				break;
			}
		case keyword:
			{
				switch (c->value_keyword)
				{
				case _if_:
				case _else_:
				case _eif_:
					if_eif_function(&c, c_function, parent);
					break;
				case _for_:
					for_function(&c, c_function, parent);
					break;
				case _while_:
					while_function(&c, c_function, parent);
					break;
				case _do_:
					break;

				case _return_:
					if (c_function != NULL)
					{
						var* re = &c_function->_return;

						calc(c->next, c_function, parent, 0, NULL, re);
						c = stop;
						return c;
					}
					break;
				case _break_:
					c = stop;
					return c;

				case _class_:
					install_class(&c);
					break;
				case _static_:
					break;
				}
			}

		default:
			{
			}
		}
		//return from function code


		if (c == NULL)
			break;
	}
	return ret;
}
