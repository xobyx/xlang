#pragma once
#include "xlang_main.h"




/*XLANG*/ node* compile (var* parent, node* out, fcall* temp, node* stop, type_def* ncalss);


/*XLANG*/ type_def* get_type_by_name(char* value);
/*XLANG*/ bool call_function(fcall* temp, var** context);
node* setup_function_parms(node** cx, fcall* function, var* context, fcall* in_function);

