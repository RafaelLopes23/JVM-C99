#ifndef READ_COUNT_FUNC_H
#define READ_COUNT_FUNC_H

#include "display.h"


// Funções para ler o arquivo .class e exibir contagens
uint16_t read_interfaces_count(const char *filename, ConstantPoolResult *pos);
uint16_t read_fields_count(const char *filename, ConstantPoolResult *pos);
uint16_t read_method_count(const char *filename, ConstantPoolResult *pos);
uint16_t read_atribute_count(const char *filename, ConstantPoolResult *pos);
uint16_t read_constant_pool_count(const char *filename);
ConstantPoolResult read_constant_pool(const char *filename);

#endif // READ_COUNT_FUNC_H