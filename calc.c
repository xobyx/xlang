#include "types.h"
#include "functions.h"
#if defined(__GNUC__)|| defined(__MINGW64__)
#define strcat_s(x,y,z) strcat(x,z)
#endif

//void step2(node** v) { *v = (*v)->next; }


#define XERROR(L) printf("ERROR on line %d - %s:%s:%l",(L),__FILE__,__FUNCTION__,__LINE__);
node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
           var* calc_result);

//[index][return]
int get_index_value2(node** nod, fcall* calling_function, var* calling_object)
{
	node* close = (*nod)->ref_node;
	var* ind = new_temp_var(T_INT);
	step(nod);
	*nod = calc(*nod, calling_function, calling_object, (node_type)0, close, ind);

	return *ind->value_int;
}

void setup_t2(var* clac, var* name_var)
{
	if (clac->type_define == NULL && name_var->type_define != NULL)
	{
		clac->type_define = name_var->type_define;

		clac->size = name_var->size;
	}
	else if (clac->type_define != NULL)
	{
		if (name_var->type_define != clac->type_define)
		{
			clac->type_define = name_var->type_define;
		}
	}
}


var* name_exp(node** nod, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node)
{
	var* mvar = NULL;
	bool fath = false;
	while ((*nod)->type_ & (var_name | dot | s_index | value | parentheses4))
	{
		switch ((*nod)->type_)
		{
		case var_name:
			if ((*nod)->opt_name_type == var_call || (*nod)->opt_name_type == var_call_ref)
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
					if (!fath)
						printf("ERROR on line %d - %s:%s:%d", (*nod)->line, __FILE__, __FUNCTION__, __LINE__);
					else
					{
						///find var on mvar functions
						fath = false;
						func_deftion* name_function = get_obj_function(mvar, (*nod)->value_char_ptr);
						fcall* fcall = create_fcall(name_function);
						//compile_var_name_start(k, funct);
						//compile(mvar, k, funct, y);
						setup_function_parms(nod, fcall, calling_object, calling_function);
						call_function(fcall, calling_object == NULL ? &mvar : &calling_object);
						mvar = &fcall->_return;
						step(nod); // after )[x]
					}
				}
				else
				{
					func_deftion* name_function;
					if (calling_object != NULL)
					{
						name_function = get_obj_function2(calling_object, (*nod)->value_char_ptr);
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
				const int index_value = get_index_value2(nod, calling_function, calling_object);
				mvar = get_array_item(mvar, index_value);
				if (stop_in_node != NULL && stop_in_node == *nod) return mvar;
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
				*nod = calc((*nod)->next, calling_function, calling_object, 0, (*nod)->ref_node, mvar);
				if (stop_in_node != NULL && stop_in_node == *nod) return mvar;
				step(nod);
				break;
			}


		default:
			break;
		}
	}


	return mvar;
}


void move(var* calc_result, void* memory, int* i, var* name_var)
{
	if (calc_result->size > 1 && name_var->size > 1)
	{
		copy_array(calc_result, memory, name_var);
	}

	else if (calc_result->type_define == T_INT)
		((int*)memory)[(*i)++] = *name_var->value_int;

	else if (calc_result->type_define == T_FLOAT)
		((float*)memory)[(*i)++] = *name_var->value_float;

	else if (calc_result->type_define == T_LONG)
		((long*)memory)[(*i)++] = *name_var->value_long;

	else if (calc_result->type_define == T_CHAR)
		((char*)memory)[(*i)++] = *name_var->value_char_ptr;
	else if (calc_result->type_define == T_STRING)
	{

		char **strp =(char**)memory + *i;

        if(*name_var->value_str_ptr==NULL)
        {
			 (*i)++;
             return;
        }
		

		char *save= *name_var->value_str_ptr ;
		if(*strp ==*name_var->value_str_ptr)
		{
			
		}
		*strp = (char*)calloc(1,strlen(*name_var->value_str_ptr) + 1);
		strcpy(*strp, save);
		(*i)++;
	}
	else if (calc_result->type_define == T_BOOL)
		((bool*)memory)[(*i)++] = *name_var->value_bool;

	else if (!is_base_type(calc_result->type_define))
	{
		///FIXME:
		((type_instance*)memory)[(*i)++] = *name_var->value_type_instsance;
	}
}


