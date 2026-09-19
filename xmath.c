#include "xmath.h"
#include "functions.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static float* new_float_val(float val)
{
	float* f = (float*)malloc(sizeof(float));
	*f = val;
	return f;
}

static float get_float_arg(fcall* fc, int index, float default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_FLOAT && p->value_float != NULL) return *p->value_float;
	if (p->type_define == T_INT && p->value_int != NULL) return (float)*p->value_int;
	if (p->type_define == T_LONG && p->value_long != NULL) return (float)*p->value_long;
	if (p->type_define == T_STRING && p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
		return (float)atof(*p->value_str_ptr);
	return default_val;
}

static bool is_arg_float(fcall* fc, int index)
{
	if (fc == NULL || index >= fc->parm_count_c) return false;
	var* p = &fc->func_parmeters[index];
	return (p != NULL && p->type_define == T_FLOAT);
}

void x_math_sqrt(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	float res = (v >= 0.0f) ? sqrtf(v) : 0.0f;
	fc->_return.value_float = new_float_val(res);
	fc->_return.type_define = T_FLOAT;
}

void x_math_pow(fcall* fc)
{
	float base = get_float_arg(fc, 0, 0.0f);
	float exp = get_float_arg(fc, 1, 1.0f);
	float res = powf(base, exp);
	fc->_return.value_float = new_float_val(res);
	fc->_return.type_define = T_FLOAT;
}

void x_math_abs(fcall* fc)
{
	if (is_arg_float(fc, 0))
	{
		float v = get_float_arg(fc, 0, 0.0f);
		fc->_return.value_float = new_float_val(fabsf(v));
		fc->_return.type_define = T_FLOAT;
	}
	else
	{
		int iv = (int)get_float_arg(fc, 0, 0.0f);
		fc->_return.value_int = new_int(1, iv < 0 ? -iv : iv);
		fc->_return.type_define = T_INT;
	}
}

void x_math_min(fcall* fc)
{
	if (is_arg_float(fc, 0) || is_arg_float(fc, 1))
	{
		float a = get_float_arg(fc, 0, 0.0f);
		float b = get_float_arg(fc, 1, 0.0f);
		fc->_return.value_float = new_float_val(a < b ? a : b);
		fc->_return.type_define = T_FLOAT;
	}
	else
	{
		int a = (int)get_float_arg(fc, 0, 0.0f);
		int b = (int)get_float_arg(fc, 1, 0.0f);
		fc->_return.value_int = new_int(1, a < b ? a : b);
		fc->_return.type_define = T_INT;
	}
}

void x_math_max(fcall* fc)
{
	if (is_arg_float(fc, 0) || is_arg_float(fc, 1))
	{
		float a = get_float_arg(fc, 0, 0.0f);
		float b = get_float_arg(fc, 1, 0.0f);
		fc->_return.value_float = new_float_val(a > b ? a : b);
		fc->_return.type_define = T_FLOAT;
	}
	else
	{
		int a = (int)get_float_arg(fc, 0, 0.0f);
		int b = (int)get_float_arg(fc, 1, 0.0f);
		fc->_return.value_int = new_int(1, a > b ? a : b);
		fc->_return.type_define = T_INT;
	}
}

void x_math_floor(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(floorf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_ceil(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(ceilf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_round(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(roundf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_sin(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(sinf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_cos(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(cosf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_tan(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	fc->_return.value_float = new_float_val(tanf(v));
	fc->_return.type_define = T_FLOAT;
}

void x_math_log(fcall* fc)
{
	float v = get_float_arg(fc, 0, 0.0f);
	float res = (v > 0.0f) ? logf(v) : 0.0f;
	fc->_return.value_float = new_float_val(res);
	fc->_return.type_define = T_FLOAT;
}
