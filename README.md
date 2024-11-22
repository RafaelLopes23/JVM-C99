# Implementação de JVM em C

## Estado Atual do Projeto

### Funcionalidades Implementadas
- ✅ **Carregador de Classes**: 
  - Parsing de arquivos `.class`
  - Leitura do constant pool
  - Leitura de campos, métodos e atributos
  - Validação básica da estrutura do arquivo

- ✅ **Gerenciador de Memória**:
  - Alocação básica de heap (1MB)
  - Operações de pilha (push/pop com tamanho 1024)

- ✅ **Interpretador de Bytecode**:
  - Operações aritméticas básicas:
    - IADD (adição)
    - ISUB (subtração)
    - IMUL (multiplicação)
    - IDIV (divisão)
  - Carregamento de constantes:
    - ICONST_*
    - BIPUSH
    - SIPUSH
  - Operações com variáveis locais:
    - ILOAD
    - ISTORE
  - Operações de pilha:
    - POP
    - DUP

### Funcionalidades Pendentes
- ⚠️ Criação e gerenciamento de objetos
- ⚠️ Invocação de métodos (exceto suporte básico a INVOKEDYNAMIC)
- ⚠️ Manipulação de strings
- ⚠️ Suporte a métodos nativos
- ⚠️ Tratamento de exceções
- ⚠️ Set completo de instruções bytecode
- ⚠️ Coletor de lixo

## Como Compilar e Executar
bash
make clean
make

Compile o arquivo Java de teste
javac Test.java

Execute a JVM com o arquivo compilado
./bin/jvm Test.class


### Estrutura do Projeto
JVM/
├── src/
│   ├── [main.c](http://_vscodecontentref_/2)         (Ponto de entrada)
│   ├── [class_loader.c](http://_vscodecontentref_/3) (Parser de arquivos .class)
│   ├── [interpreter.c](http://_vscodecontentref_/4)  (Execução de bytecode)
│   └── [memory_manager.c](http://_vscodecontentref_/5) (Gerenciamento de memória)
├── include/
│   └── [jvm.h](http://_vscodecontentref_/6)         (Arquivo de cabeçalho principal)
└── [Test.java](http://_vscodecontentref_/7) 

### Estado do Desenvolvimento
O projeto atualmente implementa uma JVM básica capaz de executar operações aritméticas simples. 
O carregador de classes está funcional e o interpretador pode executar um conjunto limitado de instruções bytecode.

### Exemplo de Código Suportado

public class Test {
    public static void main(String[] args) {
        int a = 5;
        int b = 10;
        int sum = a + b;    // Suporta IADD
        int diff = b - a;   // Suporta ISUB
        int prod = a * b;   // Suporta IMUL
        int quot = b / a;   // Suporta IDIV
    }
}

### Limitações Atuais

Não suporta System.out.println()
Não suporta criação de objetos
Não suporta strings
Conjunto limitado de operações bytecode
