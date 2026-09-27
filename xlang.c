/*              _
			   | |
__  __   ___   | |__    _   _  __  __
\ \/ /  / _ \  | '_ \  | | | | \ \/ /
 >  <  | (_) | | |_) | | |_| |  >  <
/_/\_\  \___/  |_.__/   \__, | /_/\_\
						 __/ |
						|___/        */
#include "xlang_main.h"
#if defined (_MSC_VER)
#include <direct.h>
#else
#include<unistd.h>
#endif
#include <time.h>
#include "xsys.h"
#include "xcollection.h"
#include "ximport.h"
#include "xgc.h"
#include "xast.h"
#include "xmath.h"
#include "xdis_html.h"
#include "xast_parser.h"
#include "xir.h"
#include "xir_compiler.h"
#include "xvm.h"
#include "xllvm.h"
#include "xllvm_jit.h"
#include <ctype.h>
#include "arena.h"
clock_t t;
//C:\Tests\t.xb
bool load_saved_code = false;
var_stack* varss;
var_stack* t_varss;
func_stack* funcs;
type_stack* types;
func_stack* t_funcs;
bool read_file = true;
bool save_code = false;
int print_parse_log = 0;
bool flag_stats = false;
extern Arena *g_lex_arena;
#define STR_VALUE(val) #val

void getsavedMD5(FILE* cf);
unsigned int file_md5[4];
unsigned int saved_md5[4];
char* buff;
char* saved_code_file_path;
#if defined(__GNUC__)|| defined(__MINGW64__)
#define errno_t int
#define _chdir chdir
int fopen_s(FILE **f, const char *name, const char *mode) {

	//  assert(f);
	*f = fopen(name, mode);
	/* Can't be sure about 1-to-1 mapping of errno and MS' errno_t */
	if (*f != NULL)
		return 0;
	return 2;
}
#define gets_s(x,y) fgets(x,500,stdin)
#endif
int GetDir(const char* full_path, char* dir)
{
	if (full_path == NULL || dir == NULL) return 0;
	char buff2[1024] = { 0 };
	int buffCounter = 0;
	int dirSymbolCounter = 0;

	for (unsigned int i = 0; i < strlen(full_path); i++)
	{
		if (full_path[i] != '\\' && full_path[i] != '/')
		{
			const int buff_size = 1024;
			if (buffCounter < buff_size - 1) buff2[buffCounter++] = full_path[i];
			else return -1;
		}
		else
		{
			for (int i2 = 0; i2 < buffCounter; i2++)
			{
				dir[dirSymbolCounter++] = buff2[i2];
				buff2[i2] = 0;
			}

			dir[dirSymbolCounter++] = full_path[i];
			buffCounter = 0;
		}
	}
	dir[dirSymbolCounter] = '\0';
	return dirSymbolCounter;
}

extern var_stack var_start_stack;
extern func_stack base_function;
extern type_stack simple_type_stack;

void int_xlang()
{
    if (g_lex_arena == NULL)
		g_lex_arena = arena_create(128 * 1024);
	varss = &var_start_stack;
	funcs = &base_function;
	types = &simple_type_stack;

	t_varss = (var_stack*)malloc(sizeof(var_stack));
	t_funcs = (func_stack*)malloc(sizeof(func_stack));
	var_stack_init(t_varss);
	func_stack_init(t_funcs);

	install_default_types();
	install_default_functions();
	x_collections_init();
	x_import_init();
	gc_init();
}


void change_dir(char** argv)
{
	char* dir = (char*)malloc(1024);
	memset(dir, 0, 1024);
	if (GetDir(argv[1], dir) > 0)
	{
		_chdir(dir);
	}
	free(dir);
}


int main(const int argc, char ** argv);

void clean_memory(void);

static char* repl_trim(char* str)
{
	while (*str && isspace((unsigned char)*str)) str++;
	if (*str == '\0') return str;
	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = '\0';
	return str;
}

