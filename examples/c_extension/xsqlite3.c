#include "xlang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * SQLite3 C API Declarations (Zero external header dependency)
 * ------------------------------------------------------------------------- */
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef int64_t sqlite3_int64;

int sqlite3_open(const char *filename, sqlite3 **ppDb);
int sqlite3_close(sqlite3 *db);
int sqlite3_exec(sqlite3 *db, const char *sql, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg);
int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
int sqlite3_step(sqlite3_stmt *pStmt);
int sqlite3_finalize(sqlite3_stmt *pStmt);
int sqlite3_column_count(sqlite3_stmt *pStmt);
const char *sqlite3_column_name(sqlite3_stmt *pStmt, int N);
int sqlite3_column_type(sqlite3_stmt *pStmt, int iCol);
const unsigned char *sqlite3_column_text(sqlite3_stmt *pStmt, int iCol);
int sqlite3_column_int(sqlite3_stmt *pStmt, int iCol);
double sqlite3_column_double(sqlite3_stmt *pStmt, int iCol);
sqlite3_int64 sqlite3_column_int64(sqlite3_stmt *pStmt, int iCol);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3 *db);
int sqlite3_changes(sqlite3 *db);
const char *sqlite3_errmsg(sqlite3 *db);
int sqlite3_errcode(sqlite3 *db);
const char *sqlite3_libversion(void);
sqlite3_int64 sqlite3_memory_used(void);
void sqlite3_free(void *ptr);

#define SQLITE_OK          0
#define SQLITE_ERROR       1
#define SQLITE_ROW         100
#define SQLITE_DONE        101
#define SQLITE_INTEGER     1
#define SQLITE_FLOAT       2
#define SQLITE_TEXT        3
#define SQLITE_BLOB        4
#define SQLITE_NULL        5

/* -------------------------------------------------------------------------
 * Handle Management Table
 * ------------------------------------------------------------------------- */
#define MAX_SQLITE_HANDLES 128
static sqlite3* s_handles[MAX_SQLITE_HANDLES] = {0};
static char s_last_errmsg[512] = {0};

static int alloc_db_handle(sqlite3* db)
{
	for (int i = 1; i < MAX_SQLITE_HANDLES; i++)
	{
		if (s_handles[i] == NULL)
		{
			s_handles[i] = db;
			return i;
		}
	}
	return 0;
}

static sqlite3* get_db(int handle)
{
	if (handle <= 0 || handle >= MAX_SQLITE_HANDLES)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "Invalid database handle %d", handle);
		return NULL;
	}
	if (s_handles[handle] == NULL)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "Database handle %d is closed", handle);
		return NULL;
	}
	return s_handles[handle];
}

/* -------------------------------------------------------------------------
 * Direct C Functions (Exported for LLVM AOT and Direct C Consumers)
 * ------------------------------------------------------------------------- */

XLANG_EXPORT int sqlite3_connect(const char* filename)
{
	if (!filename || *filename == '\0') filename = ":memory:";
	sqlite3* db = NULL;
	int rc = sqlite3_open(filename, &db);
	if (rc != SQLITE_OK)
	{
		if (db)
		{
			snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
			sqlite3_close(db);
		}
		else
		{
			snprintf(s_last_errmsg, sizeof(s_last_errmsg), "Cannot open SQLite database '%s'", filename);
		}
		return 0;
	}
	int h = alloc_db_handle(db);
	if (h == 0)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "Too many open database handles (max %d)", MAX_SQLITE_HANDLES);
		sqlite3_close(db);
		return 0;
	}
	return h;
}

XLANG_EXPORT int sqlite3_open_db(const char* filename)
{
	return sqlite3_connect(filename);
}

XLANG_EXPORT int sqlite3_disconnect(int handle)
{
	sqlite3* db = get_db(handle);
	if (!db) return -1;
	int rc = sqlite3_close(db);
	s_handles[handle] = NULL;
	return rc;
}

XLANG_EXPORT int sqlite3_close_db(int handle)
{
	return sqlite3_disconnect(handle);
}

XLANG_EXPORT int sqlite3_execute(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return -1;

	char* err = NULL;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
	if (rc != SQLITE_OK)
	{
		if (err)
		{
			snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", err);
			sqlite3_free(err);
		}
		else
		{
			snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		}
		return rc;
	}
	return 0;
}

