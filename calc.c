#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "functions.h"
#include "xgc.h"

/* Report an error with the script line of the node plus the C source location. */
#define XERROR(L, ...)                                                            \
	do {                                                                          \
		fprintf(stderr, "ERROR on line %d (%s:%d, %s): ", (L), __FILE__,          \
		        __LINE__, __func__);                                              \
		fprintf(stderr, __VA_ARGS__);                                             \
		fputc('\n', stderr);                                                      \
	} while (0)

/* Node types that terminate an expression (end of statement, closing brace, ...). */
#define CALC_END_MASK ((node_type)0xc043)

static inline bool is_expression_end(const node* n)
{
	if (n == NULL) return true;
	if (n->type_ == itype && n->next != NULL && n->next->type_ == dot)
		return false;
	return (n->type_ & CALC_END_MASK) != 0;
}

node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
           var* calc_result);

/* ------------------------------------------------------------------------- */
/* [index] handling                                                          */
/* ------------------------------------------------------------------------- */

int get_index_value2(node** nod, fcall* calling_function, var* calling_object)
{
	node* close_node = (*nod)->ref_node;
	var* ind = new_temp_var(T_INT);

	step(nod);
	*nod = calc(*nod, calling_function, calling_object, (node_type)0, close_node, ind);

	int res = (ind != NULL && ind->value_int != NULL) ? *ind->value_int : 0;
	free_temp_var(ind);
	return res;
}

/* Make `result` take the type (and size, if it had no type yet) of `operand`. */
void setup_t2(var* result, var* operand)
{
	if (result == NULL || operand == NULL || operand->type_define == NULL)
		return;

	if (result->type_define == NULL)
		result->size = operand->size;

	result->type_define = operand->type_define;
}

/* ------------------------------------------------------------------------- */
/* name / call / index chains:  a.b[3].f(x)                                  */
/* ------------------------------------------------------------------------- */

static var* lookup_var(const node* n, fcall* calling_function, var* calling_object)
{
	var* v = NULL;

	if (calling_function != NULL)
		v = fget_var_by_name_fc(n->value_char_ptr, calling_function);

	if (v == NULL && calling_object != NULL)
	{
		type_instance* inst = calling_object->value_type_instsance;
		if (inst == NULL && calling_object->values != NULL && !is_base_type(calling_object->type_define))
			inst = (type_instance*)calling_object->values;
		if (inst != NULL)
			v = get_var_by_name_on_stack(n->value_char_ptr, &inst->propertys);
	}

	if (v == NULL)
		v = get_globle_var_by_name(n->value_char_ptr);

	return v;
}

static var* lookup_member(const node* n, var* object)
{
	if (object == NULL)
		return NULL;

	type_instance* inst = object->value_type_instsance;
	if (inst == NULL && object->values != NULL && !is_base_type(object->type_define))
		inst = (type_instance*)object->values;

	if (inst == NULL)
		return NULL;

	return get_var_by_name_on_stack(n->value_char_ptr, &inst->propertys);
}

/*
 * Resolve one `var_name` node (variable, member or function call).
 * On success *pmvar is updated and true is returned.
 * On failure an error is reported, the node is skipped and false is returned,
 * so the caller can never loop forever on the same node.
 */
