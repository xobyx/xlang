#include "xlang.h"
#include <zlib.h>
#undef zlib_version
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------
 * Hex Encoding / Decoding Utilities for Raw Binary Buffers
 * ------------------------------------------------------------------------- */
static const char s_hex_digits[] = "0123456789abcdef";

static char* bytes_to_hex(const unsigned char* data, size_t len)
{
	char* hex = (char*)malloc(len * 2 + 1);
	if (!hex) return strdup("");
	for (size_t i = 0; i < len; i++)
	{
		hex[i * 2]     = s_hex_digits[(data[i] >> 4) & 0x0F];
		hex[i * 2 + 1] = s_hex_digits[data[i] & 0x0F];
	}
	hex[len * 2] = '\0';
	return hex;
}

static int hex_char_to_val(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static unsigned char* hex_to_bytes(const char* hex, size_t* out_len)
{
	if (!hex) { *out_len = 0; return NULL; }
	size_t hlen = strlen(hex);
	if (hlen % 2 != 0) { *out_len = 0; return NULL; }

	size_t blen = hlen / 2;
	unsigned char* buf = (unsigned char*)malloc(blen + 1);
	if (!buf) { *out_len = 0; return NULL; }

	for (size_t i = 0; i < blen; i++)
	{
		int hi = hex_char_to_val(hex[i * 2]);
		int lo = hex_char_to_val(hex[i * 2 + 1]);
		if (hi < 0 || lo < 0)
		{
			free(buf);
			*out_len = 0;
			return NULL;
		}
		buf[i] = (unsigned char)((hi << 4) | lo);
	}
	buf[blen] = '\0';
	*out_len = blen;
	return buf;
}

/* -------------------------------------------------------------------------
 * Direct C Functions (Exported for LLVM AOT and Direct C Consumers)
 * ------------------------------------------------------------------------- */

XLANG_EXPORT char* zlib_version(void)
{
	return strdup(zlibVersion());
}

XLANG_EXPORT int zlib_crc32(int crc, const char* str)
{
	if (!str) return crc;
	uLong c = (uLong)(uint32_t)crc;
	c = crc32(c, (const Bytef*)str, (uInt)strlen(str));
	return (int)(uint32_t)c;
}

XLANG_EXPORT int zlib_adler32(int adler, const char* str)
{
	if (!str) return adler;
	uLong a = (uLong)(uint32_t)adler;
	a = adler32(a, (const Bytef*)str, (uInt)strlen(str));
	return (int)(uint32_t)a;
}

XLANG_EXPORT int zlib_compress_bound(int source_len)
{
	if (source_len < 0) return 0;
	return (int)compressBound((uLong)source_len);
}

XLANG_EXPORT char* zlib_compress(const char* input)
{
	if (!input) return strdup("");
	uLong src_len = (uLong)strlen(input);
	uLong bound = compressBound(src_len);

	Bytef* comp_buf = (Bytef*)malloc(bound);
	if (!comp_buf) return strdup("");

	uLong comp_len = bound;
	int rc = compress(comp_buf, &comp_len, (const Bytef*)input, src_len);
	if (rc != Z_OK)
	{
		free(comp_buf);
		return strdup("");
	}

	char* hex_str = bytes_to_hex(comp_buf, comp_len);
	free(comp_buf);
	return hex_str;
}

XLANG_EXPORT char* zlib_uncompress(const char* hex_input)
{
	if (!hex_input || *hex_input == '\0') return strdup("");

	size_t comp_len = 0;
	unsigned char* comp_buf = hex_to_bytes(hex_input, &comp_len);
	if (!comp_buf) return strdup("");

	/* Dynamically expand destination buffer until it accommodates uncompressed text */
	uLong dest_cap = (uLong)comp_len * 4 + 256;
	Bytef* dest_buf = (Bytef*)malloc(dest_cap + 1);
	if (!dest_buf)
	{
		free(comp_buf);
		return strdup("");
	}

	while (1)
	{
		uLong dest_len = dest_cap;
		int rc = uncompress(dest_buf, &dest_len, comp_buf, (uLong)comp_len);
		if (rc == Z_OK)
		{
			dest_buf[dest_len] = '\0';
			free(comp_buf);
			return (char*)dest_buf;
		}
		if (rc == Z_BUF_ERROR && dest_cap < 64 * 1024 * 1024)
		{
			dest_cap *= 2;
			Bytef* new_buf = (Bytef*)realloc(dest_buf, dest_cap + 1);
			if (!new_buf) break;
			dest_buf = new_buf;
		}
		else
		{
			break;
		}
	}

	free(comp_buf);
	free(dest_buf);
	return strdup("");
}

XLANG_EXPORT int zlib_gz_write(const char* filename, const char* content)
{
	if (!filename || !content) return -1;
	gzFile gz = gzopen(filename, "wb");
	if (!gz) return -1;

	size_t len = strlen(content);
	int written = gzwrite(gz, content, (unsigned int)len);
	gzclose(gz);
	return written;
}

XLANG_EXPORT char* zlib_gz_read(const char* filename)
{
	if (!filename) return strdup("");
	gzFile gz = gzopen(filename, "rb");
	if (!gz) return strdup("");

	size_t cap = 4096;
	size_t total = 0;
	char* buf = (char*)malloc(cap + 1);
	if (!buf) { gzclose(gz); return strdup(""); }

	int bytes_read = 0;
	while ((bytes_read = gzread(gz, buf + total, (unsigned int)(cap - total))) > 0)
	{
		total += bytes_read;
		if (total >= cap)
		{
			cap *= 2;
			char* new_buf = (char*)realloc(buf, cap + 1);
			if (!new_buf) break;
			buf = new_buf;
		}
	}
	buf[total] = '\0';
	gzclose(gz);
	return buf;
}

XLANG_EXPORT int zlib_compress_file(const char* src_file, const char* dst_gz_file)
{
	if (!src_file || !dst_gz_file) return -1;
	FILE* in = fopen(src_file, "rb");
	if (!in) return -1;

	gzFile out = gzopen(dst_gz_file, "wb");
	if (!out) { fclose(in); return -1; }

	char buf[8192];
	size_t r = 0;
	int total = 0;
	while ((r = fread(buf, 1, sizeof(buf), in)) > 0)
	{
		int w = gzwrite(out, buf, (unsigned int)r);
		if (w <= 0) break;
		total += w;
	}
	fclose(in);
	gzclose(out);
	return total;
}

XLANG_EXPORT int zlib_decompress_file(const char* src_gz_file, const char* dst_file)
{
	if (!src_gz_file || !dst_file) return -1;
	gzFile in = gzopen(src_gz_file, "rb");
	if (!in) return -1;

	FILE* out = fopen(dst_file, "wb");
	if (!out) { gzclose(in); return -1; }

	char buf[8192];
	int r = 0;
	int total = 0;
	while ((r = gzread(in, buf, sizeof(buf))) > 0)
	{
		size_t w = fwrite(buf, 1, (size_t)r, out);
		total += (int)w;
	}
	gzclose(in);
	fclose(out);
	return total;
}

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

static XValue vec_version(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr; (void)argc; (void)args;
	return xval_str(zlib_version());
}

static XValue vec_crc32(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int crc = get_int(argc, args, 0, 0);
	const char* s = get_str(argc, args, 1, "");
	return xval_int(zlib_crc32(crc, s));
}

static XValue vec_adler32(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int adler = get_int(argc, args, 0, 1);
	const char* s = get_str(argc, args, 1, "");
	return xval_int(zlib_adler32(adler, s));
}

static XValue vec_compress_bound(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	int len = get_int(argc, args, 0, 0);
	return xval_int(zlib_compress_bound(len));
}

static XValue vec_compress(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* s = get_str(argc, args, 0, "");
	char* hex = zlib_compress(s);
	return xval_str(hex);
}

static XValue vec_uncompress(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* hex = get_str(argc, args, 0, "");
	char* orig = zlib_uncompress(hex);
	return xval_str(orig);
}

static XValue vec_gz_write(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* fn = get_str(argc, args, 0, "");
	const char* data = get_str(argc, args, 1, "");
	return xval_int(zlib_gz_write(fn, data));
}

static XValue vec_gz_read(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* fn = get_str(argc, args, 0, "");
	char* content = zlib_gz_read(fn);
	return xval_str(content);
}

static XValue vec_compress_file(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* src = get_str(argc, args, 0, "");
	const char* dst = get_str(argc, args, 1, "");
	return xval_int(zlib_compress_file(src, dst));
}

static XValue vec_decompress_file(XVm* vm, XValue rcvr, int argc, const XValue* args)
{
	(void)vm; (void)rcvr;
	const char* src = get_str(argc, args, 0, "");
	const char* dst = get_str(argc, args, 1, "");
	return xval_int(zlib_decompress_file(src, dst));
}

/* -------------------------------------------------------------------------
 * Function Descriptors Table for 'zlib'
 * ------------------------------------------------------------------------- */
static const XExtensionFunc s_zlib_funcs[] = {
	{ "version",         vec_version,         0, XLANG_TYPE_STRING, { XLANG_TYPE_VOID } },
	{ "crc32",           vec_crc32,           2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "adler32",         vec_adler32,         2, XLANG_TYPE_INT,    { XLANG_TYPE_INT, XLANG_TYPE_STRING } },
	{ "compress_bound",  vec_compress_bound,  1, XLANG_TYPE_INT,    { XLANG_TYPE_INT } },
	{ "compress",        vec_compress,        1, XLANG_TYPE_STRING, { XLANG_TYPE_STRING } },
	{ "uncompress",      vec_uncompress,      1, XLANG_TYPE_STRING, { XLANG_TYPE_STRING } },
	{ "gz_write",        vec_gz_write,        2, XLANG_TYPE_INT,    { XLANG_TYPE_STRING, XLANG_TYPE_STRING } },
	{ "gz_read",         vec_gz_read,         1, XLANG_TYPE_STRING, { XLANG_TYPE_STRING } },
	{ "compress_file",   vec_compress_file,   2, XLANG_TYPE_INT,    { XLANG_TYPE_STRING, XLANG_TYPE_STRING } },
	{ "decompress_file", vec_decompress_file, 2, XLANG_TYPE_INT,    { XLANG_TYPE_STRING, XLANG_TYPE_STRING } },
	{ NULL, NULL, 0, XLANG_TYPE_VOID, { XLANG_TYPE_VOID } }
};

/* -------------------------------------------------------------------------
 * Module Entry Point
 * ------------------------------------------------------------------------- */
XLANG_EXPORT XLANG_EXTENSION_ENTRY
{
	return ctx->register_module(ctx, "zlib", s_zlib_funcs);
}

XLANG_EXPORT int xlang_init_zlib(XExtensionContext* ctx)
{
	return xlang_module_init(ctx);
}
