#include "read_func.h"

ConstantPoolResult result_struct;

char* display_constant(FILE *file, uint16_t index, long position, uint8_t tag, IndexPosition *dic, size_t constant_positions_count, int indent) {
    
    switch (tag) {
        case 1: { // CONSTANT_Utf8
            uint16_t length;
            
            // Lê o comprimento da string UTF-8
            read_bytes(file, &length, sizeof(length), position + 1);
            length = to_big_endian_16(length); // Ajuste de endianess

            // Aloca memória para a string UTF-8
            char *utf8_string = malloc(length + 1); // +1 para o terminador nulo '\0'
            if (utf8_string == NULL) {
                perror("Erro ao alocar memória para a string UTF-8");
                exit(EXIT_FAILURE);
            }

            // Lê os bytes da string UTF-8
            read_bytes(file, utf8_string, length, position + 3);

            // Adiciona o terminador nulo para a string
            utf8_string[length] = '\0';
            
            // Exibe a string UTF-8
            printf("%s\n", utf8_string);
            if (strcmp(utf8_string, "Code") == 0){
                strcpy(utf8_string, "Code");
                return utf8_string;
            }
            
            else if (strcmp(utf8_string, "LineNumberTable") == 0){
                strcpy(utf8_string, "LineNumberTable");
                return utf8_string;
            }
            free(utf8_string);
            break;
        }
        case 3: { // CONSTANT_Integer
            uint32_t value;
            read_bytes(file, &value, sizeof(value), position + 1); // Lê o valor inteiro
            value = to_big_endian_32(value); // Ajuste de endianess
            printf("Integer: %u\n", value);
            break;
        }
        case 4: { // CONSTANT_Float
            uint32_t value;
            read_bytes(file, &value, sizeof(value), position + 1); // Lê o valor float
            value = to_big_endian_32(value); // Ajuste de endianess
            printf("Float: %f\n", *((float*)&value)); // Converte para float
            break;
        }
        case 5: { // CONSTANT_Long
            uint32_t high_bytes, low_bytes;
            
            // Lê os 4 bytes para high_bytes e low_bytes
            read_bytes(file, &high_bytes, sizeof(high_bytes), position + 1); // Lê o high_bytes
            read_bytes(file, &low_bytes, sizeof(low_bytes), position + 5);  // Lê o low_bytes
            


            // Ajuste de endianess para ambos os valores
            high_bytes = to_big_endian_32(high_bytes);
            low_bytes = to_big_endian_32(low_bytes);
            
            // Combina os 2 valores de 32 bits para formar o valor de 64 bits
            uint64_t value = ((uint64_t)high_bytes << 32) | (uint64_t)low_bytes;

            // Converte o valor combinado para um inteiro com sinal de 64 bits
            int64_t long_value = (int64_t)value;

            // Exibe o valor longo
            printf("Long: %" PRId64 "\n", long_value);
            break;
        }
        case 6: { // CONSTANT_Double
            uint32_t high_bytes, low_bytes;
            
            // Lê os 4 bytes para high_bytes e low_bytes
            read_bytes(file, &high_bytes, sizeof(high_bytes), position + 1); // Lê o high_bytes
            read_bytes(file, &low_bytes, sizeof(low_bytes), position + 5);  // Lê o low_bytes
            
            // Ajuste de endianess para ambos os valores
            high_bytes = to_big_endian_32(high_bytes);
            low_bytes = to_big_endian_32(low_bytes);
            
            // Combina os 2 valores de 32 bits para formar o valor de 64 bits
            uint64_t value = ((uint64_t)high_bytes << 32) | (uint64_t)low_bytes;
            
            // Converte o valor de 64 bits para um double
            double double_value;
            memcpy(&double_value, &value, sizeof(double)); // Converte o valor de 64 bits para double
            
            // Exibe o valor double
            printf("Double: %f\n", double_value);
    
            break;
        }
        case 7: { // CONSTANT_Class
        uint16_t class_index;
        
        // Lê o índice da classe
        read_bytes(file, &class_index, sizeof(class_index), position + 1);
        // Ajusta a ordem dos bytes para big-endian
        class_index = to_big_endian_16(class_index);
        printf("Class: Name index %u\n%*s",class_index, indent, "");
        // Exibe o nome da classe
        printf("Name(Utf8): ");

        position = find_position_by_index(dic, class_index, constant_positions_count);
        display_constant(file, class_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao índice

        break;
        }
        case 8: { // CONSTANT_String
            uint16_t string_index;
            read_bytes(file, &string_index, sizeof(string_index), position + 1); // Lê o string_index
            string_index = to_big_endian_16(string_index);
            printf("String: String index %u\n", string_index);

            position = find_position_by_index(dic, string_index, constant_positions_count);
            printf("String(Utf8): ");
            display_constant(file, string_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao índice

            break;
        }
        case 9: { // CONSTANT_Fieldref      
            uint16_t class_index, name_and_type_index;
            // Lê os índices da classe e do name_and_type
            read_bytes(file, &class_index, sizeof(class_index), position + 1); // Lê o class_index
            read_bytes(file, &name_and_type_index, sizeof(name_and_type_index), position + 3); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            class_index = to_big_endian_16(class_index);
            name_and_type_index = to_big_endian_16(name_and_type_index);

            // Exibe os índices
            printf("Fieldref: Class index %u, Name and Type index %u\n%*s", class_index, name_and_type_index,  indent, "");

            position = find_position_by_index(dic, class_index, constant_positions_count);
            // Exibe os detalhes da classe
            display_constant(file, class_index, position, 7, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Class associado ao índice

            printf("%*s", indent, "");
            position = find_position_by_index(dic, name_and_type_index, constant_positions_count);
            // Exibe os detalhes do name_and_type
            display_constant(file, name_and_type_index, position, 12, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_NameAndType associado ao índice

            break;
        }
        case 10: { // CONSTANT_Methodref
            uint16_t class_index, name_and_type_index;
            // Lê os índices da classe e do name_and_type
            read_bytes(file, &class_index, sizeof(class_index), position + 1); // Lê o class_index
            read_bytes(file, &name_and_type_index, sizeof(name_and_type_index), position + 3); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            class_index = to_big_endian_16(class_index);
            name_and_type_index = to_big_endian_16(name_and_type_index);

            // Exibe os índices
            printf("Methodref: Class index %u, Name and Type index %u\n%*s", class_index, name_and_type_index, indent, "");

            position = find_position_by_index(dic, class_index, constant_positions_count);
            // Exibe os detalhes da classe
            display_constant(file, class_index, position, 7, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Class associado ao índice

            printf("%*s", indent, "");
            position = find_position_by_index(dic, name_and_type_index, constant_positions_count);
            // Exibe os detalhes do name_and_type
            display_constant(file, name_and_type_index, position, 12, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_NameAndType associado ao índice

            break;
        }
        case 11: { // CONSTANT_InterfaceMethodref
            uint16_t class_index, name_and_type_index;
            // Lê os índices da classe e do name_and_type
            read_bytes(file, &class_index, sizeof(class_index), position + 1); // Lê o class_index
            read_bytes(file, &name_and_type_index, sizeof(name_and_type_index), position + 3); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            class_index = to_big_endian_16(class_index);
            name_and_type_index = to_big_endian_16(name_and_type_index);

            // Exibe os índices
            printf("InterfaceMethodref: Class index %u, Name and Type index %u\n", class_index, name_and_type_index);

            position = find_position_by_index(dic, class_index, constant_positions_count);
            // Exibe os detalhes da classe
            display_constant(file, class_index, position, 7, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Class associado ao índice

            position = find_position_by_index(dic, name_and_type_index, constant_positions_count);
            // Exibe os detalhes do name_and_type
            display_constant(file, name_and_type_index, position, 12, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_NameAndType associado ao índice

            break;
        }
        case 12: { // CONSTANT_NameAndType
            uint16_t name_index, type_index;

            // Lê os índices para o nome e o tipo
            read_bytes(file, &name_index, sizeof(name_index), position + 1); // Lê o name_index
            read_bytes(file, &type_index, sizeof(type_index), position + 3); // Lê o type_index
            
            // Ajusta a ordem dos bytes para big-endian
            name_index = to_big_endian_16(name_index);
            type_index = to_big_endian_16(type_index);

            // Exibe os índices de nome e tipo
            printf("Name and Type: Name index %u, Descriptor index %u\n%*s", name_index, type_index, indent, "");
            
            position = find_position_by_index(dic, name_index, constant_positions_count);
            // Exibe o nome
            printf("Name(Utf8): ");
            display_constant(file, name_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao nome
            
            position = find_position_by_index(dic, type_index, constant_positions_count);
            // Exibe o tipo
            printf("%*sDescriptor(Utf8): ", indent, "");
            display_constant(file, type_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao tipo

            break;
        }
        case 15: { // CONSTANT_MethodHandle
            uint8_t reference_kind;
            uint16_t reference_index;
            // Lê os índices da classe e do name_and_type
            read_bytes(file, &reference_kind, sizeof(reference_kind), position + 1); // Lê o class_index
            read_bytes(file, &reference_index, sizeof(reference_index), position + 2); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            reference_index = to_big_endian_16(reference_index);

            // Exibe os índices
            printf("MethodHandle: Reference Kind: %u", reference_kind);

            position = find_position_by_index(dic, reference_index, constant_positions_count);
            // Exibe os detalhes do name_and_type
            if(reference_kind == 1 || reference_kind == 2 || reference_kind == 3 || reference_kind == 4 ){
            display_constant(file, reference_index, position, 9, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Fieldref associado ao índice
            }
            else if(reference_kind == 5 || reference_kind == 8){
            display_constant(file, reference_index, position, 10, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Methodref associado ao índice
            }
            else if(reference_kind == 6 || reference_kind == 7){
            display_constant(file, reference_index, position, 9, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Fieldref associado ao índice
            }
            else if(reference_kind == 9){
            display_constant(file, reference_index, position, 11, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_InterfaceMethodref associado ao índice
            }
            break;
        }
        case 16: { // CONSTANT_MethodType
        uint16_t descriptor_index;
        
        // Lê o índice da classe
        read_bytes(file, &descriptor_index, sizeof(descriptor_index), position + 1);
        // Ajusta a ordem dos bytes para big-endian
        descriptor_index = to_big_endian_16(descriptor_index);
        printf("MethodType: Descriptor index %u\n",descriptor_index);
        // Exibe o nome da classe
        printf("Descriptor(Utf8): ");

        position = find_position_by_index(dic, descriptor_index, constant_positions_count);
        display_constant(file, descriptor_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao índice

        break;

        }
        case 17: { // CONSTANT_Dynamic
            uint16_t name_and_type_index;
            
            // Lê os índices da classe e do name_and_type
            read_bytes(file, &name_and_type_index, sizeof(name_and_type_index), position + 3); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            name_and_type_index = to_big_endian_16(name_and_type_index);

            printf("CONSTANT_Dynamic: Name and Type index %u\n",  name_and_type_index);

            position = find_position_by_index(dic, name_and_type_index, constant_positions_count);
            // Exibe os detalhes do name_and_type
            display_constant(file, name_and_type_index, position, 12, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_NameAndType associado ao índice

            break;
        }
        case 18: { // CONSTANT_InvokeDynamic
            uint16_t name_and_type_index;

            // Lê os índices da classe e do name_and_type
            read_bytes(file, &name_and_type_index, sizeof(name_and_type_index), position + 3); // Lê o name_and_type_index

            // Ajusta a ordem dos bytes para big-endian
            name_and_type_index = to_big_endian_16(name_and_type_index);

            printf("CONSTANT_InvokeDynamic: Name and Type index %u\n",  name_and_type_index);

            position = find_position_by_index(dic, name_and_type_index, constant_positions_count);

            // Exibe os detalhes do name_and_type
            display_constant(file, name_and_type_index, position, 12, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_NameAndType associado ao índice

            break;
        }
        case 19: { // CONSTANT_Module
            uint16_t name_index;

            read_bytes(file, &name_index, sizeof(name_index), position + 1); // Lê o name_index
            name_index = to_big_endian_16(name_index);
            printf("Module: Name index %u\n", name_index);

            position = find_position_by_index(dic, name_index, constant_positions_count);
            printf("Name(Utf8): ");
            display_constant(file, name_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao índice

            break;
        }
        case 20:{ // CONSTANT_Package
            uint16_t name_index;

            read_bytes(file, &name_index, sizeof(name_index), position + 1); // Lê o name_index
            name_index = to_big_endian_16(name_index);
            printf("Package: Name index %u\n", name_index);

            position = find_position_by_index(dic, name_index, constant_positions_count);
            printf("Name(Utf8): ");
            display_constant(file, name_index, position, 1, dic, constant_positions_count, indent + 10); // Exibe o CONSTANT_Utf8 associado ao índice

            break;
        }
        default:
            printf("Unknown constant type (tag %d)\n", tag);
            break;
        
    }
}

void display_atribute_info(const char *filename, ConstantPoolResult *pos, uint16_t tag){
    FILE *file = fopen(filename, "rb");

    uint16_t f_index;

    uint32_t f_atribute;
    switch (tag)
    {
    case 1: {
            read_bytes(file, &f_atribute, sizeof(f_atribute), pos->position);

            f_atribute = to_big_endian_32(f_atribute);

            printf("          Atribute Lenght: %u\n          ", f_atribute);
            pos->position = pos->position + 4;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);

            printf("Maximum Stack Size: %u\n", f_index);

            pos->position = pos->position + 2;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);

            printf("          Maximum Local Variables: %u\n", f_index);

            pos->position = pos->position + 2;

            read_bytes(file, &f_atribute, sizeof(f_atribute), pos->position);

            f_atribute = to_big_endian_32(f_atribute);

            printf("          Code Lenght: %u\n", f_atribute);

            pos->position = pos->position + 4;

            long dic_pos;
            long position = pos->position;
            while (position < pos->position + f_atribute) {
                uint8_t opcode;
                read_bytes(file, &opcode, sizeof(opcode), position); // Lê o opcode
                position++;

                printf("%04ld: %s", position - pos->position - 1, opcode_table[opcode].mnemonic);

                uint8_t operand_count = opcode_table[opcode].operand_count;
                uint8_t operands[2] = {0}; // Para armazenar operandos (máx. 2 bytes)

                if (operand_count > 0) {
                    read_bytes(file, operands, operand_count, position); // Lê os operandos
                    position += operand_count;

                    // Se for uma referência à constant pool, converte para inteiro
                    if (operand_count == 2) {
                        uint16_t index = (operands[0] << 8) | operands[1]; // Converte para big-endian
                        printf(" (#%d)\n", index);

                        for (int i = 0; i < pos->position; i++){
                        if(index == pos->constant_positions[i].index){
                            display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                            break;
                        }
                    }
                    
                    }


                }

                printf("\n"); // Nova linha para próximo opcode
            }
            pos->position = position;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);

            printf("\nException Table Lenght: %u\n", f_index);

            pos->position = pos->position + 2;

            if(f_index != 0){
                for (int i = 1; i < f_index; i++){

                    read_bytes(file, &f_index, sizeof(f_index), pos->position);

                    f_index = to_big_endian_16(f_index);
                    
                    printf("\nStart PC: %u\n", f_index);

                    pos->position = pos->position + 2;

                    read_bytes(file, &f_index, sizeof(f_index), pos->position);

                    f_index = to_big_endian_16(f_index);
                    
                    printf("\nEnd PC: %u\n", f_index);

                    pos->position = pos->position + 2;

                    read_bytes(file, &f_index, sizeof(f_index), pos->position);

                    f_index = to_big_endian_16(f_index);
                    
                    printf("\nHandler PC: %u\n", f_index);

                    pos->position = pos->position + 2;

                    read_bytes(file, &f_index, sizeof(f_index), pos->position);

                    f_index = to_big_endian_16(f_index);
                    
                    printf("\nCatch Type: %u\n", f_index);

                    pos->position = pos->position + 2;

                }
            }
            
            read_bytes(file, &f_index, sizeof(f_index), pos->position);
            
            f_index = to_big_endian_16(f_index);
                
            printf("\nCode Atribute Count: %u\n", f_index);

            pos->position = pos->position + 2;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);
            
            f_index = to_big_endian_16(f_index);
                
            printf("\nCode Atribute Name Index: %u\n          ", f_index);

            pos->position = pos->position + 2;

            char* result;

            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    result = display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    break;
                }
            }
            if (strcmp(result, "LineNumberTable") == 0){
                display_atribute_info(filename, pos, 2);
                free(result);
            }
        break;
        }
    
    case 2: {            
            
            read_bytes(file, &f_atribute, sizeof(f_atribute), pos->position);

            f_atribute = to_big_endian_32(f_atribute);
                
            printf("\nCode Atribute Lenght: %u\n          ", f_atribute);
            
            pos->position = pos->position + 4;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);
            
            f_index = to_big_endian_16(f_index);

            pos->position = pos->position + 2;

            printf("\nNr.       Start Pc        Line number\n");
            
            uint16_t temp;
            
            for (int i = 0; i < f_index; i++){

                read_bytes(file, &temp, sizeof(temp), pos->position);
                
                temp = to_big_endian_16(temp);

                printf("%i        %u                ", i, temp);

                pos->position = pos->position + 2;

                read_bytes(file, &temp, sizeof(temp), pos->position);
                
                temp = to_big_endian_16(temp);

                printf("%u\n", temp);

                pos->position = pos->position + 2;
            }

        break;
    }
    
    default:
        break;
    }
}

void read_magic_number(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    uint32_t magic;
    read_bytes(file, &magic, sizeof(magic), 0);

    fclose(file);

    magic = to_big_endian_32(magic);

    if (magic == 0xCAFEBABE) {
        printf("Magic number: 0x%X (Arquivo valido .class)\n", magic);
    } else {
        printf("Magic number: 0x%X (Nao e um arquivo valido .class)\n", magic);
    }
}

void read_minor_major(const char *filename) {
    FILE *file = abre_arquivo(filename);
    uint16_t major_version, minor_version;

    read_bytes(file, &minor_version, sizeof(minor_version), 4);
    read_bytes(file, &major_version, sizeof(major_version), 6);

    minor_version = to_big_endian_16(minor_version);
    major_version = to_big_endian_16(major_version);

    printf("Minor version: %u\n", minor_version);
    printf("Major version: %u\n", major_version);

    fclose(file);
}

void read_access_flags(const char *filename, ConstantPoolResult *pos, int id) {
    FILE *file = abre_arquivo(filename);

    uint16_t access_flags;

    read_bytes(file, &access_flags, sizeof(access_flags), pos->position);

    pos->position += 2;

    access_flags = to_big_endian_16(access_flags);

    printf("\nAccess Flags: (0x%04X), ", access_flags);

    if (id == 0) {
        if (access_flags & 0x0001) printf("ACC_PUBLIC ");
        if (access_flags & 0x0010) printf("ACC_FINAL");
        if (access_flags & 0x0020) printf("ACC_SUPER ");
        if (access_flags & 0x0200) printf("ACC_INTERFACE ");
        if (access_flags & 0x0400) printf("ACC_ABSTRACT ");
        if (access_flags & 0x1000) printf("ACC_SYNTHETIC ");
        if (access_flags & 0x2000) printf("ACC_ANNOTATION ");
        if (access_flags & 0x4000) printf("ACC_ENUM ");
        if (access_flags & 0x8000) printf("ACC_MODULE ");
    } else if (id == 1) {
        if (access_flags & 0x0001) printf("ACC_PUBLIC ");
        if (access_flags & 0x0002) printf("ACC_PRIVATE ");
        if (access_flags & 0x0004) printf("ACC_PROTECTED ");
        if (access_flags & 0x0008) printf("ACC_STATIC ");
        if (access_flags & 0x0010) printf("ACC_FINAL ");
        if (access_flags & 0x0040) printf("ACC_VOLATILE ");
        if (access_flags & 0x0080) printf("ACC_TRANSIENT ");
        if (access_flags & 0x1000) printf("ACC_SYNTHETIC ");
        if (access_flags & 0x4000) printf("ACC_ENUM ");
    } else {
        if (access_flags & 0x0001) printf("ACC_PUBLIC ");
        if (access_flags & 0x0002) printf("ACC_PRIVATE ");
        if (access_flags & 0x0004) printf("ACC_PROTECTED ");
        if (access_flags & 0x0008) printf("ACC_STATIC ");
        if (access_flags & 0x0010) printf("ACC_FINAL ");
        if (access_flags & 0x0020) printf("ACC_SYNCHRONIZED ");
        if (access_flags & 0x0040) printf("ACC_BRIDGE ");
        if (access_flags & 0x0080) printf("ACC_VARARGS ");
        if (access_flags & 0x0100) printf("ACC_NATIVE ");
        if (access_flags & 0x0400) printf("ACC_ABSTRACT ");
        if (access_flags & 0x0800) printf("ACC_STRICT ");
        if (access_flags & 0x1000) printf("ACC_SYNTHETIC ");
    }
    printf("\n");

    fclose(file);
}

void read_this_class(const char *filename, ConstantPoolResult *pos) {
    FILE *file = abre_arquivo(filename);

    uint16_t this_class_index;

    read_bytes(file, &this_class_index, sizeof(this_class_index), pos->position);

    pos->position += 2;

    this_class_index = to_big_endian_16(this_class_index);

    printf("\nThis Class: Index %u\n          ", this_class_index);

    display_constant(file, this_class_index, pos->constant_positions[this_class_index - 1].position, pos->constant_positions[this_class_index - 1].tag, pos->constant_positions, pos->constant_positions_count, 20);
}

void read_super_class(const char *filename, ConstantPoolResult *pos) {
    FILE *file = abre_arquivo(filename);

    uint16_t super_class_index;

    read_bytes(file, &super_class_index, sizeof(super_class_index), pos->position);

    pos->position += 2;

    super_class_index = to_big_endian_16(super_class_index);

    printf("\nSuper Class: Index %u\n          ", super_class_index);

    display_constant(file, super_class_index, pos->constant_positions[super_class_index - 1].position, pos->constant_positions[super_class_index - 1].tag, pos->constant_positions, pos->constant_positions_count, 20);
}
