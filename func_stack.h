#pragma once
#include "types.h"
#include "stdlib.h"


 void func_stack_init(func_stack * S) ;
 void int_func(func_deftion * n);

 func_deftion * new_func_on_stack(func_stack* n);
 
 void func_stack_push(func_stack * stk, func_deftion * el) ;

 void func_clean_stack(func_stack* s);
