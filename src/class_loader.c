#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constant pool tags
#define CONSTANT_Class              7
#define CONSTANT_Fieldref          9
#define CONSTANT_Methodref         10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String            8
#define CONSTANT_Integer           3
#define CONSTANT_Float             4
#define CONSTANT_Long              5
#define CONSTANT_Double            6
#define CONSTANT_NameAndType       12
#define CONSTANT_Utf8              1
#define CONSTANT_MethodHandle      15
#define CONSTANT_MethodType        16
#define CONSTANT_InvokeDynamic     18
#define ARRAY_TYPE_INT    10
#define ARRAY_TYPE_LONG   11
#define ARRAY_TYPE_FLOAT  6
#define ARRAY_TYPE_DOUBLE 7

void jvm_load_class(JVM *jvm, const char *class_file);
void parse_class_file(JVM *jvm, uint8_t *buffer, long file_size);

void parse_class_file(JVM *jvm, uint8_t *buffer, long file_size) {
    // Declaração de uma estrutura ClassFile para armazenar os dados do arquivo de classe
    ClassFile class_file;
    // Ponteiro para percorrer o buffer de bytes do arquivo de classe
    uint8_t *ptr = buffer;

    // Lê o valor mágico (primeiros 4 bytes) do arquivo de classe
    class_file.magic = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    ptr += 4;

    printf("Magic number: 0x%08x\n", class_file.magic);
    if (class_file.magic != 0xCAFEBABE) {
        fprintf(stderr, "Invalid class file magic number\n");
        return;
    }

    // Lê a versão menor (2 bytes) do arquivo de classe
    class_file.minor_version = (ptr[0] << 8) | ptr[1];
    ptr += 2;
    printf("Minor version: %d\n", class_file.minor_version);

    // Lê a versão maior (2 bytes) do arquivo de classe
    class_file.major_version = (ptr[0] << 8) | ptr[1];
    ptr += 2;
    printf("Major version: %d\n", class_file.major_version);

    // Lê a contagem do pool de constantes (2 bytes)
    class_file.constant_pool_count = (ptr[0] << 8) | ptr[1];
     printf("Constant pool count: %d\n", class_file.constant_pool_count);
    ptr += 2;

    // Aloca memória para o pool de constantes
    class_file.constant_pool = (cp_info *)malloc(sizeof(cp_info) * (class_file.constant_pool_count - 1));
    for (int i = 0; i < class_file.constant_pool_count - 1; i++) {
        // Lê o tag do pool de constantes
        class_file.constant_pool[i].tag = *ptr++;
        // Parseia as entradas do pool de constantes com base no tag
        //printf("Parsing constant pool entry %d with tag %d\n", i + 1, class_file.constant_pool[i].tag);
        
        switch (class_file.constant_pool[i].tag) {
            case CONSTANT_Class:
                class_file.constant_pool[i].info.Class.name_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;

            case CONSTANT_Utf8: {
                uint16_t length = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file.constant_pool[i].info.Utf8.length = length;
                class_file.constant_pool[i].info.Utf8.bytes = malloc(length + 1);
                memcpy(class_file.constant_pool[i].info.Utf8.bytes, ptr, length);
                class_file.constant_pool[i].info.Utf8.bytes[length] = '\0';
                ptr += length;
                break;
            }

            case CONSTANT_Methodref:
                class_file.constant_pool[i].info.Methodref.class_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file.constant_pool[i].info.Methodref.name_and_type_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                printf("Methodref: class_index=%d, name_and_type_index=%d\n", 
                    class_file.constant_pool[i].info.Methodref.class_index,
                    class_file.constant_pool[i].info.Methodref.name_and_type_index);
                break;

            case CONSTANT_Integer: {
                class_file.constant_pool[i].info.Integer.bytes = (ptr[0] << 24) | (ptr[1] << 16) | 
                                                                (ptr[2] << 8) | ptr[3];
                ptr += 4;
                break;
            }

            case CONSTANT_Fieldref:{
                class_file.constant_pool[i].info.Fieldref.class_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file.constant_pool[i].info.Fieldref.name_and_type_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            }

            case CONSTANT_NameAndType: {
                class_file.constant_pool[i].info.NameAndType.name_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file.constant_pool[i].info.NameAndType.descriptor_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            }

            case CONSTANT_String: {
                class_file.constant_pool[i].info.String.string_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            }

            case CONSTANT_InvokeDynamic:
                class_file.constant_pool[i].info.InvokeDynamic.bootstrap_method_attr_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file.constant_pool[i].info.InvokeDynamic.name_and_type_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                printf("InvokeDynamic: bootstrap_method_attr_index=%d, name_and_type_index=%d\n",
                    class_file.constant_pool[i].info.InvokeDynamic.bootstrap_method_attr_index,
                    class_file.constant_pool[i].info.InvokeDynamic.name_and_type_index);
                break;

            case CONSTANT_MethodHandle:
                class_file.constant_pool[i].info.MethodHandle.reference_kind = *ptr++;
                class_file.constant_pool[i].info.MethodHandle.reference_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;

            case CONSTANT_Double:
                class_file.constant_pool[i].info.Double.bytes = ((uint64_t)ptr[0] << 56) | ((uint64_t)ptr[1] << 48) |
                                                            ((uint64_t)ptr[2] << 40) | ((uint64_t)ptr[3] << 32) |
                                                            ((uint64_t)ptr[4] << 24) | ((uint64_t)ptr[5] << 16) |
                                                            ((uint64_t)ptr[6] << 8)  | (uint64_t)ptr[7];
                ptr += 8;
                i++; // Skip next entry for doubles
                break;

            default:
                fprintf(stderr, "Unknown constant pool tag: %d\n", class_file.constant_pool[i].tag);
                continue;
        }
    }

    //check for reasonable constant pool count
    if (class_file.constant_pool_count <= 0 || class_file.constant_pool_count > 65535) {
        fprintf(stderr, "Invalid constant pool count: %d\n", class_file.constant_pool_count);
        return;
    }  

    // Lê os flags de acesso (2 bytes)
    class_file.access_flags = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Lê o índice da classe atual (2 bytes)
    class_file.this_class = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Lê o índice da superclasse (2 bytes)
    class_file.super_class = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Lê a contagem de interfaces (2 bytes)
    class_file.interfaces_count = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Aloca memória para as interfaces
    class_file.interfaces = (uint16_t *)malloc(sizeof(uint16_t) * class_file.interfaces_count);
    for (int i = 0; i < class_file.interfaces_count; i++) {
        // Lê cada interface (2 bytes)
        class_file.interfaces[i] = (ptr[0] << 8) | ptr[1];
        ptr += 2;
    }

    // Lê a contagem de campos (2 bytes)
    class_file.fields_count = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Aloca memória para os campos
    class_file.fields = (field_info *)malloc(sizeof(field_info) * class_file.fields_count);
    for (int i = 0; i < class_file.fields_count; i++) {
        // Lê os flags de acesso do campo (2 bytes)
        class_file.fields[i].access_flags = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê o índice do nome do campo (2 bytes)
        class_file.fields[i].name_index = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê o índice do descritor do campo (2 bytes)
        class_file.fields[i].descriptor_index = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê a contagem de atributos do campo (2 bytes)
        class_file.fields[i].attributes_count = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Aloca memória para os atributos do campo
        class_file.fields[i].attributes = (attribute_info *)malloc(sizeof(attribute_info) * class_file.fields[i].attributes_count);
        for (int j = 0; j < class_file.fields[i].attributes_count; j++) {
            // Lê o índice do nome do atributo (2 bytes)
            class_file.fields[i].attributes[j].attribute_name_index = (ptr[0] << 8) | ptr[1];
            ptr += 2;

            // Lê o comprimento do atributo (4 bytes)
            class_file.fields[i].attributes[j].attribute_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;

            // Aloca memória para a informação do atributo
            class_file.fields[i].attributes[j].info = (uint8_t *)malloc(class_file.fields[i].attributes[j].attribute_length);
            for (uint32_t k = 0; k < class_file.fields[i].attributes[j].attribute_length; k++) {
                // Lê cada byte da informação do atributo
                class_file.fields[i].attributes[j].info[k] = *ptr++;
            }
        }
    }

    // Lê a contagem de métodos (2 bytes)
    class_file.methods_count = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Aloca memória para os métodos
    class_file.methods = (method_info *)malloc(sizeof(method_info) * class_file.methods_count);
    for (int i = 0; i < class_file.methods_count; i++) {
        // Lê os flags de acesso do método (2 bytes)
        class_file.methods[i].access_flags = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê o índice do nome do método (2 bytes)
        class_file.methods[i].name_index = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê o índice do descritor do método (2 bytes)
        class_file.methods[i].descriptor_index = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê a contagem de atributos do método (2 bytes)
        class_file.methods[i].attributes_count = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Aloca memória para os atributos do método
        class_file.methods[i].attributes = (attribute_info *)malloc(sizeof(attribute_info) * class_file.methods[i].attributes_count);
        for (int j = 0; j < class_file.methods[i].attributes_count; j++) {
            // Lê o índice do nome do atributo (2 bytes)
            class_file.methods[i].attributes[j].attribute_name_index = (ptr[0] << 8) | ptr[1];
            ptr += 2;

            // Lê o comprimento do atributo (4 bytes)
            class_file.methods[i].attributes[j].attribute_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4;

            // Aloca memória para a informação do atributo
            class_file.methods[i].attributes[j].info = (uint8_t *)malloc(class_file.methods[i].attributes[j].attribute_length);
            for (uint32_t k = 0; k < class_file.methods[i].attributes[j].attribute_length; k++) {
                // Lê cada byte da informação do atributo
                class_file.methods[i].attributes[j].info[k] = *ptr++;
            }
        }
    }

    // Lê a contagem de atributos da classe (2 bytes)
    class_file.attributes_count = (ptr[0] << 8) | ptr[1];
    ptr += 2;

    // Aloca memória para os atributos da classe
    class_file.attributes = (attribute_info *)malloc(sizeof(attribute_info) * class_file.attributes_count);
    for (int i = 0; i < class_file.attributes_count; i++) {
        // Lê o índice do nome do atributo (2 bytes)
        class_file.attributes[i].attribute_name_index = (ptr[0] << 8) | ptr[1];
        ptr += 2;

        // Lê o comprimento do atributo (4 bytes)
        class_file.attributes[i].attribute_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        ptr += 4;

        // Aloca memória para a informação do atributo
        class_file.attributes[i].info = (uint8_t *)malloc(class_file.attributes[i].attribute_length);
        for (uint32_t j = 0; j < class_file.attributes[i].attribute_length; j++) {
            // Lê cada byte da informação do atributo
            class_file.attributes[i].info[j] = *ptr++;
        }
    }

    // Armazena a estrutura ClassFile no campo class_file da JVM
    jvm->class_file = class_file;
}

