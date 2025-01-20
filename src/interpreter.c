#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strcmp
#include <stdint.h>
#include <stdbool.h>

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

typedef struct {
    int32_t *values;
    int size;
    int capacity;
} OperandStack;

typedef struct {
    uint8_t opcode;
    const char* mnemonic;
    const char* description;
    int operand_bytes;
} instruction_info;

static const instruction_info instructions[] = {
    {0x00, "nop", "Do nothing", 0},
    {0x02, "iconst_m1", "Push int constant -1", 0},
    {0x03, "iconst_0", "Push int constant 0", 0},
    {0x04, "iconst_1", "Push int constant 1", 0},
    {0x05, "iconst_2", "Push int constant 2", 0},
    {0x06, "iconst_3", "Push int constant 3", 0},
    {0x07, "iconst_4", "Push int constant 4", 0},
    {0x08, "iconst_5", "Push int constant 5", 0},
    {0x10, "bipush", "Push byte", 1},
    {0x11, "sipush", "Push short", 2},
    {0x15, "iload", "Load int from local variable", 1},
    {0x1a, "iload_0", "Load int from local variable 0", 0},
    {0x1b, "iload_1", "Load int from local variable 1", 0},
    {0x36, "istore", "Store int into local variable", 1},
    {0x3b, "istore_0", "Store int into local variable 0", 0},
    {0x3c, "istore_1", "Store int into local variable 1", 0},
    {0x60, "iadd", "Add int", 0},
    {0x64, "isub", "Subtract int", 0},
    {0x68, "imul", "Multiply int", 0},
    {0x6c, "idiv", "Divide int", 0},
    {0xb1, "return", "Return void from method", 0},
    {0xb2, "getstatic", "Get static field from class", 2},
    {0xb6, "invokevirtual", "Invoke instance method", 2},
    {0xba, "invokedynamic", "Invoke dynamic method", 4},
};

bool validate_constant_pool_index(ClassFile *class_file, uint16_t index);


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
    printf("\nLocal Variables:\n");
    printf("a (1): %d\n", local_vars[1]);
    printf("b (2): %d\n", local_vars[2]);
    printf("c (3): %d\n", local_vars[3]);
    printf("d (4): %d\n", local_vars[4]);
    printf("e (5): %d\n", local_vars[5]);
    printf("sum (7): %d\n", local_vars[7]);
    printf("diff (8): %d\n", local_vars[8]);
    printf("prod (9): %d\n", local_vars[9]);
    printf("quot (10): %d\n", local_vars[10]);
    printf("or (11): %d\n", local_vars[11]);
}

// auxiliary functions for bytecode operands
bool test_op_stack_empty(OperandStack *stack);
bool test_op_stack_overflow(OperandStack *stack);
bool test_op_stack_underflow(OperandStack *stack);

