#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strcmp
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

// pro DREM e FREM
#define IS_NAN(x)  ((*(uint64_t*)&(x) & 0x7FF8000000000000ULL) == 0x7FF8000000000000ULL)
#define IS_INF(x) (((*(uint64_t*)&(x) & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) && !IS_NAN(x))


#define CONSTANT_Class              7
#define CONSTANT_Fieldref           9
#define CONSTANT_Methodref          10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String             8
#define CONSTANT_Integer            3
#define CONSTANT_Float              4
#define CONSTANT_Long               5
#define CONSTANT_Double             6
#define CONSTANT_NameAndType        12
#define CONSTANT_Utf8               1
#define CONSTANT_MethodHandle       15
#define CONSTANT_MethodType         16
#define CONSTANT_InvokeDynamic      18
#define ARRAY_TYPE_INT    10
#define ARRAY_TYPE_LONG   11
#define ARRAY_TYPE_FLOAT  6
#define ARRAY_TYPE_DOUBLE 7

#define STACK_SIZE 1024

#define CHECK_STACK(stack, required) \
    if ((stack)->size < (required)) { \
        fprintf(stderr, "Stack underflow - need %d values but have %d\n", \
                required, (stack)->size); \
        return; \
    }

const char* get_constant_pool_string(ClassFile *class_file, uint16_t index);
const char* get_string_constant(JVM *jvm, uint16_t index);
void invoke_method(JVM *jvm, void *method_handle);

bool operand_stack_push(OperandStack *stack, int32_t value);
bool operand_stack_pop(OperandStack *stack, int32_t *value);
void operand_stack_push_cat2(OperandStack *stack, Cat2 val);
Cat2 operand_stack_pop_cat2(OperandStack *stack);
bool validate_constant_pool_index(ClassFile *class_file, uint16_t index);
void operand_stack_init(OperandStack *stack, int capacity);
void print_stack_state(OperandStack *stack);

typedef struct {
    int32_t length;
    void *elements;
    uint8_t type; // ARRAY_TYPE_INT, etc.
} Array;

typedef struct {
    ClassFile *class;
    void *fields;
} Object;

typedef struct {
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type;
} ExceptionHandler;

static void handle_nop(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++;
}

static void handle_iconst(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t value = opcode - ICONST_0;
    operand_stack_push(stack, value);
    printf("ICONST_%d: Pushed %d\n", value, value);
    (*pc)++;
}

// Math operations
static void handle_iadd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 + val2;
    operand_stack_push(stack, result);
    printf("IADD: %d + %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_isub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 - val2;
    operand_stack_push(stack, result);
    printf("ISUB: %d - %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_imul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 * val2;
    operand_stack_push(stack, result);
    printf("IMUL: %d * %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_idiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    if (val2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return;
    }
    int32_t result = val1 / val2;
    operand_stack_push(stack, result);
    printf("IDIV: %d / %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_ior(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 | val2;
    operand_stack_push(stack, result);
    printf("IOR: %d | %d = %d\n", val1, val2, result);
    (*pc)++;
}

// Load/Store operations

static void handle_fload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index ;
    if(opcode == FLOAD)
        index = bytecode[(*pc) + 1];
    else
        index = opcode - FLOAD_0;
    operand_stack_push(stack, locals[index]); 
    (*pc)++;
}


// Load/Store operations
static void handle_dload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    
    if (opcode == DLOAD) {
        index = bytecode[(*pc) + 1];
        (*pc) += 2;
    } else {
        index = opcode - DLOAD_0;
        (*pc)++;
    }
    
    Cat2 value;
    value.high = locals[index];
    value.low = locals[index + 1];
    operand_stack_push_cat2(stack, value);
    printf("DLOAD_%d: Loaded double from locals[%d,%d]\n", 
           index, index, index + 1);
}


static void handle_istore_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index, value;

    if(opcode == ISTORE)
        index = bytecode[(*pc) + 1];
    else
        index = opcode - ISTORE_0;

    operand_stack_pop(stack, &value);
    locals[index] = value;
    printf("ISTORE_%d: Stored %d\n", index, value);
    (*pc)++;
}


static void handle_iload_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index;

    if(opcode == ILOAD)
        index = bytecode[(*pc) + 1];
    else
        index = opcode - ILOAD_0;

    operand_stack_push(stack, locals[index]);
    printf("ILOAD_%d: Loaded %d\n", index, locals[index]);
    (*pc)++;
}

static void handle_bipush(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int8_t value = (int8_t)bytecode[(*pc) + 1];
    operand_stack_push(stack, value);
    printf("BIPUSH: Pushed %d\n", value);
    *pc += 2;
}

static void handle_return(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++;
}

// Stack operations
// DUP (duplicate top value)
static void handle_dup(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    operand_stack_push(stack, value);
    operand_stack_push(stack, value);
    printf("DUP: Duplicated %d\n", value);
    (*pc)++;
}

// POP (remove top value)
static void handle_pop(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    printf("POP: Removed %d\n", value);
    (*pc)++;
}

// 64-bit operations (category 2)
static void handle_dadd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    CHECK_STACK(stack, 4);  // Need 4 slots for 2 doubles
    
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    
    Cat2 result;
    result.double_ = val1.double_ + val2.double_;
    operand_stack_push_cat2(stack, result);
    
    printf("DADD: %f + %f = %f\n", val1.double_, val2.double_, result.double_);
    (*pc)++;
}

