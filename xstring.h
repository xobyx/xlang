#ifndef XSTRING_H
#define XSTRING_H

#include "types.h"

/* PCRE Regular Expressions */
void x_regex_match(fcall* fc);
void x_regex_find(fcall* fc);
void x_regex_replace(fcall* fc);

/* String Utility Functions */
void x_substr(fcall* fc);
void x_index_of(fcall* fc);
void x_trim(fcall* fc);
void x_to_lower(fcall* fc);
void x_to_upper(fcall* fc);
void x_starts_with(fcall* fc);
void x_ends_with(fcall* fc);
void x_string_split(fcall* fc);

#endif /* XSTRING_H */
