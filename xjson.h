#ifndef XJSON_H
#define XJSON_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void x_json_parse(fcall* fc);
void x_json_stringify(fcall* fc);
void x_json_is_valid(fcall* fc);

#ifdef __cplusplus
}
#endif

#endif /* XJSON_H */
