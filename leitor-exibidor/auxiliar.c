
#include "auxiliar.h"

OpcodeInfo opcode_table[256] = {
    [0x00] = {0x00, "nop", 0},
    [0x01] = {0x01, "aconst_null", 0},
    [0x02] = {0x02, "iconst_m1", 0},
    [0x03] = {0x03, "iconst_0", 0},
    [0x04] = {0x04, "iconst_1", 0},
    [0x05] = {0x05, "iconst_2", 0},
    [0x06] = {0x06, "iconst_3", 0},
    [0x07] = {0x07, "iconst_4", 0},
    [0x08] = {0x08, "iconst_5", 0},
    [0x09] = {0x09, "lconst_0", 0},
    [0x0A] = {0x0A, "lconst_1", 0},
    [0x0B] = {0x0B, "fconst_0", 0},
    [0x0C] = {0x0C, "fconst_1", 0},
    [0x0D] = {0x0D, "fconst_2", 0},
    [0x0E] = {0x0E, "dconst_0", 0},
    [0x0F] = {0x0F, "dconst_1", 0},
    [0x10] = {0x10, "bipush", 1},
    [0x11] = {0x11, "sipush", 2},
    [0x12] = {0x12, "ldc", 1},
    [0x13] = {0x13, "ldc_w", 2},
    [0x14] = {0x14, "ldc2_w", 2},
    [0x15] = {0x15, "iload", 1},
    [0x16] = {0x16, "lload", 1},
    [0x17] = {0x17, "fload", 1},
    [0x18] = {0x18, "dload", 1},
    [0x19] = {0x19, "aload", 1},
    [0x1A] = {0x1A, "iload_0", 0},
    [0x1B] = {0x1B, "iload_1", 0},
    [0x1C] = {0x1C, "iload_2", 0},
    [0x1D] = {0x1D, "iload_3", 0},
    [0x1E] = {0x1E, "lload_0", 0},
    [0x1F] = {0x1F, "lload_1", 0},
    [0x20] = {0x20, "lload_2", 0},
    [0x21] = {0x21, "lload_3", 0},
    [0x22] = {0x22, "fload_0", 0},
    [0x23] = {0x23, "fload_1", 0},
    [0x24] = {0x24, "fload_2", 0},
    [0x25] = {0x25, "fload_3", 0},
    [0x26] = {0x26, "dload_0", 0},
    [0x27] = {0x27, "dload_1", 0},
    [0x28] = {0x28, "dload_2", 0},
    [0x29] = {0x29, "dload_3", 0},
    [0x2A] = {0x2A, "aload_0", 0},
    [0x2B] = {0x2B, "aload_1", 0},
    [0x2C] = {0x2C, "aload_2", 0},
    [0x2D] = {0x2D, "aload_3", 0},
    [0x2E] = {0x2E, "iaload", 0},
    [0x2F] = {0x2F, "laload", 0},
    [0x30] = {0x30, "faload", 0},
    [0x31] = {0x31, "daload", 0},
    [0x32] = {0x32, "aaload", 0},
    [0x33] = {0x33, "baload", 0},
    [0x34] = {0x34, "caload", 0},
    [0x35] = {0x35, "saload", 0},
    [0x36] = {0x36, "istore", 1},
    [0x37] = {0x37, "lstore", 1},
    [0x38] = {0x38, "fstore", 1},
    [0x39] = {0x39, "dstore", 1},
    [0x3A] = {0x3A, "astore", 1},
    [0x3B] = {0x3B, "istore_0", 0},
    [0x3C] = {0x3C, "istore_1", 0},
    [0x3D] = {0x3D, "istore_2", 0},
    [0x3E] = {0x3E, "istore_3", 0},
    [0x3F] = {0x3F, "lstore_0", 0},
    [0x40] = {0x40, "lstore_1", 0},
    [0x41] = {0x41, "lstore_2", 0},
    [0x42] = {0x42, "lstore_3", 0},
    [0x43] = {0x43, "fstore_0", 0},
    [0x44] = {0x44, "fstore_1", 0},
    [0x45] = {0x45, "fstore_2", 0},
    [0x46] = {0x46, "fstore_3", 0},
    [0x47] = {0x47, "dstore_0", 0},
    [0x48] = {0x48, "dstore_1", 0},
    [0x49] = {0x49, "dstore_2", 0},
    [0x4A] = {0x4A, "dstore_3", 0},
    [0x4B] = {0x4B, "astore_0", 0},
    [0x4C] = {0x4C, "astore_1", 0},
    [0x4D] = {0x4D, "astore_2", 0},
    [0x4E] = {0x4E, "astore_3", 0},
    [0x4F] = {0x4F, "iastore", 0},
    [0x50] = {0x50, "lastore", 0},
    [0x51] = {0x51, "fastore", 0},
    [0x52] = {0x52, "dastore", 0},
    [0x53] = {0x53, "aastore", 0},
    [0x54] = {0x54, "bastore", 0},
    [0x55] = {0x55, "castore", 0},
    [0x56] = {0x56, "sastore", 0},
    [0x56] = {0x56, "sastore", 0},
    [0x57] = {0x57, "pop", 0},
    [0x58] = {0x58, "pop2", 0},
    [0x59] = {0x59, "dup", 0},
    [0x5A] = {0x5A, "dup_x1", 0},
    [0x5B] = {0x5B, "dup_x2", 0},
    [0x5C] = {0x5C, "dup2", 0},
    [0x5D] = {0x5D, "dup2_x1", 0},
    [0x5E] = {0x5E, "dup2_x2", 0},
    [0x5F] = {0x5F, "swap", 0},
    [0x60] = {0x60, "iadd", 0},
    [0x61] = {0x61, "ladd", 0},
    [0x62] = {0x62, "fadd", 0},
    [0x63] = {0x63, "dadd", 0},
    [0x64] = {0x64, "isub", 0},
    [0x65] = {0x65, "lsub", 0},
    [0x66] = {0x66, "fsub", 0},
    [0x67] = {0x67, "dsub", 0},
    [0x68] = {0x68, "imul", 0},
    [0x69] = {0x69, "lmul", 0},
    [0x6A] = {0x6A, "fmul", 0},
    [0x6B] = {0x6B, "dmul", 0},
    [0x6C] = {0x6C, "idiv", 0},
    [0x6D] = {0x6D, "ldiv", 0},
    [0x6E] = {0x6E, "fdiv", 0},
    [0x6F] = {0x6F, "ddiv", 0},
    [0x70] = {0x70, "irem", 0},
    [0x71] = {0x71, "lrem", 0},
    [0x72] = {0x72, "frem", 0},
    [0x73] = {0x73, "drem", 0},
    [0x74] = {0x74, "ineg", 0},
    [0x75] = {0x75, "lneg", 0},
    [0x76] = {0x76, "fneg", 0},
    [0x77] = {0x77, "dneg", 0},
    [0x78] = {0x78, "ishl", 0},
    [0x79] = {0x79, "lshl", 0},
    [0x7A] = {0x7A, "ishr", 0},
    [0x7B] = {0x7B, "lshr", 0},
    [0x7C] = {0x7C, "iushr", 0},
    [0x7D] = {0x7D, "lushr", 0},
    [0x7E] = {0x7E, "iand", 0},
    [0x7F] = {0x7F, "land", 0},
    [0x80] = {0x80, "ior", 0},
    [0x81] = {0x81, "lor", 0},
    [0x82] = {0x82, "ixor", 0},
    [0x83] = {0x83, "lxor", 0},
    [0x84] = {0x84, "iinc", 2}, // Requer dois operandos: índice e valor
    [0x85] = {0x85, "i2l", 0},
    [0x86] = {0x86, "i2f", 0},
    [0x87] = {0x87, "i2d", 0},
    [0x88] = {0x88, "l2i", 0},
    [0x89] = {0x89, "l2f", 0},
    [0x8A] = {0x8A, "l2d", 0},
    [0x8B] = {0x8B, "f2i", 0},
    [0x8C] = {0x8C, "f2l", 0},
    [0x8D] = {0x8D, "f2d", 0},
    [0x8E] = {0x8E, "d2i", 0},
    [0x8F] = {0x8F, "d2l", 0},
    [0x90] = {0x90, "d2f", 0},
    [0x91] = {0x91, "i2b", 0},
    [0x92] = {0x92, "i2c", 0},
    [0x93] = {0x93, "i2s", 0},
    [0x94] = {0x94, "lcmp", 0},
    [0x95] = {0x95, "fcmpl", 0},
    [0x96] = {0x96, "fcmpg", 0},
    [0x97] = {0x97, "dcmpl", 0},
    [0x98] = {0x98, "dcmpg", 0},
    [0x99] = {0x99, "ifeq", 2}, // Requer dois bytes de offset
    [0x9A] = {0x9A, "ifne", 2},
    [0x9B] = {0x9B, "iflt", 2},
    [0x9C] = {0x9C, "ifge", 2},
    [0x9D] = {0x9D, "ifgt", 2},
    [0x9E] = {0x9E, "ifle", 2},
    [0x9F] = {0x9F, "if_icmpeq", 2},
    [0xA0] = {0xA0, "if_icmpne", 2},
    [0xA1] = {0xA1, "if_icmplt", 2},
    [0xA2] = {0xA2, "if_icmpge", 2},
    [0xA3] = {0xA3, "if_icmpgt", 2},
    [0xA4] = {0xA4, "if_icmple", 2},
    [0xA5] = {0xA5, "if_acmpeq", 2},
    [0xA6] = {0xA6, "if_acmpne", 2},
    [0xA7] = {0xA7, "goto", 2}, // Requer dois bytes de offset
    [0xA8] = {0xA8, "jsr", 2},  // Requer dois bytes de offset
    [0xA9] = {0xA9, "ret", 1},  // Requer um byte de índice
    [0xAA] = {0xAA, "tableswitch", 0}, // Instrução complexa, tamanho variável
    [0xAB] = {0xAB, "lookupswitch", 0}, // Instrução complexa, tamanho variável
    [0xAC] = {0xAC, "ireturn", 0},
    [0xAD] = {0xAD, "lreturn", 0},
    [0xAE] = {0xAE, "freturn", 0},
    [0xAF] = {0xAF, "dreturn", 0},
    [0xB0] = {0xB0, "areturn", 0},
    [0xB1] = {0xB1, "return", 0},
    [0xB2] = {0xB2, "getstatic", 2}, // Requer dois bytes de índice
    [0xB3] = {0xB3, "putstatic", 2},
    [0xB4] = {0xB4, "getfield", 2},
    [0xB5] = {0xB5, "putfield", 2},
    [0xB6] = {0xB6, "invokevirtual", 2},
    [0xB7] = {0xB7, "invokespecial", 2},
    [0xB8] = {0xB8, "invokestatic", 2},
    [0xB9] = {0xB9, "invokeinterface", 4}, // Requer quatro bytes (índice + contagem)
    [0xBA] = {0xBA, "invokedynamic", 4},  // Requer quatro bytes (índice + zeros)
    [0xBB] = {0xBB, "new", 2},           // Requer dois bytes de índice
    [0xBC] = {0xBC, "newarray", 1},      // Requer um byte de tipo
    [0xBD] = {0xBD, "anewarray", 2},     // Requer dois bytes de índice
    [0xBE] = {0xBE, "arraylength", 0},
    [0xBF] = {0xBF, "athrow", 0},
    [0xC0] = {0xC0, "checkcast", 2},     // Requer dois bytes de índice
    [0xC1] = {0xC1, "instanceof", 2},    // Requer dois bytes de índice
    [0xC2] = {0xC2, "monitorenter", 0},
    [0xC3] = {0xC3, "monitorexit", 0},
    // Opcodes reservados ou não utilizados
    [0xC4] = {0xC4, "wide", 0},          // Instrução especial para modificar outras
    [0xC5] = {0xC5, "multianewarray", 3}, // Requer três bytes (índice + dimensões)
    [0xC6] = {0xC6, "ifnull", 2},        // Requer dois bytes de offset
    [0xC7] = {0xC7, "ifnonnull", 2},     // Requer dois bytes de offset
    [0xC8] = {0xC8, "goto_w", 4},        // Requer quatro bytes de offset
    [0xC9] = {0xC9, "jsr_w", 4},         // Requer quatro bytes de offset
    // Opcodes reservados ou não utilizados (0xCA a 0xFF)
    [0xCA] = {0xCA, "breakpoint", 0},    // Reservado para uso do debugger
    [0xCB] = {0xCB, "reserved_CB", 0},   // Reservado
    [0xCC] = {0xCC, "reserved_CC", 0},   // Reservado
    [0xCD] = {0xCD, "reserved_CD", 0},   // Reservado
    [0xCE] = {0xCE, "reserved_CE", 0},   // Reservado
    [0xCF] = {0xCF, "reserved_CF", 0},   // Reservado
    [0xD0] = {0xD0, "reserved_D0", 0},   // Reservado
    [0xD1] = {0xD1, "reserved_D1", 0},   // Reservado
    [0xD2] = {0xD2, "reserved_D2", 0},   // Reservado
    [0xD3] = {0xD3, "reserved_D3", 0},   // Reservado
    [0xD4] = {0xD4, "reserved_D4", 0},   // Reservado
    [0xD5] = {0xD5, "reserved_D5", 0},   // Reservado
    [0xD6] = {0xD6, "reserved_D6", 0},   // Reservado
    [0xD7] = {0xD7, "reserved_D7", 0},   // Reservado
    [0xD8] = {0xD8, "reserved_D8", 0},   // Reservado
    [0xD9] = {0xD9, "reserved_D9", 0},   // Reservado
    [0xDA] = {0xDA, "reserved_DA", 0},   // Reservado
    [0xDB] = {0xDB, "reserved_DB", 0},   // Reservado
    [0xDC] = {0xDC, "reserved_DC", 0},   // Reservado
    [0xDD] = {0xDD, "reserved_DD", 0},   // Reservado
    [0xDE] = {0xDE, "reserved_DE", 0},   // Reservado
    [0xDF] = {0xDF, "reserved_DF", 0},   // Reservado
    [0xE0] = {0xE0, "reserved_E0", 0},   // Reservado
    [0xE1] = {0xE1, "reserved_E1", 0},   // Reservado
    [0xE2] = {0xE2, "reserved_E2", 0},   // Reservado
    [0xE3] = {0xE3, "reserved_E3", 0},   // Reservado
    [0xE4] = {0xE4, "reserved_E4", 0},   // Reservado
    [0xE5] = {0xE5, "reserved_E5", 0},   // Reservado
    [0xE6] = {0xE6, "reserved_E6", 0},   // Reservado
    [0xE7] = {0xE7, "reserved_E7", 0},   // Reservado
    [0xE8] = {0xE8, "reserved_E8", 0},   // Reservado
    [0xE9] = {0xE9, "reserved_E9", 0},   // Reservado
    [0xEA] = {0xEA, "reserved_EA", 0},   // Reservado
    [0xEB] = {0xEB, "reserved_EB", 0},   // Reservado
    [0xEC] = {0xEC, "reserved_EC", 0},   // Reservado
    [0xED] = {0xED, "reserved_ED", 0},   // Reservado
    [0xEE] = {0xEE, "reserved_EE", 0},   // Reservado
    [0xEF] = {0xEF, "reserved_EF", 0},   // Reservado
    [0xF0] = {0xF0, "reserved_F0", 0},   // Reservado
    [0xF1] = {0xF1, "reserved_F1", 0},   // Reservado
    [0xF2] = {0xF2, "reserved_F2", 0},   // Reservado
    [0xF3] = {0xF3, "reserved_F3", 0},   // Reservado
    [0xF4] = {0xF4, "reserved_F4", 0},   // Reservado
    [0xF5] = {0xF5, "reserved_F5", 0},   // Reservado
    [0xF6] = {0xF6, "reserved_F6", 0},   // Reservado
    [0xF7] = {0xF7, "reserved_F7", 0},   // Reservado
    [0xF8] = {0xF8, "reserved_F8", 0},   // Reservado
    [0xF9] = {0xF9, "reserved_F9", 0},   // Reservado
    [0xFA] = {0xFA, "reserved_FA", 0},   // Reservado
    [0xFB] = {0xFB, "reserved_FB", 0},   // Reservado
    [0xFC] = {0xFC, "reserved_FC", 0},   // Reservado
    [0xFD] = {0xFD, "reserved_FD", 0},   // Reservado
    [0xFE] = {0xFE, "reserved_FE", 0},   // Reservado
    [0xFF] = {0xFF, "impdep1", 0},       // Instrução dependente de implementação
};

