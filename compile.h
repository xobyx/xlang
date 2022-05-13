#pragma once
#include "xlang_main.h"




/*XLANG*/ void compile (var* parent, node* out, fcall* temp, node* stop);
/*XLANG*/ void compile_var_name_start(node ** pnode, fcall * function_c, var * calling_object);
/*XLANG*/ node* add_new_func_code_to_typedef(node* c, type_def* contner_class);
/*XLANG*/ type_def* get_type_by_name(char* value);
/*XLANG*/ bool call_function(fcall* temp, var** context);
node* setup_function_parms(node** cx, fcall* function, var* context, fcall* in_function);
#define  COMPILE_1_P(a) compile(0,a,0,0)