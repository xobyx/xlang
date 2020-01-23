#pragma once


#include <windows.h>


#include <stdbool.h>
#ifndef byte
#define byte unsigned char
#endif

struct func;
typedef void (*def_function)(struct func*);

inline int ccprintf(byte color, const char* format,...)
{
	//va_list arglist;
	//va_start(arglist, format);
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	struct _CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	GetConsoleScreenBufferInfo(handle, &csbiInfo);

	SetConsoleTextAttribute(handle, color);
	va_list args;
	va_start(args, format);
	const int u = vprintf(format, args);
	va_end(args);
	SetConsoleTextAttribute(handle, csbiInfo.wAttributes);
	return u;
}

static char* base_types_name [] = {"long", "string", "char", "int", "bool", "float", "_array", "new"};

struct type;


static const char* key_word [] = {"if", "for", "while", "do", "else", "eif", "return", "break", "class", "static"};

typedef enum key_word_enum { _if_= 0, _for_, _while_, _do_, _else_, _eif_, _return_, _break_, _class_, _static_ }key_word_enum;

// long[0],string[1],char[2],int[3]
typedef struct type_stack
{
	struct type* top;
	struct type* root;
	int size;
}type_stack;

static const char operators [] = {'+', '-', '/', '*', '='};
static const char one_c [] = {
	'+', 0, '-', 0, '/', 0, '*', 0, '=', 0, '(', 0, ')', 0, '{', 0, '}', 0, '[', 0, ']', 0, ',', 0, '>', 0, '<', 0, '|',
	0, '&', 0, '!', 0, '.', 0, ':'
};
typedef unsigned short i16;

typedef enum node_type //: i16
{
	itype = 0x0001,
	keyword = 0x0002,
	var_name = 0x0004,
	value = 0x0008,
	operators_n = 0x0010,
	equles = 0x0020,
	endl = 0x0040,
	s_index = 0x0080,
	// }
	parentheses1 = 0x0100,
	// {
	parentheses1c = 0x0200,
	// ,
	comma = 0x0400,
	// []
	s_index_c = 0x0800,
	// (
	parentheses4 = 0x1000,
	//)
	parentheses4c = 0x2000,
	//.
	dot = 0x4000,
	
	twodot = 0x8000,
	//:
	PARS = (parentheses1 | parentheses1c | parentheses4 | parentheses4c | s_index_c | s_index),
	// )
	/**
	 * \brief not equles,
	 */
	HAVE_VAR_VALUE=value | var_name | keyword | itype | operators_n,
	a=value | var_name,
	NON_ONE_CHAR =0x000F,
}node_type;


inline const char* parse_obj_str(enum node_type p)
{
	switch (p)
	{
	case itype: return "itype";
	case keyword: return "keyword";
	case var_name: return "var_name";
	case value: return "value";
	case operators_n: return "operators_n";
	case equles: return "equles";
	case endl: return "endl";
	case s_index: return "s_index";
	case parentheses1: return "parentheses1";
	case parentheses1c: return "parentheses1c";
	case comma: return "comma";
	case s_index_c: return "s_index_c";
	case parentheses4: return "parentheses4";
	case parentheses4c: return "parentheses4c";
	case dot: return "dot";
	case twodot: return "twodot";
	case PARS: return "PARS";
	case HAVE_VAR_VALUE: return "HAVE_VAR_VALUE";
	case a: return "a";
	case NON_ONE_CHAR: return "NON_ONE_CHAR";
		/* etc... */
	default: ;
	}
	return NULL;
}


typedef enum  f_type
{
	f_main=0,
	constr=1,
	class_function=2
}f_type;

typedef union nType
{
	i16 value;
	enum node_type name;

	struct
	{
		i16 itype:1;
		i16 keword:1;
		i16 var_name:1;
		i16 value:1;
		i16 operators_n:1;
		i16 equles:1;
		i16 endl:1;
		i16 s_index:1; //[
		i16 parentheses1:1; //}
		i16 parentheses1c:1; //{
		i16 comma:1; //,
		i16 s_index_c:1; //]
		i16 parentheses4:1; //(
		i16 parentheses4c:1; //(
		i16 dot:1; //(
		i16 twodot:1; //(
		//i16 mbool:1;
	} nnType;
}nType;

#define F 1

 
typedef struct node
{
	

	int id;
	nType btype;
	nType fflag;
	int line ;

	struct node* parent;
	struct  node* next;

	union
	{
		
		void* value; 
		struct type * value_type ;
		int * value_int ;
		char * value_char_ptr;
		char ** value_string;
		int  _ptr_;

	};
	
	bool is_flagged;
	struct node* stack_parent;
	struct node* stack_next;
	//void* op; ///used in com if_helper///types in parse///functionn
	struct node* ref_node; ///if __ {[( close;
	union
	{
		
		void* opt; 
		struct type * opt_type_ptr ;
		int * opt_int_ptr ;
		char * opt_char_ptr;
		char ** opt_string_ptr;
		int  _opt_ptr_;

	};
}node;

typedef struct node_stack
{
	struct node* top;
	int size;
	struct node* root;
}node_stack;

enum var_access { PUBLIC=1, STATIC, PRIVATE };

