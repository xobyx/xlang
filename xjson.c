#include "xjson.h"
#include "xcollection.h"
#include "functions.h"
#include "xgc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Dynamic string buffer for JSON serialization */
typedef struct {
	char* data;
	size_t len;
	size_t cap;
} str_buf_t;

static void buf_init(str_buf_t* b)
{
	b->cap = 256;
	b->len = 0;
	b->data = (char*)malloc(b->cap);
	if (b->data) b->data[0] = '\0';
}

static void buf_append_len(str_buf_t* b, const char* s, size_t n)
{
	if (b->data == NULL) return;
	if (b->len + n + 1 > b->cap)
	{
		size_t new_cap = b->cap * 2;
		while (new_cap < b->len + n + 1) new_cap *= 2;
		char* nd = (char*)realloc(b->data, new_cap);
		if (nd == NULL) return;
		b->data = nd;
		b->cap = new_cap;
	}
	memcpy(b->data + b->len, s, n);
	b->len += n;
	b->data[b->len] = '\0';
}

static void buf_append(str_buf_t* b, const char* s)
{
	if (s == NULL) return;
	buf_append_len(b, s, strlen(s));
}

static void buf_append_escaped_str(str_buf_t* b, const char* s)
{
	buf_append_len(b, "\"", 1);
	if (s != NULL)
	{
		for (const char* p = s; *p != '\0'; p++)
		{
			switch (*p)
			{
			case '"':  buf_append_len(b, "\\\"", 2); break;
			case '\\': buf_append_len(b, "\\\\", 2); break;
			case '\b': buf_append_len(b, "\\b", 2);  break;
			case '\f': buf_append_len(b, "\\f", 2);  break;
			case '\n': buf_append_len(b, "\\n", 2);  break;
			case '\r': buf_append_len(b, "\\r", 2);  break;
			case '\t': buf_append_len(b, "\\t", 2);  break;
			default:
				if ((unsigned char)*p < 0x20)
				{
					char ubuf[8];
					snprintf(ubuf, sizeof(ubuf), "\\u%04x", (unsigned char)*p);
					buf_append(b, ubuf);
				}
				else
				{
					buf_append_len(b, p, 1);
				}
				break;
			}
		}
	}
	buf_append_len(b, "\"", 1);
}

/* ------------------------------------------------------------------------- */
/* JSON Parser                                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
	const char* src;
	size_t pos;
	size_t len;
	bool has_error;
} json_parser_t;

static void skip_ws(json_parser_t* p)
{
	while (p->pos < p->len && (p->src[p->pos] == ' ' || p->src[p->pos] == '\t' ||
	                           p->src[p->pos] == '\n' || p->src[p->pos] == '\r'))
	{
		p->pos++;
	}
}

static char peek_char(json_parser_t* p)
{
	skip_ws(p);
	if (p->pos >= p->len) return '\0';
	return p->src[p->pos];
}

static char next_char(json_parser_t* p)
{
	skip_ws(p);
	if (p->pos >= p->len) return '\0';
	return p->src[p->pos++];
}

static char* parse_json_string(json_parser_t* p)
{
	skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '"')
	{
		p->has_error = true;
		return NULL;
	}
	p->pos++; /* skip opening quote */

	str_buf_t sb;
	buf_init(&sb);

	while (p->pos < p->len)
	{
		char c = p->src[p->pos++];
		if (c == '"')
		{
			return sb.data;
		}
		if (c == '\\')
		{
			if (p->pos >= p->len) { p->has_error = true; free(sb.data); return NULL; }
			char esc = p->src[p->pos++];
			switch (esc)
			{
			case '"':  buf_append_len(&sb, "\"", 1); break;
			case '\\': buf_append_len(&sb, "\\", 1); break;
			case '/':  buf_append_len(&sb, "/", 1);  break;
			case 'b':  buf_append_len(&sb, "\b", 1); break;
			case 'f':  buf_append_len(&sb, "\f", 1); break;
			case 'n':  buf_append_len(&sb, "\n", 1); break;
			case 'r':  buf_append_len(&sb, "\r", 1); break;
			case 't':  buf_append_len(&sb, "\t", 1); break;
			case 'u':
				/* Unicode escape \uXXXX */
				if (p->pos + 4 <= p->len)
				{
					char hex[5] = { p->src[p->pos], p->src[p->pos+1], p->src[p->pos+2], p->src[p->pos+3], 0 };
					p->pos += 4;
					long codepoint = strtol(hex, NULL, 16);
					if (codepoint < 0x80)
					{
						char ch = (char)codepoint;
						buf_append_len(&sb, &ch, 1);
					}
					else if (codepoint < 0x800)
					{
						char ch[2] = { (char)(0xC0 | (codepoint >> 6)), (char)(0x80 | (codepoint & 0x3F)) };
						buf_append_len(&sb, ch, 2);
					}
					else
					{
						char ch[3] = { (char)(0xE0 | (codepoint >> 12)),
						               (char)(0x80 | ((codepoint >> 6) & 0x3F)),
						               (char)(0x80 | (codepoint & 0x3F)) };
						buf_append_len(&sb, ch, 3);
					}
				}
				break;
			default:
				buf_append_len(&sb, &esc, 1);
				break;
			}
		}
		else
		{
			buf_append_len(&sb, &c, 1);
		}
	}

	p->has_error = true;
	free(sb.data);
	return NULL;
}

