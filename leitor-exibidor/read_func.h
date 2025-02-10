#ifndef READ_FUNC_H
#define READ_FUNC_H

#include "auxiliar.h"

#define MAX_CONSTANTS 1000

typedef struct {
    long position;
    IndexPosition constant_positions[MAX_CONSTANTS];
    size_t constant_positions_count;
} ConstantPoolResult;

extern ConstantPoolResult result_struct;

char* display_constant(FILE *file, uint16_t index, long position, uint8_t tag, IndexPosition *dic, size_t constant_positions_count, int indent);
void read_magic_number(const char *filename);
void read_minor_major(const char *filename);
void read_access_flags(const char *filename, ConstantPoolResult *pos, int id);
void read_this_class(const char *filename, ConstantPoolResult *pos);
void read_super_class(const char *filename, ConstantPoolResult *pos);
void display_atribute_info(const char *filename, ConstantPoolResult *pos, uint16_t tag);

#endif // READ_FUNC_H