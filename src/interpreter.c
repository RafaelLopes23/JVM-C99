#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strcmp
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

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

#define HANDLE_ARRAY_STORE(type, format) \
    uint64_t value, index, arrayref; \
    operand_stack_pop(stack, &value); \
    operand_stack_pop(stack, &index); \
    operand_stack_pop(stack, &arrayref); \
    Array *array = jvm->heap.arrays[arrayref]; \
    if (!array || index >= array->length) return; \
    ((type*)array->elements)[index] = (type)value; \
    printf("Array store: array[%ld] = " format "\n", (long)index, (type)value); \
    (*pc)++; 

static void handle_fastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    HANDLE_ARRAY_STORE(float, "%f")
}

static void handle_dastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    HANDLE_ARRAY_STORE(double, "%f")
}

// Stack check macro
#define CHECK_STACK(stack, required) \
    if ((stack)->size < (required)) { \
        fprintf(stderr, "Stack underflow - need %d values but have %d\n", \
                required, (stack)->size); \
        return; \
    }

const char* get_constant_pool_string(ClassFile *class_file, uint16_t index);
const char* get_string_constant(JVM *jvm, uint16_t index);
void invoke_method(JVM *jvm, void *method_handle);

bool operand_stack_push(OperandStack *stack, uint64_t value);
bool operand_stack_pop(OperandStack *stack, uint64_t *value);
void operand_stack_push_cat2(OperandStack *stack, uint64_t value);
uint64_t operand_stack_pop_cat2(OperandStack *stack);
bool validate_constant_pool_index(ClassFile *class_file, uint16_t index);
void operand_stack_init(OperandStack *stack, int capacity);
void print_stack_state(OperandStack *stack);

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
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    int32_t result = val1 + val2;
    operand_stack_push(stack, (uint64_t)(int64_t)result);
    locals[7] = result;
    printf("IADD: %d + %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_isub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    int32_t result = val1 - val2;
    operand_stack_push(stack, (uint64_t)(int64_t)result);
    locals[8] = result;
    printf("ISUB: %d - %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_imul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    int32_t result = val1 * val2;
    operand_stack_push(stack, (uint64_t)(int64_t)result);
    locals[9] = result;
    printf("IMUL: %d * %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_idiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    if (val2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return;
    }
    int32_t result = val1 / val2;
    operand_stack_push(stack, (uint64_t)(int64_t)result);
    locals[10] = result;
    printf("IDIV: %d / %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_ior(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    int32_t result = val1 | val2;
    operand_stack_push(stack, (uint64_t)(int64_t)result);
    locals[11] = result;
    printf("IOR: %d | %d = %d\n", val1, val2, result);
    (*pc)++;
}


static void handle_istore_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index = opcode - ISTORE_0;
    uint64_t value_64;
    operand_stack_pop(stack, &value_64);
    int32_t value = (int32_t)value_64;
    locals[index] = value;
    printf("ISTORE_%d: Stored %d\n", index, value);
    (*pc)++;
}

static void handle_iload_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index = opcode - ILOAD_0;
    int32_t value = locals[index];
    operand_stack_push(stack, value);
    printf("ILOAD_%d: Loaded %d\n", index, value);
    (*pc)++;
}

static void handle_fload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    if(opcode == FLOAD)
        index = bytecode[(*pc) + 1];
    else
        index = opcode - FLOAD_0;
    operand_stack_push(stack, locals[index]);
    (*pc)++;
}

static void handle_faload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t index_64, arrayref_64;
    operand_stack_pop(stack, &index_64);    // Array index
    operand_stack_pop(stack, &arrayref_64);  // Array reference
    
    // Validate array reference
    if (arrayref_64 >= jvm->heap.array_count) {
        fprintf(stderr, "Invalid array reference: %" PRIu64 "\n", arrayref_64);
        return;
    }

    Array *array = jvm->heap.arrays[arrayref_64];

    // Validate array and index
    if (!array) {
        fprintf(stderr, "Array is NULL\n");
        return;
    }
    
    int32_t index = (int32_t)index_64;
    if (index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: Index %d is out of bounds for length %d\n", 
                index, array->length);
        return;
    }

    // Load float value and convert to bits for stack
    float value = ((float *)array->elements)[index];
    uint32_t value_bits = *((uint32_t *)&value);
    operand_stack_push(stack, (uint64_t)value_bits);
    
    printf("FALOAD: array[%d] = %f\n", index, value);
    (*pc)++;
}

