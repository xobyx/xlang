#include "var_stack.h"


void var_stack_init(var_stack* s)
{
	s->size = 1;
	s->top = NULL;
	s->root = NULL;
	s->m_class = NULL;
}

void int_var(var* n)
{
}

var* new_var_on_stack(var_stack* stack, char* vname, type* vtype)
{
	var* f = (var*)malloc(sizeof(var));
	memset(f, 0, sizeof(var));
	f->name=vname;
	f->var_type=vtype;
	f->size = 1;
	//int_var(f);
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

void var_clean_stack(var_stack* s)
{
	var* x;
	for (var* bi = s->root; bi != NULL; bi = x)
	{
		x = bi->stack_next;
		//if(bi->value!=0)free(bi->value);
		//if(bi->opt!=0)free(bi->opt);
		if (bi->var_type->name == T_STRING->name)
			for (int i = 0; i < bi->size; i++)
			{
				free(*(bi->val_str_ptr+i));
			}
		free(bi->value);
		free(bi);
		s->size--;
	}
}

void free_temp_var(var* bi)
{
	
	if (bi->var_type->name == T_STRING->name)
		for (int i = 0; i < bi->size; i++)
		{
			free(*(bi->val_str_ptr+i));
		}
	free(bi->value);
	free(bi);
}