static bool eval_name(node** nod, fcall* calling_function, var* calling_object,
                      var** pmvar, bool* after_dot, type_def** pstatic_class)
{
	node* n = *nod;
	var* mvar = *pmvar;
	const bool is_member = *after_dot; /* previous token was '.' */

	*after_dot = false;

	switch (n->opt_name_type)
	{
	case var_call:
	case var_call_ref:
		if (mvar == NULL && *pstatic_class == NULL)
		{
			type_def* td = get_type_by_name(n->value_char_ptr);
			if (td != NULL && n->next != NULL && n->next->type_ == dot)
			{
				*pstatic_class = td;
				step(nod);
				return true;
			}
			mvar = lookup_var(n, calling_function, calling_object);
			if (mvar == NULL || mvar->type_define == T_FUNC)
			{
				if (td != NULL)
				{
					*pstatic_class = td;
					mvar = NULL;
					step(nod);
					return true;
				}
				if (mvar == NULL)
				{
					XERROR(n->line, "unknown variable '%s'", n->value_char_ptr);
					goto fail;
				}
			}
		}
		else if (*pstatic_class != NULL && is_member)
		{
			type_def* td = *pstatic_class;
			*pstatic_class = NULL;
			mvar = get_class_property(td, n->value_char_ptr);
			if (mvar == NULL)
			{
				XERROR(n->line, "unknown member '%s' in class '%s'", n->value_char_ptr, td->type_name);
				goto fail;
			}
		}
		else if (is_member)
			mvar = lookup_member(n, mvar);
		else
		{
			XERROR(n->line, "unexpected name '%s' after a value", n->value_char_ptr);
			goto fail;
		}

		if (mvar == NULL)
		{
			XERROR(n->line, "unknown variable '%s'", n->value_char_ptr);
			goto fail;
		}
		step(nod);
		break;

	case function_call:
	{
		func_deftion* target = NULL;
		var* self = NULL;
		fcall* fc;

		if (mvar != NULL && !is_member && *pstatic_class == NULL)
		{
			XERROR(n->line, "unexpected call '%s' after a value", n->value_char_ptr);
			goto fail;
		}

		if (*pstatic_class != NULL)
		{
			type_def* td = *pstatic_class;
			*pstatic_class = NULL;
			target = get_class_function(td, n->value_char_ptr);
			self = NULL;
		}
		else if (mvar != NULL) /* obj.method(...) : `this` is the object on the left */
		{
			target = get_obj_function(mvar, n->value_char_ptr);
			self = mvar;
		}
		else /* method of the current object, or a free function */
		{
			target = NULL;
			if (calling_object != NULL)
				target = get_obj_function(calling_object, n->value_char_ptr);
			if (target == NULL)
				target = get_func_by_name(n->value_char_ptr);
			if (target == NULL)
			{
				type_def* td = get_type_by_name(n->value_char_ptr);
				if (td != NULL)
				{
					for (int i = 0; i < td->d_function_size; i++)
					{
						if (td->d_functions[i].func_name != NULL &&
							(strcmp(td->d_functions[i].func_name, td->type_name) == 0 ||
							 td->d_functions[i].function_type == constr))
						{
							target = &td->d_functions[i];
							break;
						}
					}
				}
			}
			if (target == NULL && calling_function != NULL && calling_function->deftion != NULL)
			{
				type_def* enc = get_class_of_function(calling_function->deftion);
				if (enc != NULL)
					target = get_class_function(enc, n->value_char_ptr);
			}
			self = (target != NULL && calling_object != NULL && get_obj_function(calling_object, n->value_char_ptr) != NULL)
				? calling_object : NULL;
		}

		if (target == NULL)
		{
			XERROR(n->line, "unknown function '%s'", n->value_char_ptr);
			goto fail;
		}

		fc = create_fcall(target);
		if (fc == NULL)
		{
			XERROR(n->line, "cannot create call frame for '%s'", n->value_char_ptr);
			goto fail;
		}

		/* arguments are evaluated in the *caller's* context */
		setup_function_parms(nod, fc, calling_object, calling_function);
		call_function(fc, &self);
		mvar = &fc->_return;
		step(nod); /* skip past ')' */
		break;
	}

	default:
		XERROR(n->line, "unsupported name kind");
		goto fail;
	}

	*pmvar = mvar;
	return true;

fail:
	step(nod);
	return false;
}

