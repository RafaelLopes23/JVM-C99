#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Include string.h for strcmp

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


typedef struct {
    int32_t stack[STACK_SIZE];
    int32_t top;
} OperandStack;


// auxiliary functions for bytecode operands
int test_op_stack_empty(OperandStack *stack);
int test_op_stack_overflow(OperandStack *stack);
int test_op_stack_underflow(OperandStack *stack);

int32_t operand_stack_pop(OperandStack *stack);
Cat2 pop_cat2_from_op_stack(); 
Cat2 push_cat2_to_op_stack( uint32_t  HighBytes,  uint32_t  LowBytes); 


const char* get_utf8_from_constant_pool(ClassFile *class_file, uint16_t index) {
    cp_info *constant_pool_entry = &class_file->constant_pool[index - 1];
    if (constant_pool_entry->tag == CONSTANT_Utf8) {
        return (const char*)constant_pool_entry->info.Utf8.bytes;
    }
    return NULL;
}

int test_op_stack_empty(OperandStack *stack) {
    if (stack->top == 0) {
        fprintf(stderr, "STACK IS EMPTY\n");
        exit(1);
    }
    return 1;
}
int test_op_stack_overflow(OperandStack *stack) {
    if (stack->top >= STACK_SIZE) {
        fprintf(stderr, "STACK OVERFLOW\n");
        exit(1);
    }
    return 1;
}
int test_op_stack_underflow(OperandStack *stack) {
    if (stack->top <= 0) {
        fprintf(stderr, "STACK UNDERFLOW\n");
        exit(1);
    }
    return 1;
}
 
void operand_stack_push(OperandStack *stack, int32_t value) {
    test_op_stack_overflow(stack);
    stack->stack[stack->top++] = value;
}
void operand_stack_push_cat2(OperandStack *stack, Cat2 val) {
    for (int i = 2; i < 0; i--) {
        test_op_stack_overflow(stack);
        stack->stack[stack->top++] = val.low;
    }
}

