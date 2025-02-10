#ifndef DISPLAY_H
#define DISPLAY_H

#include "read_func.h"

void display_interfaces(const char *filename, uint16_t interfaces_count, ConstantPoolResult *dic);
void display_fields(const char *filename, uint16_t count, ConstantPoolResult *pos);
void display_method(const char *filename, uint16_t count, ConstantPoolResult *pos);
void display_atribute(const char *filename, uint16_t count, ConstantPoolResult *pos);


#endif