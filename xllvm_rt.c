#include "xllvm_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include "pcre.h"

#define MAX_RT_MAPS 1024
#define MAX_RT_LISTS 1024

typedef struct {
	char* key;
	char* val_str;
	int val_int;
	double val_float;
	int type; /* 1=str, 2=int, 3=float */
} RtMapEntry;

typedef struct {
	bool active;
	RtMapEntry* entries;
	int count;
	int capacity;
} RtMap;

static RtMap g_maps[MAX_RT_MAPS];

typedef struct {
	char* val_str;
	int val_int;
	double val_float;
	int type; /* 1=str, 2=int, 3=float */
} RtListItem;

typedef struct {
	bool active;
	RtListItem* items;
	int count;
	int capacity;
} RtList;

static RtList g_lists[MAX_RT_LISTS];

static int g_rt_argc = 0;
static char** g_rt_argv = NULL;

void xllvm_rt_init(int argc, char** argv)
{
	g_rt_argc = argc;
	g_rt_argv = argv;
	memset(g_maps, 0, sizeof(g_maps));
	memset(g_lists, 0, sizeof(g_lists));
}

/* -------------------------------------------------------------------------
 * String Helpers
 * ------------------------------------------------------------------------- */
char* _str_concat(const char* s1, const char* s2)
{
	if (!s1) s1 = "";
	if (!s2) s2 = "";
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	char* res = (char*)malloc(l1 + l2 + 1);
	if (!res) return strdup("");
	memcpy(res, s1, l1);
	memcpy(res + l1, s2, l2);
	res[l1 + l2] = '\0';
	return res;
}

char* _str_from_int(int val)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%d", val);
	return strdup(buf);
}

char* _str_from_float(double val)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%g", val);
	return strdup(buf);
}

int _len(const char* s)
{
	return s ? (int)strlen(s) : 0;
}

char* _trim(const char* s)
{
	if (!s) return strdup("");
	while (*s && isspace((unsigned char)*s)) s++;
	char* copy = strdup(s);
	size_t l = strlen(copy);
	while (l > 0 && isspace((unsigned char)copy[l - 1])) {
		copy[--l] = '\0';
	}
	return copy;
}

char* _lower(const char* s)
{
	if (!s) return strdup("");
	char* copy = strdup(s);
	for (char* p = copy; *p; p++) *p = (char)tolower((unsigned char)*p);
	return copy;
}

char* _upper(const char* s)
{
	if (!s) return strdup("");
	char* copy = strdup(s);
	for (char* p = copy; *p; p++) *p = (char)toupper((unsigned char)*p);
	return copy;
}

char* substr(const char* s, int start, int len)
{
	if (!s) return strdup("");
	int slen = (int)strlen(s);
	if (start < 0) start = 0;
	if (start >= slen || len <= 0) return strdup("");
	if (start + len > slen) len = slen - start;
	char* buf = (char*)malloc(len + 1);
	if (!buf) return strdup("");
	memcpy(buf, s + start, len);
	buf[len] = '\0';
	return buf;
}

char* chr(int code)
{
	char* buf = (char*)malloc(2);
	if (!buf) return strdup("");
	buf[0] = (char)code;
	buf[1] = '\0';
	return buf;
}

char* x_readline(void)
{
	char buf[4096];
	if (fgets(buf, sizeof(buf), stdin) != NULL)
	{
		size_t len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
		if (len > 1 && buf[len - 2] == '\r') buf[len - 2] = '\0';
		return strdup(buf);
	}
	return strdup("");
}

int index_of(const char* s, const char* needle)
{
	if (!s || !needle) return -1;
	const char* p = strstr(s, needle);
	return p ? (int)(p - s) : -1;
}

int str_eq(const char* s1, const char* s2)
{
	if (s1 == s2) return 1;
	if (!s1 || !s2) return 0;
	return strcmp(s1, s2) == 0 ? 1 : 0;
}

int starts_with(const char* s, const char* prefix)
{
	if (!s || !prefix) return 0;
	size_t len_p = strlen(prefix);
	return strncmp(s, prefix, len_p) == 0 ? 1 : 0;
}

int ends_with(const char* s, const char* suffix)
{
	if (!s || !suffix) return 0;
	size_t len_s = strlen(s);
	size_t len_suf = strlen(suffix);
	if (len_s < len_suf) return 0;
	return strcmp(s + (len_s - len_suf), suffix) == 0 ? 1 : 0;
}

int regex_match(const char* s, const char* pattern)
{
	if (!s || !pattern) return 0;
	const char* err = NULL;
	int erroff = 0;
	pcre* re = pcre_compile(pattern, 0, &err, &erroff, NULL);
	if (!re) return 0;
	int ovector[30];
	int rc = pcre_exec(re, NULL, s, (int)strlen(s), 0, 0, ovector, 30);
	pcre_free(re);
	return (rc >= 0) ? 1 : 0;
}

char* regex_find(const char* s, const char* pattern)
{
	if (!s || !pattern) return strdup("");
	const char* err = NULL;
	int erroff = 0;
	pcre* re = pcre_compile(pattern, 0, &err, &erroff, NULL);
	if (!re) return strdup("");
	int ovector[30];
	int rc = pcre_exec(re, NULL, s, (int)strlen(s), 0, 0, ovector, 30);
	if (rc >= 0)
	{
		int match_len = ovector[1] - ovector[0];
		char* res = (char*)malloc(match_len + 1);
		if (res)
		{
			memcpy(res, s + ovector[0], match_len);
			res[match_len] = '\0';
			pcre_free(re);
			return res;
		}
	}
	pcre_free(re);
	return strdup("");
}

