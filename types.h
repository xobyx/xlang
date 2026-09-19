#pragma once
#ifndef TYPES_H_
#define TYPES_H_

#if defined (_MSC_VER)
#include <windows.h>
#endif


#include <stdbool.h>
#include <stdio.h>
#ifndef byte
#define byte unsigned char
#endif

struct func_deftion;
struct fcall;
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
typedef void(*function_node)(struct fcall*);
#define t_long 0
#define t_string 1
#define t_char 2
#define t_int 3
#define t_bool 4
#define t_float 5
#define t_array 6

//static const char* base_types_name[] = { "long", "string", "char", "int", "bool", "float", "_array", "new" };

struct type_def;


extern const char* key_word[];

typedef enum key_word_enum
{
	_if_ = 0,
	_for_,
	_while_,
	_do_,
	_else_,
	_eif_,
	_return_,
	_break_,
	_class_,
	_static_,
	_import_
} key_word_enum;

// long[0],string[1],char[2],int[3]
typedef struct type_stack
{
	struct type_def* top;
	struct type_def* root;
	int size;
}type_stack;

typedef unsigned short i16;

/**
 * \brief fffsdsd
 */
typedef enum node_type_enum //: i16
{
	none = 0x0000,
	itype = 0x0001,
	keyword = 0x0002,
	var_name = 0x0004,
	value = 0x0008,
	operators_n = 0x0010,
	equles = 0x0020,
	endl = 0x0040,
	s_index = 0x0080,
	parentheses1 = 0x0100,//{
	parentheses1_c = 0x0200,	// ,
	comma = 0x0400,	// []
	s_index_c = 0x0800,	
	parentheses4 = 0x1000,	//)
	parentheses4_c = 0x2000,	//.
	dot = 0x4000,
	twodot = 0x8000,	//:
	pars = parentheses1 | parentheses1_c | parentheses4 | parentheses4_c | s_index_c | s_index,
	have_var_value = value | var_name | keyword | itype | operators_n,
	a = value | var_name,
	non_one_char = 0x000F

}node_type;



typedef enum var_name_def
{
	function_def = 10,//f
	function_call,//a
	fucnction_parm,//p
	var_def,//
	var_call,//
	class_def,//k
	class_base_def,//x,
	psize,
	pindex,
	var_call_ref
}var_name_def;

typedef enum  function_type
{
	f_main = 0,
	constr = 1,
	class_function = 2
}function_type;

typedef union
{
	i16 value;
	enum node_type_enum name;

	struct
	{
		i16 itype : 1;
		i16 keword : 1;
		i16 var_name : 1;
		i16 value : 1;
		i16 operators_n : 1;
		i16 equles : 1;
		i16 endl : 1;
		i16 s_index : 1; //[
		i16 parentheses1 : 1; //}
		i16 parentheses1c : 1; //{
		i16 comma : 1; //,
		i16 s_index_c : 1; //]
		i16 parentheses4 : 1; //(
		i16 parentheses4c : 1; //(
		i16 dot : 1; //(
		i16 twodot : 1; //(
		//i16 mbool:1;
	} node_type_bit;
}node_type_raw;

#define F 1


typedef struct node
{


	int id;
	node_type_raw btype;
	node_type_raw fflag;
	int line;

	struct node* parent;
	struct  node* next;
	int opt_type;

	union
	{

		void* value_raw;
		struct type_def * value_type;
	//	int * value_int;
	//	short value_short;
		char * value_char_ptr;
	//	char ** value_string;
	//	int  _ptr_;
		key_word_enum value_keyword;

	};

	bool is_flagged;
	struct node* stack_parent;
	struct node* stack_next;
	//void* op; ///used in com if_helper///types in parse///functionn
	struct node* ref_node; ///if __ {[( close;
	struct node* next_jump;
	struct node* root;
	union  // opt
	{

		void* opt_raw;
		struct type_def * opt_type_ptr;
		int * opt_int_ptr;
		char * opt_char_ptr;
		char ** opt_string_ptr;
		int  _opt_ptr_;
		var_name_def opt_name_type;



	};
	bool taked;
}node;

typedef struct node_stack
{
	struct node* top;
	int size;
	struct node* root;
	node ** nlist;
	int max_a;

}node_stack;

enum var_access { PUBLIC = 1, STATIC, PRIVATE };

