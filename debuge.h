#pragma once
#include "types.h"
#if defined (_MSC_VER)
#include <windows.h>
#endif

typedef struct parse_obj_dict
{
	char* name;

	i16 value;
	char* mvalue;
} parse_obj_dict;

typedef union bit8_color
{
	byte bf_color;

	struct
	{
		byte foreground:4;
		byte background:4;
	} bf;
} bit8_color;

typedef struct y
{
	bit8_color color;
	node* m;
	short t;
} y;
typedef struct Debug Debug;
typedef void (*print_line_debuge_)(Debug* x, node* bx, int line);
typedef void (*do_work_)(Debug* x, node* temp);
typedef void (*test_color_)(Debug* x);
typedef int (*cprintf_)(Debug* x,byte color, const char* format,...);
typedef void (*addnode_)(Debug* x, node* y);
typedef void (*checknode_)(Debug* x, node* y);

typedef struct Debug
{
#if defined (_MSC_VER)
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	HANDLE hConsole;
	WORD wOldColorAttrs;
#endif



	node* save;
	parse_obj_dict list[16];

	y stack[10];

	print_line_debuge_ print_line_debuge;
	do_work_ do_work;
	test_color_ test_color;

	cprintf_ cprintf;
	addnode_ addnode;
	checknode_ checknode;


	int color_index;
	int t;
} Debug;


Debug* init_debug();
