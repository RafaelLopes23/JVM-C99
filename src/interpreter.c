#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strcmp
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

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
    locals[7] = result;
    printf("IADD: %d + %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_isub(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 - val2;
    operand_stack_push(stack, result);
    locals[8] = result;
    printf("ISUB: %d - %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_imul(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 * val2;
    operand_stack_push(stack, result);
    locals[9] = result;
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
    locals[10] = result;
    printf("IDIV: %d / %d = %d\n", val1, val2, result);
    (*pc)++;
}

static void handle_ior(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t val2, val1;
    operand_stack_pop(stack, &val2);
    operand_stack_pop(stack, &val1);
    int32_t result = val1 | val2;
    operand_stack_push(stack, result);
    locals[11] = result;
    printf("IOR: %d | %d = %d\n", val1, val2, result);
    (*pc)++;
}

// Load/Store operations

static void handle_istore_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index = opcode - ISTORE_0;
    int32_t value;
    operand_stack_pop(stack, &value);
    locals[index] = value;
    (*pc)++;
}

static void handle_iload_n(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint8_t opcode = bytecode[*pc];
    int32_t index = opcode - ILOAD_0;
    operand_stack_push(stack, locals[index]);
    (*pc)++;
}

static void handle_return(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    (*pc)++;
}

// Stack operations
static void handle_dup(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    operand_stack_push(stack, value);
    operand_stack_push(stack, value);
    printf("DUP: Duplicated %d\n", value);
    (*pc)++;
}

static void handle_pop(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value;
    operand_stack_pop(stack, &value);
    printf("POP: Removed %d\n", value);
    (*pc)++;
}

// 64-bit operations (category 2)
static void handle_dadd(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    Cat2 val2 = operand_stack_pop_cat2(stack);
    Cat2 val1 = operand_stack_pop_cat2(stack);
    Cat2 result;
    result.double_ = val1.double_ + val2.double_;
    operand_stack_push_cat2(stack, result);
    printf("DADD: %f + %f = %f\n", val1.double_, val2.double_, result.double_);
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

static void handle_iastore(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    int32_t value, index, arrayref;
    operand_stack_pop(stack, &value);
    operand_stack_pop(stack, &index);
    operand_stack_pop(stack, &arrayref);
    
    Array *array = (Array *)(intptr_t)arrayref;
    if (!array || index < 0 || index >= array->length) {
        // Throw ArrayIndexOutOfBoundsException
        return;
    }
    
    ((int32_t *)array->elements)[index] = value;
    (*pc)++;
}

static void handle_new(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals) {
    uint16_t index = (bytecode[(*pc) + 1] << 8) | bytecode[(*pc) + 2];
    cp_info *classref = &jvm->class_file.constant_pool[index - 1];
    
    Object *obj = malloc(sizeof(Object));
    obj->class = &jvm->class_file; // Should load actual class
    obj->fields = calloc(1, 1024); // Fixed size for now
    
    intptr_t objref = (intptr_t)obj;
    operand_stack_push(stack, (int32_t)obj);
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
        (void)methodref->info.Methodref.class_index; // Silence warning
        uint16_t class_index = methodref->info.Methodref.class_index;
        uint16_t name_and_type_index = methodref->info.Methodref.name_and_type_index;
        
        cp_info *name_and_type = &jvm->class_file.constant_pool[name_and_type_index - 1];
        uint16_t name_index = name_and_type->info.NameAndType.name_index;
        
        const char *method_name = get_constant_pool_string(&jvm->class_file, name_index);
        
        if (method_name && strcmp(method_name, "println") == 0) {
            int32_t value;
            if (operand_stack_pop(stack, &value)) {
                int32_t dummy;
                operand_stack_pop(stack, &dummy);
                printf("%d\n", value);
            }
        }
    }
    
    *pc += 3;
}

// ... more handler functions for each instruction

static instruction_handler instruction_table[256] = {0};  // Initialize all to NULL

