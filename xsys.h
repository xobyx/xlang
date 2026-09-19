#ifndef XSYS_H
#define XSYS_H

#include "types.h"

extern int g_script_argc;
extern char** g_script_argv;

void x_get_arg(fcall* fc);
void x_get_argc(fcall* fc);
void x_system_exec(fcall* fc);
void x_system_getenv(fcall* fc);
void x_system_setenv(fcall* fc);
void x_clock_ms(fcall* fc);

#endif /* XSYS_H */
