#include "stack.h"



 void var_stack_init(var_stack * S) ;
 void int_var(var * n);

var * new_var_on_stack(var_stack* stack, char* name, type* vtype);
 
 void var_stack_push(var_stack * s, var * el) ;

 void var_clean_stack(var_stack* s);
 void free_temp_var(var* var);