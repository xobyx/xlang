#include "types.h"
#include "functions.h"

#define t_long 0
#define t_string 1
#define t_char 2
#define t_int 3
#define t_bool 4
#define t_float 5
#define t_array 6
void step(node** v) { *v = (*v)->next; }
var* get_array_item(var * name, int index)
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

#define XERROR(L) printf("ERROR on line %d - %s:%s:%l",(L),__FILE__,__FUNCTION__,__LINE__);

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
node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
	var* calc_result);

//[index][return]
int get_index_value(node ** nod, fcall* calling_function, var* calling_object) {
	node* close = (*nod)->ref_node;
	var* ind = new_temp_var(T_INT);
	step(nod);
	*nod = calc(*nod, calling_function, calling_object, (node_type)0, close, ind);

	return *ind->value_int;



}
var* name_exp(node ** nod, fcall* calling_function, var* calling_object)
{
	var* mvar = NULL;
	bool fath = false;
	while ((*nod)->type_ & (var_name | dot | s_index | value | parentheses4))
	{
		switch ((*nod)->type_)
		{
		case var_name:
			if ((*nod)->opt_name_type == var_call)
			{
				if (mvar != NULL)
				{
					if (!fath)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						fath = false;
						mvar = get_var_by_name_on_stack((*nod)->value_char_ptr, &mvar->value_type_instsance->propertys);

					}

				}
				else
				{
					if (calling_function != NULL)
						mvar = fget_var_by_name_fc((*nod)->value_char_ptr, calling_function);
					if (calling_object != NULL)

						mvar = get_globle_var_by_name((*nod)->value_char_ptr);
				}
				step(nod);
			}
			else if ((*nod)->opt_name_type == function_call)
			{
				if (mvar != NULL)
				{
					if (!fath)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{

						///find var on mvar functions
						fath = false;
						func_deftion * name_function = get_obj_function(mvar, (*nod)->value_char_ptr);
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
					func_deftion * name_function;
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
			fath = true;
			step(nod);
			break;
		case s_index:
		{
			const int index_value = get_index_value(nod, calling_function, calling_object);
			mvar = get_array_item(mvar, index_value);
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
			break;
		}




	}


	return NULL;
}

node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
	var* calc_result)
{
	//bool is_mem_set = false;
	type_def* name_type;
	type_def* memory_type;
	void * name_value;
	int f = 0;
	node* mnode = (node*)cnode;
	type_def* saved_return_type = calc_result->type_define;

	int in = 0;
	char* op = NULL;


	bool indexx = false;
	void* memory = NULL;

	int i = 0;

	while ((mnode->btype.value & (node_type)0xc043) == 0)
	{
		struct var* name_var = NULL;
		if ((stop_in_type != 0 && mnode->type_ == stop_in_type) || (stop_in_node != NULL && mnode == stop_in_node))break
			;

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
				if (mnode->next->type_ == s_index)
				{
					node* close = get_close_part(mnode->next);
					var* ind = new_temp_var(T_INT);

					calculate(mnode->next->next, calling_function, calling_object, (node_type)0, close, ind);
					mnode = close;
					in = *ind->value_int;
					free_temp_var(ind);

				}

				name_value = get_array_item(name_var, in, &name_type);

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
			if (memory == NULL)  //// T_FUNC
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
			else  // type_inctance
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

		if (mnode->type_ == operators_n || is_double_equle(mnode))
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