static void handle_daload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t index_64, arrayref_64;
    operand_stack_pop(stack, &index_64);    // Array index
    operand_stack_pop(stack, &arrayref_64);  // Array reference
    
    // Validate array reference
    if (arrayref_64 >= jvm->heap.array_count) {
        fprintf(stderr, "Invalid array reference: %" PRIu64 "\n", arrayref_64);
        return;
    }

    Array *array = jvm->heap.arrays[arrayref_64];

    // Validate array and index
    if (!array) {
        fprintf(stderr, "Array is NULL\n");
        return;
    }
    
    int32_t index = (int32_t)index_64;
    if (index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: Index %d is out of bounds for length %d\n", 
                index, array->length);
        return;
    }

    // Load double value directly as 64-bit value
    double value = ((double *)array->elements)[index];
    uint64_t value_bits = *((uint64_t *)&value);
    operand_stack_push_cat2(stack, value_bits);
    
    printf("DALOAD: array[%d] = %f\n", index, value);
    (*pc)++;
}

// Stack operations
static void handle_dup(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value_64;
    operand_stack_pop(stack, &value_64);
    operand_stack_push(stack, value_64);
    operand_stack_push(stack, value_64);
    printf("DUP: Duplicated %d\n", (int32_t)value_64);
    (*pc)++;
}

static void handle_pop(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value_64;
    operand_stack_pop(stack, &value_64);
    printf("POP: Removed %d\n", (int32_t)value_64);
    (*pc)++;
}

// 64-bit operations (category 2)
static void handle_dadd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    
    double val2, val1, result;
    memcpy(&val2, &val2_bits, sizeof(double));
    memcpy(&val1, &val1_bits, sizeof(double));
    
    result = val1 + val2;
    
    uint64_t result_bits;
    memcpy(&result_bits, &result, sizeof(double));
    operand_stack_push_cat2(stack, result_bits);
    
    printf("DADD: %.17g + %.17g = %.17g\n", val1, val2, result);
    (*pc)++;
}

static void handle_ladd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    uint64_t result = val1 + val2;
    operand_stack_push_cat2(stack, result);
    (*pc)++;
}

static void handle_lsub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    uint64_t result = val1 - val2;
    operand_stack_push_cat2(stack, result);
    printf("LSUB: %" PRId64 " - %" PRId64 " = %" PRId64 "\n", 
           (int64_t)val1, (int64_t)val2, (int64_t)result);
    (*pc)++;
}

static void handle_lmul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    uint64_t result = val1 * val2;
    operand_stack_push_cat2(stack, result);
    printf("LMUL: %" PRId64 " * %" PRId64 " = %" PRId64 "\n", 
           (int64_t)val1, (int64_t)val2, (int64_t)result);
    (*pc)++;
}


static void handle_ldiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    if (val2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return;
    }
    uint64_t result = val1 / val2;
    operand_stack_push_cat2(stack, result);
    printf("LDIV: %" PRId64 " / %" PRId64 " = %" PRId64 "\n", 
           (int64_t)val1, (int64_t)val2, (int64_t)result);
    (*pc)++;
}



static void handle_lrem(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    if (val2 == 0) {
        fprintf(stderr, "Division by zero\n");
        return;
    }
    uint64_t result = (int64_t)val1 % (int64_t)val2;
    operand_stack_push_cat2(stack, result);
    printf("LREM: %" PRId64 " %% %" PRId64 " = %" PRId64 "\n", 
           (int64_t)val1, (int64_t)val2, (int64_t)result);
    (*pc)++;
}


static void handle_drem(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    
    double val2 = *(double*)&val2_bits;
    double val1 = *(double*)&val1_bits;
    
    if (IS_NAN(val1) || IS_NAN(val2) || IS_INF(val1) || val2 == 0.0) {
        fprintf(stderr, "Invalid operand for DREM\n");
        return;
    }
    
    double result = fmod(val1, val2);
    uint64_t result_bits = *(uint64_t*)&result;
    operand_stack_push_cat2(stack, result_bits);
    printf("DREM: %f %% %f = %f\n", val1, val2, result);
    (*pc)++;
}