char* regex_replace(const char* s, const char* pattern, const char* repl)
{
	if (!s || !pattern || !repl) return strdup(s ? s : "");
	const char* err = NULL;
	int erroff = 0;
	pcre* re = pcre_compile(pattern, 0, &err, &erroff, NULL);
	if (!re) return strdup(s);

	int str_len = (int)strlen(s);
	int ovector[30];
	int offset = 0;
	size_t cap = str_len + 64;
	char* result = (char*)malloc(cap);
	if (!result) { pcre_free(re); return strdup(s); }
	int res_len = 0;
	result[0] = '\0';

	while (offset < str_len)
	{
		int rc = pcre_exec(re, NULL, s, str_len, offset, 0, ovector, 30);
		if (rc < 0)
		{
			int rem = str_len - offset;
			if (res_len + rem + 1 > (int)cap)
			{
				cap = res_len + rem + 64;
				result = (char*)realloc(result, cap);
			}
			memcpy(result + res_len, s + offset, rem);
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
		memcpy(result + res_len, s + offset, prefix_len);
		res_len += prefix_len;
		memcpy(result + res_len, repl, repl_len);
		res_len += repl_len;
		result[res_len] = '\0';

		offset = ovector[1];
		if (ovector[0] == ovector[1])
			offset++;
	}
	pcre_free(re);
	return result;
}

/* -------------------------------------------------------------------------
 * Map Operations
 * ------------------------------------------------------------------------- */
int map_new(void)
{
	for (int i = 1; i < MAX_RT_MAPS; i++)
	{
		if (!g_maps[i].active)
		{
			g_maps[i].active = true;
			g_maps[i].count = 0;
			g_maps[i].capacity = 16;
			g_maps[i].entries = (RtMapEntry*)calloc(g_maps[i].capacity, sizeof(RtMapEntry));
			return i;
		}
	}
	return 0;
}

int map_put(int id, const char* key, const char* val)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return -1;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].val_str) free(m->entries[i].val_str);
			m->entries[i].val_str = strdup(val ? val : "");
			m->entries[i].type = 1;
			return 0;
		}
	}
	if (m->count >= m->capacity)
	{
		int ncap = m->capacity ? m->capacity * 2 : 16;
		m->entries = (RtMapEntry*)realloc(m->entries, ncap * sizeof(RtMapEntry));
		m->capacity = ncap;
	}
	m->entries[m->count].key = strdup(key);
	m->entries[m->count].val_str = strdup(val ? val : "");
	m->entries[m->count].val_int = 0;
	m->entries[m->count].val_float = 0.0;
	m->entries[m->count].type = 1;
	m->count++;
	return 0;
}

int map_put_int(int id, const char* key, int val)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return -1;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].val_str) { free(m->entries[i].val_str); m->entries[i].val_str = NULL; }
			m->entries[i].val_int = val;
			m->entries[i].type = 2;
			return 0;
		}
	}
	if (m->count >= m->capacity)
	{
		int ncap = m->capacity ? m->capacity * 2 : 16;
		m->entries = (RtMapEntry*)realloc(m->entries, ncap * sizeof(RtMapEntry));
		m->capacity = ncap;
	}
	m->entries[m->count].key = strdup(key);
	m->entries[m->count].val_str = NULL;
	m->entries[m->count].val_int = val;
	m->entries[m->count].val_float = 0.0;
	m->entries[m->count].type = 2;
	m->count++;
	return 0;
}

int map_put_float(int id, const char* key, double val)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return -1;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].val_str) { free(m->entries[i].val_str); m->entries[i].val_str = NULL; }
			m->entries[i].val_float = val;
			m->entries[i].type = 3;
			return 0;
		}
	}
	if (m->count >= m->capacity)
	{
		int ncap = m->capacity ? m->capacity * 2 : 16;
		m->entries = (RtMapEntry*)realloc(m->entries, ncap * sizeof(RtMapEntry));
		m->capacity = ncap;
	}
	m->entries[m->count].key = strdup(key);
	m->entries[m->count].val_str = NULL;
	m->entries[m->count].val_int = 0;
	m->entries[m->count].val_float = val;
	m->entries[m->count].type = 3;
	m->count++;
	return 0;
}

const char* map_get(int id, const char* key)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return "";
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].type == 1 && m->entries[i].val_str)
				return m->entries[i].val_str;
			if (m->entries[i].type == 2)
			{
				static char buf[32];
				snprintf(buf, sizeof(buf), "%d", m->entries[i].val_int);
				return buf;
			}
			return "";
		}
	}
	return "";
}

int map_get_int(int id, const char* key)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return 0;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].type == 2) return m->entries[i].val_int;
			if (m->entries[i].type == 1 && m->entries[i].val_str) return atoi(m->entries[i].val_str);
			return 0;
		}
	}
	return 0;
}

double map_get_float(int id, const char* key)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return 0.0;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].type == 3) return m->entries[i].val_float;
			if (m->entries[i].type == 1 && m->entries[i].val_str) return atof(m->entries[i].val_str);
			return 0.0;
		}
	}
	return 0.0;
}

int map_has(int id, const char* key)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return 0;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0) return 1;
	}
	return 0;
}

int map_remove(int id, const char* key)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active || !key) return 0;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (strcmp(m->entries[i].key, key) == 0)
		{
			if (m->entries[i].key) free(m->entries[i].key);
			if (m->entries[i].val_str) free(m->entries[i].val_str);
			m->entries[i] = m->entries[m->count - 1];
			m->count--;
			return 1;
		}
	}
	return 0;
}