uint32_t to_big_endian_32(uint32_t value) {
    uint8_t *bytes = (uint8_t *)&value;
    return (uint32_t)bytes[0] << 24 |
           (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 |
           (uint32_t)bytes[3];
}

uint16_t to_big_endian_16(uint16_t value) {
    uint8_t *bytes = (uint8_t *)&value;
    return (uint16_t)bytes[0] << 8 | (uint16_t)bytes[1];
}

FILE* abre_arquivo(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }
    return file;
}

long find_position_by_index(IndexPosition *constant_pool_map, uint16_t index, uint16_t constant_pool_count) {
    for (int i = 1; i < constant_pool_count; i++) {
        if (constant_pool_map[i].index == index) {
            return constant_pool_map[i].position;  // Retorna a posição correspondente ao índice
        }
    }
    return -1;  // Retorna -1 se o índice não for encontrado
}

// Função para ler bytes de um arquivo
void read_bytes(FILE *file, void *buffer, size_t size, long position) {
    if (fseek(file, position, SEEK_SET) != 0) { // Move o ponteiro para a posição especificada
        perror("Erro ao mover o ponteiro de leitura");
        exit(EXIT_FAILURE);
    }

    if (fread(buffer, size, 1, file) != 1) { // Lê os bytes da posição atual
        perror("Erro ao ler arquivo");
        exit(EXIT_FAILURE);
    }
}