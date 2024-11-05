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

void operand_stack_push(OperandStack *stack, int32_t value) {
    if (stack->top >= STACK_SIZE) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    stack->stack[stack->top++] = value;
}

int32_t operand_stack_pop(OperandStack *stack) {
    if (stack->top <= 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return stack->stack[--stack->top];
}

const char* get_utf8_from_constant_pool(ClassFile *class_file, uint16_t index) {
    cp_info *constant_pool_entry = &class_file->constant_pool[index - 1];
    if (constant_pool_entry->tag == CONSTANT_Utf8) {
        return (const char*)constant_pool_entry->info.Utf8.bytes;
    }
    return NULL;
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
    OperandStack operand_stack = { .top = 0 };

    for (uint32_t pc = 0; pc < bytecode_length; ) {
        uint8_t opcode = bytecode[pc++];
        printf("Executing opcode: 0x%02x at pc: %u\n", opcode, pc);
        switch (opcode) {
            case NOP:
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
                break;
            case BIPUSH: {
                int8_t byte = bytecode[pc++];
                operand_stack_push(&operand_stack, byte); // Push a byte onto the operand stack
                break;
            }
            case SIPUSH: {
                int16_t value = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 2;
                operand_stack_push(&operand_stack, value); // Push a short onto the operand stack
                break;
            }
            case ILOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load an int from a local variable
                break;
            }
            case LLOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load a long from a local variable
                break;
            }
            case FLOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load a float from a local variable
                break;
            }
            case DLOAD: {
                uint8_t index = bytecode[pc++];
                operand_stack_push(&operand_stack, jvm->jvm_stack.stack[index]); // Load a double from a local variable
                break;
            }
            case ISTORE: {
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store an int into a local variable
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
                uint8_t index = bytecode[pc++];
                jvm->jvm_stack.stack[index] = operand_stack_pop(&operand_stack); // Store a double into a local variable
                break;
            }
            case POP:
                operand_stack_pop(&operand_stack); // Pop the top of the operand stack
                break;
            case DUP: {
                int32_t value = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value);
                operand_stack_push(&operand_stack, value); // Duplicate the top of the operand stack
                break;
            }
            case IADD: {
                int32_t value2 = operand_stack_pop(&operand_stack);
                int32_t value1 = operand_stack_pop(&operand_stack);
                operand_stack_push(&operand_stack, value1 + value2); // Add two ints
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
            case INVOKEDYNAMIC: {
                printf("INVOKEDYNAMIC bytecode executed\n");
                uint16_t index = (bytecode[pc] << 8) | bytecode[pc + 1];
                pc += 2;
                cp_info *constant_pool = jvm->class_file.constant_pool;
                uint16_t bootstrap_method_attr_index = constant_pool[index].info.InvokeDynamic.bootstrap_method_attr_index;
                uint16_t name_and_type_index = constant_pool[index].info.InvokeDynamic.name_and_type_index;

                void *target_method_handle = resolve_bootstrap_method(jvm, bootstrap_method_attr_index, name_and_type_index);
                // Invoke target method
                invoke_method(jvm, target_method_handle);
                break;
            }
            // Add more bytecode instructions as needed
            default:
                fprintf(stderr, "Unknown bytecode instruction: 0x%02x\n", opcode);
                return;
        }
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
    // Fetch the main method from the class file
    ClassFile *class_file = &jvm->class_file;
    method_info *main_method = NULL;
    const char *main_method_name = "main";
    const char *main_method_descriptor = "([Ljava/lang/String;)V";
    for (int i = 0; i < class_file->methods_count; i++) {
        const char *method_name = get_constant_pool_string(class_file, class_file->methods[i].name_index);
        const char *method_descriptor = get_constant_pool_string(class_file, class_file->methods[i].descriptor_index);
        if (method_name && method_descriptor &&
            strcmp(method_name, main_method_name) == 0 &&
            strcmp(method_descriptor, main_method_descriptor) == 0) {
            main_method = &class_file->methods[i];
            break;
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

    // Execute the bytecode
    execute_bytecode(jvm, bytecode, bytecode_length);
}