static void handle_ladd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.long_ = val1.long_ + val2.long_; 
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_lsub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.long_ = val1.long_ - val2.long_;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_lmul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.long_ = val1.long_ * val2.long_;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_ldiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    if (val2.long_ == 0) {
        fprintf(stderr, "Divisão por zero\n");
        return; 
    }
    Cat2 result;
    result.long_ = val1.long_ / val2.long_;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_lrem(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    if (val2.long_ == 0) {
        fprintf(stderr, "Divisão por zero\n");
        return; 
    }
    Cat2 result;
    result.long_ = val1.long_ % val2.long_;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_drem(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);

    double value1 = val1.double_;
    double value2 = val2.double_;


    if (IS_NAN(value1) || IS_NAN(value2) || IS_INF(value1) || value2 == 0.0 || IS_INF(value2)) {
        fprintf(stderr, "!!Operando invalido (drem)\n");
        (*pc)++;
        return;
    }

    if (value1 == 0.0 && !IS_INF(value2) && value2 != 0.0) {
        operand_stack_push_cat2(stack, val1);
        (*pc)++;
        return;
    }

    if (!IS_INF(value1) && IS_INF(value2)) {
        operand_stack_push_cat2(stack, val1);
        (*pc)++;
        return;
    }


    long long int_q = (long long) value1 / value2;

    double result = value1 - (value2 * int_q);

    val1.double_ = result; 
    operand_stack_push_cat2(stack, val1);
    (*pc)++;
}

// Carregamento de Constantes (long e double)
static void handle_lconst(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int64_t value = bytecode[*pc] - LCONST_0; 
    Cat2 cat2;
    cat2.long_ = value;
    operand_stack_push_cat2(stack, cat2);
    (*pc)++;
}

static void handle_dconst(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    double value = bytecode[*pc] - DCONST_0; 
    Cat2 cat2;
    cat2.double_ = value;
    operand_stack_push_cat2(stack, cat2);
    (*pc)++;
}


static void handle_ldc2_w(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
    
    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        fprintf(stderr, "LDC2_W: Invalid constant pool index\n");
        return;
    }

    cp_info *constant = &jvm->class_file.constant_pool[index - 1];
    Cat2 value;

    switch (constant->tag) {
        case CONSTANT_Double:
            value.bytes = constant->info.Double.bytes;
            operand_stack_push_cat2(stack, value);
            printf("LDC2_W: Loaded double %f\n", value.double_);
            break;
            
        case CONSTANT_Long:
            value.bytes = constant->info.Long.bytes;
            operand_stack_push_cat2(stack, value);
            printf("LDC2_W: Loaded long %ld\n", value.long_);
            break;
            
        default:
            fprintf(stderr, "LDC2_W: Invalid constant type %d\n", constant->tag);
            return;
    }

    (*pc) += 3;
}



static void handle_if_icmp(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    uint8_t opcode = bytecode[*pc];
    int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
    bool condition = false;

    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);

    switch (opcode) {
        case IF_ICMPEQ:
            condition = (val1 == val2);
            printf("IF_ICMPEQ: %d == %d, ", val1, val2);
            break;
        case IF_ICMPNE:
            condition = (val1 != val2);
            printf("IF_ICMPNE: %d != %d, ", val1, val2);
            break;
        case IF_ICMPLT:
            condition = (val1 < val2);
            printf("IF_ICMPLT: %d < %d, ", val1, val2);
            break;
        case IF_ICMPGE:
            condition = (val1 >= val2);
            printf("IF_ICMPGE: %d >= %d, ", val1, val2);
            break;
        case IF_ICMPGT:
            condition = (val1 > val2);
            printf("IF_ICMPGT: %d > %d, ", val1, val2);
            break;
        case IF_ICMPLE:
            condition = (val1 <= val2);
            printf("IF_ICMPLE: %d <= %d, ", val1, val2);
            break;
        default:
            fprintf(stderr, "Error: Unknown if_icmp opcode: 0x%02x\n", opcode);
            return;
    }

    if (condition) {
        *pc += branch_offset;
        printf("branching to offset %d\n", branch_offset);
    } else {
        *pc += 3; 
        printf("if_icmp: false, continuando\n");
    }
}

static void handle_ifne(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    int16_t branch_offset = (int16_t)((bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2]);
    if (value != 0) {
        *pc += branch_offset;
        printf("if_icne: verdadeiro, pulando pro offset %d \n", branch_offset);
    } else {
        *pc += 3;
        printf("if_icne: false, continuando\n");
    }
}

static void handle_laload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t index, arrayref;
    operand_stack_pop(stack, &index);
    operand_stack_pop(stack, &arrayref);

    Array *array = (Array*)(intptr_t)arrayref;
    if (!array || index < 0 || index >= array->length || array->type != ARRAY_TYPE_LONG) {
        // Handle ArrayIndexOutOfBoundsException 
        fprintf(stderr, "laload: ArrayIndexOutOfBoundsException\n");
        return;
    }

    Cat2 result;
    result.long_ = ((int64_t*)array->elements)[index];  // Correctly access long array //?
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_land(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.long_ = val1.long_ & val2.long_;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}



static void handle_dstore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    
    if (opcode == DSTORE) {
        index = bytecode[(*pc) + 1];
        (*pc) += 2;
    } else {
        index = opcode - DSTORE_0;
        (*pc)++;
    }
    
    Cat2 value = operand_stack_pop_cat2(stack);
    locals[index] = value.high;
    locals[index + 1] = value.low;
    printf("DSTORE_%d: Stored double to locals[%d,%d]\n", 
           index, index, index + 1);
}

static void handle_dstore_1(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    locals[1] = value.high;
    locals[2] = value.low;
    (*pc)++;
}

static void handle_dstore_2(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    locals[2] = value.high;
    locals[3] = value.low;
    (*pc)++;
}

