#include "stack.h"



 void stack_init(node_stack * S) {
	S->size = 0;
	S->top = NULL;
	S->root = NULL;
	S->max_a=0;	
	S->nlist=NULL;


}

 node * new_node(node_stack* n)
 {
      
        node * f;
        f = (node *)malloc(sizeof(node));
        memset(f, 0, sizeof(node));
		node_stack_push(n, f);
		f->id = n->size;
		return f;
 }

 node* get_node_id(node_stack* s,int id)
 {
 int index = id -1;
 if(id > s->max_a  ) //1,[2-max_a],..[for-loop-fill]....[id-new-max-a]
 {
    s->nlist =(node**) ( realloc(s->nlist,sizeof(node*) * (id)));


    for(int m = s->max_a ;m<= index ; m++)  //
    {
        s->nlist[m] = new_node(s);
        s->nlist[m]->id= m+1;
    }
    s->max_a = id;


 }

   // s->pos_id= id;
return s->nlist[index];



 }




 void node_stack_push(node_stack * S, node * el) {
    // Increment number of elements.
	S->size++;
    // Set el to point to current stack top as its next element.
	el->stack_parent = (S->top);
	if (S->top != NULL)
		S->top->stack_next = el;
    // Set el to be the top element of S.
    S->top = el;

	if (S->root == NULL) S->root = el;

}

 void clean_stack(node_stack* s)
 {
	if (s == NULL) return;
	if (s->nlist != NULL)
	{
		free(s->nlist);
		s->nlist = NULL;
	}
	node* bi = s->root;
	while (bi != NULL)
	{
		node* next = bi->stack_next;
		if ((bi->btype.value & a) != 0)
		{
			free(bi->value_raw);
		}
		free(bi);
		bi = next;
	}
	s->root = NULL;
	s->top = NULL;
	s->size = 0;
	s->max_a = 0;
 }