/* Forward declarations */
static int parse_object(json_parser_t* p);
static int parse_array(json_parser_t* p);

static int parse_object(json_parser_t* p)
{
	skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '{')
	{
		p->has_error = true;
		return -1;
	}
	p->pos++; /* skip '{' */

	int map_id = x_map_alloc();
	if (map_id == -1) { p->has_error = true; return -1; }

	skip_ws(p);
	if (peek_char(p) == '}')
	{
		p->pos++; /* empty object */
		return map_id;
	}

	while (p->pos < p->len)
	{
		char* key = parse_json_string(p);
		if (key == NULL) { p->has_error = true; return map_id; }

		skip_ws(p);
		if (next_char(p) != ':')
		{
			free(key);
			p->has_error = true;
			return map_id;
		}

		skip_ws(p);
		char c = peek_char(p);
		if (c == '"')
		{
			char* val_str = parse_json_string(p);
			if (val_str != NULL)
			{
				x_map_insert_str(map_id, key, val_str);
				free(val_str);
			}
		}
		else if (c == '{')
		{
			int child_map = parse_object(p);
			x_map_insert_int(map_id, key, child_map);
		}
		else if (c == '[')
		{
			int child_list = parse_array(p);
			x_map_insert_int(map_id, key, child_list);
		}
		else if (c == 't' && strncmp(p->src + p->pos, "true", 4) == 0)
		{
			p->pos += 4;
			x_map_insert_int(map_id, key, 1);
		}
		else if (c == 'f' && strncmp(p->src + p->pos, "false", 5) == 0)
		{
			p->pos += 5;
			x_map_insert_int(map_id, key, 0);
		}
		else if (c == 'n' && strncmp(p->src + p->pos, "null", 4) == 0)
		{
			p->pos += 4;
			x_map_insert_str(map_id, key, "");
		}
		else if (c == '-' || (c >= '0' && c <= '9'))
		{
			size_t start = p->pos;
			bool is_float = false;
			if (p->src[p->pos] == '-') p->pos++;
			while (p->pos < p->len && ((p->src[p->pos] >= '0' && p->src[p->pos] <= '9') ||
			                           p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E' ||
			                           p->src[p->pos] == '+' || p->src[p->pos] == '-'))
			{
				if (p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E') is_float = true;
				p->pos++;
			}
			size_t nlen = p->pos - start;
			char nbuf[64];
			if (nlen >= sizeof(nbuf)) nlen = sizeof(nbuf) - 1;
			memcpy(nbuf, p->src + start, nlen);
			nbuf[nlen] = '\0';
			if (is_float)
			{
				x_map_insert_float(map_id, key, (float)atof(nbuf));
			}
			else
			{
				x_map_insert_int(map_id, key, atoi(nbuf));
			}
		}
		else
		{
			p->has_error = true;
			free(key);
			return map_id;
		}

		free(key);

		skip_ws(p);
		char sep = peek_char(p);
		if (sep == ',')
		{
			p->pos++;
			continue;
		}
		else if (sep == '}')
		{
			p->pos++;
			break;
		}
		else
		{
			p->has_error = true;
			break;
		}
	}

	return map_id;
}

