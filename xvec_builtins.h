#ifndef XVEC_BUILTINS_H
#define XVEC_BUILTINS_H

#include "xvm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize and register all core Vectorcall built-in functions */
void xvec_builtins_init(void);
void xvm_register_vector_func(const char* name, XVectorFn fn, int arity);

/* Core VM-Aware Builtins */
XValue vec_assert(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_print(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_print_f(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_println(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_eval(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_scan(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_readline(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_random(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_time(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_exit(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_echo(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_import(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Reflection Builtins */
XValue vec_typeof(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_type_name(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_has_field(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_get_field(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_set_field(XVm* vm, XValue receiver, int argc, const XValue* args);

/* String & Regex Operations */
XValue vec_len(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_str(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_substr(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_index_of(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_trim(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_to_lower(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_to_upper(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_starts_with(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_ends_with(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_regex_match(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_regex_find(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_regex_replace(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_replace(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_str_split(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_eql(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Math Operations */
XValue vec_math_sqrt(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_pow(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_abs(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_min(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_max(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_floor(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_ceil(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_round(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_sin(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_cos(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_tan(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_math_log(XVm* vm, XValue receiver, int argc, const XValue* args);

/* File Operations */
XValue vec_file_read_all(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_write_all(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_append(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_exists(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_size(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_remove(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_open(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_read(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_write(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_file_close(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Directory Operations */
XValue vec_dir_list(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_dir_create(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_dir_exists(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_dir_remove(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Socket Operations */
XValue vec_socket_create(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_connect(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_bind(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_listen(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_accept(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_send(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_recv(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_close(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_set_timeout(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_set_reuseaddr(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_sendto(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_socket_recvfrom(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_http_get(XVm* vm, XValue receiver, int argc, const XValue* args);

/* List Operations */
XValue vec_list_new(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_free(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_size(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_add(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_add_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_add_float(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_get(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_get_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_get_float(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_set(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_set_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_remove_at(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_clear(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_contains(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_contains_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_index_of(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_index_of_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_pop(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_pop_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_join(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_list_to_string(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Map Operations */
XValue vec_map_new(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_free(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_size(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_put(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_put_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_put_float(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_get(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_get_int(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_get_float(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_has(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_remove(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_clear(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_keys(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_values(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_to_string(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_map_keys_list(XVm* vm, XValue receiver, int argc, const XValue* args);

/* System & Process */
XValue vec_get_arg(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_get_argc(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_system_exec(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_system_getenv(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_system_setenv(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_proc_capture(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_proc_run(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_clock_ms(XVm* vm, XValue receiver, int argc, const XValue* args);

/* DateTime Operations */
XValue vec_datetime_now(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_format(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_year(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_month(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_day(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_hour(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_minute(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_second(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_datetime_clock_ms(XVm* vm, XValue receiver, int argc, const XValue* args);

/* JSON Operations */
XValue vec_json_parse(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_json_stringify(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_json_is_valid(XVm* vm, XValue receiver, int argc, const XValue* args);

/* GC Operations */
XValue vec_gc_collect(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_allocated_bytes(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_total_objects(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_enable(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_disable(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_set_threshold(XVm* vm, XValue receiver, int argc, const XValue* args);
XValue vec_gc_dump(XVm* vm, XValue receiver, int argc, const XValue* args);

/* Primitive Type Methods */
XValue vec_int_add(XVm* vm, XValue receiver, int argc, const XValue* args);

#ifdef __cplusplus
}
#endif

#endif /* XVEC_BUILTINS_H */