int map_size(int id)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active) return 0;
	return g_maps[id].count;
}

int map_clear(int id)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active) return 0;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		if (m->entries[i].key) free(m->entries[i].key);
		if (m->entries[i].val_str) free(m->entries[i].val_str);
	}
	m->count = 0;
	return 0;
}

char* map_keys(int id)
{
	(void)id;
	return strdup("");
}

char* map_values(int id)
{
	(void)id;
	return strdup("");
}

char* map_to_string(int id)
{
	(void)id;
	return strdup("{}");
}

int map_free(int id)
{
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active) return 0;
	map_clear(id);
	if (g_maps[id].entries) free(g_maps[id].entries);
	g_maps[id].entries = NULL;
	g_maps[id].capacity = 0;
	g_maps[id].active = false;
	return 0;
}

int map_keys_list(int id)
{
	int lid = list_new();
	if (id <= 0 || id >= MAX_RT_MAPS || !g_maps[id].active) return lid;
	RtMap* m = &g_maps[id];
	for (int i = 0; i < m->count; i++)
	{
		list_add(lid, m->entries[i].key);
	}
	return lid;
}

/* -------------------------------------------------------------------------
 * List Operations
 * ------------------------------------------------------------------------- */
int list_new(void)
{
	for (int i = 1; i < MAX_RT_LISTS; i++)
	{
		if (!g_lists[i].active)
		{
			g_lists[i].active = true;
			g_lists[i].count = 0;
			g_lists[i].capacity = 16;
			g_lists[i].items = (RtListItem*)calloc(g_lists[i].capacity, sizeof(RtListItem));
			return i;
		}
	}
	return 0;
}

int list_add(int id, const char* val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (l->count >= l->capacity)
	{
		int ncap = l->capacity ? l->capacity * 2 : 16;
		l->items = (RtListItem*)realloc(l->items, ncap * sizeof(RtListItem));
		l->capacity = ncap;
	}
	l->items[l->count].val_str = strdup(val ? val : "");
	l->items[l->count].val_int = 0;
	l->items[l->count].val_float = 0.0;
	l->items[l->count].type = 1;
	l->count++;
	return 0;
}

int list_add_int(int id, int val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (l->count >= l->capacity)
	{
		int ncap = l->capacity ? l->capacity * 2 : 16;
		l->items = (RtListItem*)realloc(l->items, ncap * sizeof(RtListItem));
		l->capacity = ncap;
	}
	l->items[l->count].val_str = NULL;
	l->items[l->count].val_int = val;
	l->items[l->count].val_float = 0.0;
	l->items[l->count].type = 2;
	l->count++;
	return 0;
}

int list_add_float(int id, double val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (l->count >= l->capacity)
	{
		int ncap = l->capacity ? l->capacity * 2 : 16;
		l->items = (RtListItem*)realloc(l->items, ncap * sizeof(RtListItem));
		l->capacity = ncap;
	}
	l->items[l->count].val_str = NULL;
	l->items[l->count].val_int = 0;
	l->items[l->count].val_float = val;
	l->items[l->count].type = 3;
	l->count++;
	return 0;
}

const char* list_get(int id, int index)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return "";
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return "";
	if (l->items[index].type == 1 && l->items[index].val_str)
		return l->items[index].val_str;
	if (l->items[index].type == 2)
	{
		static char buf[32];
		snprintf(buf, sizeof(buf), "%d", l->items[index].val_int);
		return buf;
	}
	return "";
}

int list_get_int(int id, int index)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return 0;
	if (l->items[index].type == 2) return l->items[index].val_int;
	if (l->items[index].type == 1 && l->items[index].val_str) return atoi(l->items[index].val_str);
	return 0;
}

double list_get_float(int id, int index)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0.0;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return 0.0;
	if (l->items[index].type == 3) return l->items[index].val_float;
	if (l->items[index].type == 1 && l->items[index].val_str) return atof(l->items[index].val_str);
	return 0.0;
}

int list_set(int id, int index, const char* val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return -1;
	if (l->items[index].val_str) free(l->items[index].val_str);
	l->items[index].val_str = strdup(val ? val : "");
	l->items[index].type = 1;
	return 0;
}

int list_set_int(int id, int index, int val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return -1;
	if (l->items[index].val_str) { free(l->items[index].val_str); l->items[index].val_str = NULL; }
	l->items[index].val_int = val;
	l->items[index].type = 2;
	return 0;
}

int list_set_float(int id, int index, double val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return -1;
	if (l->items[index].val_str) { free(l->items[index].val_str); l->items[index].val_str = NULL; }
	l->items[index].val_float = val;
	l->items[index].type = 3;
	return 0;
}

int list_remove_at(int id, int index)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	if (index < 0 || index >= l->count) return -1;
	if (l->items[index].val_str) free(l->items[index].val_str);
	for (int i = index; i < l->count - 1; i++)
	{
		l->items[i] = l->items[i + 1];
	}
	l->count--;
	return 0;
}

int list_size(int id)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	return g_lists[id].count;
}

int list_clear(int id)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	RtList* l = &g_lists[id];
	for (int i = 0; i < l->count; i++)
	{
		if (l->items[i].val_str) free(l->items[i].val_str);
	}
	l->count = 0;
	return 0;
}

int list_contains(int id, const char* val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active || !val) return 0;
	RtList* l = &g_lists[id];
	for (int i = 0; i < l->count; i++)
	{
		if (l->items[i].type == 1 && l->items[i].val_str && strcmp(l->items[i].val_str, val) == 0)
			return 1;
	}
	return 0;
}

