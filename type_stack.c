#include "type_stack.h"



 void type_stack_init(type_stack * S) {
	S->size = 0;
	S->top = NULL;
	S->root = NULL;
	
	
}
 void int_type(type_def * n)
{
	
	


	
	
	


}
type_def * new_type_stack(type_stack* n)
 {
        type_def* f= (type_def*)calloc(1,sizeof(type_def));
		//f-.type_id=n->size;
		int_type(f);
		type_stack_push(n,f);
		return f;

 }
 void type_stack_push(type_stack * stk, type_def * el) {
    // Increment number of elements.
	stk->size++;
	if(stk->root==NULL) stk->root=el;
    // Set el to point to current stack top as its next element.
	
	if(stk->top!=NULL)
		stk->top->stack_next=el;
    // Set el to be the top element of S.
    stk->top = el;

	
}

extern type_def SIMPLE_TYPE[];

void type_clean_stack(type_stack* s)
{
	if (s == NULL) return;
	type_def* x = NULL;
	for (type_def* bi = s->root; bi != NULL; bi = x)
	{
		x = bi->stack_next;
		bool is_static = (bi >= SIMPLE_TYPE && bi < SIMPLE_TYPE + 11);
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