// Carregamento de Constantes (long e double)
static void handle_lconst(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int64_t value = bytecode[*pc] - LCONST_0;
    operand_stack_push_cat2(stack, (uint64_t)value);
    printf("LCONST_%d: Pushed %" PRId64 "\n", (int)value, value);
    (*pc)++;
}

static void handle_dconst(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    double value = (double)(bytecode[*pc] - DCONST_0);
    uint64_t value_bits = *((uint64_t*)&value);
    operand_stack_push_cat2(stack, value_bits);
    printf("DCONST_%d: Pushed %f\n", (int)value, value);
    (*pc)++;
}

static void handle_ldc2_w(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
    
    if (!validate_constant_pool_index(&jvm->class_file, index)) {
        fprintf(stderr, "ldc2_w: Invalid constant pool index\n");
        return;
    }

    cp_info *entry = &jvm->class_file.constant_pool[index - 1];
    
    if (entry->tag == CONSTANT_Double) {
        double value = entry->info.Double.bytes;
        uint64_t value_bits;
        memcpy(&value_bits, &value, sizeof(double));
        operand_stack_push_cat2(stack, value_bits);
        printf("LDC2_W: Pushed double %.17g (bits: 0x%016" PRIx64 ")\n", 
               value, value_bits);
    }
    *pc += 3;
}


static void handle_if_icmpeq(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_64, val1_64;
    operand_stack_pop(stack, &val2_64);
    operand_stack_pop(stack, &val1_64);
    int32_t val2 = (int32_t)val2_64;
    int32_t val1 = (int32_t)val1_64;
    
    int16_t branch_offset = (int16_t)((bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2]);
    printf("IF_ICMPEQ: Comparing %d == %d\n", val1, val2);
    if (val1 == val2) {
        *pc += branch_offset;
    } else {
        *pc += 3;
    }
}

static void handle_ifne(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value_64;
    operand_stack_pop(stack, &value_64);
    int32_t value = (int32_t)value_64;
    
    int16_t branch_offset = (int16_t)((bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2]);
    printf("IFNE: Comparing %d != 0\n", value);
    if (value != 0) {
        *pc += branch_offset;
    } else {
        *pc += 3;
    }
}

// Array handling functions
static Array* create_array(int32_t length, uint8_t type) {
    Array* array = (Array*)malloc(sizeof(Array));
    if (!array) return NULL;

    array->length = length;
    array->type = type;

    // Set element size based on type
    switch (type) {
        case T_BOOLEAN:
        case T_BYTE:
            array->element_size = sizeof(int8_t);
            break;
        case T_CHAR:
        case T_SHORT:
            array->element_size = sizeof(int16_t);
            break;
        case T_INT:
        case T_FLOAT:
            array->element_size = sizeof(int32_t);
            break;
        case T_LONG:
        case T_DOUBLE:
            array->element_size = sizeof(int64_t);
            break;
        default:
            free(array);
            return NULL;
    }

    array->elements = calloc(length, array->element_size);
    if (!array->elements) {
        free(array);
        return NULL;
    }

    return array;
}


static void handle_return(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++; 
}

static void handle_ireturn(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value;
    operand_stack_pop(stack, &value);
    printf("IRETURN: Method returning %d\n", (int32_t)value);
    (*pc)++;
}

static void handle_laload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t index_64, arrayref_64;
    operand_stack_pop(stack, &index_64);
    operand_stack_pop(stack, &arrayref_64);
    
    Array *array = (Array *)(uintptr_t)arrayref_64;
    int32_t index = (int32_t)index_64;
    
    if (!array || index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: %d\n", index);
        return;
    }
    
    int64_t value = ((int64_t *)array->elements)[index];
    operand_stack_push_cat2(stack, (uint64_t)value);
    printf("LALOAD: Loaded array[%d] = %" PRId64 "\n", index, value);
    (*pc)++;
}

static void handle_dload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    if(opcode == DLOAD) {
        index = bytecode[(*pc) + 1];
        *pc += 2;
    } else {
        index = opcode - DLOAD_0;
        (*pc)++;
    }
    
    // Combine two 32-bit values into one 64-bit value
    uint64_t value = ((uint64_t)locals[index] << 32) | (uint32_t)locals[index + 1];
    operand_stack_push_cat2(stack, value);
    printf("DLOAD_%d: Loaded double value\n", index);
}

