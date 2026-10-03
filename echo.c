#include "echo.h"
#include <stdio.h>

static inline void print_indent(int level)
{
	if (level > 0)
		printf("%*s", level * 4, " ");
}

void echo_var(var variable, int indent_level)
{
	if (indent_level > 4) return;
	print_indent(indent_level);
	printf("name:%s\n", variable.name ? variable.name : "(value)");
	print_indent(indent_level);
	printf("size:%d\n", variable.size);

	if (variable.type_define != NULL && variable.values != NULL)
	{
		switch (variable.type_define->type_id)
		{
		case 0: /* long */
			print_indent(indent_level);
			printf("value long : %ld", *variable.value_long);
			break;
		case 1: /* string */
			print_indent(indent_level);
			printf("value string : %s", variable.value_str_ptr ? *variable.value_str_ptr : "(null)");
			break;
		case 2: /* char */
			print_indent(indent_level);
			printf("value char : %c", *variable.value_char_ptr);
			break;
		case 3: /* int */
			print_indent(indent_level);
			printf("value_int : %d", *variable.value_int);
			break;
		case 4: /* bool */
			print_indent(indent_level);
			printf("value_bool : %s", *variable.value_bool ? "true" : "false");
			break;
		case 5: /* float */
			print_indent(indent_level);
			printf("value float : %f", *variable.value_float);
			break;
		case 9: /* T_FUNC */
			print_indent(indent_level);
			printf("value function :\n");
			print_indent(indent_level);
			printf("{\n");
			if (variable.value_func != NULL)
				echo_func_def(*variable.value_func, indent_level + 1);
			print_indent(indent_level);
			printf("}\n");
			break;
		default:
			print_indent(indent_level);
			printf("value %s : [object id=%d]", variable.type_define->type_name ? variable.type_define->type_name : "object", variable.value_int ? *variable.value_int : 0);
			break;
		}
	}
	print_indent(indent_level);
	printf(", addr = {{%p}} , {{size = %d}} \n", variable.values, variable.size);

	if (variable.type_define != NULL && indent_level <= 2)
	{
		print_indent(indent_level);
		printf("type define : \n");
		print_indent(indent_level);
		printf("{\n");
		echo_type_def(*variable.type_define, indent_level + 1);
		print_indent(indent_level);
		printf("}\n");
	}
}

void echo_func_def(func_deftion func_def, int indent_level)
{
	print_indent(indent_level);
	printf("function name : %s \n", func_def.func_name ? func_def.func_name : "(null)");
	print_indent(indent_level);
	printf("parms count : %d \n", func_def.start_param_count);
	print_indent(indent_level);
	printf("parms: \n");
	print_indent(indent_level);
	printf("{\n");

	int next_level = indent_level + 1;
	int inner_level = indent_level + 2;
	for (int i = 0; i < func_def.start_param_count; i++)
	{
		print_indent(next_level);
		printf("{\n");
		print_indent(inner_level);
		printf("parm index :%d , name : %s\n", i, func_def.start_func_parameters_name[i]);
		print_indent(inner_level);
		if (func_def.start_func_parameters[i] != NULL)
		{
			printf("parm type : %s\n", func_def.start_func_parameters[i]->type_name ? func_def.start_func_parameters[i]->type_name : "(null)");
		}
		else
		{
			printf("typedef: ANY TYPE\n");
		}
		print_indent(next_level);
		printf("}\n");
	}
	print_indent(indent_level);
	printf("}\n");
	print_indent(indent_level);
	printf("vector func: %p \n", func_def.vector_func);

	print_indent(indent_level);
	printf("return type : %s\n", (func_def.return_type && func_def.return_type->type_name) ? func_def.return_type->type_name : "(void)");
}

void echo_type_def(type_def type_definition, int indent_level)
{
	if (indent_level > 4) return;
	print_indent(indent_level);
	printf("type name : %s \n", type_definition.type_name ? type_definition.type_name : "(null)");
	print_indent(indent_level);
	printf("type id : %d \n", type_definition.type_id);
	print_indent(indent_level);
	printf("props count : %d\n", type_definition.property_count);
	if (type_definition.property_count > 0 && indent_level <= 2)
	{
		print_indent(indent_level);
		printf("props :\n");
		print_indent(indent_level);
		printf("{\n");
		for (int i = 0; i < type_definition.property_count; i++)
		{
			echo_var(type_definition.properties[i], indent_level + 1);
		}
		print_indent(indent_level);
		printf("}\n");
	}

	print_indent(indent_level);
	printf("function count : %d\n", type_definition.function_count);
	if (type_definition.function_count > 0 && indent_level <= 2)
	{
		print_indent(indent_level);
		printf("functions : \n");
		print_indent(indent_level);
		printf("{\n");
		for (int i = 0; i < type_definition.function_count; i++)
		{
			echo_func_def(type_definition.d_functions[i], indent_level + 1);
		}
		print_indent(indent_level);
		printf("}\n");
	}
}

void echo_type_instance(type_instance* instance, int indent_level)
{
	if (instance == NULL)
		return;

	print_indent(indent_level);
	printf("props count : %u\n", instance->field_count);
	print_indent(indent_level);
	printf("prop : \n");
	print_indent(indent_level);
	printf("{\n");
	for (uint32_t i = 0; i < instance->field_count; i++)
	{
		echo_var(instance->fields[i], indent_level + 1);
	}
	print_indent(indent_level);
	printf("}\n");

	print_indent(indent_level);
	printf("function count : %d\n", instance->functions.size);
	print_indent(indent_level);
	printf("functions :\n");
	print_indent(indent_level);
	printf("{\n");
	for (func_deftion* i = instance->functions.root; i != NULL; i = i->stack_next)
	{
		echo_func_def(*i, indent_level + 1);
	}
	print_indent(indent_level);
	printf("}\n");

	if (instance->type != NULL)
	{
		print_indent(indent_level);
		printf("type define : \n");
		print_indent(indent_level);
		printf("{\n");
		echo_type_def(*instance->type, indent_level + 1);
		print_indent(indent_level);
		printf("}\n");
	}
	if (instance->base != NULL && instance->base != instance)
	{
		print_indent(indent_level);
		printf("base :\n");
		print_indent(indent_level);
		printf("{\n");
		echo_type_instance(instance->base, indent_level + 1);
		print_indent(indent_level);
		printf("}\n");
	}
}