int list_contains_int(int id, int val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	RtList* l = &g_lists[id];
	for (int i = 0; i < l->count; i++)
	{
		if (l->items[i].type == 2 && l->items[i].val_int == val)
			return 1;
	}
	return 0;
}

int list_index_of(int id, const char* val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active || !val) return -1;
	RtList* l = &g_lists[id];
	for (int i = 0; i < l->count; i++)
	{
		if (l->items[i].type == 1 && l->items[i].val_str && strcmp(l->items[i].val_str, val) == 0)
			return i;
	}
	return -1;
}

int list_index_of_int(int id, int val)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return -1;
	RtList* l = &g_lists[id];
	for (int i = 0; i < l->count; i++)
	{
		if (l->items[i].type == 2 && l->items[i].val_int == val)
			return i;
	}
	return -1;
}

char* list_pop(int id)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return strdup("");
	RtList* l = &g_lists[id];
	if (l->count <= 0) return strdup("");
	l->count--;
	if (l->items[l->count].type == 1 && l->items[l->count].val_str)
	{
		char* res = l->items[l->count].val_str;
		l->items[l->count].val_str = NULL;
		return res;
	}
	return strdup("");
}

int list_pop_int(int id)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	RtList* l = &g_lists[id];
	if (l->count <= 0) return 0;
	l->count--;
	return l->items[l->count].val_int;
}

char* list_join(int id, const char* sep)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return strdup("");
	RtList* l = &g_lists[id];
	if (l->count <= 0) return strdup("");
	if (!sep) sep = "";

	size_t total_len = 0;
	size_t sep_len = strlen(sep);
	for (int i = 0; i < l->count; i++)
	{
		if (i > 0) total_len += sep_len;
		if (l->items[i].type == 1 && l->items[i].val_str)
			total_len += strlen(l->items[i].val_str);
		else
			total_len += 16;
	}

	char* res = (char*)malloc(total_len + 1);
	if (!res) return strdup("");
	res[0] = '\0';
	for (int i = 0; i < l->count; i++)
	{
		if (i > 0) strcat(res, sep);
		if (l->items[i].type == 1 && l->items[i].val_str)
			strcat(res, l->items[i].val_str);
		else if (l->items[i].type == 2)
		{
			char num[32];
			snprintf(num, sizeof(num), "%d", l->items[i].val_int);
			strcat(res, num);
		}
	}
	return res;
}

char* list_to_string(int id)
{
	return list_join(id, ", ");
}

int list_free(int id)
{
	if (id <= 0 || id >= MAX_RT_LISTS || !g_lists[id].active) return 0;
	list_clear(id);
	if (g_lists[id].items) free(g_lists[id].items);
	g_lists[id].items = NULL;
	g_lists[id].capacity = 0;
	g_lists[id].active = false;
	return 0;
}

int str_split(const char* s, const char* delim)
{
	int lid = list_new();
	if (!s) return lid;
	size_t dlen = delim ? strlen(delim) : 0;
	if (dlen == 0)
	{
		char single[2] = {0, 0};
		for (const char* p = s; *p != '\0'; p++)
		{
			single[0] = *p;
			list_add(lid, single);
		}
		return lid;
	}
	const char* cur = s;
	const char* found = strstr(cur, delim);
	while (found != NULL)
	{
		size_t part_len = (size_t)(found - cur);
		char* part = (char*)malloc(part_len + 1);
		if (part)
		{
			memcpy(part, cur, part_len);
			part[part_len] = '\0';
			list_add(lid, part);
			free(part);
		}
		cur = found + dlen;
		found = strstr(cur, delim);
	}
	list_add(lid, cur);
	return lid;
}

/* -------------------------------------------------------------------------
 * Socket Operations
 * ------------------------------------------------------------------------- */
int socket_create(const char* type)
{
	int sock_type = (type && strcmp(type, "udp") == 0) ? SOCK_DGRAM : SOCK_STREAM;
	int fd = socket(AF_INET, sock_type, 0);
	return fd;
}

int socket_connect(int fd, const char* host, int port)
{
	if (fd < 0 || host == NULL || port <= 0) return -1;
	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port);

	struct addrinfo hints, *res = NULL, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, port_str, &hints, &res) != 0 || res == NULL)
	{
		return -1;
	}

	int ret = -1;
	for (p = res; p != NULL; p = p->ai_next)
	{
		if (connect(fd, p->ai_addr, (socklen_t)p->ai_addrlen) == 0)
		{
			ret = 0;
			break;
		}
	}
	freeaddrinfo(res);
	return ret;
}

int socket_bind(int fd, const char* host, int port)
{
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((uint16_t)port);
	if (host == NULL || strcmp(host, "0.0.0.0") == 0 || strcmp(host, "") == 0)
		sin.sin_addr.s_addr = INADDR_ANY;
	else
		inet_pton(AF_INET, host, &sin.sin_addr);
	return bind(fd, (struct sockaddr*)&sin, sizeof(sin));
}

int socket_listen(int fd, int backlog)
{
	return listen(fd, backlog > 0 ? backlog : 32);
}

int socket_accept(int fd)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	return accept(fd, (struct sockaddr*)&client_addr, &client_len);
}

int socket_send(int fd, const char* data)
{
	if (!data) return 0;
	return (int)send(fd, data, strlen(data), 0);
}

char* socket_recv(int fd, int max_bytes)
{
	if (max_bytes <= 0) max_bytes = 4096;
	char* buf = (char*)malloc(max_bytes + 1);
	if (!buf) return strdup("");
	ssize_t n = recv(fd, buf, max_bytes, 0);
	if (n < 0)
	{
		free(buf);
		return strdup("");
	}
	buf[n] = '\0';
	return buf;
}

