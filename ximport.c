#include "ximport.h"
#include "functions.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if defined(_MSC_VER)
#include <direct.h>
#define PATH_MAX 1024
#else
#include <unistd.h>
#endif

typedef struct {
	char* canonical_path;
	bool in_progress;
} imported_file_entry_t;

static imported_file_entry_t* s_imported = NULL;
static int s_imported_count = 0;
static int s_imported_cap = 0;
static char s_initial_cwd[PATH_MAX] = {0};

void x_import_init(void)
{
	s_imported = NULL;
	s_imported_count = 0;
	s_imported_cap = 0;
#if !defined(_MSC_VER)
	if (getcwd(s_initial_cwd, sizeof(s_initial_cwd)) == NULL)
		s_initial_cwd[0] = '\0';
#else
	if (_getcwd(s_initial_cwd, sizeof(s_initial_cwd)) == NULL)
		s_initial_cwd[0] = '\0';
#endif
}

void x_import_cleanup(void)
{
	if (s_imported != NULL)
	{
		for (int i = 0; i < s_imported_count; i++)
		{
			if (s_imported[i].canonical_path != NULL)
				free(s_imported[i].canonical_path);
		}
		free(s_imported);
		s_imported = NULL;
	}
	s_imported_count = 0;
	s_imported_cap = 0;
}

static FILE* try_open(const char* path, char* resolved_out, size_t resolved_size)
{
	if (path == NULL || *path == '\0') return NULL;
	FILE* f = fopen(path, "r");
	if (f == NULL) return NULL;

#if !defined(_MSC_VER)
	char real[PATH_MAX];
	if (realpath(path, real) != NULL)
	{
		size_t l = strlen(real);
		if (l >= resolved_size) l = resolved_size - 1;
		memcpy(resolved_out, real, l);
		resolved_out[l] = '\0';
	}
	else
	{
		size_t l = strlen(path);
		if (l >= resolved_size) l = resolved_size - 1;
		memcpy(resolved_out, path, l);
		resolved_out[l] = '\0';
	}
#else
	size_t l = strlen(path);
	if (l >= resolved_size) l = resolved_size - 1;
	memcpy(resolved_out, path, l);
	resolved_out[l] = '\0';
#endif
	return f;
}

static FILE* resolve_module_file(const char* name, char* resolved_path, size_t resolved_size)
{
	char test_path[PATH_MAX * 2];
	FILE* f = NULL;

	/* 1. As provided */
	f = try_open(name, resolved_path, resolved_size);
	if (f != NULL) return f;

	/* 2. With .xb extension */
	if (strstr(name, ".xb") == NULL)
	{
		snprintf(test_path, sizeof(test_path), "%s.xb", name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;
	}

	/* 3. Relative to initial working directory */
	if (s_initial_cwd[0] != '\0')
	{
		snprintf(test_path, sizeof(test_path), "%s/%s", s_initial_cwd, name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(name, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s/%s.xb", s_initial_cwd, name);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}

		snprintf(test_path, sizeof(test_path), "%s/lib/%s", s_initial_cwd, name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(name, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s/lib/%s.xb", s_initial_cwd, name);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}
	}

	/* 4. Walking up parent directories looking for lib/ */
	static const char* up_dirs[] = { "lib/", "../lib/", "../../lib/", "../../../lib/", "../../../../lib/" };
	for (int i = 0; i < (int)(sizeof(up_dirs) / sizeof(up_dirs[0])); i++)
	{
		snprintf(test_path, sizeof(test_path), "%s%s", up_dirs[i], name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(name, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s%s.xb", up_dirs[i], name);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}
	}

	/* 5. Walking up parent directories looking for the file directly */
	static const char* up_rel[] = { "../", "../../", "../../../", "../../../../" };
	for (int i = 0; i < (int)(sizeof(up_rel) / sizeof(up_rel[0])); i++)
	{
		snprintf(test_path, sizeof(test_path), "%s%s", up_rel[i], name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(name, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s%s.xb", up_rel[i], name);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}
	}

	/* 6. XLANG_HOME or XLANG_PATH environment variable */
	const char* env_path = getenv("XLANG_PATH");
	if (env_path == NULL) env_path = getenv("XLANG_HOME");
	if (env_path != NULL && *env_path != '\0')
	{
		snprintf(test_path, sizeof(test_path), "%s/%s", env_path, name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		snprintf(test_path, sizeof(test_path), "%s/lib/%s", env_path, name);
		f = try_open(test_path, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(name, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s/lib/%s.xb", env_path, name);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}
	}

	/* 7. Basename (if name contained path components) */
	const char* bname = strrchr(name, '/');
	if (bname != NULL)
	{
		bname++; /* skip '/' */
		f = try_open(bname, resolved_path, resolved_size);
		if (f != NULL) return f;

		if (strstr(bname, ".xb") == NULL)
		{
			snprintf(test_path, sizeof(test_path), "%s.xb", bname);
			f = try_open(test_path, resolved_path, resolved_size);
			if (f != NULL) return f;
		}
	}

	return NULL;
}

bool x_import_module(const char* module_name)
{
	if (module_name == NULL || *module_name == '\0')
		return false;

	char canonical[PATH_MAX] = {0};
	FILE* f = resolve_module_file(module_name, canonical, sizeof(canonical));
	if (f == NULL)
	{
		printf("\nError: Cannot import module '%s' - file not found\n", module_name);
		return false;
	}

	/* Check if already in registry */
	for (int i = 0; i < s_imported_count; i++)
	{
		if (s_imported[i].canonical_path != NULL && strcmp(s_imported[i].canonical_path, canonical) == 0)
		{
			if (s_imported[i].in_progress)
			{
				/* Circular import detected - skip safely to avoid infinite recursion */
				fclose(f);
				return true;
			}
			/* Already imported and finished */
			fclose(f);
			return true;
		}
	}

	/* Register new entry */
	if (s_imported_count >= s_imported_cap)
	{
		s_imported_cap = s_imported_cap == 0 ? 16 : s_imported_cap * 2;
		s_imported = (imported_file_entry_t*)realloc(s_imported, s_imported_cap * sizeof(imported_file_entry_t));
	}
	int entry_idx = s_imported_count++;
	s_imported[entry_idx].canonical_path = strdup(canonical);
	s_imported[entry_idx].in_progress = true;

	char* buf = get_file_buffer(f);
	fclose(f);

	if (buf != NULL)
	{
		start_parse_lines(buf, false);
		free(buf);
	}

	s_imported[entry_idx].in_progress = false;
	return true;
}
