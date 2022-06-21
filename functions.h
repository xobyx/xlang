#pragma once
#include "xlang_main.h"
#if defined(__GNUC__)|| defined(__MINGW64__)
#define strcat_s(x,y,z) strcat(x,z)
#endif

/*XLANG*/extern type_def SIMPLE_TYPE[];
//#define T(O) get_type_by_name((char*)O)



/*XLANG*/ void set_value_copy_var(var* dstn, var* scr);
/*XLANG*/ void set_value_copy_node(var* dstn, node* scr);

//#define setvaluefortype(mvar,)
/*XLANG*/ char* get_file_buffer(FILE* sf);


extern fl staic_flag2[] ;
extern int fi;

//const static char* sbase_types_name [] = {"long","string","char","int","bool","float","_array","new"};
//

#define T_LONG (SIMPLE_TYPE)
#define T_STRING (SIMPLE_TYPE+1)
#define T_CHAR (SIMPLE_TYPE+2)
#define T_INT (SIMPLE_TYPE+3)
#define T_BOOL (SIMPLE_TYPE+4)
#define T_FLOAT (SIMPLE_TYPE+5)
#define T_ARRAY (SIMPLE_TYPE+6)
#define T_OBJECT  (SIMPLE_TYPE+7)
#define T_ANY  (SIMPLE_TYPE+8)
#define T_FUNC  (SIMPLE_TYPE+9)
#define T_TYPE_INFO (SIMPLE_TYPE+10)
extern func_stack base_function;

/*XLANG*/ var* get_type_inc_obj_var(node** nop, var* context, func_deftion ** outp);
int eql(const char* n,const char* x);

/*XLANG*/ bool static_flag_check2x(node_type* m);
/*XLANG*/ node* static_flag_op2(node_type v,node* n, bool added);
/*XLANG*/ node* get_root(node* j);

void instance_type(type_def* src, void* dstn, int size);
void copy_object(struct type_instance* src, void* dstn, int size);
node* get_first_type_with_value(node* in, node_type b, void* value);
node* get_first_type(node* in, node_type b);
node* get_first_type_backword_from(node* in, node_type b);
node* get_last_type(node* in, node_type b);
void set_value(var* context, fcall* temp, node** cx);
//void fprintf(func* temp);



var* new_var(char* name, type_def* vtype);
var* new_temp_var(type_def* typ);
func_deftion* new_func();
type_def* new_type();
func_deftion * get_obj_function(var * a,char* name);
func_deftion* get_func_by_name(char* name);
///XLANGC type* get_type_by_name(char* name);
void install_default_functions();
void install_default_types();

fcall* create_fcall(func_deftion* i);
var * get_globle_var_by_name(char* name);
var * get_var_by_name_on_stack(char* name,var_stack* y);
var* fget_var_by_name_fc(char* name,fcall* y);
var* all_get_var_by_name(char* name, fcall * called_function, var* called_var);
void* install_memory_with_type(type_def * tc,const int s);
void* install_memory(var * n);

node* get_close_part(node* t);

/*XLANG*/ char** get_pptr_string(char* t);
fl* static_flag_check2();
void call_func_in(fcall* d);
void print(fcall* temp);

bool is_base_type(type_def* t);


func_deftion* add_function_gloable(char* name, type_def* return_type, int pcount, const function_node fe, char** par_name,
                           type_def** par_type);
/*XLANG*/ func_deftion* add_function(func_stack* s, char* name, type_def* return_type, int pcount, const function_node fe, char** par_name,
                   type_def** par_type);

#define S(X, B) strcat(X,B)
char* parse_obj_to_str(node_type t);
void scap_string(char* m);


bool stop_here(node_type stop_in_type, node* stop_in_node, node* mnode);
func_deftion* get_obj_function2(var* object_var, char* name);
var* get_array_item(var * name, int index);
void step(node** nod);
bool copy_array(var* out, void* out_memory, var* src);
bool is_double_oprater(node* mnode);
bool is_double_equle(node* mnode);
int get_index_value(fcall* funct, node* k);
bool eat(node** nod,enum node_type_enum next,bool must);
