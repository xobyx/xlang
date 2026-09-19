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
#include "parse.h"
#include <ctype.h>
clock_t t;
//C:\Tests\t.xb
bool load_saved_code = false;
node_stack* nodes;
var_stack* varss;
var_stack* t_varss;
func_stack* funcs;
type_stack* types;
func_stack* t_funcs;
Debug * debuge;
bool read_file = true;
bool save_code = false;
int print_parse_log = 1;

#define STR_VALUE(val) #val


void start_compile(void);

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
	nodes = (node_stack*)malloc(sizeof(node_stack));
	varss = &var_start_stack;
	funcs = &base_function;
	types = &simple_type_stack;

	t_varss = (var_stack*)malloc(sizeof(var_stack));
	t_funcs = (func_stack*)malloc(sizeof(func_stack));
	debuge = init_debug();
	stack_init(nodes);
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
	printf("  \033[1m:ast\033[0m              Toggle AST debug parsing output (currently: %s)\n",
	       print_parse_log ? "\033[32mON\033[0m" : "\033[31mOFF\033[0m");
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
	node_type which_type = none;
	char line_buf[1024 * 5];

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
			if (strcmp(trimmed, ":ast") == 0)
			{
				print_parse_log = !print_parse_log;
				printf("AST debug parsing log: %s\n", print_parse_log ? "ENABLED" : "DISABLED");
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
				parser_delim_clear(NULL);
				unclosed = false;
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
				parser_delim_clear(NULL);
				unclosed = false;
				printf("(multi-line block canceled)\n");
				continue;
			}
		}

		if (!unclosed && !repl_is_statement(trimmed))
		{
			char eval_buf[1024 * 5 + 32];
			snprintf(eval_buf, sizeof(eval_buf), "print(%s)\n", trimmed);
			start_parse_lines(eval_buf, true);
			unclosed = static_flag_check2x(&which_type);
			if (unclosed)
			{
				parser_delim_clear(NULL);
				unclosed = false;
				start_parse_lines(line_buf, true);
				unclosed = static_flag_check2x(&which_type);
			}
		}
		else
		{
			start_parse_lines(line_buf, true);
			unclosed = static_flag_check2x(&which_type);
		}
	}
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

	if (argc > 2)
	{
		g_script_argc = argc - 2;
		g_script_argv = argv + 2;
	}
	else
	{
		g_script_argc = 0;
		g_script_argv = NULL;
	}

	FILE* code_file = NULL;
	FILE* saved_code_file = NULL;

	errno_t code_file_status = fopen_s(&code_file, argv[1], "r");
	errno_t saved_code_file_status;
	if (code_file_status != 0)
	{
		printf("[%d]-file [%s] not found..\n", code_file_status, argv[1]);
		exit(-1);
	}
	buff = get_file_buffer(code_file);
	fclose(code_file);
	xdiag_set_current_file(argv[1]);
	xdiag_set_source_code(buff);

	change_dir(argv);




	if (load_saved_code)
	{
		saved_code_file_path = (char*)malloc(strlen(argv[1]) + 3 + 1);
		saved_code_file_path = strcpy(saved_code_file_path, argv[1]);
		strcat(saved_code_file_path, "cxx");
		printf("saved to %s\n", saved_code_file_path);
		saved_code_file_status = fopen_s(&saved_code_file, saved_code_file_path, "rb");
		printf("found saved to %d\n", saved_code_file_status);
		////
		if (saved_code_file_status == 0)
		{
			calc_md5(buff, file_md5);
			getsavedMD5(saved_code_file);
			if (file_md5[0] == saved_md5[0])
			{
				printf("from file : %s\n", saved_code_file_path);
				read_file_parse(saved_code_file, nodes);
			}
			else
			{
				fclose(saved_code_file);
				start_compile();
			}
			fclose(saved_code_file);
		}
	}
	else
	{
		start_compile();
	}
	if (save_code)
	{
		save_file(saved_code_file_path, nodes, file_md5);  /////save

	}

	t = clock() - t;
	const double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds

	printf("\ntook %f seconds to execute \n", time_taken);
	printf("\nvar num: %d , temp var num: %d\n", varss->size, t_varss->size);



	//getchar();
	free(saved_code_file_path);
	free(buff);
	clean_memory();
	return 0;
}




void clean_memory(void)
{
	clean_stack(nodes);
	var_clean_stack(varss);
	var_clean_stack(t_varss);
	func_clean_stack(funcs);
	func_clean_stack(t_funcs);
	type_clean_stack(types);
	free(nodes);
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
	parser_interactive_cleanup();
	xdiag_set_source_code(NULL);
}

void start_compile(void)
{
	clock_t t2 = clock();
	start_parse_lines(buff, false);
	t2 = clock() - t2;
	double time_taken = ((double)t2) / CLOCKS_PER_SEC; // in seconds

	printf("\nstart_parse_lines took %f seconds to execute \n", time_taken);






	//	free(buff);
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
