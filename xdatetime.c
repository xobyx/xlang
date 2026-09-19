#include "xdatetime.h"
#include "functions.h"
#include "xgc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

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
	else if (p->type_define == T_FLOAT || (p->type_define != NULL && p->type_define->type_id == 5))
	{
		if (p->value_float != NULL) return (int)*p->value_float;
	}
	else if (p->type_define == T_STRING || (p->type_define != NULL && p->type_define->type_id == 1))
	{
		if (p->value_str_ptr != NULL && *p->value_str_ptr != NULL)
			return atoi(*p->value_str_ptr);
	}
	return default_val;
}

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

static time_t get_target_time(fcall* fc, int index)
{
	int ts = get_int_arg(fc, index, 0);
	if (ts <= 0)
	{
		return time(NULL);
	}
	return (time_t)ts;
}

void x_datetime_now(fcall* fc)
{
	time_t t = time(NULL);
	fc->_return.value_int = new_int(1, (int)t);
	fc->_return.type_define = T_INT;
}

void x_datetime_format(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	const char* fmt = get_str_arg(fc, 1, "%Y-%m-%d %H:%M:%S");
	if (fmt == NULL || fmt[0] == '\0')
		fmt = "%Y-%m-%d %H:%M:%S";

	struct tm tm_info;
	localtime_r(&t, &tm_info);

	char buf[256];
	size_t len = strftime(buf, sizeof(buf), fmt, &tm_info);
	if (len == 0)
	{
		buf[0] = '\0';
	}

	char* copy = (char*)gc_malloc(len + 1, GC_KIND_STRING);
	if (copy != NULL)
	{
		memcpy(copy, buf, len + 1);
	}
	else
	{
		copy = (char*)calloc(1, 1);
	}
	fc->_return.value_str_ptr = get_pptr_string(copy);
	fc->_return.type_define = T_STRING;
}

void x_datetime_year(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_year + 1900);
	fc->_return.type_define = T_INT;
}

void x_datetime_month(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_mon + 1);
	fc->_return.type_define = T_INT;
}

void x_datetime_day(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_mday);
	fc->_return.type_define = T_INT;
}

void x_datetime_hour(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_hour);
	fc->_return.type_define = T_INT;
}

void x_datetime_minute(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_min);
	fc->_return.type_define = T_INT;
}

void x_datetime_second(fcall* fc)
{
	time_t t = get_target_time(fc, 0);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	fc->_return.value_int = new_int(1, tm_info.tm_sec);
	fc->_return.type_define = T_INT;
}

void x_datetime_clock_ms(fcall* fc)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long ms = (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
	/* Return lower 31 bits or full int */
	fc->_return.value_int = new_int(1, (int)(ms & 0x7fffffff));
	fc->_return.type_define = T_INT;
}