int32_t operand_stack_pop(OperandStack *stack) {
    test_op_stack_underflow(stack);
    return stack->stack[--stack->top];
}
Cat2 operand_stack_pop_cat2(OperandStack *stack) {
    Cat2 val;

    for (int i = 2; i < 0; i--) {
        test_op_stack_overflow(stack);
        stack->stack[stack->top++] = val.low;
    }

    return val;
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

const char* get_constant_pool_string(ClassFile *class_file, uint16_t index) {
    cp_info *constant_pool = class_file->constant_pool;
    if (constant_pool[index - 1].tag == CONSTANT_Utf8) {
        return (const char *)constant_pool[index - 1].info.Utf8.bytes;
    }
    return NULL;
}

void *resolve_bootstrap_method(JVM *jvm, uint16_t bootstrap_method_attr_index, uint16_t name_and_type_index) {
    // Resolve the bootstrap method handle
    cp_info *constant_pool = jvm->class_file.constant_pool;
    uint16_t method_handle_index = constant_pool[bootstrap_method_attr_index].info.MethodHandle.reference_index;
    void *method_handle = &constant_pool[method_handle_index]; // Correctly get the method handle
    // Resolve the name and type
    uint16_t name_index = constant_pool[name_and_type_index].info.NameAndType.name_index;
    uint16_t descriptor_index = constant_pool[name_and_type_index].info.NameAndType.descriptor_index;
    const char *name = get_constant_pool_string(&jvm->class_file, name_index);
    const char *descriptor = get_constant_pool_string(&jvm->class_file, descriptor_index);
    // Combine the method handle, name, and descriptor into a single structure
    struct {
        void *method_handle;
        const char *name;
        const char *descriptor;
    } *resolved_method = malloc(sizeof(*resolved_method));
    resolved_method->method_handle = method_handle;
    resolved_method->name = name;
    resolved_method->descriptor = descriptor;
    return resolved_method;
}

void execute_bytecode(JVM *jvm, uint8_t *bytecode, uint32_t bytecode_length) {
    printf("Starting bytecode execution\n");
    OperandStack operand_stack = { .top = 0 };

    for (uint32_t pc = 0; pc < bytecode_length; ) {
        uint8_t opcode = bytecode[pc];
        printf("Executing opcode: 0x%02x at pc: %d\n", opcode, pc);

        switch (opcode) {
            // Constants
            case NOP:
                pc++;
                // Do nothing
                break;
            case ACONST_NULL:
                operand_stack_push(&operand_stack, 0); // Push null (represented as 0) onto the operand stack
                break;
            case ICONST_M1:
                operand_stack_push(&operand_stack, -1); // Push the integer -1 onto the operand stack
                break;
            case ICONST_0:
                operand_stack_push(&operand_stack, 0); // Push the integer 0 onto the operand stack
                break;
            case ICONST_1:
                operand_stack_push(&operand_stack, 1); // Push the integer 1 onto the operand stack
                break;
            case ICONST_2:
                operand_stack_push(&operand_stack, 2); // Push 2 onto the operand stack
                break;
            case ICONST_3:
                operand_stack_push(&operand_stack, 3); // Push 3 onto the operand stack
                break;
            case ICONST_4:
                operand_stack_push(&operand_stack, 4); // Push 4 onto the operand stack
                break;
            case ICONST_5:
                operand_stack_push(&operand_stack, 5); // Push 5 onto the operand stack
                pc++;
                break;
            case LCONST_0:
                operand_stack_push(&operand_stack, 0L); // Push the long 0 onto the operand stack
                break;
            case LCONST_1:
                operand_stack_push(&operand_stack, 1L); // Push the long 1 onto the operand stack
                break;
            case FCONST_0:
                operand_stack_push(&operand_stack, 0.0f); // Push the float 0.0 onto the operand stack
                break;
            case FCONST_1:
                operand_stack_push(&operand_stack, 1.0f); // Push the float 1.0 onto the operand stack
                break;
            case FCONST_2:
                operand_stack_push(&operand_stack, 2.0f); // Push the float 2.0 onto the operand stack
                break;
            case DCONST_0:
                operand_stack_push(&operand_stack, 0.0); // Push the double 0.0 onto the operand stack
                break;
            case DCONST_1:
                operand_stack_push(&operand_stack, 1.0); // Push the double 1.0 onto the operand stack
                break;

            // Push values
            case BIPUSH: {
                int8_t byte = bytecode[pc++];
                operand_stack_push(&operand_stack, byte); // Push a byte onto the operand stack
                pc += 2;
                break;
            }
            case SIPUSH: {
                int16_t value = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 2;
                operand_stack_push(&operand_stack, value); // Push a short onto the operand stack
                pc += 3;
                break;
            }

            // Loads
            case ILOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load an int from a local variable
                break;
            }
            case ILOAD_0: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[0]); // Load an int from a local variable
                break;
            }case ILOAD_1: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[1]); // Load an int from a local variable
                break;
            }case ILOAD_2: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[2]); // Load an int from a local variable
                break;
            }case ILOAD_3: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[3]); // Load an int from a local variable
                break;
            }