var* name_exp(node** nod, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node)
{
	var* mvar = NULL;
	bool after_dot = false;
	type_def* static_class = NULL;

	while ((*nod)->type_ & (var_name | dot | s_index | value | parentheses4 | itype))
	{
		switch ((*nod)->type_)
		{
		case itype:
			static_class = (*nod)->value_type;
			step(nod);
			break;

		case var_name:
			if (!eval_name(nod, calling_function, calling_object, &mvar, &after_dot, &static_class))
				return NULL;
			break;

		case dot:
			after_dot = true;
			step(nod);
			break;

		case s_index:
		{
			if (mvar == NULL)
			{
				XERROR((*nod)->line, "index applied to nothing");
				step(nod);
				return NULL;
			}

			const int index_value = get_index_value2(nod, calling_function, calling_object);
			mvar = get_array_item(mvar, index_value);
			if (mvar == NULL)
			{
				XERROR((*nod)->line, "index %d out of range", index_value);
				return NULL;
			}

			if (stop_in_node != NULL && stop_in_node == *nod)
				return mvar;
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
			*nod = calc((*nod)->next, calling_function, calling_object, (node_type)0, (*nod)->ref_node, mvar);
			if (stop_in_node != NULL && stop_in_node == *nod)
				return mvar;
			step(nod);
			break;
		}

		default: /* combined flag bits etc. - stop instead of spinning forever */
			return mvar;
		}
	}

	return mvar;
}

/* ------------------------------------------------------------------------- */
/* storing a value                                                           */
/* ------------------------------------------------------------------------- */

void move(var* calc_result, void* memory, int* i, var* name_var)
{
	type_def* t = calc_result->type_define;

	if (calc_result->size > 1 && name_var->size > 1)
	{
		copy_array(calc_result, memory, name_var);
		return;
	}

	if (t == T_INT)
		((int*)memory)[(*i)++] = *name_var->value_int;
	else if (t == T_FLOAT)
		((float*)memory)[(*i)++] = *name_var->value_float;
	else if (t == T_LONG)
		((long*)memory)[(*i)++] = *name_var->value_long;
	else if (t == T_CHAR)
		((char*)memory)[(*i)++] = *name_var->value_char_ptr;
	else if (t == T_BOOL)
		((bool*)memory)[(*i)++] = *name_var->value_bool;
	else if (t == T_STRING)
	{
		const char* src = "";
		if (name_var->value_str_ptr != NULL && *name_var->value_str_ptr != NULL)
			src = *name_var->value_str_ptr;
		else if (name_var->value_char_ptr != NULL)
			src = name_var->value_char_ptr;

		const size_t len = strlen(src);
		char* copy = (char*)gc_malloc(len + 1, GC_KIND_STRING);

		if (copy == NULL)
			return;
		memcpy(copy, src, len + 1);
		((char**)memory)[(*i)++] = copy;
	}
	else if (t != NULL && !is_base_type(t))
	{
		type_instance* src = name_var->value_type_instsance;
		if (src == NULL && name_var->values != NULL)
			src = (type_instance*)name_var->values;
		if (src != NULL)
			copy_object(src, (type_instance*)memory + (*i)++, 1);
	}
}

/* ------------------------------------------------------------------------- */
/* operators                                                                 */
/* ------------------------------------------------------------------------- */

static void string_append(char** dest, const char* src)
{
	if (src == NULL)
		return;

	const size_t dlen = *dest != NULL ? strlen(*dest) : 0;
	const size_t slen = strlen(src);
	char* joined = (char*)gc_realloc(*dest, dlen + slen + 1);

	if (joined == NULL) /* *dest is still valid */
		return;

	memcpy(joined + dlen, src, slen + 1);
	*dest = joined;
}

/* MATH_OPERATORS has no float variant (it is commented out in the original,
 * so float arithmetic used to be silently ignored). '%' is not valid on float. */
static void apply_float_operator(float* mx, float to, const char* op)
{
	if (op[1] == '=')
	{
		switch (op[0])
		{
		case '=': *mx = (*mx == to); break;
		case '!': *mx = (*mx != to); break;
		case '<': *mx = (*mx <= to); break;
		case '>': *mx = (*mx >= to); break;
		default: break;
		}
		return;
	}
	switch (op[0])
	{
	case '+': *mx += to; break;
	case '-': *mx -= to; break;
	case '*': *mx *= to; break;
	case '/':
		if (to == 0.0f) { fprintf(stderr, "ERROR: division by zero\n"); return; }
		*mx /= to; break;
	case '<': *mx = (*mx < to); break;
	case '>': *mx = (*mx > to); break;
	default: break;
	}
}

