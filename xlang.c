/*              _                    
               | |                   
__  __   ___   | |__    _   _  __  __
\ \/ /  / _ \  | '_ \  | | | | \ \/ /
 >  <  | (_) | | |_) | | |_| |  >  < 
/_/\_\  \___/  |_.__/   \__, | /_/\_\
                         __/ |       
                        |___/        */
#include "xlang_main.h"
#include <direct.h>
#include <time.h>
clock_t t;
//C:\Tests\t.xb

node_stack* nodes;
var_stack* varss;
var_stack* t_varss;
func_stack* funcs;
type_stack* types;
func_stack* t_funcs;
Debug * debuge;
bool read_file = true;
static char* diro;

#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)

char* g = STR_VALUE("int") "ds";
#define PATH_LEN 256
#define MD5_LEN 32
void start_compile();

void getsavedMD5(FILE* cf);
unsigned int calc_md5[4];
unsigned int saved_md5[4];
char* buff;
char* comp;

int GetDir(char* fullPath, char* dir)
{
	
	const int buffSize = 1024;

	char buff[1024] = {0};
	int buffCounter = 0;
	int dirSymbolCounter = 0;

	for (unsigned int i = 0; i < strlen(fullPath); i++)
	{
		if (fullPath[i] != L'\\')
		{
			if (buffCounter < buffSize) buff[buffCounter++] = fullPath[i];
			else return -1;
		}
		else
		{
			for (int i2 = 0; i2 < buffCounter; i2++)
			{
				dir[dirSymbolCounter++] = buff[i2];
				buff[i2] = 0;
			}

			dir[dirSymbolCounter++] = fullPath[i];
			buffCounter = 0;
		}
	}

	return dirSymbolCounter;
}


void int_xlang()
{
	nodes = (node_stack*)malloc(sizeof(node_stack));
	varss = (var_stack*)malloc(sizeof(var_stack));
	funcs = (func_stack*)malloc(sizeof(func_stack));
	types = (type_stack*)malloc(sizeof(type_stack));

	t_varss = (var_stack*)malloc(sizeof(var_stack));
	t_funcs = (func_stack*)malloc(sizeof(func_stack));
	debuge = init_debug();
	stack_init(nodes);
	var_stack_init(varss);
	var_stack_init(t_varss);
	func_stack_init(funcs);
	func_stack_init(t_funcs);
	type_stack_init(types);

	
	install_default_types();
	install_default_functions();
}

bool b;

#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
void change_dir(char** argv)
{
	
	char* dir = (char*)malloc(1024);
	memset(dir, 0, 1024);
	GetDir(argv[1], dir);
	_chdir(dir);
	free(dir);
}

void clean_memory();

int main(const int argc, char** argv)
{
	int_xlang();
	t = clock();
	if (argc == 1)
	{
		bool v = false;
		int x = 0;
		node_type mk = (node_type)0;
		char a[1024 * 5];
		char* m = a;
		while (true)
		{
			memset(a, 0, 1024 * 5);

			if (v)
			{
				printf("\n");
				//gets(a);		
				gets_s(a,500);
			}
			else
			{
				printf("\n>>>");
				gets_s(a,500);
			}


			start_parse_lines(a, true);
			v = static_flag_check2x(&mk);
		}

		return 0;
	}
	if (argc == 3)
		b = false;
	//find_all = 1;
	FILE* sf = NULL;
	FILE* cf = NULL;

	change_dir(argv);


	errno_t se = fopen_s(&sf, argv[1], "r");

	buff = get_filebuff(sf);
	if (sf)
		fclose(sf);
	maintxt(buff, calc_md5);
#ifndef DEBUG_P
	char* out=(char*)malloc(strlen(argv[1])+3+1);
	comp = strcpy(out,argv[1]);
	strcat(out,"cxx");
	errno_t ce =fopen_s(&cf, comp, "rb");
#else
	errno_t ce = 1;
#endif


	if (ce == 0)
	{
		getsavedMD5(cf);
		if (calc_md5[0] == saved_md5[0])
		{
			printf("from file : %s\n", comp);
			read_file_parse(cf, nodes);
		}
		else
		{
			fclose(cf);

			start_compile();
		}
	}
	else
	{
		start_compile();
	}
	if (cf)
		fclose(cf);

	t = clock() - t;
	double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds

	printf("\ntook %f seconds to execute \n", time_taken);
	printf("\nvar num: %d , temp var num: %d", varss->size, t_varss->size);
	//clean_memory();
	char tt = getchar();
	char trt = getchar();
	free(buff);
	//	_CrtDumpMemoryLeaks();
}




void clean_memory()
{
	clean_stack(nodes);
	var_clean_stack(varss);
	func_clean_stack(funcs);
	type_clean_stack(types);
	free(nodes);
	free(varss);
	free(funcs);
	free(types);
}

void start_compile()
{
	clock_t t2 = clock();
	start_parse_lines(buff, false);
	t2 = clock() - t2;
	double time_taken = ((double)t2) / CLOCKS_PER_SEC; // in seconds

	printf("\nstart_parse_lines took %f seconds to execute \n", time_taken);
	get_auto_comp("xran");
#ifndef DEBUG_P
	save_file(comp, nodes, calc_md5);  /////save

#endif


	//	free(buff);
}

void get_auto_comp(char* y, char** u)
{
	//char* y =(char*)malloc(strlen(ys)+1);
	//strset(y,0);
	//strcpy(y,ys);
	//char* u =(char*)malloc(sizeof(char)*124);
	//memset(u,0,124);
	if (y != NULL) 
		for (var* i = varss->root; i != NULL; i = i->stack_next)
		{
			if (i->name != NULL && strstr(i->name, y) != 0)
			{
				strcat(*u, "var:");
				strcat(*u, i->name);
				strcat(*u, "\n");
			}
		}
	for (func* i = funcs->root; i != NULL; i = i->stack_next)
	{
		if (i->func_name != NULL && strstr(i->func_name, y) != 0)
		{
			strcat(*u, "function:");
			strcat(*u, i->func_name);
			strcat(*u, "\n");
		}
	}
	//char * ux =(char*)malloc(strlen(u)+1);
	//strset(ux,0);
	//strcpy(ux,u);
	//free(u);
}

void getsavedMD5(FILE* cf)
{
	if(cf != NULL)
	fread(saved_md5, 16, 1, cf);
}
