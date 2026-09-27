#pragma once

#define PCRE_STATIC 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcre.h"
#include "types.h"
#include "var_stack.h"
#include "func_stack.h"
#include "type_stack.h"
#include "functions.h"
#include "md5.h"
#include "xdiag.h"
#include "lexer.h"
#include "xast_parser.h"

extern var_stack * varss;
extern var_stack * t_varss;
extern func_stack * funcs;
extern type_stack * types;
extern func_stack* t_funcs;

void int_xlang(void);
void get_auto_comp(char* y, char** u);

extern int print_parse_log;