static bool repl_is_statement(const char* s)
{
	while (*s == ' ' || *s == '\t') s++;
	if (*s == '\0') return true;

	if (strncmp(s, "class ", 6) == 0 || strncmp(s, "class\t", 6) == 0 || strncmp(s, "class(", 6) == 0) return true;
	if (strncmp(s, "import ", 7) == 0 || strncmp(s, "import\t", 7) == 0 || strncmp(s, "import(", 7) == 0) return true;
	if (strncmp(s, "if ", 3) == 0 || strncmp(s, "if(", 3) == 0) return true;
	if (strncmp(s, "while ", 6) == 0 || strncmp(s, "while(", 6) == 0) return true;
	if (strncmp(s, "for ", 4) == 0 || strncmp(s, "for(", 4) == 0) return true;
	if (strncmp(s, "do ", 3) == 0 || strncmp(s, "do{", 3) == 0 || strcmp(s, "do") == 0) return true;
	if (strncmp(s, "return ", 7) == 0 || strcmp(s, "return") == 0) return true;
	if (strcmp(s, "break") == 0) return true;
	if (strncmp(s, "static ", 7) == 0) return true;
	if (strncmp(s, "print(", 6) == 0 || strncmp(s, "print ", 6) == 0) return true;

	char first_word[64] = {0};
	int len = 0;
	while (((s[len] >= 'a' && s[len] <= 'z') || (s[len] >= 'A' && s[len] <= 'Z') ||
	        (s[len] >= '0' && s[len] <= '9') || s[len] == '_') && len < 63)
	{
		first_word[len] = s[len];
		len++;
	}
	first_word[len] = '\0';

	if (get_type_by_name(first_word) != NULL)
	{
		const char* after = s + len;
		while (*after == ' ' || *after == '\t') after++;
		if ((*after >= 'a' && *after <= 'z') || (*after >= 'A' && *after <= 'Z') || *after == '_')
			return true;
	}

	const char* eq = strchr(s, '=');
	if (eq != NULL)
	{
		if (*(eq + 1) != '=' && (eq == s || (*(eq - 1) != '!' && *(eq - 1) != '<' && *(eq - 1) != '>')))
		{
			return true;
		}
	}

	return false;
}

static void repl_print_vars(void)
{
	printf("\n\033[1;36m=== Variables (%d) ===\033[0m\n", varss != NULL ? varss->size : 0);
	if (varss == NULL || varss->root == NULL)
	{
		printf("  (none)\n\n");
		return;
	}
	for (var* v = varss->root; v != NULL; v = v->stack_next)
	{
		if (v->name == NULL) continue;
		const char* tname = v->type_define ? v->type_define->type_name : "unknown";
		if (v->type_define == T_INT && v->value_int != NULL)
			printf("  \033[32m%s\033[0m %s = \033[33m%d\033[0m\n", tname, v->name, *v->value_int);
		else if (v->type_define == T_STRING && v->value_str_ptr != NULL && *v->value_str_ptr != NULL)
			printf("  \033[32m%s\033[0m %s = \033[33m\"%s\"\033[0m\n", tname, v->name, *v->value_str_ptr);
		else if (v->type_define == T_FLOAT && v->value_float != NULL)
			printf("  \033[32m%s\033[0m %s = \033[33m%f\033[0m\n", tname, v->name, *v->value_float);
		else if (v->type_define == T_BOOL && v->value_bool != NULL)
			printf("  \033[32m%s\033[0m %s = \033[33m%s\033[0m\n", tname, v->name, *v->value_bool ? "true" : "false");
		else if (v->type_define == T_LONG && v->value_long != NULL)
			printf("  \033[32m%s\033[0m %s = \033[33m%ld\033[0m\n", tname, v->name, *v->value_long);
		else
			printf("  \033[32m%s\033[0m %s = [instance]\n", tname, v->name);
	}
	printf("\n");
}

static void repl_print_classes(void)
{
	printf("\n\033[1;36m=== Classes & Types (%d) ===\033[0m\n", types != NULL ? types->size : 0);
	if (types == NULL || types->root == NULL)
	{
		printf("  (none)\n\n");
		return;
	}
	for (type_def* t = types->root; t != NULL; t = t->stack_next)
	{
		if (t->type_name == NULL) continue;
		printf("  class \033[1;32m%s\033[0m", t->type_name);
		if (t->base != NULL && t->base->type_name != NULL)
			printf(" : %s", t->base->type_name);
		printf(" (%d methods, %d properties)\n", t->d_function_size, t->d_propertys_size);
	}
	printf("\n");
}