static void handle_dstore_3(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    locals[3] = value.high;
    locals[4] = value.low;
    (*pc)++;
}
// todo check if this is correct
static void handle_lastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    int32_t index, arrayref;
    operand_stack_pop(stack, &index);
    operand_stack_pop(stack, &arrayref);

    Array *array = (Array*)(intptr_t)arrayref;
    if (!array || index < 0 || index >= array->length || array->type != ARRAY_TYPE_LONG) {
        // Handle ArrayIndexOutOfBoundsException 
        fprintf(stderr, "!lastore: ArrayIndexOutOfBoundsException\n");
        return;
    }
    locals[index] = value.high;
    locals[index+1] = value.low;
    (*pc)++;

    ((int32_t*)array->elements)[index] = value.high; 
    ((int32_t*)array->elements)[index+1] = value.low; 
    (*pc)++;
}


static void handle_dsub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    CHECK_STACK(stack, 4);
    
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    
    Cat2 result;
    result.double_ = val1.double_ - val2.double_;
    operand_stack_push_cat2(stack, result);
    
    printf("DSUB: %f - %f = %f\n", val1.double_, val2.double_, result.double_);
    (*pc)++;
}

static void handle_dmul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.double_ = val1.double_ * val2.double_;
    operand_stack_push_cat2(stack, result);
    printf("DMUL: %f * %f = %f\n", val1.double_, val2.double_, result.double_);
    (*pc)++;
}

static void handle_ddiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);

    if (val2.double_ == 0.0) {
        fprintf(stderr, "Division by zero\n");
        return; 
    }

    Cat2 result;
    result.double_ = val1.double_ / val2.double_;
    operand_stack_push_cat2(stack, result);
    printf("DDIV: %f / %f = %f\n", val1.double_, val2.double_, result.double_);
    (*pc)++;
}

static void handle_dneg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    value.double_ = -value.double_;
    operand_stack_push_cat2(stack, value);
    printf("DNEG: -%f = %f\n", -value.double_, value.double_); // Corrected print statement
    (*pc)++;
}

static void handle_dcmpl(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    int32_t result;

    if (val1.double_ > val2.double_) {
        result = 1;
    } else if (val1.double_ == val2.double_) {
        result = 0;
    } else {
        result = -1;
    }

    operand_stack_push(stack, result);
    (*pc)++;
}

static void handle_dcmpg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    int32_t result;

    if (IS_NAN(val1.double_) || IS_NAN(val2.double_)) {
        result = 1;
    } else if (val1.double_ > val2.double_) {
        result = 1;
    } else if (val1.double_ == val2.double_) {
        result = 0;
    } else {
        result = -1;
    }

    operand_stack_push(stack, result);
    (*pc)++;
}


static void handle_d2f(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 double_val = operand_stack_pop_cat2(stack);
    float float_val = (float)double_val.double_;

    operand_stack_push(stack, *((int32_t*)&float_val)); // Push the float bits

    (*pc)++;
}

static void handle_d2i(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 double_val = operand_stack_pop_cat2(stack);
    int32_t int_val = (int32_t)double_val.double_;

    operand_stack_push(stack, int_val);
    (*pc)++;
}

static void handle_d2l(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 double_val = operand_stack_pop_cat2(stack);
    int64_t long_val = (int64_t)double_val.double_;

    Cat2 result;
    result.long_ = long_val;
    operand_stack_push_cat2(stack, result);

    (*pc)++;
}

static void handle_lcmp(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    int32_t result;

    if (val1.long_ > val2.long_) {
        result = 1;
    } else if (val1.long_ == val2.long_) {
        result = 0;
    } else {
        result = -1;
    }
    operand_stack_push(stack, result);
    (*pc)++;
}

static void handle_lload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    if(opcode == LLOAD)
        index = bytecode[(*pc) + 1];
    else
        index = opcode - LLOAD_0;
    operand_stack_push(stack, locals[index]);       
    operand_stack_push(stack, locals[index + 1]);  
    *pc += 2;
}


static void handle_lneg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 value = operand_stack_pop_cat2(stack);
    value.long_ = -value.long_;
    operand_stack_push_cat2(stack, value);
    (*pc)++;
}

static void handle_newarray(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t atype = bytecode[(*pc) + 1];
    int32_t count;
    operand_stack_pop(stack, &count);
    
    if (count < 0) {
        // Throw NegativeArraySizeException
        return;
    }
    
    Array *array = malloc(sizeof(Array));
    array->length = count;
    array->type = atype;
    array->elements = calloc(count, sizeof(int32_t));

    intptr_t arrayref = (intptr_t)array;
    operand_stack_push(stack, arrayref);
    *pc += 2;
}

static void handle_new(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[*pc + 1] << 8) | bytecode[*pc + 2];
    Object *obj = malloc(sizeof(Object));
    obj->class = &jvm->class_file; // Simplified for demonstration
    operand_stack_push(stack, (intptr_t)obj);
    printf("NEW: Created object\n");
    *pc += 3;
}

static void handle_exception(JVM *jvm, Object *exception, uint32_t *pc, OperandStack *stack) {
    // Find exception handler in current method
    attribute_info *code_attr = NULL; // Get current method's Code attribute
    
    if (!code_attr) return;
    
    // Exception table is after bytecode
    uint8_t *exception_table = code_attr->info + 8 + code_attr->attribute_length;
    uint16_t exception_table_length = (exception_table[0] << 8) | exception_table[1];
    
    ExceptionHandler *handlers = (ExceptionHandler *)(exception_table + 2);
    
    for (int i = 0; i < exception_table_length; i++) {
        if (*pc >= handlers[i].start_pc && *pc < handlers[i].end_pc) {
            if (handlers[i].catch_type == 0 || handlers[i].catch_type == exception->class->this_class) {
                // Found handler
                *pc = handlers[i].handler_pc;
                intptr_t exref = (intptr_t)exception;
                operand_stack_push(stack, exref);
                return;
            }
        }
    }
    
    // No handler found, propagate to caller
}