typedef struct var
{



	char* name ;
	struct type* var_type;

	union
	{
		int  a;
		void* value; 
		struct type * value_type ;
		int * value_int ;
		long * value_long ;
		float * value_float ;
		char * value_char_ptr;
		char ** val_str_ptr;

	};
	//void* value = 0;
	

	struct var* base_type;
	int size;
	struct node* ref;
	struct var* stack_next;
	enum var_access access;
}var ;

typedef struct var_stack
{
	

	struct var* top;
	byte size;
	struct var* root;
	struct type* m_class;
}var_stack;

typedef struct func
{
	struct var_stack fun_p;
	def_function func_code;
	char* func_name;
	f_type function_type;
	struct var func_return;

	node* ref;
	struct var* context;
	//unused
	struct func* stack_next;
}func;

typedef struct func_stack
{
	

	func* top;
	int size;
	func* root;
}func_stack;


enum w_type { sup=0, child };

typedef struct type
{
	


	char* name;
	struct var_stack propertys;
	func_stack functions;
	struct type* base;
	struct type* stack_next;
	var* context;
	enum w_type w ;

	
}type;


#define type_ btype.value
#define flag_ fflag.value
#define FUNCTION "function"
#define AL_PAR_OP(x) x==node_type::parentheses4||x==node_type::parentheses4c||x==node_type::comma||x==node_type::parentheses1||x==node_type::parentheses1c||x==node_type::s_index||x==node_type::s_index_c

#define EMTYE_VALUE_REF(x) x.value=NULL;\
	x.rev=NULL; \
	x.next=NULL;\
	x.isFlag=false;

#define tp btype.nnType




//char * gramer = "<base_type> [operators|=] if(_O2==operators)[var_name|this] []"

//F parse_obj 

enum key_word_enum2
{
	IF,
	FOR,
	WHILE,
	DO
};

//struct xobject_inctance;
//struct xobject
//{
////type
//
//char * type;
//func** functions;
//xobject* base_type;
//xobject_inctance ** prob; 
//
//
//
//};
//struct xobject_inctance
//{
//	//var (__thiscall a)(var *this);
//	
//
//	char* name;
//	xobject* type;
//	void* value; 
//	
//	
//	
//
//};
//#define NUMBER_FUNCTIO  {NULL}
//#define NUMBER_PROP  {NULL}
//#define STR_FUNCTIO  {NULL}
//#define STR_PROP  {NULL}
//static xobject _STRING_={"string",STR_FUNCTIO,NULL,STR_PROP};
//static xobject _INT_={"int",NUMBER_FUNCTIO,NULL,NUMBER_PROP};
//
//
//
//static xobject u[]={_STRING_};

#define pgmVar(px) ((var*)px->parent->rev)
#define gmVar(x) ((var*)x.parent->rev)


#define MATH_      +



#define MATH_OPERATORS if(*op=='+')\
                 	*mx = *mx  + to;\
					else if(*op=='-')\
					*mx = *mx  - to;\
					else if(*op=='*')\
					*mx = *mx  * to;\
					else if(*op=='/')\
					*mx = *mx  / to;\
					else if(*op=='='&&*(op+1)=='=')\
					*mx = *mx  == to;\
					else if(*op=='&'&&*(op+1)=='&')\
					*mx = *mx  && to;\
					else if(*op=='|'&&*(op+1)=='|')\
					*mx = *mx  || to;\
					else if(*op=='='&&*(op+1)=='=')\
					*mx = *mx  == to;\
					else if(*op=='>'&&*(op+1)!='=')\
					*mx = *mx  > to;\
					else if(*op=='<'&&*(op+1)!='=')\
					*mx = *mx  < to;\
					else if(*op=='>'&&*(op+1)=='=')\
					*mx = *mx  >= to;\
					else if(*op=='<'&&*(op+1)=='=')\
					*mx = *mx  <= to;

#define Plong(x) (long*)x
#define Pchar(x) (char*)x
#define Pshort(x) (short*)x
#define Pint(x) (int*)x
#define Pbool(x) (bool*)x
#define Pfloat(x) (float*)x
#define Pstring(x) (char**)x
#define FCAST(PT,P,X)  P#PT(x)
typedef struct fl {node_type wait_type;node* waiting_node;}fl;

#ifdef __cplusplus
} /* extern "C" */

#endif
/*
template <typename T>
void math_opration(char* op, T* &mx, T to)
{
	if (*op == '+')
		*mx = *mx + to;
	else if (*op == '-')
		*mx = *mx - to;
	else if (*op == '*')
		*mx = *mx * to;
	else if (*op == '/')
		*mx = *mx / to;
	else if (*op == '=' && *(op + 1) == '=')
		*mx = *mx == to;
	else if (*op == '&' && *(op + 1) == '&')
		*mx = *mx && to;
	else if (*op == '|' && *(op + 1) == '|')
		*mx = *mx || to;
	else if (*op == '=' && *(op + 1) == '=')
		*mx = *mx == to;
	else if (*op == '>' && *(op + 1) != '=')
		*mx = *mx > to;
	else if (*op == '<' && *(op + 1) != '=')
		*mx = *mx < to;
	else if (*op == '>' && *(op + 1) == '=')
		*mx = *mx >= to;
	else if (*op == '<' && *(op + 1) == '=')
		*mx = *mx <= to;

	
}
*/