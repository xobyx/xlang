#include "xstring.h"
#include "functions.h"
#include "pcre.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static bool is_str_method_call(fcall* fc)
{
	return (fc != NULL && fc->context != NULL &&
	        (fc->context->type_define == T_STRING || fc->context->type_define == T_CHAR));
}

static const char* get_target_str(fcall* fc)
{
	if (is_str_method_call(fc))
	{
		if (fc->context->type_define == T_STRING && fc->context->value_str_ptr != NULL && *fc->context->value_str_ptr != NULL)
			return *fc->context->value_str_ptr;
		if (fc->context->type_define == T_CHAR && fc->context->value_char_ptr != NULL)
			return fc->context->value_char_ptr;
	}
	return get_str_arg(fc, 0, "");
}

static const char* get_sub_str_arg(fcall* fc, int method_idx, const char* default_val)
{
	int idx = is_str_method_call(fc) ? method_idx : (method_idx + 1);
	return get_str_arg(fc, idx, default_val);
}

static int get_sub_int_arg(fcall* fc, int method_idx, int default_val)
{
	int idx = is_str_method_call(fc) ? method_idx : (method_idx + 1);
	return get_int_arg(fc, idx, default_val);
}

void x_regex_match(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* pattern = get_sub_str_arg(fc, 0, "");
	int matched = 0;

	if (str != NULL && pattern != NULL)
	{
		const char* error = NULL;
		int erroffset = 0;
		pcre* re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
		if (re != NULL)
		{
			int ovector[30];
			int rc = pcre_exec(re, NULL, str, (int)strlen(str), 0, 0, ovector, 30);
			if (rc >= 0)
			{
				matched = 1;
			}
			pcre_free(re);
		}
	}

	fc->_return.value_int = new_int(1, matched);
	fc->_return.type_define = T_INT;
}