static void repl_print_help(void)
{
	printf("\n\033[1;36m=== xlang REPL Commands ===\033[0m\n");
	printf("  \033[1m:help, :?\033[0m         Show this help information\n");
	printf("  \033[1m:vars\033[0m             List all defined global variables and values\n");
	printf("  \033[1m:classes, :types\033[0m  List all loaded classes, methods, and properties\n");
	printf("  \033[1m:ast\033[0m              Display Structured AST for current session\n");
	printf("  \033[1m:ir, :dis\033[0m         Display Bytecode IR disassembly for current session\n");
	printf("  \033[1m:gc\033[0m               Show GC metrics and run garbage collection\n");
	printf("  \033[1m:reset\033[0m            Reset environment and clear all variables\n");
	printf("  \033[1m:quit, :exit, :q\033[0m  Exit the REPL\n\n");
}

void interupter(void)
{
	bool is_interactive = isatty(fileno(stdin));
	print_parse_log = 0;

	if (is_interactive)
	{
		printf("\033[1;36m  __  __ _                         \033[0m\n");
		printf("\033[1;36m  \\ \\/ /| | __ _ _ __   __ _       \033[0m\n");
		printf("\033[1;36m   \\  / | |/ _` | '_ \\ / _` |      \033[0m\n");
		printf("\033[1;36m   /  \\ | | (_| | | | | (_| |      \033[0m\n");
		printf("\033[1;36m  /_/\\_\\|_|\\__,_|_| |_|\\__, |      \033[0m\n");
		printf("\033[1;36m                       |___/       \033[0m\n");
		printf("\033[1mxlang 0.4.0\033[0m (Interactive REPL) on Linux\n");
		printf("Type \033[1;33m:help\033[0m for commands, \033[1;33m:quit\033[0m to exit.\n\n");
	}

	bool unclosed = false;
	char line_buf[1024 * 5];
	char accum_buf[1024 * 20] = {0};
	int accum_len = 0;
	int paren_depth = 0;
	int brace_depth = 0;

	XVm repl_vm;
	xvm_init(&repl_vm);

	while (true)
	{
		if (unclosed)
		{
			if (is_interactive)
				printf("\033[1;33m...   \033[0m");
			else
				printf("... ");
		}
		else
		{
			if (is_interactive)
				printf("\033[1;32mxlang>\033[0m ");
			else
				printf("xlang> ");
		}
		fflush(stdout);

		memset(line_buf, 0, sizeof(line_buf));
		if (fgets(line_buf, sizeof(line_buf) - 1, stdin) == NULL)
		{
			if (is_interactive)
				printf("\nGoodbye!\n");
			break;
		}

		char* trimmed = repl_trim(line_buf);

		if (!unclosed)
		{
			if (strcmp(trimmed, ":quit") == 0 || strcmp(trimmed, ":exit") == 0 || strcmp(trimmed, ":q") == 0)
			{
				if (is_interactive) printf("Goodbye!\n");
				break;
			}
			if (strcmp(trimmed, ":help") == 0 || strcmp(trimmed, ":?") == 0)
			{
				repl_print_help();
				continue;
			}
			if (strcmp(trimmed, ":vars") == 0)
			{
				repl_print_vars();
				continue;
			}
			if (strcmp(trimmed, ":classes") == 0 || strcmp(trimmed, ":types") == 0)
			{
				repl_print_classes();
				continue;
			}
			if (strcmp(trimmed, ":gc") == 0)
			{
				printf("Active objects: %zu, Allocated memory: %zu bytes\n", gc_total_objects(), gc_allocated_bytes());
				gc_collect();
				printf("After GC: %zu objects, %zu bytes\n", gc_total_objects(), gc_allocated_bytes());
				continue;
			}
			if (strcmp(trimmed, ":reset") == 0)
			{
				clean_memory();
				int_xlang();
				xvm_free(&repl_vm);
				xvm_init(&repl_vm);
				unclosed = false;
				accum_len = 0;
				accum_buf[0] = '\0';
				paren_depth = 0;
				brace_depth = 0;
				printf("Environment reset.\n");
				continue;
			}
			if (trimmed[0] == '\0')
			{
				continue;
			}
		}
		else
		{
			if (trimmed[0] == '\0')
			{
				unclosed = false;
				accum_len = 0;
				accum_buf[0] = '\0';
				paren_depth = 0;
				brace_depth = 0;
				printf("(multi-line block canceled)\n");
				continue;
			}
		}

		/* Append to accum_buf */
		size_t l_len = strlen(line_buf);
		if (accum_len + l_len < sizeof(accum_buf) - 1)
		{
			memcpy(accum_buf + accum_len, line_buf, l_len);
			accum_len += l_len;
			accum_buf[accum_len] = '\0';
		}

		/* Update paren/brace depth */
		for (size_t i = 0; i < l_len; i++)
		{
			if (line_buf[i] == '(') paren_depth++;
			else if (line_buf[i] == ')' && paren_depth > 0) paren_depth--;
			else if (line_buf[i] == '{') brace_depth++;
			else if (line_buf[i] == '}' && brace_depth > 0) brace_depth--;
		}

		if (paren_depth > 0 || brace_depth > 0)
		{
			unclosed = true;
			continue;
		}

		unclosed = false;

		/* Execute accumulated buffer */
		xdiag_reset_error_count();
		AstProgram* prog = NULL;

		if (!repl_is_statement(trimmed))
		{
			char eval_buf[1024 * 20 + 32];
			snprintf(eval_buf, sizeof(eval_buf), "print(%s)\n", trimmed);
			prog = xast_parse_source(eval_buf, "<repl>");
		}

		if (prog == NULL || xdiag_get_error_count() > 0)
		{
			if (prog) { ast_program_destroy(prog); prog = NULL; }
			xdiag_reset_error_count();
			prog = xast_parse_source(accum_buf, "<repl>");
		}

		if (prog != NULL && xdiag_get_error_count() == 0)
		{
			XIrChunk chunk;
			xir_chunk_init(&chunk);
			xir_compile_program(prog, &chunk);
			xvm_run(&repl_vm, &chunk);
			xir_chunk_free(&chunk);
			ast_program_destroy(prog);
		}

		accum_len = 0;
		accum_buf[0] = '\0';
		paren_depth = 0;
		brace_depth = 0;
	}

	xvm_free(&repl_vm);
}