void jvm_load_class(JVM *jvm, const char *class_file) {
    FILE *file = fopen(class_file, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: %s\n", class_file);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        fclose(file);
        return;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        fprintf(stderr, "Error reading file\n");
        free(buffer);
        fclose(file);
        return;
    }

    parse_class_file(jvm, buffer, file_size);
    free(buffer);
    fclose(file);
}

void parse_constant_pool(ClassFile *class_file, uint8_t *buffer, uint16_t constant_pool_count) {
    class_file->constant_pool = (cp_info *)malloc(sizeof(cp_info) * (constant_pool_count - 1));
    uint8_t *ptr = buffer;

    for (int i = 0; i < constant_pool_count - 1; i++) {
        class_file->constant_pool[i].tag = *ptr++;
        switch (class_file->constant_pool[i].tag) {
            case 7: // CONSTANT_Class
                class_file->constant_pool[i].info.Class.name_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 9: // CONSTANT_Fieldref
            case 10: // CONSTANT_Methodref
            case 11: // CONSTANT_InterfaceMethodref
                class_file->constant_pool[i].info.Fieldref.class_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file->constant_pool[i].info.Fieldref.name_and_type_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 8: // CONSTANT_String
                class_file->constant_pool[i].info.String.string_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 3: // CONSTANT_Integer
                class_file->constant_pool[i].info.Integer.bytes = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
                ptr += 4;
                break;
            case 4: // CONSTANT_Float
                class_file->constant_pool[i].info.Float.bytes = *(float *)ptr;
                ptr += 4;
                break;
            case 5: // CONSTANT_Long
                class_file->constant_pool[i].info.Long.bytes = ((int64_t)ptr[0] << 56) | ((int64_t)ptr[1] << 48) | ((int64_t)ptr[2] << 40) | ((int64_t)ptr[3] << 32) |
                                                              ((int64_t)ptr[4] << 24) | ((int64_t)ptr[5] << 16) | ((int64_t)ptr[6] << 8) | (int64_t)ptr[7];
                ptr += 8;
                break;
            case 6: {
                    uint8_t bytes[8];
                    memcpy(bytes, ptr, 8);
                    ptr += 8;
                
                    // Reverse bytes if the host is little-endian
                    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                        uint8_t temp;
                        for (int j = 0; j < 4; j++) {
                            temp = bytes[j];
                            bytes[j] = bytes[7 - j];
                            bytes[7 - j] = temp;
                        }
                    #endif
                
                    double value;
                    memcpy(&value, bytes, sizeof(double));
                    class_file->constant_pool[i].info.Double.bytes = value;
                    i++; // Skip next entry
                    break;
                }
            case 12: // CONSTANT_NameAndType
                class_file->constant_pool[i].info.NameAndType.name_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file->constant_pool[i].info.NameAndType.descriptor_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 1: // CONSTANT_Utf8
                class_file->constant_pool[i].info.Utf8.length = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file->constant_pool[i].info.Utf8.bytes = (uint8_t *)malloc(class_file->constant_pool[i].info.Utf8.length);
                for (int j = 0; j < class_file->constant_pool[i].info.Utf8.length; j++) {
                    class_file->constant_pool[i].info.Utf8.bytes[j] = *ptr++;
                }
                break;
            case 15: // CONSTANT_MethodHandle
                class_file->constant_pool[i].info.MethodHandle.reference_kind = *ptr++;
                class_file->constant_pool[i].info.MethodHandle.reference_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 16: // CONSTANT_MethodType
                class_file->constant_pool[i].info.MethodType.descriptor_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                break;
            case 18: // CONSTANT_InvokeDynamic
                class_file->constant_pool[i].info.InvokeDynamic.bootstrap_method_attr_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                class_file->constant_pool[i].info.InvokeDynamic.name_and_type_index = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                printf("InvokeDynamic: bootstrap_method_attr_index=%d, name_and_type_index=%d\n",
                    class_file->constant_pool[i].info.InvokeDynamic.bootstrap_method_attr_index,
                    class_file->constant_pool[i].info.InvokeDynamic.name_and_type_index);
                break;

            default:
                fprintf(stderr, "Unknown constant pool tag: %d\n", class_file->constant_pool[i].tag);
                break;
        }
    }
}