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

void interupter(void)
{
	bool unclosed = false;

	node_type which_type = none;
	char txt[1024 * 5];
	while (true)
	{
		memset(txt, 0, 1024 * 5);

		if (unclosed)
		{
			printf("\n...");
			if (fgets(txt, 500, stdin) == NULL)
				break;
		}
		else
		{
			printf("\n>>>");
			if (fgets(txt, 500, stdin) == NULL)
				break;
		}


		start_parse_lines(txt, true);
		unclosed = static_flag_check2x(&which_type);
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
