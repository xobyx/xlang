#pragma once
#include "xlang_main.h"




/*XLANG*/ void compile (var* parent, node* out, func* temp, node* stop);
/*XLANG*/ void compile_var_name_start(node** cx, func** tempx, var* context);
/*XLANG*/ node* add_new_func_code(node* c, type* contner_class);
/*XLANG*/ type* get_type_by_name(char* value);
/*XLANG*/ bool call_function( func* temp, var** context);
node* setup_function_parms(node** cx, func* function, var* context, func* in_function);
#define  COMPILE_1_P(a) compile(0,a,0,0)