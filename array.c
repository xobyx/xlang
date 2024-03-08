#include "types.h"
#include "functions.h"

void array_len(fcall* fcall)
{
	//hello
}



int PROP_SIZE = 1;
int FINC_SIZE = 1;
type_def array = {
	.type_name = "array",
	.d_propertys = {
		{.name = "type", .type_define = T_TYPE_INFO, .size = 1}
	},
	.d_functions = {
		{
			.start_func_parmeters = {0}, .start_func_parmeters_name = {0},
			.start_parm_count = 0, .func_code = &array_len, .func_name = "len",
			.function_type = class_function, .return_type = T_INT, .ref = 0,
			.access = PUBLIC, .stack_next = NULL
		}
	},
	.d_propertys_size = 1,
	.d_function_size = 1, .type_id = 434343, .base = NULL,
	.stack_next = NULL, .access = PUBLIC, .w = child
};
