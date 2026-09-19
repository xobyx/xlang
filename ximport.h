#ifndef XIMPORT_H
#define XIMPORT_H

#include <stdbool.h>

void x_import_init(void);
bool x_import_module(const char* module_name);
void x_import_cleanup(void);

#endif /* XIMPORT_H */
