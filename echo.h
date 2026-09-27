#pragma once
#include "types.h"

void _echo(fcall* func_call);
void echo_var(var variable, int indent_level);
void echo_func_def(func_deftion func_def, int indent_level);
void echo_type_def(type_def type_definition, int indent_level);
void echo_type_instance(type_instance* instance, int indent_level);