/* Combine the last stored element (memory[count-1]) with `operand` using `op`. */
static void apply_operator(type_def* type, void* memory, int count, char* op, var* operand)
{
	if (memory == NULL || count <= 0)
	{
		fprintf(stderr, "ERROR: operator '%s' has no left operand\n", op);
		return;
	}

	const int last = count - 1;

	if (type == T_INT)
	{
		int* mx = (int*)memory + last;
		const int to = *operand->value_int;
		if ((*op == '/' || *op == '%') && to == 0)
		{
			fprintf(stderr, "ERROR: division by zero\n");
			return;
		}
		MATH_OPERATORS
	}
	else if (type == T_FLOAT)
	{
		apply_float_operator((float*)memory + last, *operand->value_float, op);
	}
	else if (type == T_LONG)
	{
		long* mx = (long*)memory + last;
		long to = *operand->value_long;
		if ((*op == '/' || *op == '%') && to == 0)
		{
			fprintf(stderr, "ERROR: division by zero\n");
			return;
		}
		MATH_OPERATORS
	}
	else if (type == T_CHAR)
	{
		char* mx = (char*)memory + last;
		char to = *operand->value_char_ptr;
		MATH_OPERATORS
	}
	else if (type == T_BOOL)
	{
		bool* mx = (bool*)memory + last;
		bool to = *operand->value_bool;
		BOOL_OPERATORS
	}
	else if (type == T_STRING)
	{
		if (*op == '+')
		{
			if (operand->type_define == T_STRING && operand->value_str_ptr != NULL && *operand->value_str_ptr != NULL)
			{
				string_append((char**)memory + last, *operand->value_str_ptr);
			}
			else if (operand->type_define == T_INT && operand->value_int != NULL)
			{
				char ibuf[32];
				snprintf(ibuf, sizeof(ibuf), "%d", *operand->value_int);
				string_append((char**)memory + last, ibuf);
			}
			else if (operand->type_define == T_FLOAT && operand->value_float != NULL)
			{
				char fbuf[64];
				snprintf(fbuf, sizeof(fbuf), "%g", *operand->value_float);
				string_append((char**)memory + last, fbuf);
			}
			else if (operand->type_define == T_LONG && operand->value_long != NULL)
			{
				char lbuf[32];
				snprintf(lbuf, sizeof(lbuf), "%ld", *operand->value_long);
				string_append((char**)memory + last, lbuf);
			}
			else if (operand->type_define == T_BOOL && operand->value_bool != NULL)
			{
				string_append((char**)memory + last, *operand->value_bool ? "true" : "false");
			}
			else if (operand->type_define == T_CHAR && operand->value_char_ptr != NULL)
			{
				char cbuf[2] = { *operand->value_char_ptr, '\0' };
				string_append((char**)memory + last, cbuf);
			}
		}
		else if (op[0] == '=' && op[1] == '=')
		{
			const char* left = ((char**)memory)[last] ? ((char**)memory)[last] : "";
			const char* right = "";
			if (operand->type_define == T_STRING && operand->value_str_ptr != NULL && *operand->value_str_ptr != NULL)
				right = *operand->value_str_ptr;
			else if (operand->value_char_ptr != NULL)
				right = operand->value_char_ptr;
			int match = (strcmp(left, right) == 0) ? 1 : 0;
			memset((char**)memory + last, 0, sizeof(char*));
			*((int*)((char**)memory + last)) = match;
		}
		else if (op[0] == '!' && op[1] == '=')
		{
			const char* left = ((char**)memory)[last] ? ((char**)memory)[last] : "";
			const char* right = "";
			if (operand->type_define == T_STRING && operand->value_str_ptr != NULL && *operand->value_str_ptr != NULL)
				right = *operand->value_str_ptr;
			else if (operand->value_char_ptr != NULL)
				right = operand->value_char_ptr;
			int match = (strcmp(left, right) != 0) ? 1 : 0;
			memset((char**)memory + last, 0, sizeof(char*));
			*((int*)((char**)memory + last)) = match;
		}
	}
	/* else: operators on type instances are not supported */
}

