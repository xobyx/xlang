#pragma once
#include "stack.h"



 void func_stack_init(func_stack * S) ;
 void int_func(func * n);

 func * new_func_on_stack(func_stack* n);
 
 void func_stack_push(func_stack * stk, func * el) ;

 void func_clean_stack(func_stack* s);
