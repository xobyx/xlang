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

void define_class_property(char* name, type_def* container_class, type_def* prop_type, var** out_var)
{
	if (container_class != NULL)
	{
		for (int i = 0; i < container_class->d_propertys_size; i++)
		{
			if (container_class->d_propertys[i].name == name)
			{
				*out_var = &container_class->d_propertys[i];
				return;
			}
		}

		int psize = container_class->d_propertys_size;
		var* ivar = &container_class->d_propertys[psize];
		ivar->type_define = prop_type;
		ivar->name = name;
		ivar->slot_idx = psize;
		ivar->access = PUBLIC;

		FieldDescriptor* fd = &container_class->field_descriptors[psize];
		fd->name = name;
		fd->type_define = prop_type;
		fd->slot_idx = psize;
		fd->access = PUBLIC;

		container_class->d_propertys_size++;
		*out_var = ivar;
	}
}

void define_new_class_prop(char* name, type_def* contern_class, type_def* new_var_type, var** out_var)
{
	define_class_property(name, contern_class, new_var_type, out_var);
}

void define_function_var(char* name, fcall* func_call, type_def* var_type, var** out_var)
{
	var* svar = get_function_var_by_name(name, func_call);
	if (svar != NULL)
	{
		*out_var = svar;
		return;
	}
	int count = func_call->parm_count_c;
	var* params = func_call->func_parmeters;
	params[count].name = name;
	params[count].type_define = var_type;
	*out_var = &params[count];
	func_call->parm_count_c++;
}

void define_new_var_on_function(char* name, fcall* mfun, type_def* new_var_type, var** out_var)
{
	define_function_var(name, mfun, new_var_type, out_var);
}

void define_global_var(var** out_var, type_def* var_type, char* name)
{
	var* v = new_var(name, var_type);
	*out_var = v;
}

void define_new_var_globle(var** out_var, type_def* new_var_type, char* name)
{
	define_global_var(out_var, new_var_type, name);
}

node* add_new_func_code(node* c, type_def* return_type, type_def* container_class)
{
	const bool is_constructor = (container_class != NULL) &&
		(c->value_char_ptr != NULL && container_class->type_name != NULL && strcmp(c->value_char_ptr, container_class->type_name) == 0);

	func_deftion* fn_def = container_class != NULL
		                      ? &container_class->d_functions[container_class->d_function_size++]
		                      : new_func();

	memset(fn_def, 0, sizeof(func_deftion));
	fn_def->return_type = is_constructor ? container_class : return_type;
	fn_def->function_type = is_constructor ? constr : (container_class != NULL ? class_function : f_main);
	fn_def->func_name = c->value_char_ptr;

	if (container_class == NULL)
	{
		var* funcvr = new_var(fn_def->func_name, T_FUNC);
		funcvr->value_func = fn_def;
	}

	// parse parameters
	c = c->next; // (
	node* close = get_close_part(c);
	if (c->type_ != parentheses4 || close == NULL)
	{
		printf("ERROR: missing '()' for function %s on line %d in %s:%d\n", fn_def->func_name, c->line, __FILE__, __LINE__);
		exit(-1);
	}

	int i = 0;
	while (c != close)
	{
		if (c->type_ == itype)
		{
			fn_def->start_func_parmeters[i] = c->value_type;
			fn_def->start_func_parmeters_name[i] = (char*)c->next->value_raw;
			i++;
			c = c->next;
		}
		fn_def->start_parm_count = i;
		c = c->next;
	}

	node* func_decl = get_first_type(c, parentheses1);
	node* end = get_close_part(func_decl);
	if (end == NULL)
	{
		char errmsg[256];
		snprintf(errmsg, sizeof(errmsg), "missing closing '}' in function '%s'", fn_def->func_name ? fn_def->func_name : "anonymous");
		xdiag_report(DIAG_ERROR, "E0004", NULL, func_decl->line, func_decl->col > 0 ? func_decl->col : 1, 1,
		             NULL, errmsg, "ensure each opening brace '{' has a matching closing brace '}'");
		exit(-1);
	}
	fn_def->func_code = &call_func_in;
	fn_def->ref = func_decl;

	return end->next;
}

void step_forward(node** current_node)
{
	if (current_node != NULL && *current_node != NULL)
		*current_node = (*current_node)->next;
}

void step_forwrod(node** nod)
{
	step_forward(nod);
}

