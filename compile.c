#include "compile.h"
#include "ximport.h"
#include "xgc.h"
#include "xdiag.h"
#include "xcollection.h"
#include <execinfo.h>
extern int get_index_value2(node** nod, fcall* calling_function, var* calling_object);
extern node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
                  var* calc_result);

static bool g_loop_broken = false;

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
	if (svar != NULL)
	{
		*out_var = svar;
		return;
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

	const bool cons = (container_class != NULL) &&
		(c->value_char_ptr != NULL && container_class->type_name != NULL && strcmp(c->value_char_ptr, container_class->type_name) == 0);

	func_deftion* m = container_class != NULL
		                  ? container_class->d_functions + (container_class->d_function_size++)
		                  : new_func();

	memset(m, 0, sizeof(func_deftion));
	m->return_type = cons ? container_class : return_type;
	m->function_type = cons ? constr : (container_class != NULL ? class_function : f_main);
	m->func_name = c->value_char_ptr;

	if (container_class == NULL)
	{
		var* funcvr = new_var(m->func_name, T_FUNC);
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
		char errmsg[256];
		snprintf(errmsg, sizeof(errmsg), "missing closing '}' in function '%s'", m->func_name ? m->func_name : "anonymous");
		xdiag_report(DIAG_ERROR, "E0004", NULL, func_decl->line, func_decl->col > 0 ? func_decl->col : 1, 1,
		             NULL, errmsg, "ensure each opening brace '{' has a matching closing brace '}'");
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

//change node pos and return close ) 
node* setup_function_parms(node** nod, fcall* function, var* context, fcall* in_function)
{
	*nod = get_first_type(*nod, parentheses4);
	node* close = get_close_part(*nod);
	//var* y =  function->func_parmeters;
	//int m = function->deftion->start_parm_count;
	int i = 0;
	step_forwrod(nod);

	while (*nod != close && (*nod)->type_ != endl && i < 100)
	{
		var* n = function->func_parmeters + (i++);
		function->parm_count_c = i;
		n->values = NULL;
		n->type_define = NULL;
		n->size = 1;

		node* before_calc = *nod;
		*nod = calc(*nod, in_function, context, comma, close, n);
		if (*nod == NULL)
			return close;
		if (*nod == before_calc)
		{
			*nod = (*nod)->next;
			if (*nod == NULL) break;
		}

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
	if (mfunc->deftion->function_type == constr && (*context == NULL || (*context)->values == NULL))
	{
		mfunc->_return.values = install_memory_with_type(mfunc->_return.type_define, 1);
		*context = &mfunc->_return;
	}
	if (*context != NULL && (*context)->value_type_instsance == NULL && (*context)->values != NULL && !is_base_type((*context)->type_define))
	{
		(*context)->value_type_instsance = (type_instance*)(*context)->values;
	}
	mfunc->context = *context;
	mfunc->has_returned = false;

	mfunc->deftion->func_code(mfunc);
	gc_pop_frame();

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
			free_temp_var(bool_result);
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
	node* lcond = ifeif->ref_node;

	if ((*cx)->value_keyword == _if_ || (lcond != NULL && lcond->taked == false))
	{
		if ((*cx)->value_keyword != _else_)
		{
			var* bool_result = new_temp_var(T_BOOL);

			eat(cx, parentheses4,true);
			node* el = get_close_part(*cx);
			*cx = calc((*cx)->next, cfunction, calling_obj, none, el, bool_result);

			*cx = get_first_type(*cx, parentheses1);
			close = get_close_part((*cx));
			*cx = (*cx)->next;
			bool cond_val = (bool_result->value_bool != NULL) ? *bool_result->value_bool : false;
			free_temp_var(bool_result);
			if (cond_val) /// true
			{
				ifeif->taked = true;
				compile(calling_obj, *cx, cfunction, close, NULL);
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
	gc_add_root(m);

	node* el = get_close_part((*c)->next);
	node* cond = (*c)->next->next;
	*c = calc(cond, temp, calling_obj, none, el, m);

	*c = get_first_type(*c, parentheses1);
	node* close = get_close_part(*c);
	*c = (*c)->next;

	while (*m->value_bool)
	{
		compile(calling_obj, *c, temp, close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (temp != NULL && temp->has_returned)
			break;
		calc(cond, temp, calling_obj, (node_type)0, el, m);
		gc_check_auto();
	}

	gc_remove_root(m);
	free_temp_var(m);
	*c = close;
}

void do_while_function(node** c, fcall* temp, var* calling_obj)
{
	var* m = new_temp_var(T_BOOL);
	gc_add_root(m);

	node* body_open = get_first_type(*c, parentheses1);
	if (body_open == NULL)
	{
		gc_remove_root(m);
		free_temp_var(m);
		return;
	}
	node* body_close = get_close_part(body_open);
	if (body_close == NULL)
	{
		gc_remove_root(m);
		free_temp_var(m);
		return;
	}
	node* body_first = body_open->next;

	node* while_node = get_first_type_with_value(body_close, keyword, (void*)(intptr_t)_while_);
	if (while_node == NULL)
	{
		gc_remove_root(m);
		free_temp_var(m);
		*c = body_close;
		return;
	}

	node* cond_open = get_first_type(while_node, parentheses4);
	if (cond_open == NULL)
	{
		gc_remove_root(m);
		free_temp_var(m);
		*c = body_close;
		return;
	}
	node* cond_close = get_close_part(cond_open);
	node* cond = cond_open->next;

	do
	{
		compile(calling_obj, body_first, temp, body_close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (temp != NULL && temp->has_returned)
			break;
		calc(cond, temp, calling_obj, (node_type)0, cond_close, m);
		gc_check_auto();
	} while (*m->value_bool);

	gc_remove_root(m);
	free_temp_var(m);
	*c = cond_close;
}

static void assign_loop_var(var* loop_v, var* item_v)
{
	if (loop_v == NULL || item_v == NULL) return;
	loop_v->type_define = item_v->type_define;
	loop_v->size = 1;
	loop_v->values = install_memory_with_type(item_v->type_define, 1);
	if (item_v->type_define == T_INT && item_v->value_int != NULL)
	{
		*loop_v->value_int = *item_v->value_int;
	}
	else if (item_v->type_define == T_FLOAT && item_v->value_float != NULL)
	{
		*loop_v->value_float = *item_v->value_float;
	}
	else if (item_v->type_define == T_STRING && item_v->value_str_ptr != NULL && *item_v->value_str_ptr != NULL)
	{
		*loop_v->value_str_ptr = (char*)gc_calloc(1, strlen(*item_v->value_str_ptr) + 1, GC_KIND_STRING);
		strcpy(*loop_v->value_str_ptr, *item_v->value_str_ptr);
	}
	else if (item_v->type_define == T_BOOL && item_v->value_bool != NULL)
	{
		*loop_v->value_bool = *item_v->value_bool;
	}
	else if (item_v->type_define == T_LONG && item_v->value_long != NULL)
	{
		*loop_v->value_long = *item_v->value_long;
	}
	else
	{
		loop_v->values = item_v->values;
		loop_v->value_type_instsance = item_v->value_type_instsance;
	}
}

//for VAR_NAME ((start)EXP,(end)EXP[cond]) or for (item in coll) or for item in coll
void for_function(node** c, fcall* funcall, var* calling_object)
{
	node* p_for = *c;
	node* first = p_for->next;
	if (first == NULL) return;

	bool is_for_in = false;
	char* var_name_str = NULL;
	node* coll_start = NULL;
	node* stop_in_node = NULL;
	node* body_open = NULL;
	node* body_close = NULL;

	if (first->type_ == parentheses4 && first->next != NULL &&
	    first->next->type_ == var_name && first->next->next != NULL &&
	    first->next->next->type_ == keyword && first->next->next->value_keyword == _in_)
	{
		/* for (item in coll) { ... } */
		is_for_in = true;
		var_name_str = first->next->value_char_ptr;
		coll_start = first->next->next->next;
		node* p_close = get_close_part(first);
		stop_in_node = p_close;
		body_open = get_first_type(p_close, parentheses1);
		body_close = get_close_part(body_open);
	}
	else if (first->type_ == var_name && first->next != NULL &&
	         first->next->type_ == keyword && first->next->value_keyword == _in_)
	{
		/* for item in coll { ... } */
		is_for_in = true;
		var_name_str = first->value_char_ptr;
		coll_start = first->next->next;
		body_open = get_first_type(coll_start, parentheses1);
		stop_in_node = body_open;
		body_close = get_close_part(body_open);
	}

	if (is_for_in)
	{
		var coll_var;
		memset(&coll_var, 0, sizeof(var));
		calc(coll_start, funcall, calling_object, (stop_in_node == body_open) ? parentheses1 : (node_type)0, stop_in_node, &coll_var);

		var* loop_v = all_get_var_by_name(var_name_str, funcall, calling_object);
		if (loop_v == NULL)
		{
			if (funcall != NULL)
				define_new_var_on_function(var_name_str, funcall, T_STRING, &loop_v);
			else
				define_new_var_globle(&loop_v, T_STRING, var_name_str);
		}

		/* Check if collection is List */
		int list_id = -1;
		if (coll_var.type_define != NULL && strcmp(coll_var.type_define->type_name, "List") == 0)
		{
			if (coll_var.value_type_instsance != NULL)
			{
				var* id_prop = get_var_by_name_on_stack("id", &coll_var.value_type_instsance->propertys);
				if (id_prop != NULL && id_prop->value_int != NULL)
					list_id = *id_prop->value_int;
			}
		}
		else if (coll_var.type_define == T_INT && coll_var.value_int != NULL)
		{
			if (x_list_count(*coll_var.value_int) >= 0)
				list_id = *coll_var.value_int;
		}

		/* Check if collection is Map */
		int map_id = -1;
		if (coll_var.type_define != NULL && strcmp(coll_var.type_define->type_name, "Map") == 0)
		{
			if (coll_var.value_type_instsance != NULL)
			{
				var* id_prop = get_var_by_name_on_stack("id", &coll_var.value_type_instsance->propertys);
				if (id_prop != NULL && id_prop->value_int != NULL)
					map_id = *id_prop->value_int;
			}
		}

		if (list_id > 0)
		{
			int total = x_list_count(list_id);
			for (int i = 0; i < total; i++)
			{
				var* it_var = x_list_get_var(list_id, i);
				assign_loop_var(loop_v, it_var);
				compile(calling_object, body_open->next, funcall, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (funcall != NULL && funcall->has_returned)
					break;
				gc_check_auto();
			}
		}
		else if (map_id > 0)
		{
			char** keys = NULL;
			int total = x_map_get_all_keys(map_id, &keys);
			for (int i = 0; i < total; i++)
			{
				char* pkey = keys[i];
				var key_var;
				memset(&key_var, 0, sizeof(var));
				key_var.type_define = T_STRING;
				key_var.size = 1;
				key_var.value_str_ptr = &pkey;
				assign_loop_var(loop_v, &key_var);

				compile(calling_object, body_open->next, funcall, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (funcall != NULL && funcall->has_returned)
					break;
				gc_check_auto();
			}
			if (keys != NULL)
				free(keys);
		}
		else if (coll_var.type_define == T_STRING && coll_var.value_str_ptr != NULL && *coll_var.value_str_ptr != NULL)
		{
			char* str = *coll_var.value_str_ptr;
			int total = (int)strlen(str);
			for (int i = 0; i < total; i++)
			{
				char ch_buf[2] = { str[i], '\0' };
				char* pch = ch_buf;
				var ch_var;
				memset(&ch_var, 0, sizeof(var));
				ch_var.type_define = T_STRING;
				ch_var.size = 1;
				ch_var.value_str_ptr = &pch;
				assign_loop_var(loop_v, &ch_var);

				compile(calling_object, body_open->next, funcall, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (funcall != NULL && funcall->has_returned)
					break;
				gc_check_auto();
			}
		}

		*c = body_close;
		return;
	}

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
	gc_add_root(bool_var);

	*c = calc(cond, funcall, calling_object, (node_type)0, el, bool_var);

	*c = get_first_type(*c, parentheses1);
	node* for_close = get_close_part(*c);
	*c = (*c)->next;

	while (*bool_var->value_bool)
	{
		compile(calling_object, *c, funcall, for_close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (funcall != NULL && funcall->has_returned)
			break;
		calc(step_exp, funcall, calling_object, comma, NULL, for_v);
		calc(cond, funcall, calling_object, (node_type)0, el, bool_var);
		gc_check_auto();
	}

	gc_remove_root(bool_var);
	free_temp_var(bool_var);
	*c = for_close;
}


void compile_type(node* out, fcall* function_call, node* stop, type_def* b);


void install_class(node** n)
{
	eat(n, var_name,true); //class->
	char* cname = (*n)->value_char_ptr;
	type_def* mtype = get_type_by_name(cname);
	if (mtype == NULL)
		mtype = new_type();
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
	type_def* static_class = NULL;
	while (*nod != NULL && ((*nod)->type_ & (var_name | dot | s_index | itype)))
	{
		switch ((*nod)->type_)
		{
		case itype:
			static_class = (*nod)->value_type;
			step(nod);
			break;
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
						if (mvar->value_type_instsance != NULL)
							mvar = get_var_by_name_on_stack((*nod)->value_char_ptr, &mvar->value_type_instsance->propertys);
						else
							mvar = NULL;
					}
				}
				else if (static_class != NULL)
				{
					if (!isdot)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						isdot = false;
						mvar = get_class_property(static_class, (*nod)->value_char_ptr);
						static_class = NULL;
					}
				}
				else
				{
					type_def* td = get_type_by_name((*nod)->value_char_ptr);
					if (td != NULL && (*nod)->next != NULL && (*nod)->next->type_ == dot)
					{
						static_class = td;
					}
					else
					{
						if (calling_function != NULL)
							mvar = fget_var_by_name_fc((*nod)->value_char_ptr, calling_function);
						if (!mvar && calling_object != NULL)
						{
							type_instance* inst = calling_object->value_type_instsance;
							if (inst == NULL && calling_object->values != NULL && !is_base_type(calling_object->type_define))
								inst = (type_instance*)calling_object->values;
							if (inst != NULL)
								mvar = get_var_by_name_on_stack((*nod)->value_char_ptr, &inst->propertys);
						}
						if (mvar == NULL)
							mvar = get_globle_var_by_name((*nod)->value_char_ptr);
						if (mvar == NULL || mvar->type_define == T_FUNC)
						{
							if (td != NULL)
							{
								static_class = td;
								mvar = NULL;
							}
						}
					}
				}
				step(nod);
			}
			else if ((*nod)->opt_name_type == function_call)
			{
				if (static_class != NULL)
				{
					isdot = false;
					func_deftion* name_function = get_class_function(static_class, (*nod)->value_char_ptr);
					if (name_function == NULL)
					{
						printf("ERROR on line %d: static method '%s' not found in class '%s'\n",
							(*nod)->line, (*nod)->value_char_ptr, static_class->type_name);
						step(nod);
						break;
					}
					fcall* fc = create_fcall(name_function);
					setup_function_parms(nod, fc, calling_object, calling_function);
					var* self = NULL;
					call_function(fc, &self);
					mvar = &fc->_return;
					static_class = NULL;
					step(nod);
				}
				else if (mvar != NULL)
				{
					if (!isdot)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						///find var on mvar functions
						isdot = false;
						func_deftion* name_function = get_obj_function(mvar, (*nod)->value_char_ptr);
						if (name_function == NULL)
						{
							printf("ERROR on line %d: method '%s' not found\n", (*nod)->line, (*nod)->value_char_ptr);
							step(nod);
							break;
						}
						fcall* fcall = create_fcall(name_function);
						setup_function_parms(nod, fcall, calling_object, calling_function);
						call_function(fcall, &mvar);
						mvar = &fcall->_return;
						step(nod);
					}
				}
				else
				{
					func_deftion* name_function = NULL;
					if (calling_object != NULL)
					{
						name_function = get_obj_function(calling_object, (*nod)->value_char_ptr);
					}
					if (name_function == NULL)
					{
						name_function = get_func_by_name((*nod)->value_char_ptr);
					}
					if (name_function == NULL)
					{
						type_def* td = get_type_by_name((*nod)->value_char_ptr);
						if (td != NULL)
						{
							for (int i = 0; i < td->d_function_size; i++)
							{
								if (td->d_functions[i].func_name != NULL &&
									(strcmp(td->d_functions[i].func_name, td->type_name) == 0 ||
									 td->d_functions[i].function_type == constr))
								{
									name_function = &td->d_functions[i];
									break;
								}
							}
						}
					}
					if (name_function == NULL && calling_function != NULL && calling_function->deftion != NULL)
					{
						type_def* enc = get_class_of_function(calling_function->deftion);
						if (enc != NULL)
							name_function = get_class_function(enc, (*nod)->value_char_ptr);
					}

					if (name_function == NULL)
					{
						printf("ERROR on line %d: function '%s' not found\n", (*nod)->line, (*nod)->value_char_ptr);
						step(nod);
						break;
					}

					var* self = (calling_object != NULL && get_obj_function(calling_object, (*nod)->value_char_ptr) != NULL)
						? calling_object : NULL;
					fcall* fcall = create_fcall(name_function);
					setup_function_parms(nod, fcall, calling_object, calling_function);
					call_function(fcall, &self);
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
				mvar = new_temp_var(NULL);
				//change mvar->value
				*nod = calc(*nod, calling_function, calling_object, 0, (*nod)->ref_node, mvar);
				step(nod);
				break;
			}

		default:
			step(nod);
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

static node* handle_assign_or_compound(node* c, var* x, fcall* c_function, var* parent)
{
	if (c == NULL)
		return NULL;

	if (c->type_ == equles)
	{
		c = calc(c->next, c_function, parent, none, NULL, x);
	}
	else if (c->type_ == operators_n && c->next != NULL && c->next->type_ == equles)
	{
		/* Compound assignment: +=, -=, *=, /=, %= */
		char op = c->value_char_ptr ? *c->value_char_ptr : '+';
		var* rhs = new_temp_var(x ? x->type_define : NULL);
		c = calc(c->next->next, c_function, parent, none, NULL, rhs);
		if (x != NULL && rhs != NULL)
		{
			if (x->type_define == T_INT && x->value_int != NULL)
			{
				int r = (rhs->type_define == T_INT && rhs->value_int != NULL) ? *rhs->value_int :
				        (rhs->type_define == T_FLOAT && rhs->value_float != NULL) ? (int)*rhs->value_float :
				        (rhs->type_define == T_LONG && rhs->value_long != NULL) ? (int)*rhs->value_long : 0;
				if (op == '+') *x->value_int += r;
				else if (op == '-') *x->value_int -= r;
				else if (op == '*') *x->value_int *= r;
				else if (op == '/' && r != 0) *x->value_int /= r;
				else if (op == '%' && r != 0) *x->value_int %= r;
			}
			else if (x->type_define == T_FLOAT && x->value_float != NULL)
			{
				float r = (rhs->type_define == T_FLOAT && rhs->value_float != NULL) ? *rhs->value_float :
				          (rhs->type_define == T_INT && rhs->value_int != NULL) ? (float)*rhs->value_int : 0.0f;
				if (op == '+') *x->value_float += r;
				else if (op == '-') *x->value_float -= r;
				else if (op == '*') *x->value_float *= r;
				else if (op == '/' && r != 0.0f) *x->value_float /= r;
			}
			else if (x->type_define == T_LONG && x->value_long != NULL)
			{
				long r = (rhs->type_define == T_LONG && rhs->value_long != NULL) ? *rhs->value_long :
				         (rhs->type_define == T_INT && rhs->value_int != NULL) ? (long)*rhs->value_int : 0L;
				if (op == '+') *x->value_long += r;
				else if (op == '-') *x->value_long -= r;
				else if (op == '*') *x->value_long *= r;
				else if (op == '/' && r != 0) *x->value_long /= r;
				else if (op == '%' && r != 0) *x->value_long %= r;
			}
			else if (x->type_define == T_STRING && op == '+')
			{
				const char* rstr = (rhs->type_define == T_STRING && rhs->value_str_ptr != NULL && *rhs->value_str_ptr != NULL) ? *rhs->value_str_ptr :
				                   (rhs->value_char_ptr != NULL) ? rhs->value_char_ptr : "";
				const char* xstr = (x->value_str_ptr != NULL && *x->value_str_ptr != NULL) ? *x->value_str_ptr :
				                   (x->value_char_ptr != NULL) ? x->value_char_ptr : "";
				size_t xlen = strlen(xstr);
				size_t rlen = strlen(rstr);
				char* new_str = (char*)gc_malloc(xlen + rlen + 1, GC_KIND_STRING);
				if (new_str != NULL)
				{
					memcpy(new_str, xstr, xlen);
					memcpy(new_str + xlen, rstr, rlen + 1);
					if (x->value_str_ptr != NULL)
						*x->value_str_ptr = new_str;
					else
						x->value_char_ptr = new_str;
				}
			}
		}
		free_temp_var(rhs);
	}
	else if (c->type_ == operators_n && c->next != NULL && c->next->type_ == operators_n &&
	         c->value_char_ptr != NULL && c->next->value_char_ptr != NULL &&
	         *c->value_char_ptr == *c->next->value_char_ptr)
	{
		/* Increment / Decrement: ++, -- */
		char op = *c->value_char_ptr;
		if (x != NULL)
		{
			if (x->type_define == T_INT && x->value_int != NULL)
			{
				if (op == '+') (*x->value_int)++;
				else if (op == '-') (*x->value_int)--;
			}
			else if (x->type_define == T_FLOAT && x->value_float != NULL)
			{
				if (op == '+') (*x->value_float) += 1.0f;
				else if (op == '-') (*x->value_float) -= 1.0f;
			}
			else if (x->type_define == T_LONG && x->value_long != NULL)
			{
				if (op == '+') (*x->value_long)++;
				else if (op == '-') (*x->value_long)--;
			}
		}
		c = c->next->next;
		while (c != NULL && c->type_ != endl)
			c = c->next;
	}
	else
	{
		while (c != NULL && c->type_ != endl)
			c = c->next;
	}
	return c;
}

node* compile(var* parent, node* out, fcall* c_function, node* stop, type_def* ncalss)
{
	//func* temp=NULL;
	/////call from function them self....rooted already
	node* in;
	node* c;
	node* ret = NULL;
	bool is_static_decl = false;
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
		if (c_function != NULL && c_function->has_returned)
		{
			break;
		}
		if (g_loop_broken)
		{
			break;
		}

		switch (c->type_)
		{
		case var_name:
			{
				is_static_decl = false;
				var* x = name_exp_assign(&c, c_function, parent);
				c = handle_assign_or_compound(c, x, c_function, parent);
			}
			break;
		case itype:
			{
				if (c->next != NULL && c->next->type_ == dot)
				{
					is_static_decl = false;
					var* x = name_exp_assign(&c, c_function, parent);
					c = handle_assign_or_compound(c, x, c_function, parent);
					break;
				}
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
						if (is_static_decl && ncalss != NULL && ncalss->d_function_size > 0)
						{
							ncalss->d_functions[ncalss->d_function_size - 1].access = STATIC;
						}
						is_static_decl = false;
					}
					else if (c->opt_name_type == var_def)
					{
						var* n_var = add_var_to(&c, c_function, parent, ncalss, var_type, asize);
						if (is_static_decl && n_var != NULL)
						{
							n_var->access = STATIC;
						}
						is_static_decl = false;

						if (eat(&c, equles,false))
						{
							c = calc(c->next, c_function, parent, none,NULL, n_var);
						}
						else if (c->next != NULL && c->next->type_ == parentheses4)
						{
							n_var->values = install_memory_with_type(n_var->type_define, n_var->size);
							if (!is_base_type(n_var->type_define))
								n_var->value_type_instsance = (type_instance*)n_var->values;

							node* p4 = c->next;
							node* pclose = get_close_part(p4);
							int arg_count = 0;
							if (p4->next != pclose)
							{
								int commas = 0;
								for (node* a = p4->next; a != NULL && a != pclose; a = a->next)
								{
									if (a->type_ == comma)
										commas++;
								}
								arg_count = commas + 1;
							}

							func_deftion* constr_fn = NULL;
							for (type_def* curr = var_type; curr != NULL; curr = curr->base)
							{
								for (int i = 0; i < curr->d_function_size; i++)
								{
									if (curr->d_functions[i].func_name != NULL &&
										(strcmp(curr->d_functions[i].func_name, var_type->type_name) == 0 ||
										 curr->d_functions[i].function_type == constr))
									{
										if (curr->d_functions[i].start_parm_count == arg_count)
										{
											constr_fn = &curr->d_functions[i];
											break;
										}
										if (constr_fn == NULL)
											constr_fn = &curr->d_functions[i];
									}
								}
								if (constr_fn != NULL && constr_fn->start_parm_count == arg_count) break;
							}

							if (constr_fn != NULL)
							{
								fcall* fc = create_fcall(constr_fn);
								node* close = setup_function_parms(&p4, fc, parent, c_function);
								call_function(fc, &n_var);
								gc_free_any(fc);
								c = close;
							}
							else
							{
								printf("ERROR on line %d: constructor for '%s' with %d parameters not found\n",
								       c->line, var_type->type_name, arg_count);
							}
						}
						else //endl
						{
							n_var->values = install_memory_with_type(n_var->type_define, n_var->size);
							if (!is_base_type(n_var->type_define))
								n_var->value_type_instsance = (type_instance*)n_var->values;
							func_deftion* constr_fn = NULL;
							for (type_def* curr = var_type; curr != NULL; curr = curr->base)
							{
								for (int i = 0; i < curr->d_function_size; i++)
								{
									if (curr->d_functions[i].func_name != NULL &&
										(strcmp(curr->d_functions[i].func_name, var_type->type_name) == 0 ||
										 curr->d_functions[i].function_type == constr))
									{
										if (curr->d_functions[i].start_parm_count == 0)
										{
											constr_fn = &curr->d_functions[i];
											break;
										}
									}
								}
								if (constr_fn != NULL) break;
							}
							if (constr_fn != NULL)
							{
								fcall* fc = create_fcall(constr_fn);
								call_function(fc, &n_var);
								gc_free_any(fc);
							}
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
					do_while_function(&c, c_function, parent);
					break;

				case _return_:
					if (c_function != NULL)
					{
						var* re = &c_function->_return;

						calc(c->next, c_function, parent, 0, NULL, re);
						c_function->has_returned = true;
					}
					c = stop;
					return c;
				case _break_:
					g_loop_broken = true;
					c = stop;
					return c;

				case _class_:
					is_static_decl = false;
					install_class(&c);
					break;
				case _static_:
					is_static_decl = true;
					break;
				case _import_:
					{
						const char* mod_name = NULL;
						node* p = c->next;
						if (p != NULL && p->type_ == parentheses4)
						{
							p = p->next;
						}
						if (p != NULL)
						{
							if (p->type_ == value && p->opt_type_ptr == T_STRING)
							{
								mod_name = (char*)p->value_raw;
							}
							else if (p->type_ == var_name && p->value_char_ptr != NULL)
							{
								mod_name = p->value_char_ptr;
							}
						}
						if (mod_name != NULL)
						{
							x_import_module(mod_name);
						}
						while (c != NULL && c->type_ != endl && c != stop)
						{
							c = c->next;
						}
						break;
					}
				case _new_:
				case _in_:
					break;
				}
			}

		default:
			{
			}
		}
		if (c_function != NULL && c_function->has_returned)
		{
			c = stop;
			break;
		}
		//return from function code
		gc_check_auto();

		if (c == NULL)
			break;
	}
	return ret;
}