static bool is_xbc_file(const char* path)
{
	if (!path) return false;
	size_t len = strlen(path);
	if (len >= 4 && strcmp(path + len - 4, ".xbc") == 0) return true;

	FILE* f = fopen(path, "rb");
	if (!f) return false;
	uint32_t magic = 0;
	size_t read_bytes = fread(&magic, 1, 4, f);
	fclose(f);
	return (read_bytes == 4 && magic == XBC_MAGIC);
}

int main(const int argc, char** argv)
{
	int_xlang();
	t = clock();
	if (argc == 1)
	{
		xdiag_set_current_file("<stdin>");
		interupter();
		clean_memory();
		return 0;
	}

	bool flag_compile = false;
	const char* output_bc_path = NULL;
	bool flag_dump_ast = false;
	bool flag_dump_ir = false;
	bool flag_vm = false;
	bool flag_trace_vm = false;
	bool flag_jit = false;
	bool flag_view = false;
	bool flag_emit_llvm = false;
	bool flag_build = false;
	bool flag_release = false;
	bool flag_debug = false;
	const char* script_path = NULL;
	int script_idx = -1;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0)
		{
			flag_compile = true;
		}
		else if (strcmp(argv[i], "--emit-llvm") == 0 || strcmp(argv[i], "-S") == 0)
		{
			flag_emit_llvm = true;
		}
		else if (strcmp(argv[i], "build") == 0 || strcmp(argv[i], "--build") == 0)
		{
			flag_build = true;
		}
		else if (strcmp(argv[i], "--release") == 0)
		{
			flag_release = true;
		}
		else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-g") == 0)
		{
			flag_debug = true;
		}
		else if (strcmp(argv[i], "--jit") == 0)
		{
			flag_jit = true;
		}
		else if (strcmp(argv[i], "-o") == 0)
		{
			i++;
			if (i < argc) output_bc_path = argv[i];
		}
		else if (strcmp(argv[i], "--dump-ast") == 0)
		{
			flag_dump_ast = true;
		}
		else if (strcmp(argv[i], "--dump-ir") == 0)
		{
			flag_dump_ir = true;
		}
		else if (strcmp(argv[i], "--vm") == 0)
		{
			flag_vm = true;
		}
		else if (strcmp(argv[i], "--trace-vm") == 0)
		{
			flag_vm = true;
			flag_trace_vm = true;
		}
		else if (strcmp(argv[i], "--view") == 0)
		{
			flag_view = true;
		}
		else if (strcmp(argv[i], "--trace-parser") == 0)
		{
			print_parse_log = 1;
		}
		else if (strcmp(argv[i], "--stats") == 0)
		{
			flag_stats = true;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
		{
			printf("xlang 0.4.0 - Language & Runtime\n");
			printf("Usage: xlang [options] <script.xb | bytecode.xbc> [args...]\n");
			printf("       xlang build [--debug|--release] <script.xb> [-o <binary>]\n\n");
			printf("Options:\n");
			printf("  -c, --compile   Compile script to bytecode (.xbc)\n");
			printf("  -o <file>       Specify output file path (bytecode, LLVM IR, or binary)\n");
			printf("  --emit-llvm, -S Emit textual LLVM IR (.ll) for script\n");
			printf("  build           Compile script to standalone native binary via LLVM\n");
			printf("  --debug, -g     Build debug binary with assertions and debug symbols (default)\n");
			printf("  --release       Build optimized release binary with assertions elided\n");
			printf("  --jit           Execute script via in-process LLVM ORC JIT\n");
			printf("  --dump-ast      Parse script and display Structured AST\n");
			printf("  --dump-ir       Compile/load bytecode and disassemble\n");
			printf("  --vm            Execute script using the Bytecode Virtual Machine\n");
			printf("  --trace-vm      Execute in VM with instruction execution tracing\n");
			printf("  --trace-parser  Trace lexer/parser token stream\n");
			printf("  --stats         Display execution timing and memory statistics\n");
			printf("  --view          Generate side-by-side source/IR HTML viewer\n");
			printf("  -h, --help      Show this help message\n\n");
			clean_memory();
			return 0;
		}
		else if (argv[i][0] != '-' && script_path == NULL)
		{
			script_path = argv[i];
			script_idx = i;
		}
	}

	if (flag_release)
		set_assert_enabled(false);
	else
		set_assert_enabled(true);

	if (script_path == NULL)
	{
		xdiag_set_current_file("<stdin>");
		interupter();
		clean_memory();
		return 0;
	}

	if (argc > script_idx + 1)
	{
		g_script_argc = argc - script_idx - 1;
		g_script_argv = argv + script_idx + 1;
	}
	else
	{
		g_script_argc = 0;
		g_script_argv = NULL;
	}

	/* Check if executing pre-compiled bytecode (.xbc) */
	if (is_xbc_file(script_path))
	{
		XIrChunk chunk;
		xir_chunk_init(&chunk);
		if (!xir_load_file(&chunk, script_path))
		{
			fprintf(stderr, "Error: Failed to load bytecode file '%s'\n", script_path);
			clean_memory();
			return 1;
		}

		if (flag_dump_ir)
		{
			xir_disassemble_chunk(&chunk, script_path);
			xir_chunk_free(&chunk);
			clean_memory();
			return 0;
		}

		XVm vm;
		xvm_init(&vm);
		vm.print_trace = flag_trace_vm;
		XVmResult res = xvm_run(&vm, &chunk);
		xvm_free(&vm);
		xir_chunk_free(&chunk);
		clean_memory();
		return (res == VM_OK) ? 0 : 1;
	}

	FILE* code_file = NULL;
	errno_t code_file_status = fopen_s(&code_file, script_path, "r");
	if (code_file_status != 0)
	{
		printf("[%d]-file [%s] not found..\n", code_file_status, script_path);
		exit(-1);
	}
	buff = get_file_buffer(code_file);
	fclose(code_file);
	xdiag_set_current_file(script_path);
	xdiag_set_source_code(buff);

	if (flag_dump_ast || flag_dump_ir || flag_vm || flag_compile || flag_view || flag_emit_llvm || flag_build || flag_jit)
	{
		print_parse_log = 0;
	}

	if (flag_compile)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}
		XIrChunk chunk;
		xir_chunk_init(&chunk);
		xir_compile_program(prog, &chunk);

		char out_path_buf[512] = {0};
		const char* target_out = output_bc_path;
		if (!target_out)
		{
			snprintf(out_path_buf, sizeof(out_path_buf), "%s", script_path);
			char* dot = strrchr(out_path_buf, '.');
			if (dot) strcpy(dot, ".xbc");
			else strcat(out_path_buf, ".xbc");
			target_out = out_path_buf;
		}

		bool ok = xir_save_file(&chunk, target_out);
		if (ok)
		{
			printf("Compiled '%s' -> '%s' (%d bytes of bytecode)\n", script_path, target_out, chunk.count);
		}
		else
		{
			fprintf(stderr, "Failed to write bytecode to '%s'\n", target_out);
		}
		xir_chunk_free(&chunk);
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return ok ? 0 : 1;
	}

	if (flag_emit_llvm)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}

		char out_path_buf[512] = {0};
		const char* target_out = output_bc_path;
		if (!target_out)
		{
			snprintf(out_path_buf, sizeof(out_path_buf), "%s", script_path);
			char* dot = strrchr(out_path_buf, '.');
			if (dot) strcpy(dot, ".ll");
			else strcat(out_path_buf, ".ll");
			target_out = out_path_buf;
		}

		XLLVMConfig cfg = xllvm_default_config();
		if (flag_release)
		{
			cfg.is_release = true;
			cfg.enable_asserts = false;
		}
		else if (flag_debug)
		{
			cfg.is_release = false;
			cfg.enable_asserts = true;
		}
		bool ok = xllvm_emit_file(prog, script_path, target_out, &cfg);
		if (ok)
		{
			printf("Emitted LLVM IR '%s' -> '%s'\n", script_path, target_out);
		}
		else
		{
			fprintf(stderr, "Failed to write LLVM IR to '%s'\n", target_out);
		}
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return ok ? 0 : 1;
	}

	if (flag_build)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}

		char ll_path[512] = {0};
		snprintf(ll_path, sizeof(ll_path), "%s.ll", script_path);

		char bin_path_buf[512] = {0};
		const char* bin_target = output_bc_path;
		if (!bin_target)
		{
			snprintf(bin_path_buf, sizeof(bin_path_buf), "%s", script_path);
			char* dot = strrchr(bin_path_buf, '.');
			if (dot) *dot = '\0';
			bin_target = bin_path_buf;
		}

		bool is_release_build = flag_release;
		XLLVMConfig cfg = xllvm_default_config();
		cfg.is_release = is_release_build;
		cfg.enable_asserts = !is_release_build;

		bool ok = xllvm_emit_file(prog, script_path, ll_path, &cfg);
		if (!ok)
		{
			fprintf(stderr, "Error: Failed to emit intermediate LLVM IR to '%s'\n", ll_path);
			ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}

		int clang_avail = system("which clang > /dev/null 2>&1");
		if (clang_avail != 0)
		{
			printf("Emitted LLVM IR: %s\n", ll_path);
			fprintf(stderr, "Note: 'clang' compiler not found in PATH to assemble native binary '%s'.\n", bin_target);
			fprintf(stderr, "You can compile the generated LLVM IR manually using:\n");
			fprintf(stderr, "  clang -O2 %s -lm -o %s\n", ll_path, bin_target);
			ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}

		const char* rt_obj = NULL;
		if (is_release_build)
		{
			if (access("obj/Release/xllvm_rt.o", R_OK) == 0)
				rt_obj = "obj/Release/xllvm_rt.o";
			else if (access("bin/Release/xllvm_rt.o", R_OK) == 0)
				rt_obj = "bin/Release/xllvm_rt.o";
			else if (access("/usr/local/lib/xlang/xllvm_rt.o", R_OK) == 0)
				rt_obj = "/usr/local/lib/xlang/xllvm_rt.o";
			else if (access("obj/Debug/xllvm_rt.o", R_OK) == 0)
				rt_obj = "obj/Debug/xllvm_rt.o";
			else if (access("bin/Debug/xllvm_rt.o", R_OK) == 0)
				rt_obj = "bin/Debug/xllvm_rt.o";
		}
		else
		{
			if (access("obj/Debug/xllvm_rt.o", R_OK) == 0)
				rt_obj = "obj/Debug/xllvm_rt.o";
			else if (access("bin/Debug/xllvm_rt.o", R_OK) == 0)
				rt_obj = "bin/Debug/xllvm_rt.o";
			else if (access("obj/Release/xllvm_rt.o", R_OK) == 0)
				rt_obj = "obj/Release/xllvm_rt.o";
			else if (access("bin/Release/xllvm_rt.o", R_OK) == 0)
				rt_obj = "bin/Release/xllvm_rt.o";
			else if (access("/usr/local/lib/xlang/xllvm_rt.o", R_OK) == 0)
				rt_obj = "/usr/local/lib/xlang/xllvm_rt.o";
		}

		const char* opt_flags = is_release_build ? "-O2 -s" : "-g -O0";
		char compile_cmd[1280];
		if (rt_obj)
			snprintf(compile_cmd, sizeof(compile_cmd), "clang %s -Wno-override-module \"%s\" \"%s\" -L. -lpcre -lm -o \"%s\"", opt_flags, ll_path, rt_obj, bin_target);
		else
			snprintf(compile_cmd, sizeof(compile_cmd), "clang %s -Wno-override-module \"%s\" -L. -lpcre -lm -o \"%s\"", opt_flags, ll_path, bin_target);
		int compile_res = system(compile_cmd);
		if (compile_res == 0)
		{
			printf("Successfully built native %s binary: %s\n", is_release_build ? "release" : "debug", bin_target);
		}
		else
		{
			fprintf(stderr, "Error: Native compilation command failed with exit code %d\n", compile_res);
		}

		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return (compile_res == 0) ? 0 : 1;
	}

	if (flag_dump_ast)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}
		xast_dump_program(prog);
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return 0;
	}

	if (flag_dump_ir)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (prog == NULL || xdiag_get_error_count() > 0)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}
		XIrChunk chunk;
		xir_chunk_init(&chunk);
		xir_compile_program(prog, &chunk);
		xir_disassemble_chunk(&chunk, script_path);
		xir_chunk_free(&chunk);
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return 0;
	}

	if (flag_jit && !flag_vm)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}

		int ret = xllvm_jit_run_program(prog, script_path, g_script_argc, g_script_argv);
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return ret;
	}

	if (flag_view)
	{
		xdiag_reset_error_count();
		AstProgram* prog = xast_parse_source(buff, script_path);
		if (xdiag_get_error_count() > 0 || prog == NULL)
		{
			if (prog) ast_program_destroy(prog);
			free(buff);
			clean_memory();
			return 1;
		}
		XIrChunk chunk;
		xir_chunk_init(&chunk);
		xir_compile_program(prog, &chunk);

		/* Build output HTML path: replace .xb extension with .ir.html */
		char html_path[1024] = {0};
		snprintf(html_path, sizeof(html_path), "%s", script_path);
		char* dot = strrchr(html_path, '.');
		if (dot) strcpy(dot, ".ir.html");
		else strcat(html_path, ".ir.html");

		bool ok = xdis_html_write(&chunk, script_path, html_path);
		if (ok)
		{
			printf("IR viewer written to: %s\n", html_path);
#if defined(_WIN32) || defined(_MSC_VER)
			char open_cmd[1280];
			snprintf(open_cmd, sizeof(open_cmd), "start \"\" \"%s\"", html_path);
			system(open_cmd);
#elif defined(__APPLE__)
			char open_cmd[1280];
			snprintf(open_cmd, sizeof(open_cmd), "open \"%s\"", html_path);
			system(open_cmd);
#else
			char open_cmd[1280];
			snprintf(open_cmd, sizeof(open_cmd), "xdg-open \"%s\" &", html_path);
			system(open_cmd);
#endif
		}
		else
		{
			fprintf(stderr, "Failed to write IR viewer HTML to '%s'\n", html_path);
		}

		xir_chunk_free(&chunk);
		ast_program_destroy(prog);
		free(buff);
		clean_memory();
		return ok ? 0 : 1;
	}

	/* Standard execution via Virtual Machine */
	change_dir(argv + script_idx - 1);

	xdiag_reset_error_count();
	AstProgram* prog = xast_parse_source(buff, script_path);
	if (xdiag_get_error_count() > 0 || prog == NULL)
	{
		if (prog) ast_program_destroy(prog);
		free(saved_code_file_path);
		free(buff);
		clean_memory();
		return 1;
	}

	XIrChunk chunk;
	xir_chunk_init(&chunk);
	xir_compile_program(prog, &chunk);

	XVm vm;
	xvm_init(&vm);
	vm.print_trace = flag_trace_vm;

	XLLVMJit* jit = NULL;
	if (flag_jit)
	{
		jit = xllvm_jit_create();
		if (jit)
		{
			if (xllvm_jit_add_program(jit, prog, script_path))
			{
				xvm_enable_jit(&vm, 20);
				vm.jit_engine = jit;
				xllvm_jit_bind_vm(jit, &vm, prog);
			}
			else
			{
				xllvm_jit_free(jit);
				jit = NULL;
			}
		}
	}

	XVmResult res = xvm_run(&vm, &chunk);
	if (jit) xllvm_jit_free(jit);
	xvm_free(&vm);
	xir_chunk_free(&chunk);
	ast_program_destroy(prog);

	t = clock() - t;
	if (flag_stats)
	{
		const double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds
		printf("\ntook %f seconds to execute \n", time_taken);
	}

	free(saved_code_file_path);
	free(buff);
	clean_memory();
	return (res == VM_OK) ? 0 : 1;
}