static void handle_land(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    uint64_t result = val1 & val2;
    operand_stack_push_cat2(stack, result);
    printf("LAND: %" PRId64 " & %" PRId64 " = %" PRId64 "\n", 
           (int64_t)val1, (int64_t)val2, (int64_t)result);
    (*pc)++;
}



static void handle_dstore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t index = bytecode[(*pc) + 1];
    uint64_t value_64;
    operand_stack_pop(stack, &value_64);
    
    // Store the 64-bit value across two local variables
    locals[index] = (int32_t)(value_64 >> 32);
    locals[index + 1] = (int32_t)value_64;
    
    printf("DSTORE: Stored double at index %d\n", index);
    *pc += 2;
}
// todo check if this is correct
static void handle_lastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value_64, index_64, arrayref_64;
    
    operand_stack_pop(stack, &value_64);      // Value to store
    operand_stack_pop(stack, &index_64);      // Array index
    operand_stack_pop(stack, &arrayref_64);   // Array reference
    
    Array *array = (Array *)(uintptr_t)arrayref_64;
    int32_t index = (int32_t)index_64;
    
    if (!array || index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: %d\n", index);
        return;
    }
    
    ((int64_t *)array->elements)[index] = (int64_t)value_64;
    printf("LASTORE: array[%d] = %" PRId64 "\n", index, (int64_t)value_64);
    (*pc)++;
}

static void handle_dsub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    
    double val2, val1, result;
    memcpy(&val2, &val2_bits, sizeof(double));
    memcpy(&val1, &val1_bits, sizeof(double));
    result = val1 - val2;
    
    uint64_t result_bits;
    memcpy(&result_bits, &result, sizeof(double));
    operand_stack_push_cat2(stack, result_bits);
    
    printf("DSUB: %.17g - %.17g = %.17g\n", val1, val2, result);
    (*pc)++;
}

static void handle_dmul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    
    double val2, val1, result;
    memcpy(&val2, &val2_bits, sizeof(double));
    memcpy(&val1, &val1_bits, sizeof(double));
    
    result = val1 * val2;
    
    uint64_t result_bits;
    memcpy(&result_bits, &result, sizeof(double));
    operand_stack_push_cat2(stack, result_bits);
    
    printf("DMUL: %.17g * %.17g = %.17g\n", val1, val2, result);
    (*pc)++;
}


static void handle_ddiv(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    
    double val2, val1, result;
    memcpy(&val2, &val2_bits, sizeof(double));
    memcpy(&val1, &val1_bits, sizeof(double));
    
    if (val2 == 0.0) {
        result = val1 > 0 ? INFINITY : -INFINITY;
    } else {
        result = val1 / val2;
    }
    
    uint64_t result_bits;
    memcpy(&result_bits, &result, sizeof(double));
    operand_stack_push_cat2(stack, result_bits);
    
    printf("DDIV: %.17g / %.17g = %.17g\n", val1, val2, result);
    (*pc)++;
}

static void handle_dneg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value_bits = operand_stack_pop_cat2(stack);
    double value = *(double*)&value_bits;
    double result = -value;
    uint64_t result_bits = *(uint64_t*)&result;
    operand_stack_push_cat2(stack, result_bits);
    printf("DNEG: -%f = %f\n", value, result);
    (*pc)++;
}

static void handle_dcmpl(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    double val2 = *((double*)&val2_bits);
    double val1 = *((double*)&val1_bits);
    int32_t result;

    if (IS_NAN(val1) || IS_NAN(val2)) {
        result = -1;
    } else if (val1 > val2) {
        result = 1;
    } else if (val1 == val2) {
        result = 0;
    } else {
        result = -1;
    }

    operand_stack_push(stack, (uint64_t)(int64_t)result);
    printf("DCMPL: %f cmp %f = %d\n", val1, val2, result);
    (*pc)++;
}