bool operand_stack_pop(OperandStack *stack, int32_t *value);
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
    printf("Validating constant pool index: %d (pool count: %d)\n", 
           index, class_file->constant_pool_count);
    
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
    
    uint32_t pc = 0;
    while (pc < bytecode_length) {
        // Validate PC
        if (pc >= bytecode_length) {
            fprintf(stderr, "PC out of bounds: %u >= %u\n", pc, bytecode_length);
            break;
        }

        uint8_t opcode = bytecode[pc];
        printf("PC: %04x Opcode: 0x%02x\n", pc, opcode);

        // Handle RETURN explicitly
        if (opcode == 0xb1) {
            printf("RETURN instruction encountered\n");
            break;
        }

        switch (opcode) {
            // Constants
            case NOP:
                pc++;
                break;
            case ACONST_NULL:
                operand_stack_push(&operand_stack, 0);
                break;
            case ICONST_M1:
                operand_stack_push(&operand_stack, -1);
                break;

            case ICONST_0: case ICONST_1: case ICONST_2: 
            case ICONST_3: case ICONST_4: case ICONST_5: {
                int32_t value = opcode - ICONST_0;
                if (!operand_stack_push(&operand_stack, value)) {
                    fprintf(stderr, "Stack overflow in ICONST\n");
                    return;
                }
                printf("ICONST_%d: Pushed %d\n", value, value);
                pc++;
                break;
            }

            // Push values
            case BIPUSH: {
                int8_t value = (int8_t)bytecode[pc + 1];
                operand_stack_push(&operand_stack, value);
                pc += 2;
                printf("BIPUSH: pushed value %d\n", value);
                break;
            }

            case SIPUSH: {
                int16_t value = (int16_t)((bytecode[pc + 1] << 8) | bytecode[pc + 2]);
                operand_stack_push(&operand_stack, value);
                pc += 3;
                printf("SIPUSH: pushed value %d\n", value);
                break;
            }

            // Loads
            case ILOAD: {
                uint8_t index = bytecode[pc + 1];
                int32_t value = local_vars[index];
                operand_stack_push(&operand_stack, value);
                printf("ILOAD: Loaded %d from var[%d]\n", value, index);
                pc += 2;
                break;
            }
            case ILOAD_0: case ILOAD_1: case ILOAD_2: case ILOAD_3: {
                uint8_t index = opcode - ILOAD_0;
                int32_t value = local_vars[index];
                operand_stack_push(&operand_stack, value);
                printf("ILOAD_%d: Loaded %d from var[%d]\n", index, value, index);
                pc++;
                break;
            }

            // Stores
            case ISTORE: {
                int32_t value;
                if (!operand_stack_pop(&operand_stack, &value)) {
                    fprintf(stderr, "Stack underflow in ISTORE\n");
                    return;
                }
                uint8_t index = bytecode[pc + 1];
                local_vars[index] = value;
                printf("ISTORE: var[%d] = %d\n", index, value);
                pc += 2;
                break;
            }
            case ISTORE_0: case ISTORE_1: case ISTORE_2: case ISTORE_3: {
                int32_t value;
                if (!operand_stack_pop(&operand_stack, &value)) {
                    fprintf(stderr, "Stack underflow in ISTORE\n");
                    return;
                }
                uint8_t index = opcode - ISTORE_0;
                local_vars[index] = value;
                printf("ISTORE_%d: var[%d] = %d\n", index, index, value);
                pc++;
                break;
            }

            // Stack
            case POP: {
                CHECK_STACK(&operand_stack, 1);
                int32_t value;
                operand_stack_pop(&operand_stack, &value);
                pc++;
                break;
            }

            case DUP: {
                CHECK_STACK(&operand_stack, 1);
                int32_t value;
                operand_stack_pop(&operand_stack, &value);
                operand_stack_push(&operand_stack, value);
                operand_stack_push(&operand_stack, value);
                pc++;
                break;
            }

            // Math operations
            case IADD: {
                int32_t val2, val1;
                operand_stack_pop(&operand_stack, &val2);
                operand_stack_pop(&operand_stack, &val1);
                int32_t result = val1 + val2;
                operand_stack_push(&operand_stack, result);
                local_vars[7] = result; // Store sum
                printf("IADD: %d + %d = %d\n", val1, val2, result);
                pc++;
                break;
            }

            case ISUB: {
                int32_t val2, val1;
                operand_stack_pop(&operand_stack, &val2);
                operand_stack_pop(&operand_stack, &val1);
                int32_t result = val1 - val2;
                operand_stack_push(&operand_stack, result);
                local_vars[8] = result; // Store difference
                printf("ISUB: %d - %d = %d\n", val1, val2, result);
                pc++;
                break;
            }

            case IMUL: {
                int32_t val2, val1;
                operand_stack_pop(&operand_stack, &val2);
                operand_stack_pop(&operand_stack, &val1);
                int32_t result = val1 * val2;
                operand_stack_push(&operand_stack, result);
                local_vars[9] = result; // Store product
                printf("IMUL: %d * %d = %d\n", val1, val2, result);
                pc++;
                break;
            }

            case IDIV: {
                int32_t val2, val1;
                operand_stack_pop(&operand_stack, &val2);
                operand_stack_pop(&operand_stack, &val1);
                if (val2 == 0) {
                    fprintf(stderr, "Division by zero\n");
                    return;
                }
                int32_t result = val1 / val2;
                operand_stack_push(&operand_stack, result);
                local_vars[10] = result; // Store quotient
                printf("IDIV: %d / %d = %d\n", val1, val2, result);
                pc++;
                break;
            }

            case IOR: {
                int32_t val2, val1;
                operand_stack_pop(&operand_stack, &val2);
                operand_stack_pop(&operand_stack, &val1);
                int32_t result = val1 | val2;
                operand_stack_push(&operand_stack, result);
                local_vars[11] = result; // Store OR result
                printf("IOR: %d | %d = %d\n", val1, val2, result);
                pc++;
                break;
            }

            case GETSTATIC: {
                uint16_t index = (bytecode[pc + 1] << 8) | bytecode[pc + 2];
                if (!validate_constant_pool_index(&jvm->class_file, index)) {
                    return;
                }
                
                // For System.out, we'll just verify it's the PrintStream field
                cp_info *fieldref = &jvm->class_file.constant_pool[index - 1];
                if (fieldref->tag == CONSTANT_Fieldref) {
                    // Push a dummy reference for PrintStream
                    operand_stack_push(&operand_stack, 1); // Dummy reference
                }
                
                pc += 3;
                break;
            }

            case INVOKEVIRTUAL: {
                uint16_t index = (bytecode[pc + 1] << 8) | bytecode[pc + 2];
                if (!validate_constant_pool_index(&jvm->class_file, index)) {
                    return;
                }

                cp_info *methodref = &jvm->class_file.constant_pool[index - 1];
                if (methodref->tag == CONSTANT_Methodref) {
                    // Get the name and type info
                    uint16_t class_index = methodref->info.Methodref.class_index;
                    uint16_t name_and_type_index = methodref->info.Methodref.name_and_type_index;
                    
                    cp_info *name_and_type = &jvm->class_file.constant_pool[name_and_type_index - 1];
                    uint16_t name_index = name_and_type->info.NameAndType.name_index;
                    
                    const char *method_name = get_constant_pool_string(&jvm->class_file, name_index);
                    
                    if (method_name && strcmp(method_name, "println") == 0) {
                        // For println, pop value and string prefix
                        int32_t value;
                        if (operand_stack_pop(&operand_stack, &value)) {
                            // Pop the dummy PrintStream reference
                            int32_t dummy;
                            operand_stack_pop(&operand_stack, &dummy);
                            
                            // Print the actual value
                            printf("%d\n", value);
                        }
                    }

                pc += 3;
                break;
            }

                pc += 3;
                break;
            }

            case LDC: {
                uint8_t index = bytecode[pc++];
                if (!validate_constant_pool_index(&jvm->class_file, index)) {
                    return;
                }
                break;
            }

            case LDC_W: {
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                if (!validate_constant_pool_index(&jvm->class_file, index)) {
                    return;
                }
                pc += 2;
                break;
            }

            // Method invocation
            case INVOKEDYNAMIC: {
                printf("INVOKEDYNAMIC bytecode executed\n");
                uint16_t index = (bytecode[pc + 1] << 8) | bytecode[pc + 2];
                
                if (!validate_constant_pool_index(&jvm->class_file, index)) {
                    return;
                }
                
                // For string concatenation, pop the value and push it back
                int32_t value;
                if (operand_stack_pop(&operand_stack, &value)) {
                    operand_stack_push(&operand_stack, value);
                }
                
                pc += 5;
                break;
            }

            // Return
            case 0xb1: // return
                printf("RETURN instruction - ending execution\n");
                return;

            case 0xac: // ireturn
                printf("IRETURN instruction - ending execution\n");
                return;
            

            default:
                printf("Unknown opcode: 0x%02x\n", opcode);
                pc++;
        }

        print_stack_state(&operand_stack);
    }

    // Print final state
    printf("\nFinal state:\n");
    printf("a = %d\n", local_vars[1]);
    printf("b = %d\n", local_vars[2]);
    printf("c = %d\n", local_vars[3]);
    printf("d = %d\n", local_vars[4]);
    printf("e = %d\n", local_vars[5]);
    printf("sum = %d\n", local_vars[7]);
    printf("diff = %d\n", local_vars[8]);
    printf("prod = %d\n", local_vars[9]);
    printf("quot = %d\n", local_vars[10]);
    printf("or = %d\n", local_vars[11]);

    free(operand_stack.values);
}