// advance node position and return closing ')'
node* setup_function_params(node** current_node, fcall* func_call, var* context, fcall* caller_function)
{
	*current_node = get_first_type(*current_node, parentheses4);
	node* close = get_close_part(*current_node);
	int i = 0;
	step_forward(current_node);

	while (*current_node != close && (*current_node)->type_ != endl && i < 100)
	{
		var* param = &func_call->func_parmeters[i++];
		func_call->parm_count_c = i;
		param->values = NULL;
		param->type_define = NULL;
		param->size = 1;

		node* before_calc = *current_node;
		*current_node = calc(*current_node, caller_function, context, comma, close, param);
		if (*current_node == NULL)
			return close;
		if (*current_node == before_calc)
		{
			*current_node = (*current_node)->next;
			if (*current_node == NULL) break;
		}

		if ((*current_node)->type_ == comma)
		{
			*current_node = (*current_node)->next;
		}
	}
	func_call->parm_count_c = i;

	return close;
}

node* setup_function_parms(node** nod, fcall* function, var* context, fcall* in_function)
{
	return setup_function_params(nod, function, context, in_function);
}

bool call_function(fcall* mfunc, var** context)
{
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

	mfunc->deftion->func_code(mfunc);
	gc_pop_frame();

	return false;
}

typedef struct if_block
{
	byte setted;
	byte value;
} if_block;


node* get_last_jump(node* b)
{
	node* i = b;
	do
	{
		i = i->next_jump;
	}
	while (i->next_jump != NULL);
	return i;
}

node* gelastjump(node* b)
{
	return get_last_jump(b);
}

void eval_if_stmt(node** cx, fcall* calling_function, var* context_obj)
{
	node* if_node = *cx;
	node* prev_cond = if_node->ref_node;
	node* close = NULL;
	bool skip = (*cx)->value_keyword != _if_ && (prev_cond != NULL && prev_cond->taked == true);

	if ((*cx)->value_keyword != _else_)
	{
		eat(cx, parentheses4, true); // (
		if (skip == false)
		{
			eat(cx, endl, false);
			var* bool_result = new_temp_var(T_BOOL);
			calc((*cx)->next, calling_function, context_obj, none, (*cx)->ref_node, bool_result);
			skip = ! *bool_result->value_bool;
			free_temp_var(bool_result);
		}
		*cx = (*cx)->ref_node;
	}

	eat(cx, endl, false);
	eat(cx, parentheses1, true);
	close = get_close_part(*cx);

	if (skip)
	{
		if_node->taked = prev_cond != NULL ? prev_cond->taked : false;
	}
	else
	{
		if_node->taked = true;
		compile(context_obj, *cx, calling_function, close, NULL);
	}
	*cx = close;
}

