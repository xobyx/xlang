#include "xsys.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int g_script_argc = 0;
char** g_script_argv = NULL;

static const char* get_str_arg(fcall* fc, int index, const char* default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return *p->value_str_ptr;
	}
	else if (p->type_define == T_CHAR || (p->type_define != NULL && p->type_define->type_id == 2))
	{
		if (p->value_char_ptr != NULL)
			return p->value_char_ptr;
	}
	return default_val;
}

static int get_int_arg(fcall* fc, int index, int default_val)
{
	if (fc == NULL || index >= fc->parm_count_c) return default_val;
	var* p = &fc->func_parmeters[index];
	if (p == NULL) return default_val;

	if (p->type_define == T_INT || (p->type_define != NULL && p->type_define->type_id == 3))
	{
		if (p->value_int != NULL) return *p->value_int;
	}
	else if (p->type_define == T_LONG || (p->type_define != NULL && p->type_define->type_id == 0))
	{
		if (p->value_long != NULL) return (int)*p->value_long;
	}
	else if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return atoi(*p->value_str_ptr);
	}
	return default_val;
}

void x_get_arg(fcall* fc)
{
	int index = get_int_arg(fc, 0, -1);
	if (index >= 0 && index < g_script_argc && g_script_argv != NULL && g_script_argv[index] != NULL)
	{
		size_t len = strlen(g_script_argv[index]);
		char* s = (char*)malloc(len + 1);
		if (s != NULL)
		{
			strcpy(s, g_script_argv[index]);
			fc->_return.value_str_ptr = get_pptr_string(s);
			fc->_return.type_define = T_STRING;
			return;
		}
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_get_argc(fcall* fc)
{
	fc->_return.value_int = new_int(1, g_script_argc);
	fc->_return.type_define = T_INT;
}

void x_system_exec(fcall* fc)
{
	const char* cmd = get_str_arg(fc, 0, "");
	int rc = -1;

	if (cmd != NULL && *cmd != '\0')
	{
		rc = system(cmd);
	}

	fc->_return.value_int = new_int(1, rc);
	fc->_return.type_define = T_INT;
}

void x_system_getenv(fcall* fc)
{
	const char* name = get_str_arg(fc, 0, "");
	if (name != NULL && *name != '\0')
	{
		char* val = getenv(name);
		if (val != NULL)
		{
			size_t len = strlen(val);
			char* s = (char*)malloc(len + 1);
			if (s != NULL)
			{
				strcpy(s, val);
				fc->_return.value_str_ptr = get_pptr_string(s);
				fc->_return.type_define = T_STRING;
				return;
			}
		}
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_system_setenv(fcall* fc)
{
	const char* name = get_str_arg(fc, 0, "");
	const char* val = get_str_arg(fc, 1, "");
	int rc = -1;

	if (name != NULL && *name != '\0' && val != NULL)
	{
#ifdef _WIN32
		rc = _putenv_s(name, val);
#else
		rc = setenv(name, val, 1);
#endif
	}

	fc->_return.value_int = new_int(1, rc == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

#include <time.h>

void x_clock_ms(fcall* fc)
{
	int ms = 0;
#if defined(_WIN32)
	ms = (int)GetTickCount();
#elif defined(CLOCK_MONOTONIC)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ms = (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
	ms = (int)(clock() * 1000 / CLOCKS_PER_SEC);
#endif
	fc->_return.value_int = new_int(1, ms);
	fc->_return.type_define = T_INT;
}
