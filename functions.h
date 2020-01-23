#pragma once
#include "xlang_main.h"


/*XLANG*/ type SIMPLE_TYPE[8];
//#define T(O) get_type_by_name((char*)O)



/*XLANG*/ void set_value_copy_var(var* dstn, var* scr);
/*XLANG*/ void set_value_copy_node(var* dstn, node* scr);

//#define setvaluefortype(mvar,)
/*XLANG*/ char* get_filebuff(FILE* sf);





//const static char* sbase_types_name [] = {"long","string","char","int","bool","float","_array","new"};
#define T_LONG (SIMPLE_TYPE)
#define T_STRING (SIMPLE_TYPE+1)
#define T_CHAR (SIMPLE_TYPE+2)
#define T_INT (SIMPLE_TYPE+3)
#define T_BOOL (SIMPLE_TYPE+4)
#define T_FLOAT (SIMPLE_TYPE+5)
#define T_ARRAY (SIMPLE_TYPE+6)
#define T_NEW  (SIMPLE_TYPE+7)

func_stack base_function;

/*XLANG*/ var* get_obj_var(node** nop, var* context, func** outp);
int eql(const char* n,const char* x);

/*XLANG*/ bool static_flag_check2x(node_type* m);
/*XLANG*/ node* static_flag_op2(node_type v,node* n, bool added);
/*XLANG*/ node* getRoot(node* j);
/*XLANG*/ type* copy_type_a(type* src, bool func_cpy);
void copy_type(type* src,type* dstn ,bool func_cpy);
node* getFirstType_with_value(node* in, node_type b, void* value);
node* getFirstType(node* in, node_type b);
node* getFirstType_backword_from(node* in, node_type b);
node* getLastType(node* in, node_type b);
void set_value(var* context, func* temp, node** cx);
//void fprintf(func* temp);
bool var_bool_value(var* m);


var* new_var(char* name, type* vtype);
var* new_temp_var(type* typ);
func* new_func();
type* new_type();
func * get_func_by_name_with_var(var * a,char* name);
func * get_func_by_name(char* name);
///XLANGC type* get_type_by_name(char* name);
void install_default_functions();
void install_default_types();
void get_var_value(func* temp, node* c, var** m);
func* copy_func(func* i);
var * get_var_by_name(char* name);
var * fget_var_by_name(var_stack* y,char* name);
//var* sfget_var_by_name(var_stack* y, char* name,int* out);
void* install_memory_with_type(type * tc,int s);
void* install_memory(var * n);
node* calculate(node* m, func* funct, var* contxt, node_type stop, node* end, var* out);
node* calculate4(var* ms, node* m, node_type z, func* km);
node* calculate3(var* ms, node* m, func* km);

node* get_close_part(node* t);

/*XLANG*/ char** get_pptr_string(char* t);
fl* static_flag_check2();
void call_func_in(func* d);
void print(func* temp);

bool is_base_type(type* t);


func* add_function_gloable(char* name, type* return_type, int pcount, const def_function fe, char** par_name,
                           type** par_type);
/*XLANG*/ func* add_function(func_stack* s, char* name, type* return_type, int pcount, const def_function fe, char** par_name,
                   type** par_type);

#define S(X, B) strcat(X,B)
char* parse_obj_to_str(node_type t);
void scap_string(char* m);


