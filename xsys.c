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

#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "xgc.h"

void x_proc_capture(fcall* fc)
{
	const char* cmd = get_str_arg(fc, 0, "");
	if (cmd == NULL || *cmd == '\0')
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	FILE* fp = popen(cmd, "r");
	if (fp == NULL)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	size_t cap = 1024;
	size_t len = 0;
	char* buf = (char*)malloc(cap);
	char chunk[512];
	while (fgets(chunk, sizeof(chunk), fp) != NULL)
	{
		size_t clen = strlen(chunk);
		if (len + clen + 1 > cap)
		{
			cap = (len + clen + 1) * 2;
			buf = (char*)realloc(buf, cap);
		}
		if (buf != NULL)
		{
			memcpy(buf + len, chunk, clen);
			len += clen;
		}
	}
	if (buf != NULL)
		buf[len] = '\0';
	pclose(fp);

	char* result = (char*)gc_malloc(len + 1, GC_KIND_STRING);
	if (result != NULL && buf != NULL)
	{
		memcpy(result, buf, len + 1);
	}
	else
	{
		result = buf ? buf : (char*)calloc(1, 1);
	}
	if (buf != NULL && result != buf)
		free(buf);

	fc->_return.value_str_ptr = get_pptr_string(result);
	fc->_return.type_define = T_STRING;
}

void x_proc_run(fcall* fc)
{
	const char* cmd = get_str_arg(fc, 0, "");
	if (cmd == NULL || *cmd == '\0')
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	FILE* fp = popen(cmd, "r");
	if (fp == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	size_t cap = 1024;
	size_t len = 0;
	char* buf = (char*)malloc(cap);
	char chunk[512];
	while (fgets(chunk, sizeof(chunk), fp) != NULL)
	{
		size_t clen = strlen(chunk);
		if (len + clen + 1 > cap)
		{
			cap = (len + clen + 1) * 2;
			buf = (char*)realloc(buf, cap);
		}
		if (buf != NULL)
		{
			memcpy(buf + len, chunk, clen);
			len += clen;
		}
	}
	if (buf != NULL)
		buf[len] = '\0';
	int status = pclose(fp);
#ifndef _WIN32
	int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : status;
#else
	int exit_code = status;
#endif

	type_def* pr_td = get_type_by_name("ProcessResult");
	if (pr_td == NULL || pr_td->d_propertys_size == 0)
	{
		extern type_def* new_type(void);
		extern void define_new_class_prop(char* name, type_def* contern_class, type_def* new_var_type, var** out_var);
		if (pr_td == NULL)
		{
			pr_td = new_type();
			pr_td->type_name = "ProcessResult";
		}
		var* dummy = NULL;
		define_new_class_prop("stdout", pr_td, T_STRING, &dummy);
		define_new_class_prop("exit_code", pr_td, T_INT, &dummy);
		type_def_compute_field_offsets(pr_td);
	}
	if (pr_td != NULL && !is_base_type(pr_td))
	{
		type_instance* inst = type_instance_create(pr_td);
		var* so_prop = type_instance_get_field(inst, "stdout");
		if (so_prop != NULL)
		{
			char* out_copy = (char*)gc_malloc(len + 1, GC_KIND_STRING);
			if (out_copy != NULL && buf != NULL)
				memcpy(out_copy, buf, len + 1);
			else
				out_copy = (char*)calloc(1, 1);
			so_prop->value_str_ptr = get_pptr_string(out_copy);
			so_prop->type_define = T_STRING;
		}
		var* ec_prop = type_instance_get_field(inst, "exit_code");
		if (ec_prop != NULL && ec_prop->value_int != NULL)
		{
			*ec_prop->value_int = exit_code;
		}
		if (buf != NULL)
			free(buf);
		fc->_return.type_define = pr_td;
		fc->_return.values = inst;
		fc->_return.value_type_instsance = inst;
		fc->_return.size = 1;
	}
	else
	{
		if (buf != NULL)
			free(buf);
		fc->_return.value_int = new_int(1, exit_code);
		fc->_return.type_define = T_INT;
	}
}