void x_regex_find(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* pattern = get_sub_str_arg(fc, 0, "");

	if (str != NULL && pattern != NULL)
	{
		const char* error = NULL;
		int erroffset = 0;
		pcre* re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
		if (re != NULL)
		{
			int ovector[30];
			int rc = pcre_exec(re, NULL, str, (int)strlen(str), 0, 0, ovector, 30);
			if (rc >= 0)
			{
				int match_len = ovector[1] - ovector[0];
				char* out = (char*)malloc(match_len + 1);
				if (out != NULL)
				{
					memcpy(out, str + ovector[0], match_len);
					out[match_len] = '\0';
					pcre_free(re);
					fc->_return.value_str_ptr = get_pptr_string(out);
					fc->_return.type_define = T_STRING;
					return;
				}
			}
			pcre_free(re);
		}
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_regex_replace(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* pattern = get_sub_str_arg(fc, 0, "");
	const char* repl = get_sub_str_arg(fc, 1, "");

	if (str == NULL || pattern == NULL || repl == NULL)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	const char* error = NULL;
	int erroffset = 0;
	pcre* re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
	if (re == NULL)
	{
		size_t len = strlen(str);
		char* copy = (char*)malloc(len + 1);
		strcpy(copy, str);
		fc->_return.value_str_ptr = get_pptr_string(copy);
		fc->_return.type_define = T_STRING;
		return;
	}

	int str_len = (int)strlen(str);
	int ovector[30];
	int offset = 0;

	size_t cap = str_len + 64;
	char* result = (char*)malloc(cap);
	int res_len = 0;
	result[0] = '\0';

	while (offset < str_len)
	{
		int rc = pcre_exec(re, NULL, str, str_len, offset, 0, ovector, 30);
		if (rc < 0)
		{
			// Append rest of string
			int rem = str_len - offset;
			if (res_len + rem + 1 > (int)cap)
			{
				cap = res_len + rem + 64;
				result = (char*)realloc(result, cap);
			}
			memcpy(result + res_len, str + offset, rem);
			res_len += rem;
			result[res_len] = '\0';
			break;
		}

		int prefix_len = ovector[0] - offset;
		int repl_len = (int)strlen(repl);

		if (res_len + prefix_len + repl_len + 1 > (int)cap)
		{
			cap = res_len + prefix_len + repl_len + 64;
			result = (char*)realloc(result, cap);
		}

		memcpy(result + res_len, str + offset, prefix_len);
		res_len += prefix_len;
		memcpy(result + res_len, repl, repl_len);
		res_len += repl_len;
		result[res_len] = '\0';

		offset = ovector[1];
		if (ovector[0] == ovector[1])
		{
			offset++;
		}
	}

	pcre_free(re);
	fc->_return.value_str_ptr = get_pptr_string(result);
	fc->_return.type_define = T_STRING;
}

void x_substr(fcall* fc)
{
	const char* str = get_target_str(fc);
	int start = get_sub_int_arg(fc, 0, 0);
	int len = get_sub_int_arg(fc, 1, -1);

	int total_len = str ? (int)strlen(str) : 0;
	if (total_len == 0 || start >= total_len)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	if (start < 0)
	{
		start = total_len + start;
		if (start < 0) start = 0;
	}

	if (len < 0 || start + len > total_len)
	{
		len = total_len - start;
	}

	char* sub = (char*)malloc(len + 1);
	if (sub != NULL)
	{
		memcpy(sub, str + start, len);
		sub[len] = '\0';
		fc->_return.value_str_ptr = get_pptr_string(sub);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_index_of(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* search = get_sub_str_arg(fc, 0, "");
	int pos = -1;

	if (str != NULL && search != NULL && *search != '\0')
	{
		char* found = strstr(str, search);
		if (found != NULL)
		{
			pos = (int)(found - str);
		}
	}

	fc->_return.value_int = new_int(1, pos);
	fc->_return.type_define = T_INT;
}

void x_trim(fcall* fc)
{
	const char* str = get_target_str(fc);
	if (str == NULL || *str == '\0')
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	const char* start = str;
	while (*start && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n'))
		start++;

	const char* end = str + strlen(str) - 1;
	while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
		end--;

	int len = (int)(end - start + 1);
	if (len < 0) len = 0;

	char* out = (char*)malloc(len + 1);
	if (out != NULL)
	{
		memcpy(out, start, len);
		out[len] = '\0';
		fc->_return.value_str_ptr = get_pptr_string(out);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_to_lower(fcall* fc)
{
	const char* str = get_target_str(fc);
	size_t len = str ? strlen(str) : 0;
	char* out = (char*)malloc(len + 1);
	if (out != NULL)
	{
		for (size_t i = 0; i < len; i++)
			out[i] = (char)tolower((unsigned char)str[i]);
		out[len] = '\0';
		fc->_return.value_str_ptr = get_pptr_string(out);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_to_upper(fcall* fc)
{
	const char* str = get_target_str(fc);
	size_t len = str ? strlen(str) : 0;
	char* out = (char*)malloc(len + 1);
	if (out != NULL)
	{
		for (size_t i = 0; i < len; i++)
			out[i] = (char)toupper((unsigned char)str[i]);
		out[len] = '\0';
		fc->_return.value_str_ptr = get_pptr_string(out);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* empty = (char*)calloc(1, 1);
	fc->_return.value_str_ptr = get_pptr_string(empty);
	fc->_return.type_define = T_STRING;
}

void x_starts_with(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* prefix = get_sub_str_arg(fc, 0, "");
	int res = 0;

	if (str != NULL && prefix != NULL)
	{
		size_t pre_len = strlen(prefix);
		if (strncmp(str, prefix, pre_len) == 0)
			res = 1;
	}

	fc->_return.value_int = new_int(1, res);
	fc->_return.type_define = T_INT;
}

void x_ends_with(fcall* fc)
{
	const char* str = get_target_str(fc);
	const char* suffix = get_sub_str_arg(fc, 0, "");
	int res = 0;

	if (str != NULL && suffix != NULL)
	{
		size_t str_len = strlen(str);
		size_t suf_len = strlen(suffix);
		if (str_len >= suf_len)
		{
			if (strcmp(str + (str_len - suf_len), suffix) == 0)
				res = 1;
		}
	}

	fc->_return.value_int = new_int(1, res);
	fc->_return.type_define = T_INT;
}