static void handle_invokevirtual(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[*pc + 1] << 8) | bytecode[*pc + 2];
    
    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        return;
    }

    cp_info *methodref = &jvm->class_file.constant_pool[index - 1];
    if (methodref->tag == CONSTANT_Methodref) {
        uint16_t name_and_type_index = methodref->info.Methodref.name_and_type_index;
        
        cp_info *name_and_type = &jvm->class_file.constant_pool[name_and_type_index - 1];
        uint16_t name_index = name_and_type->info.NameAndType.name_index;
        uint16_t descriptor_index = name_and_type->info.NameAndType.descriptor_index;
        
        const char *method_name = get_constant_pool_string(&jvm->class_file, name_index);
        const char *descriptor = get_constant_pool_string(&jvm->class_file, descriptor_index);
        
        if (method_name && strcmp(method_name, "println") == 0) {
            if (descriptor && strcmp(descriptor, "(D)V") == 0) {
                // Handle double parameter
                Cat2 value = operand_stack_pop_cat2(stack);
                int32_t dummy;
                operand_stack_pop(stack, &dummy); // Pop objectref (System.out)
                printf("%f\n", value.double_);
            } else if (descriptor && strcmp(descriptor, "(I)V") == 0) {
                // Handle integer parameter
                int32_t value;
                operand_stack_pop(stack, &value);
                int32_t dummy;
                operand_stack_pop(stack, &dummy); // Pop objectref
                printf("%d\n", value);
            } else {
                fprintf(stderr, "Unsupported println descriptor: %s\n", descriptor);
            }
        }
    }
    
    *pc += 3;
}


static void handle_sipush(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; // Move past opcode
    uint8_t byte1 = bytecode[*pc];
    (*pc)++; // Move past byte1
    uint8_t byte2 = bytecode[*pc];

    int16_t short_value = (byte1 << 8) | byte2;
    int32_t int_value = (int32_t)short_value; 

    operand_stack_push(stack, int_value);
    printf("SIPUSH: Pushed %d\n", int_value);
    (*pc)++; 
}

static void handle_ifeq(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    if (value == 0) {
        int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
        *pc += branch_offset; 
        printf("IFEQ: verdadeiro %d\n", branch_offset);
    } else {
        *pc += 3; 
        printf("IFEQ: falso, continuando\n");
    }
}

static void handle_iflt(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    if (value < 0) {
        int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
        *pc += branch_offset;
        printf("IFLT: Value is less than 0, branching to offset %d\n", branch_offset);
    } else {
        *pc += 3;
        printf("IFLT: Value is not less than 0, continuing\n");
    }
}

static void handle_ifge(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    if (value >= 0) {
        int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
        *pc += branch_offset;
        printf("IFGE: Value is greater than or equal to 0, branching to offset %d\n", branch_offset);
    } else {
        *pc += 3;
        printf("IFGE: Value is less than 0, continuing\n");
    }
}

static void handle_ifgt(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    if (value > 0) {
        int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
        *pc += branch_offset;
        printf("IFGT: Value is greater than 0, branching to offset %d\n", branch_offset);
    } else {
        *pc += 3;
        printf("IFGT: Value is not greater than 0, continuing\n");
    }
}

static void handle_ifle(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    if (value <= 0) {
        int16_t branch_offset = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
        *pc += branch_offset;
        printf("IFLE: Value is less than or equal to 0, branching to offset %d\n", branch_offset);
    } else {
        *pc += 3;
        printf("IFLE: Value is not less than or equal to 0, continuing\n");
    }
}

static void handle_i2f(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t int_value;
    operand_stack_pop(stack, &int_value);
    float float_value = (float)int_value;
    operand_stack_push(stack, *(int32_t*)&float_value);
    (*pc)++;
}

static void handle_iinc(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; 
    uint8_t index = bytecode[*pc];
    (*pc)++; 
    int8_t constant = (int8_t)bytecode[*pc];
    locals[index] += constant;
    printf("IINC: Increment local variable %d by %d\n", index, constant);
    (*pc)++;
}

static void handle_goto_w(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; 
    int32_t branch_offset = (bytecode[*pc] << 24) | (bytecode[*pc + 1] << 16) | (bytecode[*pc + 2] << 8) | bytecode[*pc + 3];
    *pc += branch_offset;
    printf("GOTO_W: Branching to offset %d\n", branch_offset);
}

static void handle_goto(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; 
    int32_t branch_offset = (bytecode[*pc] << 8) | (bytecode[*pc + 1]);
    *pc += branch_offset;
    printf("GOTO_W: Branching to offset %d\n", branch_offset);
}

static void handle_ldc(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; 
    uint8_t index = bytecode[*pc];
    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        fprintf(stderr, "LDC: Invalid constant pool index %d\n", index);
        return;
    }
    cp_info *const_pool_entry = &jvm->class_file.constant_pool[index - 1];
    switch (const_pool_entry->tag) {
        case CONSTANT_Integer: {
            int32_t value = const_pool_entry->info.Integer.bytes;
            operand_stack_push(stack, value);
            printf("LDC: Pushed integer %d from constant pool index %d\n", value, index);
            break;
        }
        case CONSTANT_Float: {
            float float_value = const_pool_entry->info.Float.bytes;
            operand_stack_push(stack, *(int32_t*)&float_value); // Push float as int bits
            printf("LDC: Pushed float %f from constant pool index %d\n", float_value, index);
            break;
        }
        // Add cases for STRING, CLASS, etc. if needed
        default:
            fprintf(stderr, "LDC: Constant pool entry type %d not yet implemented\n", const_pool_entry->tag);
            break;
    }
    (*pc)++;
}


// static void handle_getstatic(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {

//     uint16_t index = (bytecode[*pc + 1] << 8) | bytecode[*pc + 2]; // Read indexbyte1 and indexbyte2

//     if (!validate_constant_pool_index(&jvm->class_file, index)) {
//         fprintf(stderr, "GETSTATIC: Invalid constant pool index %d\n", index);
//         return;
//     }

