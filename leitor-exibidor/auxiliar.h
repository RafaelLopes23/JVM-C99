
#ifndef AUXILIAR_H
#define AUXILIAR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

typedef struct {
    uint16_t index;
    long position;
    uint8_t tag;
} IndexPosition;

typedef struct {
    uint8_t opcode;
    const char *mnemonic;
    uint8_t operand_count;
} OpcodeInfo;

extern OpcodeInfo opcode_table[256];

uint32_t to_big_endian_32(uint32_t value);
uint16_t to_big_endian_16(uint16_t value);
FILE* abre_arquivo(const char *filename);
long find_position_by_index(IndexPosition *constant_pool_map, uint16_t index, uint16_t constant_pool_count);
void read_bytes(FILE *file, void *buffer, size_t size, long position);

#endif // AUXILIAR_H