int socket_close(int fd)
{
	return close(fd);
}

int socket_set_timeout(int fd, int sec)
{
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
}

int socket_set_reuseaddr(int fd, int enable)
{
	int opt = enable ? 1 : 0;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int socket_sendto(int fd, const char* data, const char* host, int port)
{
	if (!data) return 0;
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((uint16_t)port);
	inet_pton(AF_INET, host, &sin.sin_addr);
	return (int)sendto(fd, data, strlen(data), 0, (struct sockaddr*)&sin, sizeof(sin));
}

char* socket_recvfrom(int fd, int max_bytes)
{
	if (max_bytes <= 0) max_bytes = 4096;
	char* buf = (char*)malloc(max_bytes + 1);
	if (!buf) return strdup("");
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	ssize_t n = recvfrom(fd, buf, max_bytes, 0, (struct sockaddr*)&sin, &slen);
	if (n < 0)
	{
		free(buf);
		return strdup("");
	}
	buf[n] = '\0';
	return buf;
}

/* -------------------------------------------------------------------------
 * System Operations
 * ------------------------------------------------------------------------- */
int clock_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int get_argc(void)
{
	return g_rt_argc;
}

char* get_arg(int index)
{
	if (index >= 0 && index < g_rt_argc && g_rt_argv != NULL)
		return strdup(g_rt_argv[index]);
	return strdup("");
}

int system_exec(const char* cmd)
{
	return cmd ? system(cmd) : 0;
}

char* system_getenv(const char* name)
{
	const char* val = name ? getenv(name) : NULL;
	return val ? strdup(val) : strdup("");
}

int system_setenv(const char* name, const char* val)
{
	return (name && val) ? setenv(name, val, 1) : -1;
}

/* -------------------------------------------------------------------------
 * GC Operations (Native Stubs - Weak to allow override when linked into xlang)
 * ------------------------------------------------------------------------- */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) int gc_collect(void) { return 0; }
__attribute__((weak)) int gc_allocated_bytes(void) { return 0; }
__attribute__((weak)) int gc_total_objects(void) { return 0; }
__attribute__((weak)) int gc_enable(void) { return 1; }
__attribute__((weak)) int gc_disable(void) { return 0; }
__attribute__((weak)) int gc_set_threshold(int th) { return th; }
__attribute__((weak)) int gc_dump(void) { return 0; }
#else
int gc_collect(void) { return 0; }
int gc_allocated_bytes(void) { return 0; }
int gc_total_objects(void) { return 0; }
int gc_enable(void) { return 1; }
int gc_disable(void) { return 0; }
int gc_set_threshold(int th) { return th; }
int gc_dump(void) { return 0; }
#endif

/* -------------------------------------------------------------------------
 * Math Operations
 * ------------------------------------------------------------------------- */
double math_sqrt(double x) { return (x >= 0.0) ? sqrt(x) : 0.0; }
double math_pow(double base, double exp) { return pow(base, exp); }
double math_abs(double x) { return fabs(x); }
double math_min(double a, double b) { return fmin(a, b); }
double math_max(double a, double b) { return fmax(a, b); }
double math_floor(double x) { return floor(x); }
double math_ceil(double x) { return ceil(x); }
double math_round(double x) { return round(x); }
double math_sin(double x) { return sin(x); }
double math_cos(double x) { return cos(x); }
double math_tan(double x) { return tan(x); }
double math_log(double x) { return (x > 0.0) ? log(x) : 0.0; }

/* -------------------------------------------------------------------------
 * File Operations
 * ------------------------------------------------------------------------- */
#define MAX_RT_FILES 64
static FILE* s_rt_open_files[MAX_RT_FILES] = {0};

char* file_read_all(const char* path)
{
	if (!path || *path == '\0') return strdup("");
	FILE* f = fopen(path, "rb");
	if (!f) return strdup("");
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (sz < 0) sz = 0;
	char* buf = (char*)malloc(sz + 1);
	if (!buf) { fclose(f); return strdup(""); }
	size_t n = fread(buf, 1, sz, f);
	buf[n] = '\0';
	fclose(f);
	return buf;
}

int file_write_all(const char* path, const char* content)
{
	if (!path || *path == '\0') return -1;
	FILE* f = fopen(path, "wb");
	if (!f) return -1;
	size_t len = content ? strlen(content) : 0;
	size_t n = fwrite(content ? content : "", 1, len, f);
	fclose(f);
	return (int)n;
}

int file_append(const char* path, const char* content)
{
	if (!path || *path == '\0') return -1;
	FILE* f = fopen(path, "ab");
	if (!f) return -1;
	size_t len = content ? strlen(content) : 0;
	size_t n = fwrite(content ? content : "", 1, len, f);
	fclose(f);
	return (int)n;
}

int file_exists(const char* path)
{
	if (!path || *path == '\0') return 0;
	FILE* f = fopen(path, "rb");
	if (f) { fclose(f); return 1; }
	return 0;
}

int file_size(const char* path)
{
	if (!path || *path == '\0') return -1;
	FILE* f = fopen(path, "rb");
	if (!f) return -1;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fclose(f);
	return (int)sz;
}

int file_remove(const char* path)
{
	if (!path || *path == '\0') return -1;
	return remove(path) == 0 ? 0 : -1;
}