void clean_memory(void)
{
	var_clean_stack(varss);
	var_clean_stack(t_varss);
	func_clean_stack(funcs);
	func_clean_stack(t_funcs);
	type_clean_stack(types);
	if (varss != &var_start_stack)
		free(varss);
	if (funcs != &base_function)
		free(funcs);
	if (types != &simple_type_stack)
		free(types);
	free(t_varss);
	free(t_funcs);
	x_collections_cleanup();
	x_import_cleanup();
	gc_cleanup();
	xdiag_set_source_code(NULL);
	xdiag_reset_error_count();
	if (g_lex_arena != NULL)
	{
		arena_destroy(g_lex_arena);
		g_lex_arena = NULL;
	}
	set_assert_enabled(true);
}

void get_auto_comp(char* input, char** sugg)
{
	//char* y =(char*)malloc(strlen(ys)+1);
	//strset(y,0);
	//strcpy(y,ys);
	//char* u =(char*)malloc(sizeof(char)*124);
	//memset(u,0,124);
	if (input != NULL)
		for (var* i = varss->root; i != NULL; i = i->stack_next)
		{
			if (i->name != NULL && strstr(i->name, input) != 0)
			{
				strcat(*sugg, "var:");
				strcat(*sugg, i->name);
				strcat(*sugg, "\n");
			}
		}
	for (func_deftion* i = funcs->root; i != NULL; i = i->stack_next)
	{
		if (i->func_name != NULL && strstr(i->func_name, input) != 0)
		{
			strcat(*sugg, "function:");
			strcat(*sugg, i->func_name);
			strcat(*sugg, "\n");
		}
	}
	for (type_def* i = types->root; i != NULL; i = i->stack_next)
	{
		if (i->type_name != NULL && strstr(i->type_name, input) != 0)
		{
			strcat(*sugg, "type:");
			strcat(*sugg, i->type_name);
			strcat(*sugg, "\n");
		}
	}
	//char * ux =(char*)malloc(strlen(u)+1);
	//strset(ux,0);
	//strcpy(ux,u);
	//free(u);
}

void getsavedMD5(FILE* cf)
{
	if (cf != NULL)
		fread(saved_md5, 16, 1, cf);
}