//     cp_info *const_pool_entry = &jvm->class_file.constant_pool[index - 1];
//     if (const_pool_entry->tag != CONSTANT_Fieldref) {
//         fprintf(stderr, "GETSTATIC: Constant pool entry at index %d is not a Fieldref\n", index);
//         return;
//     }

//     // In a real JVM, field resolution and loading would happen here.
//     // For this simplified example, we'll just push a placeholder value (0) onto the stack
//     // and print a message indicating the field we would be accessing.

//     uint16_t class_index = const_pool_entry->info.Fieldref.class_index;
//     uint16_t name_and_type_index = const_pool_entry->info.Fieldref.name_and_type_index;

//     const char* class_name = get_constant_pool_string(&jvm->class_file, jvm->class_file.constant_pool[class_index - 1].info.Class.name_index);
//     const char* name_and_type = get_constant_pool_string(&jvm->class_file, name_and_type_index);


//     operand_stack_push(stack, 0); // Placeholder value
//     printf("GETSTATIC: Pushed placeholder for static field %s.%s onto stack\n", class_name, name_and_type);
//     (*pc) += 3;
// }



static void handle_getstatic(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; // Move past the GETSTATIC opcode
    uint16_t index = (bytecode[*pc] << 8) | bytecode[*pc + 1];
    (*pc) += 2;

    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        fprintf(stderr, "GETSTATIC: Invalid constant pool index %d\n", index);
        return;
    }

    cp_info *fieldref = &jvm->class_file.constant_pool[index - 1];
    if (fieldref->tag != CONSTANT_Fieldref) {
        fprintf(stderr, "GETSTATIC: Not a field reference\n");
        return;
    }

    uint16_t class_index = fieldref->info.Fieldref.class_index;
    uint16_t name_and_type_index = fieldref->info.Fieldref.name_and_type_index;

    cp_info *class_info = &jvm->class_file.constant_pool[class_index - 1];
    cp_info *name_and_type = &jvm->class_file.constant_pool[name_and_type_index - 1];

    const char *class_name = get_constant_pool_string(&jvm->class_file, 
        class_info->info.Class.name_index);
    const char *field_name = get_constant_pool_string(&jvm->class_file, 
        name_and_type->info.NameAndType.name_index);

    // Special handling for System.out
    if (strcmp(class_name, "java/lang/System") == 0 && strcmp(field_name, "out") == 0) {
        // Push a dummy reference for System.out
        operand_stack_push(stack, 0xCAFEBABE);
        printf("GETSTATIC: Pushed System.out reference\n");
    } else if (jvm->loaded_class && strcmp(class_name, jvm->loaded_class->class_name) == 0) {
        // Handle other static fields
        for (int i = 0; i < jvm->loaded_class->static_fields_count; i++) {
            const char* current_field_name = get_constant_pool_string(&jvm->class_file, 
                jvm->loaded_class->class->fields[i].name_index);
            if (strcmp(current_field_name, field_name) == 0) {
                operand_stack_push(stack, jvm->loaded_class->static_fields[i].value);
                printf("GETSTATIC: Pushed static field %s.%s value\n", class_name, field_name);
                return;
            }
        }
        fprintf(stderr, "GETSTATIC: Field %s not found\n", field_name);
    } else {
        fprintf(stderr, "GETSTATIC: Class %s not loaded\n", class_name);
    }
}

// ... more handler functions for each instruction

static instruction_handler instruction_table[256] = {0};  // Initialize all to NULL