static void handle_dcmpg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2_bits = operand_stack_pop_cat2(stack);
    uint64_t val1_bits = operand_stack_pop_cat2(stack);
    double val2 = *((double*)&val2_bits);
    double val1 = *((double*)&val1_bits);
    int32_t result;

    if (IS_NAN(val1) || IS_NAN(val2)) {
        result = 1;
    } else if (val1 > val2) {
        result = 1;
    } else if (val1 == val2) {
        result = 0;
    } else {
        result = -1;
    }

    operand_stack_push(stack, (uint64_t)(int64_t)result);
    printf("DCMPG: %f cmp %f = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_d2f(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t double_bits = operand_stack_pop_cat2(stack);
    double double_val = *((double*)&double_bits);
    float float_val = (float)double_val;
    uint32_t float_bits = *((uint32_t*)&float_val);
    operand_stack_push(stack, (uint64_t)float_bits);
    printf("D2F: %f to %f\n", double_val, float_val);
    (*pc)++;
}

static void handle_d2i(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t double_bits = operand_stack_pop_cat2(stack);
    double double_val = *((double*)&double_bits);
    int32_t int_val = (int32_t)double_val;
    operand_stack_push(stack, (uint64_t)(int64_t)int_val);
    printf("D2I: %f to %d\n", double_val, int_val);
    (*pc)++;
}

static void handle_d2l(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t double_bits = operand_stack_pop_cat2(stack);
    double double_val = *((double*)&double_bits);
    int64_t long_val = (int64_t)double_val;
    operand_stack_push_cat2(stack, (uint64_t)long_val);
    printf("D2L: %f to %" PRId64 "\n", double_val, long_val);
    (*pc)++;
}

static void handle_lcmp(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t val2 = operand_stack_pop_cat2(stack);
    uint64_t val1 = operand_stack_pop_cat2(stack);
    int32_t result;

    if ((int64_t)val1 > (int64_t)val2) {
        result = 1;
    } else if ((int64_t)val1 == (int64_t)val2) {
        result = 0;
    } else {
        result = -1;
    }

    operand_stack_push(stack, (uint64_t)(int64_t)result);
    printf("LCMP: %" PRId64 " cmp %" PRId64 " = %d\n", 
           (int64_t)val1, (int64_t)val2, result);
    (*pc)++;
}

static void handle_lload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index;
    if (opcode == LLOAD) {
        index = bytecode[(*pc) + 1];
        *pc += 2;
    } else {
        index = opcode - LLOAD_0;
        (*pc)++;
    }
    
    uint64_t value = ((uint64_t)locals[index] << 32) | (uint32_t)locals[index + 1];
    operand_stack_push_cat2(stack, value);
    printf("LLOAD_%d: Loaded %" PRId64 "\n", index, (int64_t)value);
}

static void handle_lneg(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint64_t value = operand_stack_pop_cat2(stack);
    int64_t neg_value = -(int64_t)value;
    operand_stack_push_cat2(stack, (uint64_t)neg_value);
    printf("LNEG: -%" PRId64 " = %" PRId64 "\n", (int64_t)value, neg_value);
    (*pc)++;
}

static void handle_newarray(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t atype = bytecode[(*pc) + 1];
    uint64_t count_64;
    operand_stack_pop(stack, &count_64);
    int32_t count = (int32_t)count_64;
    
    Array *array = create_array(count, atype);
    if (!array) return;

    // Store array in heap
    uint32_t index = jvm->heap.array_count++;
    if (index >= jvm->heap.array_cap) {
        size_t new_cap = (jvm->heap.array_cap == 0) ? 16 : jvm->heap.array_cap * 2;
        Array **new_arrays = realloc(jvm->heap.arrays, new_cap * sizeof(Array*));
        if (!new_arrays) return;
        jvm->heap.arrays = new_arrays;
        jvm->heap.array_cap = new_cap;
    }
    jvm->heap.arrays[index] = array;

    operand_stack_push(stack, index);
    printf("NEWARRAY: Created array of type %d with length %d\n", atype, count);
    *pc += 2;
}

static void handle_bipush(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int8_t value = (int8_t)bytecode[(*pc) + 1];
    operand_stack_push(stack, value);
    printf("BIPUSH: Pushed %d\n", value);
    *pc += 2;
}

static void handle_astore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t index = bytecode[(*pc) + 1];
    uint64_t value;
    operand_stack_pop(stack, &value);
    locals[index] = (int32_t)value; // Cast to int32_t for local variable storage
    printf("ASTORE %d: Stored array reference %d\n", index, (int32_t)value);
    *pc += 2;
}