void if_eif_function(node** cx, fcall* cfunction, var* calling_obj)
{
	eval_if_stmt(cx, cfunction, calling_obj);
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


void eval_while_stmt(node** current_node, fcall* calling_function, var* context_obj)
{
	var* cond_var = new_temp_var(T_BOOL);
	gc_add_root(cond_var);

	node* el = get_close_part((*current_node)->next);
	node* cond = (*current_node)->next->next;
	*current_node = calc(cond, calling_function, context_obj, none, el, cond_var);

	*current_node = get_first_type(*current_node, parentheses1);
	node* close = get_close_part(*current_node);
	*current_node = (*current_node)->next;

	while (*cond_var->value_bool)
	{
		compile(context_obj, *current_node, calling_function, close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (calling_function != NULL && calling_function->has_returned)
			break;
		calc(cond, calling_function, context_obj, (node_type)0, el, cond_var);
		gc_check_auto();
	}

	gc_remove_root(cond_var);
	free_temp_var(cond_var);
	*current_node = close;
}

void while_function(node** c, fcall* temp, var* calling_obj)
{
	eval_while_stmt(c, temp, calling_obj);
}

void eval_do_while_stmt(node** current_node, fcall* calling_function, var* context_obj)
{
	var* cond_var = new_temp_var(T_BOOL);
	gc_add_root(cond_var);

	node* body_open = get_first_type(*current_node, parentheses1);
	if (body_open == NULL)
	{
		gc_remove_root(cond_var);
		free_temp_var(cond_var);
		return;
	}
	node* body_close = get_close_part(body_open);
	if (body_close == NULL)
	{
		gc_remove_root(cond_var);
		free_temp_var(cond_var);
		return;
	}
	node* body_first = body_open->next;

	node* while_node = get_first_type_with_value(body_close, keyword, (void*)(intptr_t)_while_);
	if (while_node == NULL)
	{
		gc_remove_root(cond_var);
		free_temp_var(cond_var);
		*current_node = body_close;
		return;
	}

	node* cond_open = get_first_type(while_node, parentheses4);
	if (cond_open == NULL)
	{
		gc_remove_root(cond_var);
		free_temp_var(cond_var);
		*current_node = body_close;
		return;
	}
	node* cond_close = get_close_part(cond_open);
	node* cond = cond_open->next;

	do
	{
		compile(context_obj, body_first, calling_function, body_close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (calling_function != NULL && calling_function->has_returned)
			break;
		calc(cond, calling_function, context_obj, (node_type)0, cond_close, cond_var);
		gc_check_auto();
	} while (*cond_var->value_bool);

	gc_remove_root(cond_var);
	free_temp_var(cond_var);
	*current_node = cond_close;
}

void do_while_function(node** c, fcall* temp, var* calling_obj)
{
	eval_do_while_stmt(c, temp, calling_obj);
}

static void assign_loop_var(var* loop_v, var* item_v)
{
	if (loop_v == NULL || item_v == NULL) return;
	loop_v->type_define = item_v->type_define;
	loop_v->size = 1;
	loop_v->values = install_memory(loop_v);
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

void eval_for_stmt(node** current_node, fcall* calling_function, var* context_obj)
{
	node* p_for = *current_node;
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
		calc(coll_start, calling_function, context_obj, (stop_in_node == body_open) ? parentheses1 : (node_type)0, stop_in_node, &coll_var);

		var* loop_v = find_var_in_scope(var_name_str, calling_function, context_obj);
		if (loop_v == NULL)
		{
			if (calling_function != NULL)
				define_function_var(var_name_str, calling_function, T_STRING, &loop_v);
			else
				define_global_var(&loop_v, T_STRING, var_name_str);
		}

		/* Check if collection is List */
		int list_id = -1;
		if (coll_var.type_define != NULL && strcmp(coll_var.type_define->type_name, "List") == 0)
		{
			if (coll_var.value_type_instsance != NULL)
			{
				list_id = type_instance_get_id(coll_var.value_type_instsance);
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
				map_id = type_instance_get_id(coll_var.value_type_instsance);
			}
		}

		if (list_id > 0)
		{
			int total = x_list_count(list_id);
			for (int i = 0; i < total; i++)
			{
				var* it_var = x_list_get_var(list_id, i);
				assign_loop_var(loop_v, it_var);
				compile(context_obj, body_open->next, calling_function, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (calling_function != NULL && calling_function->has_returned)
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

				compile(context_obj, body_open->next, calling_function, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (calling_function != NULL && calling_function->has_returned)
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

				compile(context_obj, body_open->next, calling_function, body_close, NULL);
				if (g_loop_broken)
				{
					g_loop_broken = false;
					break;
				}
				if (calling_function != NULL && calling_function->has_returned)
					break;
				gc_check_auto();
			}
		}

		*current_node = body_close;
		return;
	}

	*current_node = (*current_node)->next; // for_var
	var* for_v = find_var_in_scope((*current_node)->value_char_ptr, calling_function, context_obj);
	*current_node = (*current_node)->next; // (
	node* el = get_close_part(*current_node); // )
	*current_node = (*current_node)->next; // start_exp
	node* start_exp = *current_node;
	*current_node = calc(start_exp, calling_function, context_obj, comma, NULL, for_v);
	*current_node = (*current_node)->next; // step_exp
	node* step_exp = *current_node;

	*current_node = get_first_type(*current_node, comma)->next;

	node* cond = *current_node; // cond_exp
	var* bool_var = new_temp_var(T_BOOL);
	gc_add_root(bool_var);

	*current_node = calc(cond, calling_function, context_obj, (node_type)0, el, bool_var);

	*current_node = get_first_type(*current_node, parentheses1);
	node* for_close = get_close_part(*current_node);
	*current_node = (*current_node)->next;

	while (*bool_var->value_bool)
	{
		compile(context_obj, *current_node, calling_function, for_close, NULL);
		if (g_loop_broken)
		{
			g_loop_broken = false;
			break;
		}
		if (calling_function != NULL && calling_function->has_returned)
			break;
		calc(step_exp, calling_function, context_obj, comma, NULL, for_v);
		calc(cond, calling_function, context_obj, (node_type)0, el, bool_var);
		gc_check_auto();
	}

	gc_remove_root(bool_var);
	free_temp_var(bool_var);
	*current_node = for_close;
}

void for_function(node** c, fcall* funcall, var* calling_object)
{
	eval_for_stmt(c, funcall, calling_object);
}

void compile_type(node* out, fcall* function_call, node* stop, type_def* b);

void eval_class_decl(node** current_node)
{
	eat(current_node, var_name, true);
	char* cname = (*current_node)->value_char_ptr;
	type_def* mtype = get_type_by_name(cname);
	if (mtype == NULL)
		mtype = new_type();
	mtype->type_name = cname;

	if (eat(current_node, parentheses4, false))
	{
		if (eat(current_node, var_name, false))
		{
			mtype->base = get_type_by_name((*current_node)->value_char_ptr);
		}
		eat(current_node, parentheses4_c, true);
	}

	node* start = get_first_type(*current_node, parentheses1);
	node* tm = get_close_part(start);

	compile(NULL, start->next, NULL, tm, mtype);
	type_def_compute_field_offsets(mtype);
	*current_node = tm;
}

void install_class(node** n)
{
	eval_class_decl(n);
}


var* eval_member_access(node** nod, fcall* calling_function, var* calling_object)
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
							mvar = type_instance_get_field(mvar->value_type_instsance, (*nod)->value_char_ptr);
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
							mvar = get_function_var_by_name((*nod)->value_char_ptr, calling_function);
						if (!mvar && calling_object != NULL)
						{
							if (strcmp((*nod)->value_char_ptr, "this") == 0)
								mvar = calling_object;
							else
							{
								type_instance* inst = calling_object->value_type_instsance;
								if (inst == NULL && calling_object->values != NULL && !is_base_type(calling_object->type_define))
									inst = (type_instance*)calling_object->values;
								if (inst != NULL)
									mvar = type_instance_get_field(inst, (*nod)->value_char_ptr);
							}
						}
						if (mvar == NULL)
							mvar = get_global_var_by_name((*nod)->value_char_ptr);
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
					setup_function_params(nod, fc, calling_object, calling_function);
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
						setup_function_params(nod, fcall, calling_object, calling_function);
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
					setup_function_params(nod, fcall, calling_object, calling_function);
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
				mvar->size = 1;
				mvar->values = install_memory(mvar);
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

var* name_exp_assign(node** nod, fcall* calling_function, var* calling_object)
{
	return eval_member_access(nod, calling_function, calling_object);
}

// declare a variable in current scope: local, class property, or global
var* declare_variable(node** current_node, fcall* calling_function, var* calling_object, type_def* target_class, type_def* var_type, int var_size)
{
	var* out_var = NULL;
	char* var_name = (*current_node)->value_char_ptr;
	if (calling_function != NULL)
	{
		define_function_var(var_name, calling_function, var_type, &out_var);
	}
	else if (target_class != NULL)
	{
		define_class_property(var_name, target_class, var_type, &out_var);
	}
	else
	{
		define_global_var(&out_var, var_type, var_name);
	}

	out_var->size = var_size;
	return out_var;
}

var* add_var_to(node** c, fcall* c_function, var* calling_object, type_def* ncalss, type_def* var_type, int vsize)
{
	return declare_variable(c, c_function, calling_object, ncalss, var_type, vsize);
}

static node* handle_assign_or_compound(node* current_node, var* target_var, fcall* calling_function, var* context_obj)
{
	if (current_node == NULL)
		return NULL;

	if (current_node->type_ == equals)
	{
		current_node = calc(current_node->next, calling_function, context_obj, none, NULL, target_var);
	}
	else if (current_node->type_ == operators_n && current_node->next != NULL && current_node->next->type_ == equals)
	{
		/* Compound assignment: +=, -=, *=, /=, %= */
		char op = current_node->value_char_ptr ? *current_node->value_char_ptr : '+';
		var* rhs = new_temp_var(target_var ? target_var->type_define : NULL);
		current_node = calc(current_node->next->next, calling_function, context_obj, none, NULL, rhs);
		if (target_var != NULL && rhs != NULL)
		{
			if (target_var->type_define == T_INT && target_var->value_int != NULL)
			{
				int r = (rhs->type_define == T_INT && rhs->value_int != NULL) ? *rhs->value_int :
				        (rhs->type_define == T_FLOAT && rhs->value_float != NULL) ? (int)*rhs->value_float :
				        (rhs->type_define == T_LONG && rhs->value_long != NULL) ? (int)*rhs->value_long : 0;
				if (op == '+') *target_var->value_int += r;
				else if (op == '-') *target_var->value_int -= r;
				else if (op == '*') *target_var->value_int *= r;
				else if (op == '/' && r != 0) *target_var->value_int /= r;
				else if (op == '%' && r != 0) *target_var->value_int %= r;
			}
			else if (target_var->type_define == T_FLOAT && target_var->value_float != NULL)
			{
				float r = (rhs->type_define == T_FLOAT && rhs->value_float != NULL) ? *rhs->value_float :
				          (rhs->type_define == T_INT && rhs->value_int != NULL) ? (float)*rhs->value_int : 0.0f;
				if (op == '+') *target_var->value_float += r;
				else if (op == '-') *target_var->value_float -= r;
				else if (op == '*') *target_var->value_float *= r;
				else if (op == '/' && r != 0.0f) *target_var->value_float /= r;
			}
			else if (target_var->type_define == T_LONG && target_var->value_long != NULL)
			{
				long r = (rhs->type_define == T_LONG && rhs->value_long != NULL) ? *rhs->value_long :
				         (rhs->type_define == T_INT && rhs->value_int != NULL) ? (long)*rhs->value_int : 0L;
				if (op == '+') *target_var->value_long += r;
				else if (op == '-') *target_var->value_long -= r;
				else if (op == '*') *target_var->value_long *= r;
				else if (op == '/' && r != 0) *target_var->value_long /= r;
				else if (op == '%' && r != 0) *target_var->value_long %= r;
			}
			else if (target_var->type_define == T_STRING && op == '+')
			{
				const char* rstr = (rhs->type_define == T_STRING && rhs->value_str_ptr != NULL && *rhs->value_str_ptr != NULL) ? *rhs->value_str_ptr :
				                   (rhs->value_char_ptr != NULL) ? rhs->value_char_ptr : "";
				const char* xstr = (target_var->value_str_ptr != NULL && *target_var->value_str_ptr != NULL) ? *target_var->value_str_ptr :
				                   (target_var->value_char_ptr != NULL) ? target_var->value_char_ptr : "";
				size_t xlen = strlen(xstr);
				size_t rlen = strlen(rstr);
				char* new_str = (char*)gc_malloc(xlen + rlen + 1, GC_KIND_STRING);
				if (new_str != NULL)
				{
					memcpy(new_str, xstr, xlen);
					memcpy(new_str + xlen, rstr, rlen + 1);
					if (target_var->value_str_ptr != NULL)
						*target_var->value_str_ptr = new_str;
					else
						target_var->value_char_ptr = new_str;
				}
			}
		}
		free_temp_var(rhs);
	}
	else if (current_node->type_ == operators_n && current_node->next != NULL && current_node->next->type_ == operators_n &&
	         current_node->value_char_ptr != NULL && current_node->next->value_char_ptr != NULL &&
	         *current_node->value_char_ptr == *current_node->next->value_char_ptr)
	{
		/* Increment / Decrement: ++, -- */
		char op = *current_node->value_char_ptr;
		if (target_var != NULL)
		{
			if (target_var->type_define == T_INT && target_var->value_int != NULL)
			{
				if (op == '+') (*target_var->value_int)++;
				else if (op == '-') (*target_var->value_int)--;
			}
			else if (target_var->type_define == T_FLOAT && target_var->value_float != NULL)
			{
				if (op == '+') (*target_var->value_float) += 1.0f;
				else if (op == '-') (*target_var->value_float) -= 1.0f;
			}
			else if (target_var->type_define == T_LONG && target_var->value_long != NULL)
			{
				if (op == '+') (*target_var->value_long)++;
				else if (op == '-') (*target_var->value_long)--;
			}
		}
		current_node = current_node->next->next;
		while (current_node != NULL && current_node->type_ != endl)
			current_node = current_node->next;
	}
	else
	{
		while (current_node != NULL && current_node->type_ != endl)
			current_node = current_node->next;
	}
	return current_node;
}

node* eval_ast_nodes(var* context_object, node* root_node, fcall* calling_function, node* stop_node, type_def* target_class)
{
	node* in_node;
	node* current_node;
	node* ret_node = NULL;
	bool is_static_decl = false;
	if (stop_node == NULL)
		in_node = get_root(root_node);
	else
		in_node = root_node;

	for (current_node = in_node; current_node != NULL; current_node = current_node->next)
	{
		ret_node = current_node;
		if (stop_node != NULL && (current_node == stop_node || (current_node->parent != NULL && current_node->parent == stop_node)))
		{
			break;
		}
		if (calling_function != NULL && calling_function->has_returned)
		{
			break;
		}
		if (g_loop_broken)
		{
			break;
		}

		switch (current_node->type_)
		{
		case var_name:
			{
				is_static_decl = false;
				var* target_var = eval_member_access(&current_node, calling_function, context_object);
				current_node = handle_assign_or_compound(current_node, target_var, calling_function, context_object);
			}
			break;
		case itype:
			{
				if (current_node->next != NULL && current_node->next->type_ == dot)
				{
					is_static_decl = false;
					var* target_var = eval_member_access(&current_node, calling_function, context_object);
					current_node = handle_assign_or_compound(current_node, target_var, calling_function, context_object);
					break;
				}
				type_def* var_type = current_node->value_type;
				int array_size = 1;
				if (eat(&current_node, s_index, false))
				{
					var* vsize = new_temp_var(T_INT);

					current_node = calc(current_node->next, calling_function, context_object, none, current_node->ref_node, vsize);
					array_size = *vsize->value_int;
					free_temp_var(vsize);
				}
				if (eat(&current_node, var_name, true))
				{
					if (current_node->opt_name_type == function_def)
					{
						current_node = add_new_func_code(current_node, var_type, target_class);
						if (is_static_decl && target_class != NULL && target_class->d_function_size > 0)
						{
							target_class->d_functions[target_class->d_function_size - 1].access = STATIC;
						}
						is_static_decl = false;
					}
					else if (current_node->opt_name_type == var_def)
					{
						var* n_var = declare_variable(&current_node, calling_function, context_object, target_class, var_type, array_size);
						if (is_static_decl && n_var != NULL)
						{
							n_var->access = STATIC;
							if (target_class != NULL && n_var->slot_idx >= 0 && n_var->slot_idx < target_class->d_propertys_size)
							{
								target_class->field_descriptors[n_var->slot_idx].access = STATIC;
							}
						}
						is_static_decl = false;

						if (eat(&current_node, equals, false))
						{
							current_node = calc(current_node->next, calling_function, context_object, none, NULL, n_var);
						}
						else if (current_node->next != NULL && current_node->next->type_ == parentheses4)
						{
							printf("ERROR on line %d: direct constructor call in declaration is disallowed; use '%s %s = new %s(...)'\n",
							       current_node->line, var_type ? var_type->type_name : "Type",
							       current_node->value_char_ptr ? current_node->value_char_ptr : "var",
							       var_type ? var_type->type_name : "Type");
							exit(1);
						}
						else // endl
						{
							if (target_class == NULL)
							{
								if (var_type != NULL && !is_base_type(var_type))
								{
									printf("ERROR on line %d: implicit default instantiation is disallowed; use '%s %s = new %s()'\n",
									       current_node->line, var_type->type_name,
									       current_node->value_char_ptr ? current_node->value_char_ptr : "var",
									       var_type->type_name);
									exit(1);
								}
								n_var->values = install_memory(n_var);
							}
						}
					}
				}

				ret_node = current_node;
				break;
			}
		case keyword:
			{
				switch (current_node->value_keyword)
				{
				case _if_:
				case _else_:
				case _eif_:
					eval_if_stmt(&current_node, calling_function, context_object);
					break;
				case _for_:
					eval_for_stmt(&current_node, calling_function, context_object);
					break;
				case _while_:
					eval_while_stmt(&current_node, calling_function, context_object);
					break;
				case _do_:
					eval_do_while_stmt(&current_node, calling_function, context_object);
					break;

				case _return_:
					if (calling_function != NULL)
					{
						var* re = &calling_function->_return;

						calc(current_node->next, calling_function, context_object, 0, NULL, re);
						calling_function->has_returned = true;
					}
					current_node = stop_node;
					return current_node;
				case _break_:
					g_loop_broken = true;
					current_node = stop_node;
					return current_node;

				case _class_:
					is_static_decl = false;
					eval_class_decl(&current_node);
					break;
				case _static_:
					is_static_decl = true;
					break;
				case _import_:
					{
						const char* mod_name = NULL;
						node* p = current_node->next;
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
						while (current_node != NULL && current_node->type_ != endl && current_node != stop_node)
						{
							current_node = current_node->next;
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
		if (calling_function != NULL && calling_function->has_returned)
		{
			current_node = stop_node;
			break;
		}
		// return from function code
		gc_check_auto();

		if (current_node == NULL)
			break;
	}
	return ret_node;
}

node* compile(var* parent, node* out, fcall* temp, node* stop, type_def* ncalss)
{
	return eval_ast_nodes(parent, out, temp, stop, ncalss);
}
