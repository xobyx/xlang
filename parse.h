#pragma once
#include "xlang_main.h"

/*XLANG*/  void pre_parse_line(char* ts, int linex);
/*XLANG*/ void start_parse_lines(char * text, bool interactive);
/*XLANG*/ char**   get_lines_array(char * subject);