node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
           var* calc_result)
{
	int ref = 0;
	
	
	var* name_var = NULL;
	node* mnode = (node*)cnode;
	type_def* saved_return_type = calc_result != NULL ? calc_result->type_define : NULL;


	char* op = NULL;


	void* memory = NULL;

	int i = 0;

	while ((mnode->btype.value & (node_type)0xc043) == 0)
	{
		if (stop_here(stop_in_type, stop_in_node, mnode)) break;
		name_var = NULL;

		if (mnode->type_ & (var_name | value | parentheses4))
		{
			if (mnode->type_ == var_name && mnode->opt_name_type == var_call_ref)
			{
				ref = true;
			}
			name_var = name_exp(&mnode, calling_function, calling_object, stop_in_type, stop_in_node);

			if (op == NULL) //
			{
				setup_t2(calc_result, name_var);

				if (memory == NULL)
				{
					if (ref)
					{
						memory = name_var->values;
					}
					else
					{
						
						memory = calc_result->values == NULL ? install_memory(calc_result) : calc_result->values;
					}
				}
				//copy array [int string]
				move(calc_result, memory, &i, name_var);
			}
			else //v2 op != NULL
			{
				if (calc_result->type_define == T_INT)
				{
					//TODO:no need allready done by in
					//int y = get_index_value(funct, k);
					int* mx = (int*)memory + (i - 1);
					const int to = *name_var->value_int;
					///math_opration(op,mx,to);
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_FLOAT)
				{
					float* mx = (float*)memory + (i - 1);
					float to = *name_var->value_float;
					//MATH_OPERATORS
				}
				else if (calc_result->type_define == T_LONG)
				{
					long* mx = (long*)memory + (i - 1);
					long to = *(name_var->value_long);
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_CHAR)
				{
					char* mx = &((char*)memory)[i - 1];
					char to = *name_var->value_char_ptr;
					MATH_OPERATORS
				}
				else if (calc_result->type_define == T_BOOL)
				{
					//bool* mx = &((bool*)memory)[i - 1];
					bool* mx = (bool*)memory + (i - 1);
					bool to = *(bool*)name_var->values;
					BOOL_OPERATORS
				}
				else if (calc_result->type_define == T_STRING)
				{
					char** mem = ((char**)memory) + (i - 1);
					char* to = *name_var->value_str_ptr;
					//char* mtype = mb == NULL ? (char*)k->opt : (char*)mb->var_type->name;
					if(to==NULL)
					{
						
					}					
					else if (*op == '+')
					{
						//strcat(mx,to);
						if(*mem ==NULL )
						{
							*mem = (char*)calloc(1,strlen(to) + 1);
							strcpy(*mem, to);
						}
						else
						{
							long mxlen = strlen(*mem);
							long tolen = strlen(to);
							*mem = realloc(*mem, mxlen + tolen + 1);
							strcat_s(*mem, mxlen + tolen + 1, to);							
						}
					}
				}
				else // type_inctance
				{
				}
				op = NULL;
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
				step(&mnode);
				step(&mnode);
			}
			else
			{
				op = mnode->value_char_ptr;
				step(&mnode);
			}
		}
		else ///error no unacceable
		{
			step(&mnode);
			//printf("error cal");
			//exit(-1);
		}
	}

	if (saved_return_type != NULL)
	{
		calc_result->type_define = saved_return_type;
	}


	calc_result->values = memory;

//	if (name_var != NULL) { printf("\nname_var.(%s).\n",name_var->name); }

	return mnode;
}
