#pragma once
#include "xlang_main.h"




/* Primary AST evaluation / tree-walk interpreter functions */
node* eval_ast_nodes(var* context_object, node* root_node, fcall* calling_function, node* stop_node, type_def* target_class);
node* compile(var* parent, node* out, fcall* temp, node* stop, type_def* ncalss);

type_def* get_type_by_name(char* value);
bool call_function(fcall* func_call, var** context);

node* setup_function_params(node** current_node, fcall* func_call, var* context, fcall* caller_function);
node* setup_function_parms(node** cx, fcall* function, var* context, fcall* in_function);

void define_class_property(char* name, type_def* container_class, type_def* prop_type, var** out_var);
void define_new_class_prop(char* name, type_def* contern_class, type_def* new_var_type, var** out_var);

void define_function_var(char* name, fcall* func_call, type_def* var_type, var** out_var);
void define_new_var_on_function(char* name, fcall* mfun, type_def* new_var_type, var** out_var);

void define_global_var(var** out_var, type_def* var_type, char* name);
void define_new_var_globle(var** out_var, type_def* new_var_type, char* name);

void step_forward(node** current_node);
void step_forwrod(node** nod);

node* get_last_jump(node* b);
node* gelastjump(node* b);

var* eval_member_access(node** nod, fcall* calling_function, var* calling_object);
var* name_exp_assign(node** nod, fcall* calling_function, var* calling_object);

var* declare_variable(node** current_node, fcall* calling_function, var* calling_object, type_def* target_class, type_def* var_type, int var_size);
var* add_var_to(node** c, fcall* c_function, var* calling_object, type_def* ncalss, type_def* var_type, int vsize);

void eval_if_stmt(node** current_node, fcall* calling_function, var* context_obj);
void if_eif_function(node** c, fcall* c_function, var* parent);

void eval_while_stmt(node** current_node, fcall* calling_function, var* context_obj);
void while_function(node** c, fcall* c_function, var* parent);

void eval_do_while_stmt(node** current_node, fcall* calling_function, var* context_obj);
void do_while_function(node** c, fcall* c_function, var* parent);

void eval_for_stmt(node** current_node, fcall* calling_function, var* context_obj);
void for_function(node** c, fcall* funcall, var* calling_object);

void eval_class_decl(node** current_node);
void install_class(node** n);

