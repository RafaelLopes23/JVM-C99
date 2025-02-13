CC = gcc
CFLAGS = -Wall -g -std=c99 -Iinclude
INCLUDES = -Iinclude
SRC = src
OBJ = obj
BIN = bin
 
# Lista de arquivos fonte no diretório src e no diretório leitor-exibidor
SOURCES = $(wildcard $(SRC)/*.c) \
          leitor-exibidor/auxiliar.c \
          leitor-exibidor/read_func.c \
          leitor-exibidor/display.c \
          leitor-exibidor/read_count_func.c
 
# Lista de objetos a serem gerados (substitui barras / por \ para Windows)
OBJECTS = $(patsubst %.c, $(OBJ)\\%.o, $(SOURCES))
EXECUTABLE = $(BIN)\\jvm.exe
 
all: $(EXECUTABLE)
 
$(EXECUTABLE): $(OBJECTS)
    @if not exist "$(BIN)" mkdir "$(BIN)"
    $(CC) $(CFLAGS) $(OBJECTS) -o $@
 
# Regra para compilar arquivos .c em .o
$(OBJ)\\%.o: %.c
    @if not exist "$(dir $@)" mkdir "$(dir $@)"
    $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
 
clean:
    if exist "$(OBJ)" rmdir /S /Q "$(OBJ)"
    if exist "$(BIN)" rmdir /S /Q "$(BIN)"
 
.PHONY: all clean
