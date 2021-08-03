#include "stack.h"



 void stack_init(node_stack * S) {
	S->size = 0;
	S->top = NULL;
	S->root = NULL;
	
	
}
 void int_node(node * n)
{
	n->value_raw=NULL;
	n->next =NULL;
	n->parent =NULL;
	n->value_raw =NULL;
	n->opt_raw=NULL;
	n->stack_parent=NULL;
	n->stack_next=NULL;
	n->type_=0;
	n->ref_node=NULL;
	

	
	
	


}
 node * new_node(node_stack* n)
 {
        node * f= (node *) malloc(sizeof(node));
	 	memset(f,0,sizeof(node));
		//int_node(f);
		node_stack_push(n,f);
		f->id=n->size;
		return f;

 }
 void node_stack_push(node_stack * S, node * el) {
    // Increment number of elements.
	S->size++;
    // Set el to point to current stack top as its next element.
	el->stack_parent = (S->top);
	if(S->top!=NULL)
		S->top->stack_next=el;
    // Set el to be the top element of S.
    S->top = el;

	if(S->root==NULL) S->root=el;
}
 
 void clean_stack(node_stack* s)
 {
	node*x=NULL;
	for (node * bi = s->root;bi!=NULL;bi=x)
	{
		x=bi->stack_next;	
		//if(bi->value!=0)free(bi->value);
		//if(bi->opt!=0)free(bi->opt);
		if((bi->btype.value & a) !=0)
		{
			free(bi->value_raw);
		}
	    

		//free(bi->opt);
		
		free(bi);
		s->size--;
		
	}

 }