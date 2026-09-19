#include "func_stack.h"
#include "functions.h"

#include <stdlib.h>
#include <string.h>


 void func_stack_init(func_stack * S) {
	S->size = 0;
	S->top = NULL;
	S->root = NULL;
	
	
	
}
 void int_func(func_deftion * n)
{
	
	

	
	
	


}
func_deftion * new_func_on_stack(func_stack* n)
 {
        func_deftion* f= (func_deftion*)malloc(sizeof(func_deftion));
		memset(f,0,sizeof(func_deftion));
		func_stack_push(n,f);
		return f;

 }
 void func_stack_push(func_stack * stk, func_deftion * el) {
    // Increment number of elements.
	stk->size++;
	if(stk->root==NULL) stk->root=el;
    // Set el to point to current stack top as its next element.
	
	if(stk->top!=NULL)
		stk->top->stack_next=el;
    // Set el to be the top element of S.
    stk->top = el;

	
}

extern func_deftion simple_function_array[];

void func_clean_stack(func_stack* s)
{
	if (s == NULL) return;
	func_deftion* x = NULL;
	for (func_deftion* bi = s->root; bi != NULL; bi = x)
	{
		x = bi->stack_next;
		bool is_static = (bi >= simple_function_array && bi < simple_function_array + SIMPLE_FUNC_COUNT);
		if (!is_static)
		{
			free(bi);
		}
		s->size--;
	}
	s->root = NULL;
	s->top = NULL;
	s->size = 0;
}