#include "type_stack.h"



 void type_stack_init(type_stack * S) {
	S->size = 0;
	S->top = NULL;
	S->root = NULL;
	
	
}
 void int_type(type * n)
{
	
	


	
	
	


}
type * new_type_stack(type_stack* n)
 {
        type* f= (type*)calloc(1,sizeof(type));
		//f->id=n->size;
		int_type(f);
		type_stack_push(n,f);
		return f;

 }
 void type_stack_push(type_stack * stk, type * el) {
    // Increment number of elements.
	stk->size++;
	if(stk->root==NULL) stk->root=el;
    // Set el to point to current stack top as its next element.
	
	if(stk->top!=NULL)
		stk->top->stack_next=el;
    // Set el to be the top element of S.
    stk->top = el;

	
}

 void type_clean_stack(type_stack* s)
 {
	type*x=NULL;
	for (type * bi = s->root;bi!=NULL;bi=x)
	{
		x=bi->stack_next;		
		//if(bi->value!=0)free(bi->value);
		//if(bi->opt!=0)free(bi->opt);
	  //  var_clean_stack(&bi->propertys);
	//	func_clean_stack(&bi->functions);
		
		free(bi);
		s->size--;
		
	}

 }