int file_open(const char* path, const char* mode)
{
	if (!path || *path == '\0') return -1;
	FILE* f = fopen(path, mode ? mode : "r");
	if (!f) return -1;
	for (int i = 1; i < MAX_RT_FILES; i++)
	{
		if (s_rt_open_files[i] == NULL)
		{
			s_rt_open_files[i] = f;
			return i;
		}
	}
	fclose(f);
	return -1;
}

char* file_read(int fd, int bytes)
{
	if (fd <= 0 || fd >= MAX_RT_FILES || s_rt_open_files[fd] == NULL) return strdup("");
	if (bytes <= 0) bytes = 4096;
	if (bytes > 10 * 1024 * 1024) bytes = 10 * 1024 * 1024;
	char* buf = (char*)malloc(bytes + 1);
	if (!buf) return strdup("");
	size_t n = fread(buf, 1, bytes, s_rt_open_files[fd]);
	buf[n] = '\0';
	return buf;
}

int file_write(int fd, const char* data)
{
	if (fd <= 0 || fd >= MAX_RT_FILES || s_rt_open_files[fd] == NULL || !data) return -1;
	size_t len = strlen(data);
	size_t n = fwrite(data, 1, len, s_rt_open_files[fd]);
	fflush(s_rt_open_files[fd]);
	return (int)n;
}

int file_close(int fd)
{
	if (fd <= 0 || fd >= MAX_RT_FILES || s_rt_open_files[fd] == NULL) return -1;
	int rc = fclose(s_rt_open_files[fd]);
	s_rt_open_files[fd] = NULL;
	return rc == 0 ? 0 : -1;
}

/* -------------------------------------------------------------------------
 * Directory Operations
 * ------------------------------------------------------------------------- */
typedef struct {
	int id;
} RtObjWrapper;

void* dir_list(const char* path)
{
	int lid = list_new();
	DIR* d = opendir((path && *path) ? path : ".");
	if (d)
	{
		struct dirent* entry;
		while ((entry = readdir(d)) != NULL)
		{
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			list_add(lid, entry->d_name);
		}
		closedir(d);
	}
	RtObjWrapper* w = (RtObjWrapper*)malloc(sizeof(RtObjWrapper));
	if (w) w->id = lid;
	return w;
}

int dir_create(const char* path)
{
	if (!path || *path == '\0') return -1;
	return mkdir(path, 0755) == 0 ? 0 : -1;
}

int dir_exists(const char* path)
{
	if (!path || *path == '\0') return 0;
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return 1;
	return 0;
}

int dir_remove(const char* path)
{
	if (!path || *path == '\0') return -1;
	return rmdir(path) == 0 ? 0 : -1;
}

/* -------------------------------------------------------------------------
 * Process Operations
 * ------------------------------------------------------------------------- */
typedef struct {
	char* stdout_str;
	int exit_code;
} RtProcResult;

char* proc_capture(const char* cmd)
{
	if (!cmd || *cmd == '\0') return strdup("");
	FILE* fp = popen(cmd, "r");
	if (!fp) return strdup("");
	size_t cap = 1024, len = 0;
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
		if (buf) { memcpy(buf + len, chunk, clen); len += clen; }
	}
	if (buf) buf[len] = '\0';
	pclose(fp);
	return buf ? buf : strdup("");
}

void* proc_run(const char* cmd)
{
	if (!cmd || *cmd == '\0')
	{
		RtProcResult* r = (RtProcResult*)malloc(sizeof(RtProcResult));
		if (r) { r->stdout_str = strdup(""); r->exit_code = -1; }
		return r;
	}
	FILE* fp = popen(cmd, "r");
	if (!fp)
	{
		RtProcResult* r = (RtProcResult*)malloc(sizeof(RtProcResult));
		if (r) { r->stdout_str = strdup(""); r->exit_code = -1; }
		return r;
	}
	size_t cap = 1024, len = 0;
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
		if (buf) { memcpy(buf + len, chunk, clen); len += clen; }
	}
	if (buf) buf[len] = '\0';
	int status = pclose(fp);
	int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : status;

	RtProcResult* r = (RtProcResult*)malloc(sizeof(RtProcResult));
	if (r)
	{
		r->stdout_str = buf ? buf : strdup("");
		r->exit_code = exit_code;
	}
	return r;
}

/* -------------------------------------------------------------------------
 * DateTime Operations
 * ------------------------------------------------------------------------- */
int datetime_now(void)
{
	return (int)time(NULL);
}

char* datetime_format(int ts, const char* fmt)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	if (!fmt || !*fmt) fmt = "%Y-%m-%d %H:%M:%S";
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	char buf[256];
	size_t n = strftime(buf, sizeof(buf), fmt, &tm_info);
	if (n == 0) buf[0] = '\0';
	return strdup(buf);
}

int datetime_year(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_year + 1900;
}

int datetime_month(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_mon + 1;
}

int datetime_day(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_mday;
}

int datetime_hour(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_hour;
}

int datetime_minute(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_min;
}

int datetime_second(int ts)
{
	time_t t = (ts > 0) ? (time_t)ts : time(NULL);
	struct tm tm_info;
	localtime_r(&t, &tm_info);
	return tm_info.tm_sec;
}

int datetime_clock_ms(void)
{
	return clock_ms();
}

/* -------------------------------------------------------------------------
 * JSON Operations
 * ------------------------------------------------------------------------- */
typedef struct {
	const char* src;
	size_t pos;
	size_t len;
	bool has_error;
} RtJsonParser;

static void rt_json_skip_ws(RtJsonParser* p)
{
	while (p->pos < p->len && (p->src[p->pos] == ' ' || p->src[p->pos] == '\t' ||
	                           p->src[p->pos] == '\n' || p->src[p->pos] == '\r'))
		p->pos++;
}

