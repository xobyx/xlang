// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
/*#ifdef _DEBUG

__pragma(warning(disable: 4996));
#define _CRT_SECURE_NO_WARNINGS 1

#endif
*/
//#define _CRTDBG_MAP_ALLOC
//#define DEBUG_P 1
//#include <stdlib.h>  
//#include <crtdbg.h>
#define PCRE_STATIC 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcre.h"
#include "types.h"
#include "stack.h"
#include "var_stack.h"
#include "func_stack.h"
#include "type_stack.h"
#include "debuge.h"
#include "functions.h"
#include "compile.h"
#include "parse.h"
#include "md5.h"

/**
 * \brief 
 * \param r file name
 * \param a full stack
 * \param MD5_hash 
 */


#define dont_compile 1
extern void save_file(char* r,node_stack* a,unsigned int MD5_hash[4]);
extern void read_file_parse( FILE* f,node_stack* nodes);
extern  node_stack * nodes;
extern   var_stack * varss;
extern var_stack * t_varss;
extern func_stack * funcs;
extern struct Debug  *debuge;
extern type_stack * types;
extern func_stack* t_funcs;
/*XLANG*/ void int_xlang();
/*XLANG*/  void   get_auto_comp(char* y, char** u);
	// TODO: reference additional headers your program requires here

extern int print_parse_log;