XLANG_EXPORT char* sqlite3_query_value(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return strdup("");

	sqlite3_stmt* stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK || !stmt)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		return strdup("");
	}

	rc = sqlite3_step(stmt);
	char* result = NULL;
	if (rc == SQLITE_ROW)
	{
		const char* txt = (const char*)sqlite3_column_text(stmt, 0);
		result = txt ? strdup(txt) : strdup("");
	}
	else
	{
		result = strdup("");
	}
	sqlite3_finalize(stmt);
	return result;
}

XLANG_EXPORT int sqlite3_query_int(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return 0;

	sqlite3_stmt* stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK || !stmt)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		return 0;
	}

	rc = sqlite3_step(stmt);
	int val = 0;
	if (rc == SQLITE_ROW)
	{
		val = sqlite3_column_int(stmt, 0);
	}
	sqlite3_finalize(stmt);
	return val;
}

XLANG_EXPORT double sqlite3_query_float(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return 0.0;

	sqlite3_stmt* stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK || !stmt)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		return 0.0;
	}

	rc = sqlite3_step(stmt);
	double val = 0.0;
	if (rc == SQLITE_ROW)
	{
		val = sqlite3_column_double(stmt, 0);
	}
	sqlite3_finalize(stmt);
	return val;
}

/* Dynamic string buffer helper for JSON / CSV formatting */
typedef struct {
	char* data;
	size_t len;
	size_t cap;
} StrBuf;

static void buf_init(StrBuf* b)
{
	b->cap = 256;
	b->len = 0;
	b->data = (char*)malloc(b->cap);
	if (b->data) b->data[0] = '\0';
}

static void buf_append(StrBuf* b, const char* s)
{
	if (!s || !b->data) return;
	size_t slen = strlen(s);
	if (b->len + slen + 1 >= b->cap)
	{
		b->cap = (b->len + slen + 1) * 2;
		char* nd = (char*)realloc(b->data, b->cap);
		if (!nd) return;
		b->data = nd;
	}
	memcpy(b->data + b->len, s, slen);
	b->len += slen;
	b->data[b->len] = '\0';
}

static void buf_append_escaped_json(StrBuf* b, const char* s)
{
	buf_append(b, "\"");
	if (s)
	{
		for (const char* p = s; *p; p++)
		{
			switch (*p)
			{
			case '\"': buf_append(b, "\\\""); break;
			case '\\': buf_append(b, "\\\\"); break;
			case '\n': buf_append(b, "\\n"); break;
			case '\r': buf_append(b, "\\r"); break;
			case '\t': buf_append(b, "\\t"); break;
			default: {
				char tmp[2] = {*p, '\0'};
				buf_append(b, tmp);
				break;
			}
			}
		}
	}
	buf_append(b, "\"");
}

XLANG_EXPORT char* sqlite3_query_json(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return strdup("[]");

	sqlite3_stmt* stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK || !stmt)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		return strdup("[]");
	}

	StrBuf b;
	buf_init(&b);
	buf_append(&b, "[");

	int cols = sqlite3_column_count(stmt);
	int row_count = 0;

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		if (row_count > 0) buf_append(&b, ", ");
		buf_append(&b, "{");
		for (int c = 0; c < cols; c++)
		{
			if (c > 0) buf_append(&b, ", ");
			const char* col_name = sqlite3_column_name(stmt, c);
			buf_append_escaped_json(&b, col_name ? col_name : "col");
			buf_append(&b, ": ");

			int type = sqlite3_column_type(stmt, c);
			char numbuf[64];
			switch (type)
			{
			case SQLITE_INTEGER:
				snprintf(numbuf, sizeof(numbuf), "%ld", (long)sqlite3_column_int64(stmt, c));
				buf_append(&b, numbuf);
				break;
			case SQLITE_FLOAT:
				snprintf(numbuf, sizeof(numbuf), "%.6g", sqlite3_column_double(stmt, c));
				buf_append(&b, numbuf);
				break;
			case SQLITE_NULL:
				buf_append(&b, "null");
				break;
			case SQLITE_TEXT:
			default:
				buf_append_escaped_json(&b, (const char*)sqlite3_column_text(stmt, c));
				break;
			}
		}
		buf_append(&b, "}");
		row_count++;
	}

	buf_append(&b, "]");
	sqlite3_finalize(stmt);
	return b.data ? b.data : strdup("[]");
}