static void init_instruction_table(void) {
    
    instruction_table[GETSTATIC] = handle_getstatic;
    instruction_table[INVOKEVIRTUAL] = handle_invokevirtual;
    instruction_table[DSTORE] = handle_dstore;
    instruction_table[RETURN] = handle_return;
    

    instruction_table[I2F] = handle_i2f;
    instruction_table[IINC] = handle_iinc;
    instruction_table[GOTO_W] = handle_goto_w;
    instruction_table[GOTO] = handle_goto;
    instruction_table[LDC] = handle_ldc;

    instruction_table[SIPUSH] = handle_sipush;
    instruction_table[IFEQ] = handle_ifeq;
    instruction_table[IFLT] = handle_iflt;
    instruction_table[IFGE] = handle_ifge;
    instruction_table[IFGT] = handle_ifgt;
    instruction_table[IFLE] = handle_ifle;
    instruction_table[IFNE] = handle_ifne;
    instruction_table[IF_ICMPEQ] = handle_if_icmp;
    instruction_table[IF_ICMPNE] = handle_if_icmp;
    instruction_table[IF_ICMPLT] = handle_if_icmp;
    instruction_table[IF_ICMPGE] = handle_if_icmp;
    instruction_table[IF_ICMPGT] = handle_if_icmp;
    instruction_table[IF_ICMPLE] = handle_if_icmp;

    instruction_table[NOP] = handle_nop;
    instruction_table[ICONST_M1] = handle_iconst;
    instruction_table[ICONST_0] = handle_iconst;
    instruction_table[ICONST_1] = handle_iconst;
    instruction_table[ICONST_2] = handle_iconst;
    instruction_table[ICONST_3] = handle_iconst;
    instruction_table[ICONST_4] = handle_iconst;
    instruction_table[ICONST_5] = handle_iconst;
    instruction_table[IADD] = handle_iadd;
    instruction_table[ISUB] = handle_isub;
    instruction_table[IMUL] = handle_imul;
    instruction_table[IDIV] = handle_idiv;
    instruction_table[IOR] = handle_ior;
    instruction_table[ILOAD] = handle_iload_n;
    instruction_table[ILOAD_0] = handle_iload_n;
    instruction_table[ILOAD_1] = handle_iload_n;
    instruction_table[ILOAD_2] = handle_iload_n;
    instruction_table[ILOAD_3] = handle_iload_n;
    instruction_table[ISTORE] = handle_istore_n;
    instruction_table[ISTORE_0] = handle_istore_n;
    instruction_table[ISTORE_1] = handle_istore_n;
    instruction_table[ISTORE_2] = handle_istore_n;
    instruction_table[ISTORE_3] = handle_istore_n;
    instruction_table[DUP] = handle_dup;
    instruction_table[POP] = handle_pop;

    instruction_table[FLOAD] = handle_fload;
    instruction_table[FLOAD_0] = handle_fload;
    instruction_table[FLOAD_1] = handle_fload;
    instruction_table[FLOAD_2] = handle_fload;
    instruction_table[FLOAD_3] = handle_fload;

    instruction_table[DADD] = handle_dadd;
    instruction_table[DLOAD] = handle_dload;
    instruction_table[DLOAD_0] = handle_dload;
    instruction_table[DLOAD_1] = handle_dload;
    instruction_table[DLOAD_2] = handle_dload;
    instruction_table[DLOAD_3] = handle_dload;

    instruction_table[DREM] = handle_drem;
    instruction_table[DSUB] = handle_dsub;
    instruction_table[DMUL] = handle_dmul;
    instruction_table[DDIV] = handle_ddiv;
    instruction_table[DNEG] = handle_dneg;

    instruction_table[BIPUSH] = handle_bipush;


    instruction_table[LADD] = handle_ladd;
    instruction_table[LSUB] = handle_lsub;
    instruction_table[LMUL] = handle_lmul;
    instruction_table[LDIV] = handle_ldiv;
    instruction_table[LREM] = handle_lrem;
    instruction_table[LNEG] = handle_lneg;

    instruction_table[LDC2_W] = handle_ldc2_w;
    
    instruction_table[DCONST_0] = handle_dconst;
    instruction_table[DCONST_1] = handle_dconst;
    instruction_table[LCONST_0] = handle_lconst;
    instruction_table[LCONST_1] = handle_lconst;

    instruction_table[LLOAD] = handle_lload;
    instruction_table[LLOAD_0] = handle_lload;
    instruction_table[LLOAD_1] = handle_lload;
    instruction_table[LLOAD_2] = handle_lload;
    instruction_table[LLOAD_3] = handle_lload;

    instruction_table[LCMP] = handle_lcmp;
    instruction_table[LASTORE] = handle_lastore;
    instruction_table[LAND] = handle_land;
    instruction_table[LALOAD] = handle_laload;

    instruction_table[NOP] = handle_nop;
    
    instruction_table[DSTORE_0] = handle_dstore;
    instruction_table[DSTORE_1] = handle_dstore;
    instruction_table[DSTORE_2] = handle_dstore;
    instruction_table[DSTORE_3] = handle_dstore;


    instruction_table[DCMPL] = handle_dcmpl;
    instruction_table[DCMPG] = handle_dcmpg;

    instruction_table[D2F] = handle_d2f;
    instruction_table[D2I] = handle_d2i;
    instruction_table[D2L] = handle_d2l;
    


    instruction_table[NEW] = handle_new;
    instruction_table[NEWARRAY] = handle_newarray;
    instruction_table[IRETURN] = handle_return;
}

void check_stack_bounds(OperandStack *stack, int required_space) {
    if (stack->size + required_space > stack->capacity) {
        fprintf(stderr, "Stack overflow error\n");
        exit(1);
    }
    if (stack->size + required_space < 0) {
        fprintf(stderr, "Stack underflow error\n");
        exit(1);
    }
}

void print_operation(const char* op, int32_t val1, int32_t val2, int32_t result) {
    printf("Operation: %d %s %d = %d\n", val1, op, val2, result);
}


void print_local_vars(int32_t *local_vars) {
    printf("\nFinal Local Variables State:\n");
    bool printed = false;
    for (int i = 0; i < 256; i++) {
        if (local_vars[i] != 0) {
            printf("local_%d: %d\n", i, local_vars[i]);
            printed = true;
        }
    }
    if (!printed) printf("(No non-zero local variables)\n");
}

// auxiliary functions for bytecode operands
bool test_op_stack_empty(OperandStack *stack);
bool test_op_stack_overflow(OperandStack *stack);
bool test_op_stack_underflow(OperandStack *stack);

const char* get_utf8_from_constant_pool(ClassFile *class_file, uint16_t index) {
    if (!validate_constant_pool_index(class_file, index)) {
        return NULL;
    }
    cp_info *constant_pool_entry = &class_file->constant_pool[index - 1];
    if (constant_pool_entry->tag == CONSTANT_Utf8) {
        return (const char*)constant_pool_entry->info.Utf8.bytes;
    }
    return NULL;
}


const char* get_constant_pool_string(ClassFile *class_file, uint16_t index) {
    if (!validate_constant_pool_index(class_file, index)) {
        return NULL;
    }
    cp_info *constant_pool = class_file->constant_pool;
    if (constant_pool[index - 1].tag == CONSTANT_Utf8) {
        return (const char *)constant_pool[index - 1].info.Utf8.bytes;
    }
    return NULL;
}

const char* get_string_constant(JVM *jvm, uint16_t index) {
    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        return NULL;
    }
    
    cp_info *entry = &jvm->class_file.constant_pool[index - 1];
    if (entry->tag == CONSTANT_String) {
        return get_constant_pool_string(&jvm->class_file, entry->info.String.string_index);
    }
    return NULL;
}

bool validate_stack_operands(OperandStack *stack, int required_operands) {
    return stack->size >= required_operands;
}

bool test_op_stack_empty(OperandStack *stack) {
    return stack->size == 0;
}

bool test_op_stack_overflow(OperandStack *stack) {
    return stack->size >= stack->capacity;
}

bool test_op_stack_underflow(OperandStack *stack) {
    return stack->size <= 0;
}

