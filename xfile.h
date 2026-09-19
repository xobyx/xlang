#ifndef XFILE_H
#define XFILE_H

#include "types.h"

void x_file_read_all(fcall* fc);
void x_file_write_all(fcall* fc);
void x_file_append(fcall* fc);
void x_file_exists(fcall* fc);
void x_file_remove(fcall* fc);
void x_file_size(fcall* fc);

/* Stream-oriented file operations */
void x_file_open(fcall* fc);
void x_file_read(fcall* fc);
void x_file_write(fcall* fc);
void x_file_close(fcall* fc);

/* Directory operations */
void x_dir_list(fcall* fc);
void x_dir_create(fcall* fc);
void x_dir_exists(fcall* fc);
void x_dir_remove(fcall* fc);

#endif /* XFILE_H */