static char rt_json_peek(RtJsonParser* p)
{
	rt_json_skip_ws(p);
	return (p->pos < p->len) ? p->src[p->pos] : '\0';
}

static char rt_json_next(RtJsonParser* p)
{
	rt_json_skip_ws(p);
	return (p->pos < p->len) ? p->src[p->pos++] : '\0';
}

static char* rt_json_parse_string(RtJsonParser* p)
{
	rt_json_skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '"') { p->has_error = true; return NULL; }
	p->pos++;
	size_t cap = 64, len = 0;
	char* buf = (char*)malloc(cap);
	while (p->pos < p->len)
	{
		char c = p->src[p->pos++];
		if (c == '"') { buf[len] = '\0'; return buf; }
		if (c == '\\')
		{
			if (p->pos >= p->len) { p->has_error = true; free(buf); return NULL; }
			c = p->src[p->pos++];
			if (c == 'n') c = '\n';
			else if (c == 't') c = '\t';
			else if (c == 'r') c = '\r';
		}
		if (len + 2 > cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
		buf[len++] = c;
	}
	p->has_error = true;
	free(buf);
	return NULL;
}

static int rt_json_parse_value(RtJsonParser* p, int map_id, const char* key, int list_id);

static int rt_json_parse_object(RtJsonParser* p)
{
	rt_json_skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '{') { p->has_error = true; return -1; }
	p->pos++;
	int mid = map_new();
	rt_json_skip_ws(p);
	if (p->pos < p->len && p->src[p->pos] == '}') { p->pos++; return mid; }

	while (p->pos < p->len)
	{
		char* key = rt_json_parse_string(p);
		if (!key) { p->has_error = true; return mid; }
		rt_json_skip_ws(p);
		if (rt_json_next(p) != ':') { free(key); p->has_error = true; return mid; }
		rt_json_parse_value(p, mid, key, -1);
		free(key);
		rt_json_skip_ws(p);
		char c = rt_json_peek(p);
		if (c == '}') { p->pos++; break; }
		if (c == ',') { p->pos++; continue; }
		p->has_error = true;
		break;
	}
	return mid;
}

static int rt_json_parse_array(RtJsonParser* p)
{
	rt_json_skip_ws(p);
	if (p->pos >= p->len || p->src[p->pos] != '[') { p->has_error = true; return -1; }
	p->pos++;
	int lid = list_new();
	rt_json_skip_ws(p);
	if (p->pos < p->len && p->src[p->pos] == ']') { p->pos++; return lid; }

	while (p->pos < p->len)
	{
		rt_json_parse_value(p, -1, NULL, lid);
		rt_json_skip_ws(p);
		char c = rt_json_peek(p);
		if (c == ']') { p->pos++; break; }
		if (c == ',') { p->pos++; continue; }
		p->has_error = true;
		break;
	}
	return lid;
}

static int rt_json_parse_value(RtJsonParser* p, int map_id, const char* key, int list_id)
{
	rt_json_skip_ws(p);
	char c = rt_json_peek(p);
	if (c == '"')
	{
		char* str = rt_json_parse_string(p);
		if (str)
		{
			if (map_id >= 0 && key) map_put(map_id, key, str);
			else if (list_id >= 0) list_add(list_id, str);
			free(str);
		}
		return 1;
	}
	if (c == '{')
	{
		int sub_m = rt_json_parse_object(p);
		if (map_id >= 0 && key) map_put_int(map_id, key, sub_m);
		else if (list_id >= 0) list_add_int(list_id, sub_m);
		return 1;
	}
	if (c == '[')
	{
		int sub_l = rt_json_parse_array(p);
		if (map_id >= 0 && key) map_put_int(map_id, key, sub_l);
		else if (list_id >= 0) list_add_int(list_id, sub_l);
		return 1;
	}
	if (c == 't' || c == 'f')
	{
		bool is_true = (c == 't');
		p->pos += is_true ? 4 : 5;
		if (map_id >= 0 && key) map_put(map_id, key, is_true ? "true" : "false");
		else if (list_id >= 0) list_add_int(list_id, is_true ? 1 : 0);
		return 1;
	}
	if (c == 'n')
	{
		p->pos += 4;
		if (map_id >= 0 && key) map_put(map_id, key, "");
		else if (list_id >= 0) list_add(list_id, "");
		return 1;
	}
	if (isdigit((unsigned char)c) || c == '-')
	{
		const char* start = p->src + p->pos;
		bool is_flt = false;
		while (p->pos < p->len && (isdigit((unsigned char)p->src[p->pos]) || p->src[p->pos] == '.' || p->src[p->pos] == '-' || p->src[p->pos] == 'e' || p->src[p->pos] == 'E' || p->src[p->pos] == '+'))
		{
			if (p->src[p->pos] == '.') is_flt = true;
			p->pos++;
		}
		char nbuf[64] = {0};
		size_t nlen = (p->src + p->pos) - start;
		if (nlen < sizeof(nbuf)) strncpy(nbuf, start, nlen);
		if (is_flt)
		{
			double fv = atof(nbuf);
			if (map_id >= 0 && key) { map_put_float(map_id, key, fv); map_put(map_id, key, nbuf); }
			else if (list_id >= 0) list_add_float(list_id, fv);
		}
		else
		{
			int iv = atoi(nbuf);
			if (map_id >= 0 && key) { map_put_int(map_id, key, iv); map_put(map_id, key, nbuf); }
			else if (list_id >= 0) list_add_int(list_id, iv);
		}
		return 1;
	}
	p->has_error = true;
	return 0;
}