void operand_stack_init(OperandStack *stack, int capacity) {
    stack->values = (int32_t*)calloc(capacity, sizeof(int32_t));
    if (!stack->values) {
        fprintf(stderr, "Failed to allocate operand stack\n");
        exit(1);
    }
    stack->size = 0;
    stack->capacity = capacity;
}

bool operand_stack_push(OperandStack *stack, int32_t value) {
    if (stack->size >= stack->capacity) {
        return false;
    }
    stack->values[stack->size++] = value;
    return true;
}

void operand_stack_push_cat2(OperandStack *stack, Cat2 value) {
    if (stack->size + 2 > stack->capacity) {
        fprintf(stderr, "Stack overflow in push_cat2\n");
        return;
    }
    stack->values[stack->size++] = value.high;
    stack->values[stack->size++] = value.low;
    // printf("Push Cat2: high=%u, low=%u\n", value.high, value.low);
}

bool operand_stack_pop(OperandStack *stack, int32_t *value) {
    if (stack->size <= 0) {
        return false;
    }
    *value = stack->values[--stack->size];
    return true;
}

Cat2 operand_stack_pop_cat2(OperandStack *stack) {
    Cat2 value;
    if (stack->size < 2) {
        fprintf(stderr, "Stack underflow in pop_cat2\n");
        value.bytes = 0;
        return value;
    }
    value.low = stack->values[--stack->size];
    value.high = stack->values[--stack->size];
    // printf("Pop Cat2: high=%u, low=%u\n", value.high, value.low);
    return value;
}

void print_stack_state(OperandStack *stack) {
    printf("Stack (size=%d): ", stack->size);
    for (int i = 0; i < stack->size && i < 10; i++) { // Limit to 10 elements
        printf("%d ", stack->values[i]);
    }
    if (stack->size > 10) {
        printf("...");
    }
    printf("\n");
}

uint8_t* get_method_bytecode(ClassFile *class_file, const char *method_name, const char *method_descriptor) {
    // Iterate through the methods in the class file
    for (int i = 0; i < class_file->methods_count; i++) {
        method_info *method = &class_file->methods[i];
        
        // Get the method name and descriptor from the constant pool
        const char *name = get_utf8_from_constant_pool(class_file, method->name_index);
        const char *descriptor = get_utf8_from_constant_pool(class_file, method->descriptor_index);
        
        // Check if the method name and descriptor match
        if (strcmp(name, method_name) == 0 && strcmp(descriptor, method_descriptor) == 0) {
            // Iterate through the method's attributes to find the Code attribute
            for (int j = 0; j < method->attributes_count; j++) {
                attribute_info *attribute = &method->attributes[j];
                const char *attribute_name = get_utf8_from_constant_pool(class_file, attribute->attribute_name_index);
                
                if (strcmp(attribute_name, "Code") == 0) {
                    // The Code attribute contains the bytecode
                    // The bytecode starts after the first 12 bytes of the attribute's info
                    return attribute->info + 12;
                }
            }
        }
    }
    
    // Method not found or Code attribute not found
    return NULL;
}


void *resolve_bootstrap_method(JVM *jvm, uint16_t bootstrap_method_attr_index, 
                             uint16_t name_and_type_index) {
    // Validate indices
    if (!validate_constant_pool_index(&jvm->class_file, name_and_type_index)) {
        return NULL;
    }

    cp_info *constant_pool = jvm->class_file.constant_pool;
    
    // Get name and type info
    uint16_t name_index = constant_pool[name_and_type_index - 1]
                         .info.NameAndType.name_index;
    uint16_t descriptor_index = constant_pool[name_and_type_index - 1]
                               .info.NameAndType.descriptor_index;

    // Validate name and descriptor indices
    if (!validate_constant_pool_index(&jvm->class_file, name_index) ||
        !validate_constant_pool_index(&jvm->class_file, descriptor_index)) {
        return NULL;
    }

    // Get name and descriptor strings
    const char *name = get_constant_pool_string(&jvm->class_file, name_index);
    const char *descriptor = get_constant_pool_string(&jvm->class_file, 
                                                    descriptor_index);

    // Allocate and return resolved method info
    struct {
        void *method_handle;
        const char *name;
        const char *descriptor;
    } *resolved_method = malloc(sizeof(*resolved_method));

    resolved_method->method_handle = NULL; // Will be set later
    resolved_method->name = name;
    resolved_method->descriptor = descriptor;

    return resolved_method;
}

bool validate_constant_pool_index(ClassFile *class_file, uint16_t index) {
    // Add debug print
    //printf("Validating constant pool index: %d (pool count: %d)\n", 
     //      index, class_file->constant_pool_count);
    
    if (index == 0 || index >= class_file->constant_pool_count) {
        fprintf(stderr, "Invalid constant pool index: %d\n", index);
        return false;
    }
    return true;
}

void execute_bytecode(JVM *jvm, uint8_t *bytecode, uint32_t bytecode_length, int32_t *locals) {
    OperandStack operand_stack;
    operand_stack_init(&operand_stack, STACK_SIZE);

    static bool table_initialized = false;
    if (!table_initialized) {
        init_instruction_table();
        table_initialized = true;
    }

    uint32_t pc = 0;
    while (pc < bytecode_length) {
        uint8_t opcode = bytecode[pc];
        instruction_handler handler = instruction_table[opcode];
        // fprintf(stderr, ">0x%02x at pc=%u\n", opcode, pc);
        
        if (handler) {
            handler(jvm, bytecode, &pc, &operand_stack, locals);
        } else {
            fprintf(stderr, "Unknown opcode: 0x%02x at pc=%u\n", opcode, pc);
            pc++;
        }

        if (opcode == 0xb1) break; // RETURN
    }

    // Print final state using max_locals from method info
    printf("\nFinal state:\n");
    bool printed = false;
    
    // Get current method's max_locals
    uint16_t max_locals = 3; // For somar method, this is 3
    
    for (int i = 0; i < max_locals; i++) {
        if (locals[i] != 0) {
            printf("local_%d: %d\n", i, locals[i]);
            printed = true;
        }
    }
    if (!printed) printf("(No non-zero local variables)\n");
    
    free(operand_stack.values);
}