static void handle_istore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t index = bytecode[(*pc) + 1];
    uint64_t value;
    operand_stack_pop(stack, &value);
    locals[index] = (int32_t)value;
    printf("ISTORE %d: Stored %d\n", index, (int32_t)value);
    (*pc) += 2;
}

static void handle_astore_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index = opcode - ASTORE_0;
    uint64_t value;
    operand_stack_pop(stack, &value);
    locals[index] = (int32_t)value;
    printf("ASTORE_%d: Stored array reference %d\n", index, (int32_t)value);
    (*pc)++;
}

static void handle_aload_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index = opcode - ALOAD_0;
    int32_t value = locals[index];
    operand_stack_push(stack, value);
    printf("ALOAD_%d: Loaded array reference %d\n", index, value);
    (*pc)++;
}

static void handle_dstore_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index = opcode - DSTORE_0;
    uint64_t value = operand_stack_pop_cat2(stack);
    
    // Store the 64-bit value across two locals
    locals[index] = (int32_t)(value >> 32);
    locals[index + 1] = (int32_t)value;
    
    double dvalue;
    memcpy(&dvalue, &value, sizeof(double));
    printf("DSTORE_%d: Stored double %.17g\n", index, dvalue);
    (*pc)++;
}


static void handle_dload_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    uint8_t index = opcode - DLOAD_0;
    
    uint64_t value = ((uint64_t)(uint32_t)locals[index] << 32) | 
                     (uint64_t)(uint32_t)locals[index + 1];
    operand_stack_push_cat2(stack, value);
    
    double dvalue;
    memcpy(&dvalue, &value, sizeof(double));
    printf("DLOAD_%d: Loaded double %.17g\n", index, dvalue);
    (*pc)++;
}

// Array store
static void handle_iastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    // Pop the value to store, the index, and the array reference from the stack
    uint64_t value, index, arrayref;
    operand_stack_pop(stack, &value);    // Value to store
    operand_stack_pop(stack, &index);    // Array index
    operand_stack_pop(stack, &arrayref); // Array reference (index into the heap's array table)

    // Validate the array reference
    if (arrayref >= jvm->heap.array_count) {
        fprintf(stderr, "Invalid array reference: %" PRIu64 "\n", arrayref);
        return;
    }

    // Get the array from the heap's array table
    Array *array = jvm->heap.arrays[arrayref];

    // Validate the array and index
    if (!array) {
        fprintf(stderr, "Array is NULL\n");
        return;
    }
    if (index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: Index %" PRIu64 " is out of bounds for length %d\n", index, array->length);
        return;
    }

    // Store the value in the array
    ((int32_t *)array->elements)[index] = (int32_t)value;

    // Debug print
    //printf("IASTORE: array[%" PRIu64 "] = %d\n", index, (int32_t)value);

    // Increment the program counter
    (*pc)++;
}

static void handle_iaload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    // Pop the index and array reference from the stack
    uint64_t index, arrayref;
    operand_stack_pop(stack, &index);    // Array index
    operand_stack_pop(stack, &arrayref); // Array reference (index into the heap's array table)

    // Validate the array reference
    if (arrayref >= jvm->heap.array_count) {
        fprintf(stderr, "Invalid array reference: %" PRIu64 "\n", arrayref);
        return;
    }

    // Get the array from the heap's array table
    Array *array = jvm->heap.arrays[arrayref];

    // Validate the array and index
    if (!array) {
        fprintf(stderr, "Array is NULL\n");
        return;
    }
    if (index < 0 || index >= array->length) {
        fprintf(stderr, "ArrayIndexOutOfBoundsException: Index %" PRIu64 " is out of bounds for length %d\n", index, array->length);
        return;
    }

    // Load the value from the array
    int32_t value = ((int32_t *)array->elements)[index];

    // Push the value onto the operand stack
    operand_stack_push(stack, (uint64_t)value);

    // Debug print
    //printf("IALOAD: Loaded array[%" PRIu64 "] = %d\n", index, value);

    // Increment the program counter
    (*pc)++;
}