typedef struct var
{



	char* name;


	union
	{
		int  a;
		void* values;
		struct type_instance * value_type_instsance;
		int * value_int;
		long * value_long;
		float * value_float;
		char * value_char_ptr;
		char ** value_str_ptr;
		bool * value_bool;
		struct func_deftion* value_func;
		struct type_def* value_type;

	};

	struct type_instance * holder;

	struct type_def* type_define;
	struct type_def* base_type;
	int size;
	struct node* ref;
	struct var* stack_next;
	enum var_access access;
}var;
struct type_def;
typedef struct var_stack
{


	struct var* top;
	int size;
	struct var* root;
	struct type_instance* stack_holder;
}var_stack;

typedef struct func_deftion
{
	struct type_def* start_func_parmeters[100];
	char* start_func_parmeters_name[100];
	int start_parm_count;
	function_node func_code;
	char* func_name;
	function_type function_type;
	struct type_def* return_type;

	node* ref;
	enum var_access access;
	//unused
	struct func_deftion* stack_next;
}func_deftion;
typedef struct fcall
{
	var func_parmeters[100];

	int parm_count_c;
	struct func_deftion* deftion;
	struct var _return;
	struct var* context;
	bool has_returned;
}fcall;
typedef struct func_stack
{


	func_deftion* top;
	int size;
	func_deftion* root;
}func_stack;


enum w_type { super = 0, child };

typedef struct type_def
{
	char* type_name;
	var d_propertys[100];
	func_deftion d_functions[100];

	int d_propertys_size;
	int d_function_size;
	int type_id;
	

	struct type_def* base;
	struct type_def* stack_next;
	enum var_access access;
	enum w_type w;


}type_def;

typedef struct type_instance
{
	var* context;
	struct type_def* type;
	struct var_stack propertys;
	func_stack functions;
	struct type_instance* base;
	int size;
}type_instance;

#define type_ btype.value
#define flag_ fflag.value
#define FUNCTION "function"
#define AL_PAR_OP(x) (x)==node_type::parentheses4||(x)==node_type::parentheses4c||(x)==node_type::comma||(x)==node_type::parentheses1||(x)==node_type::parentheses1c||(x)==node_type::s_index||(x)==node_type::s_index_c

#define EMTYE_VALUE_REF(x) x.value=NULL;\
	(x).rev=NULL; \
	(x).next=NULL;\
	(x).isFlag=false;






//char * gramer = "<base_type> [operators|=] if(_O2==operators)[var_name|this] []"

//F parse_obj



#define pgmVar(px) ((var*)(px)->parent->rev)
#define gmVar(x) ((var*)(x).parent->rev)





#define MATH_OPERATORS if(*op=='+')\
                 	*mx = *mx  + to;\
					else if(*op=='-')\
					*mx = *mx  - to;\
					else if(*op=='*')\
					*mx = (*mx)  * (to);\
					else if(*op=='/')\
					*mx = *mx  / to;\
					else if(*op=='%')\
					*mx = *mx  % to;\
					else if(*op=='&'&& *(op+1)=='&')\
					*mx = *mx  && to;\
					else if(*op=='|'&& *(op+1)=='|')\
					*mx = *mx  || to;\
					else if(*op=='&')\
					*mx = *mx  & to;\
					else if(*op=='|')\
					*mx = *mx  | to;\
					else if(*op=='>'&& *(op+1)=='>')\
					*mx = (*mx)  >>(to);\
					else if(*op=='<'&& *(op+1)=='<')\
					*mx = *mx  <<(to);\
					else if(*op=='=' && *(op+1)=='=')\
					*mx = *mx == to;\
					else if(*op=='!' && *(op+1)=='=')\
					*mx = *mx != to;\
					else if(*op=='>' && *(op+1)=='=')\
					*mx = *mx >= to;\
					else if(*op=='<' && *(op+1)=='=')\
					*mx = *mx <= to;\
					else if(*op=='>')\
					*mx = *mx > to;\
					else if(*op=='<')\
					*mx = *mx < to;

#define BOOL_OPERATORS	if(*op=='='&&*(op+1)=='=')\
					*mx = *mx  == to;\
					else if(*op=='!'&&*(op+1)=='=')\
					*mx = *mx  != to;\
					else if(*op=='&'&&*(op+1)=='&')\
					*mx = *mx  && to;\
					else if(*op=='|'&&*(op+1)=='|')\
					*mx = *mx  || to;



#define Plong(x) (long*)x
#define Pchar(x) (char*)x
#define Pshort(x) (short*)x
#define Pint(x) (int*)x
#define Pbool(x) (bool*)x
#define Pfloat(x) (float*)x
#define Pstring(x) (char**)x
#define FCAST(PT,P,X)  P#PT(x)
typedef struct waiter { node_type wait_type; node* waiting_node; }fl;





#endif
