#ifndef XJSON_H
#define XJSON_H

#ifdef __cplusplus
extern "C" {
#endif

int x_json_parse_to_id(const char* json_str, int* out_is_list);
char* x_json_stringify_id(int id, int is_list);
int x_json_validate(const char* json_str);

#ifdef __cplusplus
}
#endif

#endif /* XJSON_H */