static void handle_new(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
    
    Object *obj = malloc(sizeof(Object));
    obj->class = &jvm->class_file; // Should load actual class
    obj->fields = calloc(1, 1024); // Fixed size for now

    operand_stack_push(stack, (intptr_t)obj);
    *pc += 3;
}

static void handle_aload(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t index = bytecode[(*pc) + 1];
    int32_t value = locals[index];
    operand_stack_push(stack, value);
    printf("ALOAD %d: Loaded array reference %d\n", index, value);
    (*pc) += 2;
}

static void handle_missing_opcode(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    printf("PC: %04x Opcode: 0x%02x\n", *pc, bytecode[*pc]);
    (*pc)++;
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
        
        const char *method_name = get_constant_pool_string(&jvm->class_file, name_index);
        
        if (method_name && strcmp(method_name, "println") == 0) {
            uint64_t value;
            if (operand_stack_pop(stack, &value)) {
                uint64_t dummy;
                operand_stack_pop(stack, &dummy);
                printf("%d\n", (int32_t)value);
            }
        }
    }
    
    *pc += 3;
}

// ... more handler functions for each instruction

static instruction_handler instruction_table[256] = {0};  // Initialize all to NULL

static void init_instruction_table(void) {

    for (int i = 0; i < 256; i++) {
        instruction_table[i] = handle_missing_opcode;
    }

    instruction_table[INVOKEVIRTUAL] = handle_invokevirtual;
    instruction_table[LNEG] = handle_lneg;
    instruction_table[RETURN] = handle_return;
    instruction_table[IFNE] = handle_ifne;
    instruction_table[IF_ICMPEQ] = handle_if_icmpeq;

    instruction_table[NOP] = handle_nop;
    instruction_table[ICONST_M1] = handle_iconst;
    instruction_table[ICONST_0] = handle_iconst;
    instruction_table[ICONST_1] = handle_iconst;
    instruction_table[ICONST_2] = handle_iconst;
    instruction_table[ICONST_3] = handle_iconst;
    instruction_table[ICONST_4] = handle_iconst;
    instruction_table[ICONST_5] = handle_iconst;
    instruction_table[IRETURN] = handle_ireturn;
    instruction_table[IADD] = handle_iadd;
    instruction_table[ISUB] = handle_isub;
    instruction_table[IMUL] = handle_imul;
    instruction_table[IDIV] = handle_idiv;
    instruction_table[IOR] = handle_ior;
    instruction_table[ILOAD_0] = handle_iload_n;
    instruction_table[ILOAD_1] = handle_iload_n;
    instruction_table[ILOAD_2] = handle_iload_n;
    instruction_table[ILOAD_3] = handle_iload_n;
    instruction_table[ISTORE_0] = handle_istore_n;
    instruction_table[ISTORE_1] = handle_istore_n;
    instruction_table[ISTORE_2] = handle_istore_n;
    instruction_table[ISTORE_3] = handle_istore_n;
    instruction_table[DUP] = handle_dup;
    instruction_table[POP] = handle_pop;
    instruction_table[NEWARRAY] = handle_newarray;
    instruction_table[IASTORE] = handle_iastore;
    instruction_table[LASTORE] = handle_lastore;
    instruction_table[FASTORE] = handle_fastore;
    instruction_table[DASTORE] = handle_dastore;
    instruction_table[FALOAD] = handle_faload;
    instruction_table[DALOAD] = handle_daload;

    instruction_table[FLOAD] = handle_fload;
    instruction_table[FLOAD_1] = handle_fload;
    instruction_table[FLOAD_2] = handle_fload;
    instruction_table[FLOAD_3] = handle_fload;

    instruction_table[DADD] = handle_dadd;

    instruction_table[DREM] = handle_drem;
    instruction_table[DSUB] = handle_dsub;
    instruction_table[DMUL] = handle_dmul;
    instruction_table[DDIV] = handle_ddiv;
    instruction_table[DNEG] = handle_dneg;


    instruction_table[LADD] = handle_ladd;
    instruction_table[LSUB] = handle_lsub;
    instruction_table[LMUL] = handle_lmul;
    instruction_table[LDIV] = handle_ldiv;
    instruction_table[LREM] = handle_lrem;

    instruction_table[LDC2_W] = handle_ldc2_w;;
    
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
    instruction_table[DSTORE_0] = handle_dstore_n;
    instruction_table[DSTORE_1] = handle_dstore_n;
    instruction_table[DSTORE_2] = handle_dstore_n;
    instruction_table[DSTORE_3] = handle_dstore_n;
    
    instruction_table[DLOAD_0] = handle_dload_n;
    instruction_table[DLOAD_1] = handle_dload_n;
    instruction_table[DLOAD_2] = handle_dload_n;
    instruction_table[DLOAD_3] = handle_dload_n;


    instruction_table[DCMPL] = handle_dcmpl;
    instruction_table[DCMPG] = handle_dcmpg;

    instruction_table[D2F] = handle_d2f;
    instruction_table[D2I] = handle_d2i;
    instruction_table[D2L] = handle_d2l;
    
    instruction_table[ALOAD_0] = handle_aload_n;
    instruction_table[ALOAD_1] = handle_aload_n;
    instruction_table[ALOAD_2] = handle_aload_n;
    instruction_table[ALOAD_3] = handle_aload_n;

    instruction_table[ASTORE_0] = handle_astore_n;
    instruction_table[ASTORE_1] = handle_astore_n;
    instruction_table[ASTORE_2] = handle_astore_n;
    instruction_table[ASTORE_3] = handle_astore_n;

    instruction_table[NEWARRAY] = handle_newarray;
    instruction_table[IASTORE] = handle_iastore;
    instruction_table[IALOAD] = handle_iaload;
    instruction_table[LASTORE] = handle_lastore;
    instruction_table[LALOAD] = handle_laload;
    instruction_table[BIPUSH] = handle_bipush;    // 0x10
    instruction_table[ISTORE] = handle_istore;    // 0x36
    instruction_table[ASTORE] = handle_astore;    // 0x4C
    instruction_table[ALOAD] = handle_aload;
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
    printf("\nFinal state:\n\n");
    printf("Final Local Variables State:\n");
    
    bool printed = false;
    
    // For clarity, only print non-zero local variables
    for (int i = 0; i < 256; i++) {
        if (local_vars[i] != 0) {
            printed = true;
            printf("local_%d: %d\n", i, local_vars[i]);
        }
    }
    
    if (!printed) {
        printf("(No non-zero local variables)\n");
    }
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
        // Allocate and create null-terminated string
        char *str = malloc(constant_pool_entry->info.Utf8.length + 1);
        memcpy(str, constant_pool_entry->info.Utf8.bytes, constant_pool_entry->info.Utf8.length);
        str[constant_pool_entry->info.Utf8.length] = '\0';
        return str;
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
    stack->values = (uint64_t*)calloc(capacity, sizeof(uint64_t));
    if (!stack->values) {
        fprintf(stderr, "Failed to allocate operand stack\n");
        exit(1);
    }
    stack->size = 0;
    stack->capacity = capacity;
}