// todo: fix these cat2 to load 2 of 32 bits
            case LLOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[0]); // Load an int from a local variable
                break;
            }case ILOAD_1: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[1]); // Load an int from a local variable
                break;
            }case ILOAD_2: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[2]); // Load an int from a local variable
                break;
            }case ILOAD_3: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[3]); // Load an int from a local variable
                break;
            }
       case LLOAD: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[index];
                val.low = jvm->jvm_stack.stack[index+1]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }
            case FLOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load a float from a local variable
                break;
            }
            case DLOAD: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[index];
                val.low = jvm->jvm_stack.stack[index+1]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }
            case DLOAD_0: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[index];
                val.low = jvm->jvm_stack.stack[index+1]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }
            case DLOAD_1: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[index];
                val.low = jvm->jvm_stack.stack[index+1]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }
            case DLOAD_0: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[0];
                val.low = jvm->jvm_stack.stack[1]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }
            case DLOAD_1: {
                uint8_t index = bytecode[pc++];
                Cat2 val;
                val.high = jvm->jvm_stack.stack[1];
                val.low = jvm->jvm_stack.stack[2]; // todo: test if these are indeed  the low and high bytes
                operand_stack_push_cat2(&operand_stack, val); 
                break;
            }

            // Stores
            case ISTORE: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store an int into a local variable
                pc +=2;
                break;
            }
            case ISTORE_0: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[0] = operand_stack_pop(&operand_stack); // Store an int into a local variable
                break;
            }
            case ISTORE_1: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[1] = operand_stack_pop(&operand_stack); // Store an int into a local variable
                break;
            }
            case ISTORE_2: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[2] = operand_stack_pop(&operand_stack); // Store an int into a local variable
                break;
            }
            case ISTORE_3: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[3] = operand_stack_pop(&operand_stack); // Store an int into a local variable
                pc++;
                break;
            }

            case LSTORE: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a long into a local variable
                break;
            }
            case FSTORE: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a float into a local variable
                break;
            }
            
            case DSTORE: {
                // todo ensure it pops/stores 64 bits
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }
            case DSTORE_0: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }
            case DSTORE_1: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }
            case DSTORE_2: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }
            case DSTORE_3: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }



            // Stack
            case POP:
                operand_stack_pop(&operand_stack); // Pop the top of the operand stack
                break;
            case DUP: {
                int32_t value = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value);
                operand_stack_push(&operand_stack, value); // Duplicate the top of the operand stack
                break;
            }

            // Math operations
            case IADD: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value1 + value2); // Add two ints
                break;
            }
            case DADD: {
                Cat2 val1 = operand_stack_pop_cat2(&operand_stack);
                Cat2 val2 = operand_stack_pop_cat2(&operand_stack);
                Cat2 valFinal;
                valFinal.double_ = val1.double_ + val2.double_;

                operand_stack_push_cat2(&operand_stack, valFinal); // Add two doubles
                break;
            }
            case ISUB: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value1 - value2); // Subtract two ints
                break;
            }
            case IMUL: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value1 * value2); // Multiply two ints
                break;
            }
            case IDIV: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                if (value2 == 0) {
                    fprintf(stderr, "Division by zero\n");
                    return;
                }
                operand_stack_push(&operand_stack, value1 / value2); // Divide two ints
                break;
            }
            case IOR: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value1 | value2); // Bitwise OR of two integers
                pc++;
                break;
            }

            case GETSTATIC: {
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 2;
                // Handle getting static field
                break;
            }

            case INVOKEVIRTUAL: {
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 3;
                // Handle method invocation
                break;
            }

            case LDC: {
                uint8_t index = bytecode[pc++];
                // Load constant from constant pool
                break;
            }

            case LDC_W: {
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 2;
                // Load constant from constant pool (wide index)
                break;
            }

            // Method invocation
            case INVOKEDYNAMIC: {
                printf("INVOKEDYNAMIC bytecode executed\n");
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 5;
                cp_info *constant_pool = jvm->class_file.constant_pool;
                uint16_t bootstrap_method_attr_index = constant_pool[index].info.InvokeDynamic.bootstrap_method_attr_index;
                uint16_t name_and_type_index = constant_pool[index].info.InvokeDynamic.name_and_type_index;

                void *target_method_handle = resolve_bootstrap_method(jvm, bootstrap_method_attr_index, name_and_type_index);
                // Invoke target method
                invoke_method(jvm, target_method_handle);
                break;
            }

            // Return
            case IRETURN: {
                return;
            }

            // Add more bytecode instructions as needed
            default:
                fprintf(stderr, "Unknown bytecode instruction: 0x%02x at pc: %d\n", opcode, pc);
                return; // Return instead of continuing with unknown instruction
        }
        pc++; // Move to next instruction after processing current one
    }
}