int json_is_valid(const char* str)
{
	if (!str || *str == '\0') return 0;
	RtJsonParser p;
	p.src = str;
	p.pos = 0;
	p.len = strlen(str);
	p.has_error = false;
	rt_json_skip_ws(&p);
	char c = rt_json_peek(&p);
	if (c == '{')
	{
		rt_json_parse_object(&p);
		rt_json_skip_ws(&p);
		return (!p.has_error && p.pos == p.len) ? 1 : 0;
	}
	else if (c == '[')
	{
		rt_json_parse_array(&p);
		rt_json_skip_ws(&p);
		return (!p.has_error && p.pos == p.len) ? 1 : 0;
	}
	return 0;
}

void* json_parse(const char* str)
{
	if (!str || *str == '\0') return NULL;
	RtJsonParser p;
	p.src = str;
	p.pos = 0;
	p.len = strlen(str);
	p.has_error = false;
	rt_json_skip_ws(&p);
	int mid = 0;
	if (rt_json_peek(&p) == '{')
		mid = rt_json_parse_object(&p);
	else if (rt_json_peek(&p) == '[')
		mid = rt_json_parse_array(&p);
	RtObjWrapper* w = (RtObjWrapper*)malloc(sizeof(RtObjWrapper));
	if (w) w->id = mid;
	return w;
}

char* json_stringify(void* m)
{
	if (!m) return strdup("{}");
	int mid = ((uintptr_t)m < MAX_RT_MAPS) ? (int)(uintptr_t)m : *(int*)m;
	if (mid <= 0 || mid >= MAX_RT_MAPS || !g_maps[mid].active) return strdup("{}");
	RtMap* map = &g_maps[mid];

	size_t cap = 256;
	char* buf = (char*)malloc(cap);
	if (!buf) return strdup("{}");
	strcpy(buf, "{");
	size_t len = 1;

	for (int i = 0; i < map->count; i++)
	{
		char entry_buf[512];
		if (map->entries[i].type == 2)
			snprintf(entry_buf, sizeof(entry_buf), "\"%s\": %d", map->entries[i].key, map->entries[i].val_int);
		else if (map->entries[i].type == 3)
			snprintf(entry_buf, sizeof(entry_buf), "\"%s\": %g", map->entries[i].key, map->entries[i].val_float);
		else
			snprintf(entry_buf, sizeof(entry_buf), "\"%s\": \"%s\"", map->entries[i].key, map->entries[i].val_str ? map->entries[i].val_str : "");

		size_t elen = strlen(entry_buf);
		if (len + elen + 4 > cap)
		{
			cap = (len + elen + 4) * 2;
			buf = (char*)realloc(buf, cap);
		}
		if (i > 0) { strcat(buf, ", "); len += 2; }
		strcat(buf, entry_buf);
		len += elen;
	}
	strcat(buf, "}");
	return buf;
}

/* -------------------------------------------------------------------------
 * HTTP Operations
 * ------------------------------------------------------------------------- */
char* http_get(const char* url)
{
	if (!url || *url == '\0') return strdup("");
	char host[256] = {0};
	char path[512] = {0};
	int port = 80;

	const char* p = url;
	if (strncmp(p, "http://", 7) == 0) p += 7;
	else if (strncmp(p, "https://", 8) == 0) { p += 8; port = 443; }

	const char* slash = strchr(p, '/');
	const char* colon = strchr(p, ':');
	if (colon && (!slash || colon < slash))
	{
		size_t hlen = (size_t)(colon - p);
		if (hlen >= sizeof(host)) hlen = sizeof(host) - 1;
		strncpy(host, p, hlen);
		port = atoi(colon + 1);
		if (slash) strncpy(path, slash, sizeof(path) - 1);
		else strcpy(path, "/");
	}
	else if (slash)
	{
		size_t hlen = (size_t)(slash - p);
		if (hlen >= sizeof(host)) hlen = sizeof(host) - 1;
		strncpy(host, p, hlen);
		strncpy(path, slash, sizeof(path) - 1);
	}
	else
	{
		strncpy(host, p, sizeof(host) - 1);
		strcpy(path, "/");
	}

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return strdup("");

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port);
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, port_str, &hints, &res) != 0 || !res)
	{
		close(fd);
		return strdup("");
	}

	int connected = 0;
	for (struct addrinfo* cur = res; cur != NULL; cur = cur->ai_next)
	{
		if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0)
		{
			connected = 1;
			break;
		}
	}
	freeaddrinfo(res);
	if (!connected)
	{
		close(fd);
		return strdup("");
	}

	char req[1024];
	snprintf(req, sizeof(req),
	         "GET %s HTTP/1.1\r\n"
	         "Host: %s\r\n"
	         "User-Agent: xlang/2.0\r\n"
	         "Connection: close\r\n"
	         "Accept: */*\r\n\r\n",
	         path, host);
	send(fd, req, strlen(req), 0);

	size_t cap = 4096, total = 0;
	char* body = (char*)malloc(cap);
	char chunk[1024];
	ssize_t n;
	while ((n = recv(fd, chunk, sizeof(chunk), 0)) > 0)
	{
		if (total + (size_t)n + 1 > cap)
		{
			cap = (total + (size_t)n + 1) * 2;
			body = (char*)realloc(body, cap);
		}
		if (body) { memcpy(body + total, chunk, (size_t)n); total += (size_t)n; }
	}
	close(fd);
	if (body) body[total] = '\0';
	return body ? body : strdup("");
}