bool operand_stack_push(OperandStack *stack, uint64_t value) {
    if (stack->size >= stack->capacity) {
        return false;
    }
    stack->values[stack->size++] = value;
    return true;
}


void operand_stack_push_cat2(OperandStack *stack, uint64_t value) {
    operand_stack_push(stack, value);
}

bool operand_stack_pop(OperandStack *stack, uint64_t *value) {
    if (stack->size <= 0) {
        return false;
    }
    *value = stack->values[--stack->size];
    return true;
}

uint64_t operand_stack_pop_cat2(OperandStack *stack) {
    uint64_t value;
    operand_stack_pop(stack, &value);
    return value;
}

void print_stack_state(OperandStack *stack) {
    printf("Stack (size=%d): ", stack->size);
    for (int i = 0; i < stack->size && i < 10; i++) {
        printf("%" PRId64 " ", (int64_t)stack->values[i]);
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

void jvm_free(JVM *jvm) {
    if (!jvm) return;
    
    // Free arrays in heap
    if (jvm->heap.arrays) {
        for (uint32_t i = 0; i < jvm->heap.array_count; i++) {
            if (jvm->heap.arrays[i]) {
                free(jvm->heap.arrays[i]->elements);
                free(jvm->heap.arrays[i]);
            }
        }
        free(jvm->heap.arrays);
        jvm->heap.arrays = NULL;
    }
    
    jvm->heap.array_count = 0;
    jvm->heap.array_cap = 0;
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