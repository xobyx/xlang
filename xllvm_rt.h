#ifndef XLLVM_RT_H
#define XLLVM_RT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime Initialization */
void xllvm_rt_init(int argc, char** argv);

/* String Helpers */
char* _str_concat(const char* s1, const char* s2);
char* _str_from_int(int val);
char* _str_from_float(double val);
int _len(const char* s);
char* _trim(const char* s);
char* _lower(const char* s);
char* _upper(const char* s);
char* substr(const char* s, int start, int len);
char* chr(int code);
char* x_readline(void);
int index_of(const char* s, const char* needle);
int str_eq(const char* s1, const char* s2);
int starts_with(const char* s, const char* prefix);
int ends_with(const char* s, const char* suffix);
int regex_match(const char* s, const char* pattern);
char* regex_find(const char* s, const char* pattern);
char* regex_replace(const char* s, const char* pattern, const char* repl);
int str_split(const char* s, const char* delim);

/* Math Operations */
double math_sqrt(double x);
double math_pow(double base, double exp);
double math_abs(double x);
double math_min(double a, double b);
double math_max(double a, double b);
double math_floor(double x);
double math_ceil(double x);
double math_round(double x);
double math_sin(double x);
double math_cos(double x);
double math_tan(double x);
double math_log(double x);

/* File Operations */
char* file_read_all(const char* path);
int file_write_all(const char* path, const char* content);
int file_append(const char* path, const char* content);
int file_exists(const char* path);
int file_size(const char* path);
int file_remove(const char* path);
int file_open(const char* path, const char* mode);
char* file_read(int fd, int bytes);
int file_write(int fd, const char* data);
int file_close(int fd);

/* Directory Operations */
void* dir_list(const char* path);
int dir_create(const char* path);
int dir_exists(const char* path);
int dir_remove(const char* path);

/* Process Operations */
char* proc_capture(const char* cmd);
void* proc_run(const char* cmd);

/* DateTime Operations */
int datetime_now(void);
char* datetime_format(int ts, const char* fmt);
int datetime_year(int ts);
int datetime_month(int ts);
int datetime_day(int ts);
int datetime_hour(int ts);
int datetime_minute(int ts);
int datetime_second(int ts);
int datetime_clock_ms(void);

/* JSON Operations */
int json_is_valid(const char* str);
void* json_parse(const char* str);
char* json_stringify(void* m);

/* HTTP Operations */
char* http_get(const char* url);

/* Map Operations */
int map_new(void);
int map_put(int id, const char* key, const char* val);
int map_put_int(int id, const char* key, int val);
int map_put_float(int id, const char* key, double val);
const char* map_get(int id, const char* key);
int map_get_int(int id, const char* key);
double map_get_float(int id, const char* key);
int map_has(int id, const char* key);
int map_remove(int id, const char* key);
int map_size(int id);
int map_clear(int id);
char* map_keys(int id);
char* map_values(int id);
char* map_to_string(int id);
int map_free(int id);
int map_keys_list(int id);

/* List Operations */
int list_new(void);
int list_add(int id, const char* val);
int list_add_int(int id, int val);
int list_add_float(int id, double val);
const char* list_get(int id, int index);
int list_get_int(int id, int index);
double list_get_float(int id, int index);
int list_set(int id, int index, const char* val);
int list_set_int(int id, int index, int val);
int list_set_float(int id, int index, double val);
int list_remove_at(int id, int index);
int list_size(int id);
int list_clear(int id);
int list_contains(int id, const char* val);
int list_contains_int(int id, int val);
int list_index_of(int id, const char* val);
int list_index_of_int(int id, int val);
char* list_pop(int id);
int list_pop_int(int id);
char* list_join(int id, const char* sep);
char* list_to_string(int id);
int list_free(int id);

/* Socket Operations */
int socket_create(const char* type);
int socket_connect(int fd, const char* host, int port);
int socket_bind(int fd, const char* host, int port);
int socket_listen(int fd, int backlog);
int socket_accept(int fd);
int socket_send(int fd, const char* data);
char* socket_recv(int fd, int max_bytes);
int socket_close(int fd);
int socket_set_timeout(int fd, int sec);
int socket_set_reuseaddr(int fd, int enable);
int socket_sendto(int fd, const char* data, const char* host, int port);
char* socket_recvfrom(int fd, int max_bytes);

/* System Operations */
int clock_ms(void);
int get_argc(void);
char* get_arg(int index);
int system_exec(const char* cmd);
char* system_getenv(const char* name);
int system_setenv(const char* name, const char* val);

/* GC Operations */
int gc_collect(void);
int gc_allocated_bytes(void);
int gc_total_objects(void);
int gc_enable(void);
int gc_disable(void);
int gc_set_threshold(int th);
int gc_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* XLLVM_RT_H */
