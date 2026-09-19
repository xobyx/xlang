#ifndef XDATETIME_H
#define XDATETIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void x_datetime_now(fcall* fc);
void x_datetime_format(fcall* fc);
void x_datetime_year(fcall* fc);
void x_datetime_month(fcall* fc);
void x_datetime_day(fcall* fc);
void x_datetime_hour(fcall* fc);
void x_datetime_minute(fcall* fc);
void x_datetime_second(fcall* fc);
void x_datetime_clock_ms(fcall* fc);

#ifdef __cplusplus
}
#endif

#endif /* XDATETIME_H */
