#pragma once
#include "xlang_main.h"

#if defined(__GNUC__) || defined(__MINGW64__)
#ifndef strcat_s
#define strcat_s(x, y, z) strcat(x, z)
#endif
#endif

/* Type constants mapping to SIMPLE_TYPE */
extern type_def SIMPLE_TYPE[];

#define T_LONG      (&SIMPLE_TYPE[0])
#define T_STRING    (&SIMPLE_TYPE[1])
#define T_CHAR      (&SIMPLE_TYPE[2])
#define T_INT       (&SIMPLE_TYPE[3])
#define T_BOOL      (&SIMPLE_TYPE[4])
#define T_FLOAT     (&SIMPLE_TYPE[5])
#define T_OBJECT    (&SIMPLE_TYPE[6])
#define T_ARRAY     (&SIMPLE_TYPE[7])
#define T_ANY       (&SIMPLE_TYPE[8])
#define T_FUNC      (&SIMPLE_TYPE[9])
#define T_TYPE_INFO (&SIMPLE_TYPE[10])

#define SIMPLE_FUNC_COUNT 163
extern func_stack base_function;

/* Type System & Object Model */
void install_default_types(void);
type_def* new_type(void);
type_def* get_type_by_name(char* name);
bool is_base_type(type_def* t);

int type_def_compute_field_offsets(type_def* td);
int type_def_find_field_slot(const type_def* td, const char* name);
FieldDescriptor* type_def_get_field_descriptor(const type_def* td, const char* name);
FieldDescriptor* type_def_get_field_descriptor_by_slot(const type_def* td, int slot);

type_instance* type_instance_create(type_def* td);
var* type_instance_get_field(type_instance* inst, const char* name);
var* type_instance_get_field_by_slot(type_instance* inst, int slot);
int type_instance_get_id(type_instance* inst);
void instance_type(type_def* type_prototype, void* dest_array, int size);

void define_class_property(const char* name, type_def* container_class, type_def* prop_type, var** out_var);
void define_new_class_prop(char* name, type_def* container_class, type_def* new_var_type, var** out_var);

func_deftion* get_class_function(type_def* t, const char* name);
var* get_class_property(type_def* t, const char* name);
type_def* get_class_of_function(func_deftion* fd);
func_deftion* get_obj_function(var* object_var, char* name);

/* Variable Management & Scope Lookup */
var* new_var(char* name, type_def* vtype);
var* new_temp_var(type_def* typ);
var* get_global_var_by_name(char* name);

/* Memory & Values */
int* new_int(int count, int value);
void* install_memory(var* target_var);
void* install_memory_with_type(type_def* type_def_ptr, const int element_count);
void set_value_copy_var(var* dest, var* src);

/* Function Call Management */
void install_default_functions(void);
func_deftion* new_func(void);
func_deftion* get_func_by_name(char* name);

/* String & Buffer Utilities */
int starts_with_keyword(const char* text, const char* keyword);
int eql(const char* n, const char* x);
void unescape_string(char* str);
char* get_file_buffer(FILE* sf);
char** get_pptr_string(char* t);

/* Array & Collection Operations */
void assign_array_index(var* nvalue, var* marray, int index);
var* get_array_item(var* name, int index);
bool copy_array(var* out, void* out_memory, var* src);

/* Assertion Configuration */
void set_assert_enabled(bool enabled);
bool is_assert_enabled(void);

/* Deprecated Compatibility Aliases */
static inline var* get_globle_var_by_name(char* name) { return get_global_var_by_name(name); }
static inline void scap_string(char* m) { unescape_string(m); }
static inline func_deftion* get_obj_function2(var* o, char* n) { return get_obj_function(o, n); }
