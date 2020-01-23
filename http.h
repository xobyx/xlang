#pragma once
#include "xlang_main.h"

static char* s_parm_name[4] = {"a","b","c","d"};
static type* s_parm_type[4] = {{"string"}};

char* getHttp(char* method,char* link ,int size ,char** mHeaders);
struct URL
	{
		char ip[100];
        int port;
        char page[200];

	}static IKK;


void http(func* s);