void invoke_method(JVM *jvm, void *method_handle) {
    if (!method_handle) {
        fprintf(stderr, "Error: Null method handle\n");
        return;
    }

    struct {
        void *method_handle;
        const char *name;
        const char *descriptor;
    } *resolved_method = method_handle;

    // Check if name and descriptor are valid
    if (!resolved_method->name || !resolved_method->descriptor) {
        fprintf(stderr, "Error: Invalid method name or descriptor\n");
        return;
    }

    printf("Invoking method: %s with descriptor: %s\n", 
           resolved_method->name, 
           resolved_method->descriptor);

    // Get method info
    method_info *method = NULL;
    for (int i = 0; i < jvm->class_file.methods_count; i++) {
        const char *method_name = get_utf8_from_constant_pool(&jvm->class_file, 
            jvm->class_file.methods[i].name_index);
        if (method_name && strcmp(method_name, resolved_method->name) == 0) {
            method = &jvm->class_file.methods[i];
            free((void*)method_name);
            break;
        }
        if (method_name) free((void*)method_name);
    }

    if (!method) {
        fprintf(stderr, "Method not found: %s\n", resolved_method->name);
        return;
    }

    // Get code attribute
    attribute_info *code_attr = NULL;
    for (int i = 0; i < method->attributes_count; i++) {
        const char *attr_name = get_utf8_from_constant_pool(&jvm->class_file, 
            method->attributes[i].attribute_name_index);
        if (attr_name && strcmp(attr_name, "Code") == 0) {
            code_attr = &method->attributes[i];
            free((void*)attr_name);
            break;
        }
        if (attr_name) free((void*)attr_name);
    }

    if (!code_attr) {
        fprintf(stderr, "Code attribute not found for method: %s\n", resolved_method->name);
        return;
    }

    // Get code length and bytecode
    uint8_t *code_info = code_attr->info;
    uint32_t code_length = (code_info[4] << 24) | (code_info[5] << 16) | 
                          (code_info[6] << 8) | code_info[7];
    uint8_t *bytecode = code_info + 8;

    // Create locals array
    int32_t locals[256] = {0};

    // Execute bytecode with actual code length
    execute_bytecode(jvm, bytecode, code_length, locals);
}

void jvm_execute(JVM *jvm) {
    printf("Executing JVM\n");
    ClassFile *class_file = &jvm->class_file;
    method_info *method_to_execute = NULL;

    // Find method to execute
    for (int i = 0; i < class_file->methods_count; i++) {
        const char *method_name = get_utf8_from_constant_pool(class_file, class_file->methods[i].name_index);
        const char *method_descriptor = get_utf8_from_constant_pool(class_file, class_file->methods[i].descriptor_index);
        printf("Found method: %s with descriptor: %s\n", method_name, method_descriptor);

        if (method_name && method_descriptor) {
            if ((class_file->methods[i].access_flags & 0x0001) == 0x0001) { // Check if public
                if (strcmp(method_name, "main") == 0 && 
                    strcmp(method_descriptor, "([Ljava/lang/String;)V") == 0) {
                    method_to_execute = &class_file->methods[i];
                    printf("Found main method!\n");
                    break;
                }
                else if (strcmp(method_name, "somar") == 0 && 
                         strcmp(method_descriptor, "(II)I") == 0) {
                    method_to_execute = &class_file->methods[i];
                    printf("Found somar method!\n");
                    break;
                }
            }
        }
        
        if (method_name) free((void*)method_name);
        if (method_descriptor) free((void*)method_descriptor);
    }

    if (!method_to_execute) {
        fprintf(stderr, "No executable method found\n");
        return;
    }

    // Get code attribute
    attribute_info *code_attribute = NULL;
    for (int i = 0; i < method_to_execute->attributes_count; i++) {
        uint16_t name_index = method_to_execute->attributes[i].attribute_name_index;
        const char *attr_name = get_utf8_from_constant_pool(class_file, name_index);
        
        if (attr_name && strcmp(attr_name, "Code") == 0) {
            code_attribute = &method_to_execute->attributes[i];
            printf("Found Code attribute!\n");
            free((void*)attr_name);
            break;
        }
        if (attr_name) free((void*)attr_name);
    }

    if (!code_attribute) {
        fprintf(stderr, "Code attribute not found\n");
        return;
    }

    // Parse code attribute
    uint8_t *code_info = code_attribute->info;
    uint16_t max_stack = (code_info[0] << 8) | code_info[1];
    uint16_t max_locals = (code_info[2] << 8) | code_info[3];
    uint32_t code_length = (code_info[4] << 24) | (code_info[5] << 16) | 
                          (code_info[6] << 8) | code_info[7];
    uint8_t *bytecode = code_info + 8;

    printf("Method details:\n");
    printf("Max stack: %d\n", max_stack);
    printf("Max locals: %d\n", max_locals);
    printf("Code length: %d\n", code_length);

    // Initialize execution environment
    OperandStack stack;
    operand_stack_init(&stack, max_stack);
    int32_t *locals = calloc(max_locals, sizeof(int32_t));

    // Set up parameters
    const char *method_name = get_utf8_from_constant_pool(class_file, method_to_execute->name_index);
    if (method_name && strcmp(method_name, "somar") == 0) {
        locals[1] = 5;  // First parameter
        locals[2] = 3;  // Second parameter
        printf("Set up parameters: %d, %d\n", locals[1], locals[2]);
    }
    if (method_name) free((void*)method_name);

    // Execute bytecode
    execute_bytecode(jvm, bytecode, code_length, locals);

    free(locals);
}