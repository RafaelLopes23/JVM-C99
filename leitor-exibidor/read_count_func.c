#include "read_count_func.h"


uint16_t read_interfaces_count(const char *filename, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);

    uint16_t interfaces_count;

    read_bytes(file, &interfaces_count, sizeof(interfaces_count), pos->position); // Lê os interfaces_count

    pos->position = pos->position + 2;

    // Ajusta a ordem dos bytes para big-endian, se necessário
    interfaces_count = to_big_endian_16(interfaces_count);

    // Exibe os interfaces_count
    printf("\nInterfaces: %u\n", interfaces_count);

    return interfaces_count;
}

uint16_t read_fields_count(const char *filename, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);

    uint16_t fields_count;

    read_bytes(file, &fields_count, sizeof(fields_count), pos->position); // Lê os fields_count

    pos->position = pos->position + 2;

    // Ajusta a ordem dos bytes para big-endian, se necessário
    fields_count = to_big_endian_16(fields_count);

    // Exibe os fields_count
    printf("\nFields: %u\n", fields_count);

    return fields_count;
}

uint16_t read_method_count(const char *filename, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);

    uint16_t method_count;

    read_bytes(file, &method_count, sizeof(method_count), pos->position); // Lê os method_count

    pos->position = pos->position + 2;

    // Ajusta a ordem dos bytes para big-endian, se necessário
    method_count = to_big_endian_16(method_count);

    // Exibe os method_count
    printf("\nMetodos: %u\n", method_count);

    return method_count;
}

uint16_t read_atribute_count(const char *filename, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);

    uint16_t atribute_count;

    read_bytes(file, &atribute_count, sizeof(atribute_count), pos->position); // Lê os atribute_count

    pos->position = pos->position + 2;

    // Ajusta a ordem dos bytes para big-endian, se necessário
    atribute_count = to_big_endian_16(atribute_count);

    // Exibe os atribute_count
    printf("\nAtributos: %u\n", atribute_count);

    return atribute_count;
}

uint16_t read_constant_pool_count(const char *filename){
    FILE* file = abre_arquivo(filename);
    uint16_t constant_pool_count;

    read_bytes(file, &constant_pool_count, sizeof(constant_pool_count), 8); // Lê o constant_pool_count

    // Ajusta a ordem dos bytes para big-endian, se necessário
    constant_pool_count = (uint16_t)to_big_endian_16((uint32_t)constant_pool_count);

    // Exibe as versões
    printf("Constant Pool Count: %u\n", constant_pool_count);

    fclose(file); // Fecha o arquivo após a leitura

    return constant_pool_count;
}
// Função para ler o arquivo .class e exibir constant_pool
ConstantPoolResult read_constant_pool(const char *filename){
    FILE* file = abre_arquivo(filename);
    uint16_t constant_pool_count;

    constant_pool_count = read_constant_pool_count(filename);

    IndexPosition  constant_positions[MAX_CONSTANTS];
    size_t constant_positions_count = 0;

    long position = 10; // A posição inicial da tabela de constantes
    for (int i = 1; i < constant_pool_count; i++) {
        uint8_t tag;
        read_bytes(file, &tag, sizeof(tag), position); // Lê o tag da constante
        uint16_t entry_size = 0;
        if (tag == 1) { // CONSTANT_Utf8
            uint16_t length;
            read_bytes(file, &length, sizeof(length), position + 1);
            length = to_big_endian_16(length);
            entry_size = 3 + length; // Tag + Length + String content
            } else if (tag == 3 || tag == 4) { // CONSTANT_Integer or CONSTANT_Float
                entry_size = 5; // Tag + 4 bytes for value
            } else if (tag == 5 || tag == 6) { // CONSTANT_Long or CONSTANT_Double
                entry_size = 9; // Tag + 8 bytes for value
            } else if (tag == 7 || tag == 8) { // CONSTANT_Class or CONSTANT_String
                entry_size = 3; // Tag + 2 bytes for index
            } else if (tag == 9 || tag == 10 || tag == 11) { // CONSTANT_Fieldref, CONSTANT_Methodref, CONSTANT_InterfaceMethodref
                entry_size = 5; // Tag + 2 bytes for class_index + 2 bytes for name_and_type_index
            } else if (tag == 12) { // CONSTANT_NameAndType
                entry_size = 5; // Tag + 2 bytes for name_index + 2 bytes for descriptor_index
            } else if (tag == 15) { // CONSTANT_MethodHandle
                entry_size = 4; // Tag + 1 byte for reference_kind + 2 bytes for reference_index
            } else if (tag == 16) { // CONSTANT_MethodType
                entry_size = 3; // Tag + 2 bytes for descriptor_index
            } else if (tag == 17 || tag == 18) { // CONSTANT_Dynamic or CONSTANT_InvokeDynamic
                entry_size = 5; // Tag + 2 bytes for bootstrap_method_attr_index + 2 bytes for name_and_type_index
            } else if (tag == 19 || tag == 20) { // CONSTANT_Module or CONSTANT_Package
                entry_size = 3; // Tag + 2 bytes for name_index
            } else {
                entry_size = 1; // Tag alone for unknown constants
            }   
        constant_positions[constant_positions_count].index = i;
        if(tag == 5 || tag == 6){
            i = i + 1;
        }
        constant_positions[constant_positions_count].position = position;
        constant_positions[constant_positions_count].tag = tag; // Salva a tag
        constant_positions_count++;

        position += entry_size; // Avança para a próxima constante

    }
    
    for (int i = 0; i < constant_positions_count; i++){
        printf("\n#%i = ", constant_positions[i].index);
        if (constant_positions[i].tag == 1){
            printf("Utf8: ");
        }
        display_constant(file, constant_positions[i].index, constant_positions[i].position, constant_positions[i].tag, constant_positions, constant_positions_count, 10);
    }

    fclose(file); // Fecha o arquivo após a leitura

    ConstantPoolResult result;
    result.position = position;
    memcpy(result.constant_positions, constant_positions, sizeof(constant_positions));
    result.constant_positions_count = constant_positions_count;
    return result;
}