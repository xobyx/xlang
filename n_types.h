//
//#include "var_stack.h"
//struct node;
//
//enum class f_type
//{
//	main=0,
//	constr,
//	class_function
//};
//
//struct nvar
//{
//	//var (__thiscall a)(var *this);
//
//	nvar(): var_type(0), value(0), line(0), base_type(0), size(0), ref(0)
//	{
//	}
//
//
//	char name[10];
//	char* var_type;
//	void* value;
//	char hash[5];
//	int line;
//	nvar* base_type;
//	int size;
//	node* ref;
//};
//
//struct nfunc
//{
//	nfunc(): func_code(0), function_type(f_type::main), parm_count(0), ref(0), parent(0)
//	{
//	}
//
//	nvar func_parms[10];
//	void* func_code;
//	char func_name[10];
//	f_type function_type;
//	nvar func_return;
//	int parm_count;
//	node* ref;
//	//unused
//	nfunc* parent;
//};
//
//struct type
//{
//	char name[10];
//	var_stack propertys;
//	nfunc functions[100];
//	type* base;
//};
//
//
//#define type_ btype.value
//#define flag_ fflag.value
//#define FUNCTION "function"
////#define AL_PAR_OP(x
////) x==parse_obj::comma||x==parse_obj::parentheses1||x==parse_obj::parentheses1c||x==parse_obj::size||x==parse_obj::index
//
//#define EMTYE_VALUE_REF(x) x.value=NULL;\
//	x.rev=NULL; \
//	x.next=NULL;\
//	x.isFlag=false;
//
//#define tp btype.nnType
//
//typedef void (*def_function)(nfunc*);
//
//
//#define PL(z) z=='+'?+:z=='-'?-:z=='/'?/:z=='*'?*:.
//
////char * gramer = "<base_type> [operators|=] if(_O2==operators)[var_name|this] []"
//
////F parse_obj 
//
//enum key_word_enum
//{
//	IF,
//	FOR,
//	WHILE,
//	DO
//};
//
//struct xobject;
//
//typedef xobject* (*function_call)(xobject*);
//
//struct xtype
//{
//	//type
//
//	char* type;
//	
//	
//	xtype* base_type;
//	
//};
//
//
//struct _object
//{
//	//var (__thiscall a)(var *this);
//
//	xtype* type;
//	char* name;
//	
//	
//	function_call get_value;
//	function_call set_value;
//	_object** prob;
//}x_object;
//
//static xobject* get_value(xobject* r)
//{
//	return *r->prob;
//}
//
//static xobject* set_value(xobject* r)
//{
//	*r->prob=r;
//	return NULL;
//}
//static xtype base ={
//"base",0
//};
// xobject super = {
//
//	"super",//name;
//	 &base,//type;
//	
//	get_value,//function_call get_value;
//	set_value//function_call set_value;
//	//xobject** prob;
//
//
//};
//xtype t_string ={
//	"string",
//	&base
//
//};
//xobject * get_len 
//xobject string=
//{
//"",
//&t_string,
//0,
//0,
//};
//
//
//#define NUMBER_FUNCTIO  {NULL}
//#define NUMBER_PROP  {NULL}
//#define STR_FUNCTIO  {NULL}
//#define STR_PROP  {NULL}
//static xobject _STRING_ = {"string",STR_FUNCTIO,NULL,STR_PROP};
//static xobject _INT_ = {"int",NUMBER_FUNCTIO,NULL,NUMBER_PROP};
//
//
//static xobject u[] = {_STRING_};