static int parse_array(json_parser_t* p)
{
	skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '[')
	{
		p->has_error = true;
		return -1;
	}
	p->pos++; /* skip '[' */

	int list_id = x_list_alloc();
	if (list_id == -1) { p->has_error = true; return -1; }

	skip_ws(p);
	if (peek_char(p) == ']')
	{
		p->pos++; /* empty array */
		return list_id;
	}

	while (p->pos < p->len)
	{
		skip_ws(p);
		char c = peek_char(p);
		if (c == '"')
		{
			char* val_str = parse_json_string(p);
			if (val_str != NULL)
			{
				x_list_append_str(list_id, val_str);
				free(val_str);
			}
		}
		else if (c == '{')
		{
			int child_map = parse_object(p);
			x_list_append_int(list_id, child_map);
		}
		else if (c == '[')
		{
			int child_list = parse_array(p);
			x_list_append_int(list_id, child_list);
		}
		else if (c == 't' && strncmp(p->src + p->pos, "true", 4) == 0)
		{
			p->pos += 4;
			x_list_append_int(list_id, 1);
		}
		else if (c == 'f' && strncmp(p->src + p->pos, "false", 5) == 0)
		{
			p->pos += 5;
			x_list_append_int(list_id, 0);
		}
		else if (c == 'n' && strncmp(p->src + p->pos, "null", 4) == 0)
		{
			p->pos += 4;
			x_list_append_str(list_id, "");
		}
		else if (c == '-' || (c >= '0' && c <= '9'))
		{
			size_t start = p->pos;
			bool is_float = false;
			if (p->src[p->pos] == '-') p->pos++;
			while (p->pos < p->len && ((p->src[p->pos] >= '0' && p->src[p->pos] <= '9') ||
			                           p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E' ||
			                           p->src[p->pos] == '+' || p->src[p->pos] == '-'))
			{
				if (p->src[p->pos] == '.' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E') is_float = true;
				p->pos++;
			}
			size_t nlen = p->pos - start;
			char nbuf[64];
			if (nlen >= sizeof(nbuf)) nlen = sizeof(nbuf) - 1;
			memcpy(nbuf, p->src + start, nlen);
			nbuf[nlen] = '\0';
			if (is_float)
			{
				x_list_append_float(list_id, (float)atof(nbuf));
			}
			else
			{
				x_list_append_int(list_id, atoi(nbuf));
			}
		}
		else
		{
			p->has_error = true;
			return list_id;
		}

		skip_ws(p);
		char sep = peek_char(p);
		if (sep == ',')
		{
			p->pos++;
			continue;
		}
		else if (sep == ']')
		{
			p->pos++;
			break;
		}
		else
		{
			p->has_error = true;
			break;
		}
	}

	return list_id;
}

/* ------------------------------------------------------------------------- */
/* JSON Stringifier                                                          */
/* ------------------------------------------------------------------------- */

static void stringify_map(str_buf_t* sb, int map_id);
static void stringify_list(str_buf_t* sb, int list_id);

static void stringify_map(str_buf_t* sb, int map_id)
{
	buf_append_len(sb, "{", 1);
	char** keys = NULL;
	int count = x_map_get_all_keys(map_id, &keys);
	for (int i = 0; i < count; i++)
	{
		if (i > 0) buf_append_len(sb, ", ", 2);
		buf_append_escaped_str(sb, keys[i]);
		buf_append_len(sb, ": ", 2);

		int t = x_map_fetch_type(map_id, keys[i]);
		if (t == 1) /* int */
		{
			char numbuf[32];
			snprintf(numbuf, sizeof(numbuf), "%d", x_map_fetch_int(map_id, keys[i]));
			buf_append(sb, numbuf);
		}
		else if (t == 2) /* float */
		{
			char numbuf[32];
			snprintf(numbuf, sizeof(numbuf), "%g", x_map_fetch_float(map_id, keys[i]));
			buf_append(sb, numbuf);
		}
		else /* str */
		{
			const char* s = x_map_fetch_str(map_id, keys[i]);
			buf_append_escaped_str(sb, s);
		}
	}
	if (keys != NULL) free(keys);
	buf_append_len(sb, "}", 1);
}

static void stringify_list(str_buf_t* sb, int list_id)
{
	buf_append_len(sb, "[", 1);
	int count = x_list_count(list_id);
	for (int i = 0; i < count; i++)
	{
		if (i > 0) buf_append_len(sb, ", ", 2);
		int t = x_list_item_type(list_id, i);
		if (t == 1) /* int */
		{
			char numbuf[32];
			snprintf(numbuf, sizeof(numbuf), "%d", x_list_item_int(list_id, i));
			buf_append(sb, numbuf);
		}
		else if (t == 2) /* float */
		{
			char numbuf[32];
			snprintf(numbuf, sizeof(numbuf), "%g", x_list_item_float(list_id, i));
			buf_append(sb, numbuf);
		}
		else /* str */
		{
			const char* s = x_list_item_str(list_id, i);
			buf_append_escaped_str(sb, s);
		}
	}
	buf_append_len(sb, "]", 1);
}

/* ------------------------------------------------------------------------- */
/* Module Entry Points                                                       */
/* ------------------------------------------------------------------------- */

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