XLANG_EXPORT char* sqlite3_query_csv(int handle, const char* sql)
{
	sqlite3* db = get_db(handle);
	if (!db || !sql) return strdup("");

	sqlite3_stmt* stmt = NULL;
	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK || !stmt)
	{
		snprintf(s_last_errmsg, sizeof(s_last_errmsg), "%s", sqlite3_errmsg(db));
		return strdup("");
	}

	StrBuf b;
	buf_init(&b);

	int cols = sqlite3_column_count(stmt);
	for (int c = 0; c < cols; c++)
	{
		if (c > 0) buf_append(&b, ",");
		buf_append(&b, sqlite3_column_name(stmt, c));
	}
	buf_append(&b, "\n");

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		for (int c = 0; c < cols; c++)
		{
			if (c > 0) buf_append(&b, ",");
			const char* txt = (const char*)sqlite3_column_text(stmt, c);
			if (txt)
			{
				/* Quote if contains comma or newline */
				if (strchr(txt, ',') || strchr(txt, '\n') || strchr(txt, '"'))
				{
					buf_append(&b, "\"");
					for (const char* p = txt; *p; p++)
					{
						if (*p == '"') buf_append(&b, "\"\"");
						else { char tmp[2] = {*p, '\0'}; buf_append(&b, tmp); }
					}
					buf_append(&b, "\"");
				}
				else
				{
					buf_append(&b, txt);
				}
			}
		}
		buf_append(&b, "\n");
	}

	sqlite3_finalize(stmt);
	return b.data ? b.data : strdup("");
}

XLANG_EXPORT int sqlite3_last_id(int handle)
{
	sqlite3* db = get_db(handle);
	if (!db) return 0;
	return (int)sqlite3_last_insert_rowid(db);
}

XLANG_EXPORT int sqlite3_changes_count(int handle)
{
	sqlite3* db = get_db(handle);
	if (!db) return 0;
	return sqlite3_changes(db);
}

XLANG_EXPORT char* sqlite3_error_message(int handle)
{
	sqlite3* db = get_db(handle);
	if (!db) return strdup(s_last_errmsg);
	return strdup(sqlite3_errmsg(db));
}

XLANG_EXPORT char* sqlite3_engine_version(void)
{
	return strdup(sqlite3_libversion());
}

XLANG_EXPORT int sqlite3_memory_usage(void)
{
	return (int)sqlite3_memory_used();
}

/* -------------------------------------------------------------------------
 * Aliases for module 'sqlite'
 * ------------------------------------------------------------------------- */
XLANG_EXPORT int sqlite_open(const char* path) { return sqlite3_connect(path); }
XLANG_EXPORT int sqlite_close(int db) { return sqlite3_disconnect(db); }
XLANG_EXPORT int sqlite_exec(int db, const char* sql) { return sqlite3_execute(db, sql); }
XLANG_EXPORT char* sqlite_query(int db, const char* sql) { return sqlite3_query_json(db, sql); }
XLANG_EXPORT char* sqlite_query_value(int db, const char* sql) { return sqlite3_query_value(db, sql); }
XLANG_EXPORT int sqlite_query_int(int db, const char* sql) { return sqlite3_query_int(db, sql); }
XLANG_EXPORT double sqlite_query_float(int db, const char* sql) { return sqlite3_query_float(db, sql); }
XLANG_EXPORT char* sqlite_query_json(int db, const char* sql) { return sqlite3_query_json(db, sql); }
XLANG_EXPORT char* sqlite_query_csv(int db, const char* sql) { return sqlite3_query_csv(db, sql); }
XLANG_EXPORT int sqlite_last_id(int db) { return sqlite3_last_id(db); }
XLANG_EXPORT int sqlite_changes(int db) { return sqlite3_changes_count(db); }
XLANG_EXPORT char* sqlite_errmsg(int db) { return sqlite3_error_message(db); }
XLANG_EXPORT char* sqlite_version(void) { return sqlite3_engine_version(); }
XLANG_EXPORT int sqlite_memory_used(void) { return sqlite3_memory_used(); }

/* -------------------------------------------------------------------------
 * Vectorcall Wrappers (For Bytecode VM and Dynamic Invocation)
 * ------------------------------------------------------------------------- */

static inline int get_int(int argc, const XValue* args, int idx, int def)
{
	if (idx >= argc) return def;
	if (args[idx].type == XLANG_VAL_INT) return (int)args[idx].as.ival;
	if (args[idx].type == XLANG_VAL_FLOAT) return (int)args[idx].as.fval;
	return def;
}

static inline const char* get_str(int argc, const XValue* args, int idx, const char* def)
{
	if (idx >= argc) return def;
	if (args[idx].type == XLANG_VAL_STRING && args[idx].as.sval) return args[idx].as.sval;
	return def;
}