static void init_instruction_table(void) {
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
    instruction_table[DADD] = handle_dadd;
    instruction_table[NEW] = handle_new;
    instruction_table[NEWARRAY] = handle_newarray;
    instruction_table[IASTORE] = handle_iastore;
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

Cat2 pop_cat2_from_op_stack(); 
Cat2 push_cat2_to_op_stack( uint32_t  HighBytes,  uint32_t  LowBytes); 


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

void operand_stack_push_cat2(OperandStack *stack, Cat2 val) {
    if (stack->size + 2 > stack->capacity) {
        fprintf(stderr, "Stack overflow in push_cat2\n");
        return;
    }
    stack->values[stack->size++] = val.high;
    stack->values[stack->size++] = val.low;
}

bool operand_stack_pop(OperandStack *stack, int32_t *value) {
    if (stack->size <= 0) {
        return false;
    }
    *value = stack->values[--stack->size];
    return true;
}

Cat2 operand_stack_pop_cat2(OperandStack *stack) {
    Cat2 val;
    if (stack->size < 2) {
        fprintf(stderr, "Stack underflow in pop_cat2\n");
        val.high = val.low = 0;
        return val;
    }
    val.low = stack->values[--stack->size];
    val.high = stack->values[--stack->size];
    return val;
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

void execute_bytecode(JVM *jvm, uint8_t *bytecode, uint32_t bytecode_length) {
    int32_t local_vars[256] = {0};
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
            handler(jvm, bytecode, &pc, &operand_stack, local_vars);
        } else {
            fprintf(stderr, "Unknown opcode: 0x%02x\n", opcode);
            pc++;
        }

        if (opcode == IRETURN) break;
    }

    print_local_vars(local_vars);
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

    // Get the bytecode
    uint8_t *bytecode = get_method_bytecode(&jvm->class_file, 
                                          resolved_method->name, 
                                          resolved_method->descriptor);
    if (!bytecode) {
        fprintf(stderr, "Failed to get bytecode for method: %s\n", 
                resolved_method->name);
        return;
    }

    // Execute bytecode
    execute_bytecode(jvm, bytecode, /* bytecode_length */ 0);
}

void jvm_execute(JVM *jvm) {
    printf("Executing JVM\n");
    ClassFile *class_file = &jvm->class_file;
    method_info *main_method = NULL;

    // Debug print to see all methods
    for (int i = 0; i < class_file->methods_count; i++) {
        const char *method_name = get_constant_pool_string(class_file, class_file->methods[i].name_index);
        const char *method_descriptor = get_constant_pool_string(class_file, class_file->methods[i].descriptor_index);
        printf("Found method: %s with descriptor: %s\n", method_name, method_descriptor);
    }

    // Look for main method
    for (int i = 0; i < class_file->methods_count; i++) {
        const char *method_name = get_constant_pool_string(class_file, class_file->methods[i].name_index);
        const char *method_descriptor = get_constant_pool_string(class_file, class_file->methods[i].descriptor_index);
        
        if (method_name && method_descriptor) {
            printf("Checking method: %s with descriptor: %s\n", method_name, method_descriptor);
            if (strcmp(method_name, "main") == 0 &&
                strcmp(method_descriptor, "([Ljava/lang/String;)V") == 0) {
                main_method = &class_file->methods[i];
                printf("Found main method!\n");
                break;
            }
        }
    }

    if (main_method == NULL) {
        fprintf(stderr, "Main method not found\n");
        return;
    }

    // Fetch the bytecode array from the main method's code attribute
    attribute_info *code_attribute = NULL;
    const char *code_attribute_name = "Code";
    for (int i = 0; i < main_method->attributes_count; i++) {
        const char *attribute_name = get_constant_pool_string(class_file, main_method->attributes[i].attribute_name_index);
        if (attribute_name && strcmp(attribute_name, code_attribute_name) == 0) {
            code_attribute = &main_method->attributes[i];
            break;
        }
    }
    if (code_attribute == NULL) {
        fprintf(stderr, "Code attribute not found\n");
        return;
    }

    // Fix bytecode length calculation
    uint32_t code_length = 
        ((uint32_t)code_attribute->info[4] << 24) |
        ((uint32_t)code_attribute->info[5] << 16) |
        ((uint32_t)code_attribute->info[6] << 8) |
        (uint32_t)code_attribute->info[7];

    // Debug print
    printf("Code length: %u\n", code_length);

    // Validate code length
    if (code_length > code_attribute->attribute_length - 8) {
        fprintf(stderr, "Invalid code length\n");
        return;
    }

    // Get bytecode array with proper offset
    uint8_t *bytecode = code_attribute->info + 8;

    // Debug print first few bytes
    printf("First few bytecode bytes: ");
    for (int i = 0; i < 8 && i < code_length; i++) {
        printf("%02x ", bytecode[i]);
    }
    printf("\n");

    // Execute the bytecode
    execute_bytecode(jvm, bytecode, code_length);
}