/* ------------------------------------------------------------------------- */
/* expression evaluation                                                     */
/* ------------------------------------------------------------------------- */

node* calc(node* cnode, fcall* calling_function, var* calling_object, node_type stop_in_type, node* stop_in_node,
           var* calc_result)
{
	node* mnode = cnode;
	void* memory = NULL;   /* buffer the result is written into */
	int count = 0;         /* elements written to `memory` so far */
	char* op = NULL;       /* pending operator waiting for its right operand */
	char op_buf[3] = {0};  /* storage for two-character operators such as "==" */

	if (calc_result == NULL)
		calc_result = new_temp_var(NULL); /* evaluate for side effects only */

	type_def* saved_return_type = calc_result->type_define;

	while (!is_expression_end(mnode))
	{
		if (stop_here(stop_in_type, stop_in_node, mnode))
			break;

		if (mnode->type_ & (var_name | value | parentheses4 | itype))
		{
			const bool by_ref = mnode->type_ == var_name && mnode->opt_name_type == var_call_ref;
			node* before = mnode;
			var* operand = name_exp(&mnode, calling_function, calling_object, stop_in_type, stop_in_node);

			if (operand == NULL)
			{
				if (mnode == before) /* always make progress */
					step(&mnode);
				op = NULL;
				continue;
			}

			if (op == NULL)
			{
				setup_t2(calc_result, operand);

				if (memory == NULL)
				{
					if (by_ref)
						memory = operand->values;
					else
						memory = calc_result->values != NULL ? calc_result->values : install_memory(calc_result);
				}

				if (memory != NULL)
					move(calc_result, memory, &count, operand);
			}
			else if (count == 0 && *op == '-')
			{
				setup_t2(calc_result, operand);

				if (memory == NULL)
				{
					if (by_ref)
						memory = operand->values;
					else
						memory = calc_result->values != NULL ? calc_result->values : install_memory(calc_result);
				}

				if (operand->type_define == T_INT && operand->value_int != NULL)
					*operand->value_int = -(*operand->value_int);
				else if (operand->type_define == T_FLOAT && operand->value_float != NULL)
					*operand->value_float = -(*operand->value_float);
				else if (operand->type_define == T_LONG && operand->value_long != NULL)
					*operand->value_long = -(*operand->value_long);

				if (memory != NULL)
					move(calc_result, memory, &count, operand);
				op = NULL;
			}
			else
			{
				bool is_eq = (op != NULL && ((op[0] == '=' && op[1] == '=') || (op[0] == '!' && op[1] == '=')));
				apply_operator(calc_result->type_define, memory, count, op, operand);
				if (is_eq && saved_return_type == NULL)
					calc_result->type_define = T_BOOL;
				op = NULL;
			}
		}
		else if (mnode->type_ == operators_n || is_double_equle(mnode))
		{
			if (is_double_oprater(mnode) && mnode->next != NULL && mnode->value_char_ptr != NULL && mnode->next->value_char_ptr != NULL)
			{
				op_buf[0] = *mnode->value_char_ptr;
				op_buf[1] = *mnode->next->value_char_ptr;
				op_buf[2] = '\0';
				op = op_buf;
				step(&mnode);
				step(&mnode);
			}
			else if (mnode->value_char_ptr != NULL)
			{
				op = mnode->value_char_ptr;
				step(&mnode);
			}
			else
			{
				step(&mnode);
			}
		}
		else /* unexpected node - skip it */
		{
			step(&mnode);
		}
	}

	if (saved_return_type != NULL)
		calc_result->type_define = saved_return_type;

	if (memory != NULL)
		calc_result->values = memory;

	return mnode;
}