void x_json_parse(fcall* fc)
{
	const char* json_str = get_str_arg(fc, 0, "");
	if (json_str == NULL || json_str[0] == '\0')
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
		return;
	}

	json_parser_t p;
	p.src = json_str;
	p.pos = 0;
	p.len = strlen(json_str);
	p.has_error = false;

	skip_ws(&p);
	char c = peek_char(&p);
	if (c == '{')
	{
		int map_id = parse_object(&p);
		type_def* map_td = get_type_by_name("Map");
		if (map_td != NULL && !is_base_type(map_td))
		{
			type_instance* inst = (type_instance*)install_memory_with_type(map_td, 1);
			var* id_prop = get_var_by_name_on_stack("id", &inst->propertys);
			if (id_prop != NULL && id_prop->value_int != NULL)
				*id_prop->value_int = map_id;
			fc->_return.type_define = map_td;
			fc->_return.values = inst;
			fc->_return.value_type_instsance = inst;
			fc->_return.size = 1;
		}
		else
		{
			fc->_return.value_int = new_int(1, map_id);
			fc->_return.type_define = T_INT;
		}
	}
	else if (c == '[')
	{
		int list_id = parse_array(&p);
		type_def* list_td = get_type_by_name("List");
		if (list_td != NULL && !is_base_type(list_td))
		{
			type_instance* inst = (type_instance*)install_memory_with_type(list_td, 1);
			var* id_prop = get_var_by_name_on_stack("id", &inst->propertys);
			if (id_prop != NULL && id_prop->value_int != NULL)
				*id_prop->value_int = list_id;
			fc->_return.type_define = list_td;
			fc->_return.values = inst;
			fc->_return.value_type_instsance = inst;
			fc->_return.size = 1;
		}
		else
		{
			fc->_return.value_int = new_int(1, list_id);
			fc->_return.type_define = T_INT;
		}
	}
	else
	{
		fc->_return.value_int = new_int(1, -1);
		fc->_return.type_define = T_INT;
	}
}

void x_json_stringify(fcall* fc)
{
	str_buf_t sb;
	buf_init(&sb);

	if (fc != NULL && fc->parm_count_c > 0)
	{
		var* arg = &fc->func_parmeters[0];
		if (arg != NULL)
		{
			/* Check if argument is a Map or List object */
			if (arg->value_type_instsance != NULL)
			{
				var* id_prop = get_var_by_name_on_stack("id", &arg->value_type_instsance->propertys);
				if (id_prop != NULL && id_prop->value_int != NULL)
				{
					int id = *id_prop->value_int;
					if (arg->type_define != NULL && strcmp(arg->type_define->type_name, "List") == 0)
					{
						stringify_list(&sb, id);
					}
					else
					{
						stringify_map(&sb, id);
					}
				}
				else
				{
					buf_append(&sb, "{}");
				}
			}
			else if (arg->type_define == T_INT && arg->value_int != NULL)
			{
				int id = *arg->value_int;
				/* Check if it refers to a valid map or list id */
				if (x_map_count(id) > 0 || x_map_contains_key(id, ""))
				{
					stringify_map(&sb, id);
				}
				else if (x_list_count(id) > 0)
				{
					stringify_list(&sb, id);
				}
				else
				{
					char numbuf[32];
					snprintf(numbuf, sizeof(numbuf), "%d", id);
					buf_append(&sb, numbuf);
				}
			}
			else if (arg->type_define == T_STRING)
			{
				const char* s = (arg->value_str_ptr && *arg->value_str_ptr) ? *arg->value_str_ptr :
				                (arg->value_char_ptr ? arg->value_char_ptr : "");
				buf_append_escaped_str(&sb, s);
			}
			else if (arg->type_define == T_FLOAT && arg->value_float != NULL)
			{
				char numbuf[32];
				snprintf(numbuf, sizeof(numbuf), "%g", *arg->value_float);
				buf_append(&sb, numbuf);
			}
			else if (arg->type_define == T_BOOL && arg->value_bool != NULL)
			{
				buf_append(&sb, *arg->value_bool ? "true" : "false");
			}
			else
			{
				buf_append(&sb, "null");
			}
		}
	}

	char* result = (char*)gc_malloc(sb.len + 1, GC_KIND_STRING);
	if (result != NULL)
	{
		memcpy(result, sb.data, sb.len + 1);
	}
	else
	{
		result = (char*)calloc(1, 1);
	}
	free(sb.data);

	fc->_return.value_str_ptr = get_pptr_string(result);
	fc->_return.type_define = T_STRING;
}

void x_json_is_valid(fcall* fc)
{
	const char* json_str = get_str_arg(fc, 0, "");
	int valid = 0;
	if (json_str != NULL && json_str[0] != '\0')
	{
		json_parser_t p;
		p.src = json_str;
		p.pos = 0;
		p.len = strlen(json_str);
		p.has_error = false;

		skip_ws(&p);
		char c = peek_char(&p);
		if (c == '{')
		{
			parse_object(&p);
			skip_ws(&p);
			if (!p.has_error && p.pos == p.len) valid = 1;
		}
		else if (c == '[')
		{
			parse_array(&p);
			skip_ws(&p);
			if (!p.has_error && p.pos == p.len) valid = 1;
		}
	}
	fc->_return.value_int = new_int(1, valid);
	fc->_return.type_define = T_INT;
}
