#include "read_count_func.h"

void leitor(const char *arquivo) {
    printf("Chamando leitor-exibidor para o arquivo: %s\n", arquivo);
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <arquivo.class>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    if (strcmp(argv[2], "--leitor") == 0) {
        read_magic_number(argv[1]); // Lê e exibe o magic number do arquivo .class
        read_minor_major(argv[1]); // Lê e exibe o minor major number
        ConstantPoolResult pos = read_constant_pool(argv[1]);
        printf("\n--------------------------------------------------------------------------------------------------------------------------------------\n");
        read_access_flags(argv[1], &pos, 0);
        read_this_class(argv[1], &pos);
        read_super_class(argv[1], &pos);
        printf("\n--------------------------------------------------------------------------------------------------------------------------------------\n");
        uint16_t i_count = read_interfaces_count(argv[1], &pos);
        display_interfaces(argv[1], i_count, &pos);
        printf("\n--------------------------------------------------------------------------------------------------------------------------------------\n");
        uint16_t f_count = read_fields_count(argv[1], &pos);
        display_fields(argv[1], f_count, &pos);
        printf("\n--------------------------------------------------------------------------------------------------------------------------------------\n");
        uint16_t m_count = read_method_count(argv[1], &pos);
        display_method(argv[1], m_count, &pos);
        printf("\n--------------------------------------------------------------------------------------------------------------------------------------\n");
        uint16_t a_count = read_atribute_count(argv[1], &pos);
        display_atribute(argv[1], a_count, &pos);
        return EXIT_SUCCESS;
    }
}