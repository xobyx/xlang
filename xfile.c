#include "xfile.h"
#include "functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_HANDLES 64
static FILE* s_open_files[MAX_FILE_HANDLES] = {0};

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

void x_file_read_all(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	if (path == NULL || *path == '\0')
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	FILE* f = fopen(path, "rb");
	if (f == NULL)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size < 0) size = 0;
	char* buf = (char*)malloc(size + 1);
	if (buf == NULL)
	{
		fclose(f);
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	size_t read_bytes = fread(buf, 1, size, f);
	buf[read_bytes] = '\0';
	fclose(f);

	fc->_return.value_str_ptr = get_pptr_string(buf);
	fc->_return.type_define = T_STRING;
}

void x_file_write_all(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	const char* content = get_str_arg(fc, 1, "");

	if (path == NULL || *path == '\0')
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	FILE* f = fopen(path, "wb");
	if (f == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	size_t len = content ? strlen(content) : 0;
	size_t written = fwrite(content, 1, len, f);
	fclose(f);

	fc->_return.value_int = new_int(1, (int)written);
	fc->_return.type_define = T_INT;
}

void x_file_append(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	const char* content = get_str_arg(fc, 1, "");

	if (path == NULL || *path == '\0')
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	FILE* f = fopen(path, "ab");
	if (f == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	size_t len = content ? strlen(content) : 0;
	size_t written = fwrite(content, 1, len, f);
	fclose(f);

	fc->_return.value_int = new_int(1, (int)written);
	fc->_return.type_define = T_INT;
}

void x_file_exists(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	int exists = 0;

	if (path != NULL && *path != '\0')
	{
		FILE* f = fopen(path, "rb");
		if (f != NULL)
		{
			exists = 1;
			fclose(f);
		}
	}

	fc->_return.value_int = new_int(1, exists);
	fc->_return.type_define = T_INT;
}

void x_file_remove(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	int rc = -1;

	if (path != NULL && *path != '\0')
	{
		rc = remove(path);
	}

	fc->_return.value_int = new_int(1, rc == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}

void x_file_size(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	int size = -1;

	if (path != NULL && *path != '\0')
	{
		FILE* f = fopen(path, "rb");
		if (f != NULL)
		{
			fseek(f, 0, SEEK_END);
			size = (int)ftell(f);
			fclose(f);
		}
	}

	fc->_return.value_int = new_int(1, size);
	fc->_return.type_define = T_INT;
}

void x_file_open(fcall* fc)
{
	const char* path = get_str_arg(fc, 0, "");
	const char* mode = get_str_arg(fc, 1, "r");

	if (path == NULL || *path == '\0')
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	FILE* f = fopen(path, mode);
	if (f == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	int slot = -1;
	for (int i = 1; i < MAX_FILE_HANDLES; i++)
	{
		if (s_open_files[i] == NULL)
		{
			s_open_files[i] = f;
			slot = i;
			break;
		}
	}

	if (slot < 0)
	{
		fclose(f);
	}

	fc->_return.value_int = new_int(1, slot);
	fc->_return.type_define = T_INT;
}

void x_file_read(fcall* fc)
{
	int handle = get_int_arg(fc, 0, -1);
	int bytes_to_read = get_int_arg(fc, 1, 4096);
	if (bytes_to_read <= 0) bytes_to_read = 4096;
	if (bytes_to_read > 10 * 1024 * 1024) bytes_to_read = 10 * 1024 * 1024;

	if (handle <= 0 || handle >= MAX_FILE_HANDLES || s_open_files[handle] == NULL)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	char* buf = (char*)malloc(bytes_to_read + 1);
	if (buf == NULL)
	{
		char* empty = (char*)calloc(1, 1);
		fc->_return.value_str_ptr = get_pptr_string(empty);
		fc->_return.type_define = T_STRING;
		return;
	}

	size_t n = fread(buf, 1, bytes_to_read, s_open_files[handle]);
	buf[n] = '\0';

	fc->_return.value_str_ptr = get_pptr_string(buf);
	fc->_return.type_define = T_STRING;
}

void x_file_write(fcall* fc)
{
	int handle = get_int_arg(fc, 0, -1);
	const char* data = get_str_arg(fc, 1, "");

	if (handle <= 0 || handle >= MAX_FILE_HANDLES || s_open_files[handle] == NULL || data == NULL)
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	size_t len = strlen(data);
	size_t written = fwrite(data, 1, len, s_open_files[handle]);
	fflush(s_open_files[handle]);

	fc->_return.value_int = new_int(1, (int)written);
	fc->_return.type_define = T_INT;
}

void x_file_close(fcall* fc)
{
	int handle = get_int_arg(fc, 0, -1);
	int rc = -1;

	if (handle > 0 && handle < MAX_FILE_HANDLES && s_open_files[handle] != NULL)
	{
		rc = fclose(s_open_files[handle]);
		s_open_files[handle] = NULL;
	}

	fc->_return.value_int = new_int(1, rc == 0 ? 0 : -1);
	fc->_return.type_define = T_INT;
}
