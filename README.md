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
```bash
make clean
make
```

Compile o arquivo Java de teste
```
javac Test.java
```

Execute a JVM com o arquivo compilado
```
./bin/jvm Test.class
```
## Como Compilar e Executar o Leitor-Exibidor

Compile
```
gcc -o leitor leitor-exibidor/main.c leitor-exibidor/auxiliar.c leitor-exibidor/read_func.c leitor-exibidor/display.c leitor-exibidor/read_count_func.c

```

Execute a o Leitor com o arquivo .class desejado

```
./leitor exemplos\Carta.class --leitor
```
Onde "--leitor" é a flag para chamar o leitor-exibidor

### Estrutura do Projeto

```JVM/
├── src/
│   ├── [main.c](http://_vscodecontentref_/2)         (Ponto de entrada)
│   ├── [class_loader.c](http://_vscodecontentref_/3) (Parser de arquivos .class)
│   ├── [interpreter.c](http://_vscodecontentref_/4)  (Execução de bytecode)
│   └── [memory_manager.c](http://_vscodecontentref_/5) (Gerenciamento de memória)
├── include/
│   └── [jvm.h](http://_vscodecontentref_/6)         (Arquivo de cabeçalho principal)
└── [Test.java](http://_vscodecontentref_/7) 
```

### Estado do Desenvolvimento
O projeto atualmente implementa uma JVM básica capaz de executar operações aritméticas simples. 
O carregador de classes está funcional e o interpretador pode executar um conjunto limitado de instruções bytecode.

### Exemplo de Código Suportado
```
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
```
### Limitações Atuais

Não suporta criação de objetos, arrays
Não suporta strings
Conjunto limitado de operações bytecode
- 94 bytecodes implementados 
```
GETSTATIC
INVOKEVIRTUAL
LNEG
DSTORE
RETURN
I2F
IINC
GOTO_W
GOTO
LDC
SIPUSH
IFEQ
IFLT
IFGE
IFGT
IFLE
IFNE
IF_ICMPEQ
IF_ICMPNE
IF_ICMPLT
IF_ICMPGE
IF_ICMPGT
IF_ICMPLE
NOP
ICONST_M1
ICONST_0
ICONST_1
ICONST_2
ICONST_3
ICONST_4
ICONST_5
IADD
ISUB
IMUL
IDIV
IOR
ILOAD
ILOAD_0
ILOAD_1
ILOAD_2
ILOAD_3
ISTORE_0
ISTORE_1
ISTORE_2
ISTORE_3
DUP
POP
FLOAD
FLOAD_1
FLOAD_2
FLOAD_3
DADD
DLOAD
DLOAD_0
DLOAD_1
DLOAD_2
DLOAD_3
DREM
DSUB
DMUL
DDIV
DNEG
BIPUSH
LADD
LSUB
LMUL
LDIV
LREM
LDC2_W
DCONST_0
DCONST_1
LCONST_0
LCONST_1
LLOAD
LLOAD_0
LLOAD_1
LLOAD_2
LLOAD_3
LCMP
LASTORE
LAND
LALOAD
DSTORE_0
DSTORE_1
DSTORE_2
DSTORE_3
DCMPL
DCMPG
D2F
D2I
D2L
NEW
NEWARRAY
IRETURN
```

- opcodes não implementados

````
AALOAD
AASTORE
ACONST_NULL
ALOAD
ALOAD_0
ALOAD_1
ALOAD_2
ALOAD_3
ANEWARRAY
ARETURN
ARRAYLENGTH
ASTORE
ASTORE_0
ASTORE_1
ASTORE_2
ASTORE_3
ATHROW
BALOAD
BASTORE
CALOAD
CASTORE
CHECKCAST
DALOAD
DASTORE
DRETURN
DUP2
DUP2_X1
DUP2_X2
DUP_X1
DUP_X2
F2D
F2I
F2L
FADD
FALOAD
FASTORE
FCMPG
FCMPL
FCONST_0
FCONST_1
FCONST_2
FDIV
FMUL
FNEG
FREM
FRETURN
FSTORE
FSTORE_0
FSTORE_1
FSTORE_2
FSTORE_3
FSUB
GETFIELD
I2B
I2C
I2D
I2F
I2L
I2S
IADD
IALOAD
IAND
IASTORE
IF_ACMPEQ
IF_ACMPNE
IFNONNULL
IFNULL
INEG
INSTANCEOF
INVOKEDYNAMIC
INVOKEINTERFACE
INVOKESPECIAL
INVOKESTATIC
IREM
IRETURN
ISHL
ISHR
IUSHR
IXOR
JSR
JSR_W
L2D
L2F
L2I
LDC_W
LOOKUPSWITCH
LOR
LREM
LRETURN
LSHL
LSHR
LSTORE
LSTORE_0
LSTORE_1
LSTORE_2
LSTORE_3
LSUB
LUSHR
LXOR
MONITORENTER
MONITOREXIT
MULTIANEWARRAY
POP2
PUTFIELD
PUTSTATIC
RET
SALOAD
SASTORE
SWAP
TABLESWITCH
WIDE
````