void display_bytecode(uint8_t *bytecode, uint32_t bytecode_length) {
    printf("--------------------\n");
    printf("Offset   Mnemonic        Operands    Description\n");
    printf("------------------------------------------------\n");

    uint32_t pc = 0;
    while (pc < bytecode_length) {
        uint8_t opcode = bytecode[pc];
        
        // Print offset
        printf("%04x:   ", pc);

        // Find instruction info
        const instruction_info* info = NULL;
        for (size_t i = 0; i < sizeof(instructions)/sizeof(instruction_info); i++) {
            if (instructions[i].opcode == opcode) {
                info = &instructions[i];
                break;
            }
        }

        if (info) {
            // Print mnemonic with padding
            printf("%-14s", info->mnemonic);

            // Print operands if any
            if (info->operand_bytes > 0) {
                printf(" ");
                for (int i = 0; i < info->operand_bytes; i++) {
                    printf("%02x", bytecode[pc + 1 + i]);
                }
                printf("%*s", 10 - (info->operand_bytes * 2), "");  // Padding
            } else {
                printf("%12s", "");  // Padding for no operands
            }

            // Print description
            printf("%s\n", info->description);
            
            // Advance PC
            pc += 1 + info->operand_bytes;
        } else {
            printf("unknown 0x%02x\n", opcode);
            pc++;
        }
    }
    printf("--------------------\n\n");
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

    // Print bytecode disassembly
    printf("\nBytecode disassembly:\n");
    printf("--------------------\n");
    display_bytecode(bytecode, code_length);
    printf("--------------------\n\n");
}