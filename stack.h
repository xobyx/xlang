#pragma once

#include "xlang_main.h"



 void node_stack_push(node_stack * S, node * el);
 
 void stack_init(node_stack * S);
 void clean_stack(node_stack* s);
 node * new_node(node_stack* n);
 node* get_node_id(node_stack* s,int id);

