#include "xllvm_rt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

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
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons((uint16_t)port);
	inet_pton(AF_INET, host, &sin.sin_addr);
	return connect(fd, (struct sockaddr*)&sin, sizeof(sin));
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
__attribute__((weak)) int gc_set_threshold(int th) { (void)th; return 0; }
__attribute__((weak)) int gc_dump(void) { return 0; }
#else
int gc_collect(void) { return 0; }
int gc_allocated_bytes(void) { return 0; }
int gc_total_objects(void) { return 0; }
int gc_enable(void) { return 1; }
int gc_disable(void) { return 0; }
int gc_set_threshold(int th) { (void)th; return 0; }
int gc_dump(void) { return 0; }
#endif
