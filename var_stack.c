#include "var_stack.h"
#include "xgc.h"


void var_stack_init(var_stack* s)
{
	s->size = 0;
	s->top = NULL;
	s->root = NULL;
	s->stack_holder = NULL;
}

void int_var(var* n)
{
}

var* new_var_on_stack(var_stack* stack, char* vname, type_def* vtype)
{
	var* f = (var*)gc_calloc(1, sizeof(var), GC_KIND_VAR);
	f->name = vname;
	f->type_define = vtype;
	f->size = 1;
	var_stack_push(stack, f);
	return f;
}

void var_stack_push(var_stack* stk, var* el)
{
	// Increment number of elements.
	stk->size++;
	if (NULL == stk->root) stk->root = el;
	// Set el to point to current stack top as its next element.

	if (NULL != stk->top)
		stk->top->stack_next = el;
	// Set el to be the top element of s.
	stk->top = el;
}

extern var start_var;
extern var tinfo[];

void var_clean_stack(var_stack* s)
{
	if (s == NULL) return;
	var* x;
	for (var* bi = s->root; bi != NULL; bi = x)
	{
		x = bi->stack_next;
		bool is_static = (bi == &start_var || (bi >= tinfo && bi < tinfo + 6));
		if (!is_static)
		{
			if (bi->type_define && bi->type_define->type_name == T_STRING->type_name)
			{
				for (int i = 0; i < bi->size; i++)
				{
					if (bi->value_str_ptr && *(bi->value_str_ptr + i))
					{
						if (gc_is_managed(*(bi->value_str_ptr + i)))
							gc_free(*(bi->value_str_ptr + i));
						*(bi->value_str_ptr + i) = NULL;
					}
				}
			}
			if (bi->type_define != T_FUNC && bi->values != NULL && gc_is_managed(bi->values))
			{
				gc_free_any(bi->values);
				bi->values = NULL;
			}
			gc_free_any(bi);
		}
		s->size--;
	}
	s->root = NULL;
	s->top = NULL;
	s->size = 0;
}

void free_temp_var(var* bi)
{
	if (bi == NULL) return;
	bool found = false;
	if (t_varss != NULL)
	{
		var* prev = NULL;
		for (var* cur = t_varss->root; cur != NULL; prev = cur, cur = cur->stack_next)
		{
			if (cur == bi)
			{
				found = true;
				if (prev != NULL)
					prev->stack_next = bi->stack_next;
				else
					t_varss->root = bi->stack_next;

				if (t_varss->top == bi)
					t_varss->top = prev;

				if (t_varss->size > 0)
					t_varss->size--;
				break;
			}
		}
	}
	if (!found) return;
	if (bi->type_define && bi->type_define->type_name == T_STRING->type_name)
	{
		for (int i = 0; i < bi->size; i++)
		{
			if (bi->value_str_ptr && *(bi->value_str_ptr + i))
			{
				if (gc_is_managed(*(bi->value_str_ptr + i)))
					gc_free(*(bi->value_str_ptr + i));
				*(bi->value_str_ptr + i) = NULL;
			}
		}
	}
	if (bi->type_define != T_FUNC && bi->values != NULL && gc_is_managed(bi->values))
	{
		gc_free_any(bi->values);
		bi->values = NULL;
	}
	gc_free_any(bi);
}

