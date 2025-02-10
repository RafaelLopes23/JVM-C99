#include "display.h"


// Função para ler o arquivo .class e exibir as interfaces
void display_interfaces(const char *filename, uint16_t interfaces_count, ConstantPoolResult *dic) {
    FILE* file = abre_arquivo(filename);
    if(interfaces_count == 0){    
        printf("Nenhuma Interface Encontrada\n");

    }
    else{
        for (int i = 0; i < interfaces_count; i++) {
            printf("\n#%i\n", i + 1);

            uint16_t index;
            read_bytes(file, &index, sizeof(index), dic->position);
            index = to_big_endian_16(index);
        
            for (int i = 0; i < dic->position; i++){
                if(index == dic->constant_positions[i].index){
                    printf("Class Index: %u\n          ", index);
                    display_constant(file, dic->constant_positions[i].index, dic->constant_positions[i].position, dic->constant_positions[i].tag, dic->constant_positions, dic->constant_positions_count, 20);
                    dic->position = dic->position + 2;  
                    break;             
                }
            }
        }
    }
}

// Função para ler o arquivo .class e exibir os fields
void display_fields(const char *filename, uint16_t count, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);
    if(count == 0){
        printf("Nenhum Fields Encontrado\n");
    }
    else{
        for (int i = 0; i < count; i++){
            printf("\n#%i", i + 1);

            read_access_flags(filename, pos, 1);

            uint16_t f_index;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("          Name Index: %u\n                    Name(Utf8):", f_index);
            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    pos->position = pos->position + 2;
                    break;
                }
            }

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("          Descriptor Index: %u\n                    Descriptor(Utf8):", f_index);

            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    pos->position = pos->position + 2;
                    break;
                }
            }

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);

            printf("Atribute Count: %u\n", f_index);
            pos->position = pos->position + 2;
            if (f_index != 0){
                
                read_bytes(file, &f_index, sizeof(f_index), pos->position);

                f_index = to_big_endian_16(f_index);

                printf("          Atribute Name Index: %u\n                    Utf8: ", f_index);
                for (int i = 0; i < pos->position; i++){
                    if(f_index == pos->constant_positions[i].index){
                        display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                        pos->position = pos->position + 2;
                        break;
                    }
                }

                uint32_t f_atribute;

                read_bytes(file, &f_atribute, sizeof(f_atribute), pos->position);

                f_atribute = to_big_endian_32(f_atribute);

                printf("          Atribute Lenght: %u\n          ", f_atribute);
                pos->position = pos->position + 4;

                read_bytes(file, &f_index, sizeof(f_index), pos->position);

                f_index = to_big_endian_16(f_index);
                printf("          Value Index: %u\n                              ", f_index);
                for (int i = 0; i < pos->position; i++){
                    if(f_index == pos->constant_positions[i].index){
                        display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                        pos->position = pos->position + f_atribute ;
                        break;
                    }
                }
            }
        }
    }
}

// Função para ler o arquivo .class e exibir os metodos
void display_method(const char *filename, uint16_t count, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);
    if(count == 0){
        printf("Nenhum Method Encontrado\n");
    }
    else{
        for (int i = 0; i < count; i++){

            printf("\n#%i", i + 1);

            read_access_flags(filename, pos, 2);

            uint16_t f_index;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("          Name Index: %u\n                    Name(Utf8):", f_index);
            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    pos->position = pos->position + 2;
                    break;
                }
            }

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("          Descriptor Index: %u\n                    Descriptor(Utf8):", f_index);

            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    pos->position = pos->position + 2;
                    break;
                }
            }

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);

            printf("Atribute Count: %u\n", f_index);

            uint16_t amount = f_index;

            char* result;

            pos->position = pos->position + 2;
            if (f_index != 0){
                for (int i = 0; i < amount; i++){
                    read_bytes(file, &f_index, sizeof(f_index), pos->position);

                    f_index = to_big_endian_16(f_index);

                    printf("          Atribute Name Index: %u\n                    Utf8: ", f_index);
                    for (int i = 0; i < pos->position; i++){
                        if(f_index == pos->constant_positions[i].index){
                            result = display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                            pos->position = pos->position + 2;
                            break;
                        }
                    }

                    if (strcmp(result, "Code") == 0){
                        display_atribute_info(filename, pos, 1);
                        free(result);
                    }
                }   
            }
        }
    }
}

// Função para ler o arquivo .class e exibir os atributos
void display_atribute(const char *filename, uint16_t count, ConstantPoolResult *pos){
    FILE* file = abre_arquivo(filename);
    if(count == 0){
        printf("Nenhum Atribute Encontrado\n");
    }
    else{
        for (int i = 0; i < count; i++){
            printf("\n#%i\n", i + 1);

            uint16_t f_index;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("Atribute Name Index: %u\n          Name(Utf8): ", f_index);
            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    pos->position = pos->position + 2;
                    break;
                }
            }

            uint32_t f_atribute;

            read_bytes(file, &f_atribute, sizeof(f_atribute), pos->position);

            f_atribute = to_big_endian_32(f_atribute);

            printf("Atribute Lenght: %u\n", f_atribute);

            pos->position = pos->position + 4;

            read_bytes(file, &f_index, sizeof(f_index), pos->position);

            f_index = to_big_endian_16(f_index);
            printf("Name Index: %u\n          ", f_index);
            for (int i = 0; i < pos->position; i++){
                if(f_index == pos->constant_positions[i].index){
                    display_constant(file, pos->constant_positions[i].index, pos->constant_positions[i].position, pos->constant_positions[i].tag, pos->constant_positions, pos->constant_positions_count, 10);
                    break;
                }
            }
        }
    
    }
}