static XValue vec_connect(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* path = get_str(argc, args, 0, ":memory:");
	return xval_int(sqlite3_connect(path));
}

static XValue vec_disconnect(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	return xval_int(sqlite3_disconnect(h));
}

static XValue vec_execute(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	return xval_int(sqlite3_execute(h, sql));
}

static XValue vec_query_value(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	char* res = sqlite3_query_value(h, sql);
	return xval_str(res);
}

static XValue vec_query_int(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	return xval_int(sqlite3_query_int(h, sql));
}

static XValue vec_query_float(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	return xval_float(sqlite3_query_float(h, sql));
}

static XValue vec_query_json(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	char* res = sqlite3_query_json(h, sql);
	return xval_str(res);
}

static XValue vec_query_csv(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	const char* sql = get_str(argc, args, 1, "");
	char* res = sqlite3_query_csv(h, sql);
	return xval_str(res);
}

static XValue vec_last_id(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	return xval_int(sqlite3_last_id(h));
}

static XValue vec_changes_count(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	return xval_int(sqlite3_changes_count(h));
}

static XValue vec_error_message(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int h = get_int(argc, args, 0, 0);
	char* msg = sqlite3_error_message(h);
	return xval_str(msg);
}

static XValue vec_version(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr; (void)argc; (void)args;
	return xval_str(sqlite3_engine_version());
}

static XValue vec_memory_used(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr; (void)argc; (void)args;
	return xval_int(sqlite3_memory_usage());
}

/* -------------------------------------------------------------------------
 * Function Descriptors Table for 'sqlite3'
 * ------------------------------------------------------------------------- */
static const XExtensionFunc s_sqlite3_funcs[] = {
	{ "connect",       vec_connect,       1, XLANG_TYPE_INT,    { XLANG_TYPE_STRING } },
	{ "open_db",       vec_connect,       1, XLANG_TYPE_INT,    { XLANG_TYPE_STRING } },
	{ "disconnect",    vec_disconnect,    1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "close_db",      vec_disconnect,    1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "execute",       vec_execute,       2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_value",   vec_query_value,   2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_int",     vec_query_int,     2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_float",   vec_query_float,   2, XLANG_TYPE_FLOAT,  { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_json",    vec_query_json,    2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_csv",     vec_query_csv,     2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "last_id",       vec_last_id,       1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "changes_count", vec_changes_count, 1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "error_message", vec_error_message, 1, XLANG_TYPE_STRING, { XLANG_TYPE_INT } },
	{ "engine_version", vec_version,       0, XLANG_TYPE_STRING, { XLANG_TYPE_VOID } },
	{ "memory_usage",  vec_memory_used,   0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

/* -------------------------------------------------------------------------
 * Function Descriptors Table for 'sqlite'
 * ------------------------------------------------------------------------- */
static const XExtensionFunc s_sqlite_funcs[] = {
	{ "open",          vec_connect,       1, XLANG_TYPE_INT,    { XLANG_TYPE_STRING } },
	{ "close",         vec_disconnect,    1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "exec",          vec_execute,       2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query",         vec_query_json,    2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_value",   vec_query_value,   2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_int",     vec_query_int,     2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_float",   vec_query_float,   2, XLANG_TYPE_FLOAT,  { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_json",    vec_query_json,    2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "query_csv",     vec_query_csv,     2, XLANG_TYPE_STRING, { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "last_id",       vec_last_id,       1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "changes",       vec_changes_count, 1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "errmsg",        vec_error_message, 1, XLANG_TYPE_STRING, { XLANG_TYPE_INT } },
	{ "version",       vec_version,       0, XLANG_TYPE_STRING, { XLANG_TYPE_VOID } },
	{ "memory_used",   vec_memory_used,   0, XLANG_TYPE_INT,    { XLANG_TYPE_VOID } },
	{ NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

/* -------------------------------------------------------------------------
 * Module Entry Point
 * ------------------------------------------------------------------------- */
XLANG_EXPORT XLANG_EXTENSION_ENTRY
{
	ctx->register_module(ctx, "sqlite3", s_sqlite3_funcs);
	ctx->register_module(ctx, "sqlite", s_sqlite_funcs);
	return 0;
}

XLANG_EXPORT int xlang_init_sqlite3(XExtensionContext* ctx)
{
	return xlang_module_init(ctx);
}

XLANG_EXPORT int xlang_init_sqlite(XExtensionContext* ctx)
{
	return xlang_module_init(ctx);
}