void display_bytecode(uint8_t *bytecode, uint32_t length) {
    uint32_t pc = 0;
    while (pc < length) {
        printf("%04x: ", pc);
        uint8_t opcode = bytecode[pc++];
        
        switch (opcode) {
            // Constants
            case NOP:
                printf("nop");
                break;
            case ACONST_NULL:
                printf("aconst_null");
                break;
            case ICONST_M1:
                printf("iconst_m1");
                break;
            case ICONST_0:
                printf("iconst_0");
                break;
            case ICONST_1:
                printf("iconst_1");
                break;
            case ICONST_2:
                printf("iconst_2");
                break;
            case ICONST_3:
                printf("iconst_3");
                break;
            case ICONST_4:
                printf("iconst_4");
                break;
            case ICONST_5:
                printf("iconst_5");
                break;
            case LCONST_0:
                printf("lconst_0");
                break;
            case LCONST_1:
                printf("lconst_1");
                break;
            case FCONST_0:
                printf("fconst_0");
                break;
            case FCONST_1:
                printf("fconst_1");
                break;
            case FCONST_2:
                printf("fconst_2");
                break;
            case DCONST_0:
                printf("dconst_0");
                break;
            case DCONST_1:
                printf("dconst_1");
                break;

            // Load instructions
            case ILOAD:
                printf("iload %d", bytecode[pc++]);
                break;
            case ILOAD_0:
                printf("iload_0");
                break;
            case ILOAD_1:
                printf("iload_1");
                break;
            case ILOAD_2:
                printf("iload_2");
                break;
            case ILOAD_3:
                printf("iload_3");
                break;
            case DLOAD:
                printf("dload %d", bytecode[pc++]);
                break;
            case DLOAD_0:
                printf("dload_0");
                break;
            case DLOAD_1:
                printf("dload_1");
                break;

            // Store instructions
            case ISTORE:
                printf("istore %d", bytecode[pc++]);
                break;
            case ISTORE_0:
                printf("istore_0");
                break;
            case ISTORE_1:
                printf("istore_1");
                break;
            case ISTORE_2:
                printf("istore_2");
                break;
            case ISTORE_3:
                printf("istore_3");
                break;
            case DSTORE:
                printf("dstore %d", bytecode[pc++]);
                break;

            // Stack operations
            case POP:
                printf("pop");
                break;
            case DUP:
                printf("dup");
                break;

            // Math operations
            case IADD:
                printf("iadd");
                break;
            case ISUB:
                printf("isub");
                break;
            case IMUL:
                printf("imul");
                break;
            case IDIV:
                printf("idiv");
                break;
            case IOR:
                printf("ior");
                break;
            case DADD:
                printf("dadd");
                break;

            // Method invocation
            case INVOKEDYNAMIC: {
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 4; // Skip 2 bytes of index and 2 bytes of padding
                printf("invokedynamic #%d", index);
                break;
            }

                        case GETSTATIC:
                printf("getstatic #%d", (bytecode[pc] << 8) | bytecode[pc + 1]);
                pc += 2;
                break;

            case INVOKEVIRTUAL:
                printf("invokevirtual #%d", (bytecode[pc] << 8) | bytecode[pc + 1]);
                pc += 2;
                break;

            case LDC:
                printf("ldc #%d", bytecode[pc++]);
                break;

            case LDC_W:
                printf("ldc_w #%d", (bytecode[pc] << 8) | bytecode[pc + 1]);
                pc += 2;
                break;

            case SIPUSH:
                printf("sipush %d", (int16_t)((bytecode[pc] << 8) | bytecode[pc + 1]));
                pc += 2;
                break;

            case BIPUSH:
                printf("bipush %d", (int8_t)bytecode[pc++]);
                break;

            // Return
            case IRETURN:
                printf("ireturn");
                break;

            default:
                printf("unknown 0x%02x", opcode);
                break;
        }
        printf("\n");
    }
}

void invoke_method(JVM *jvm, void *method_handle) {
    struct {
        void *method_handle;
        const char *name;
        const char *descriptor;
    } *resolved_method = method_handle;

    printf("Invoking method: %s %s\n", resolved_method->name, resolved_method->descriptor);

    // Fetch the method's bytecode
    uint8_t *bytecode = get_method_bytecode(&jvm->class_file, resolved_method->name, resolved_method->descriptor);
    if (bytecode == NULL) {
        fprintf(stderr, "Failed to fetch bytecode for method: %s\n", resolved_method->name);
        return;
    }

    // Execute the bytecode
    execute_bytecode(jvm, bytecode, /* bytecode_length */ 0); // Adjust bytecode_length as needed
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
    uint8_t *bytecode = code_attribute->info;
    uint32_t bytecode_length = code_attribute->attribute_length;

    printf("\nBytecode disassembly:\n");
    printf("--------------------\n");
    display_bytecode(bytecode, bytecode_length);
    printf("--------------------\n\n");


    // Execute the bytecode
    execute_bytecode(jvm, bytecode, bytecode_length);
}