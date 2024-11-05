#ifndef JVM_H
#define JVM_H

#include <stdint.h>

typedef struct {
    uint8_t tag;
    union {
        struct {
            uint16_t name_index;
        } Class;
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } Fieldref;
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } Methodref;
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        } InterfaceMethodref;
        struct {
            uint16_t string_index;
        } String;
        struct {
            int32_t bytes;
        } Integer;
        struct {
            float bytes;
        } Float;
        struct {
            int64_t bytes;
        } Long;
        struct {
            double bytes;
        } Double;
        struct {
            uint16_t name_index;
            uint16_t descriptor_index;
        } NameAndType;
        struct {
            uint16_t length;
            uint8_t *bytes;
        } Utf8;
        struct {
            uint8_t reference_kind;
            uint16_t reference_index;
        } MethodHandle;
        struct {
            uint16_t descriptor_index;
        } MethodType;
        struct {
            uint16_t bootstrap_method_attr_index;
            uint16_t name_and_type_index;
        } InvokeDynamic;
    } info;
} cp_info;

typedef struct {
    uint16_t attribute_name_index;
    uint32_t attribute_length;
    uint8_t *info;
} attribute_info;

typedef struct {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info *attributes;
} field_info;

typedef struct {
    uint16_t access_flags;
    uint16_t name_index;
    uint16_t descriptor_index;
    uint16_t attributes_count;
    attribute_info *attributes;
} method_info;

typedef struct {
    uint32_t magic;
    uint16_t minor_version;
    uint16_t major_version;
    uint16_t constant_pool_count;
    cp_info *constant_pool;
    uint16_t access_flags;
    uint16_t this_class;
    uint16_t super_class;
    uint16_t interfaces_count;
    uint16_t *interfaces;
    uint16_t fields_count;
    field_info *fields;
    uint16_t methods_count;
    method_info *methods;
    uint16_t attributes_count;
    attribute_info *attributes;
} ClassFile;

typedef struct {
    uint8_t *heap;
    size_t heap_size;
    size_t heap_top;
} Heap;

typedef struct {
    int32_t *stack;
    size_t stack_size;
    size_t stack_top;
} JVMStack;

typedef struct {
    JVMStack jvm_stack;
    Heap heap;
} JVMState;


typedef struct {
    JVMStack jvm_stack;
    Heap heap;
    ClassFile class_file; // Add this field to store the parsed class file
    // Add other JVM state and data structures here
} JVM;


typedef enum {
    NOP = 0x00,
    ACONST_NULL = 0x01,
    ICONST_M1 = 0x02,
    ICONST_0 = 0x03,
    ICONST_1 = 0x04,
    ICONST_2 = 0x05,
    ICONST_3 = 0x06,
    ICONST_4 = 0x07,
    ICONST_5 = 0x08,
    LCONST_0 = 0x09,
    LCONST_1 = 0x0A,
    FCONST_0 = 0x0B,
    FCONST_1 = 0x0C,
    FCONST_2 = 0x0D,
    DCONST_0 = 0x0E,
    DCONST_1 = 0x0F,
    BIPUSH = 0x10,
    SIPUSH = 0x11,
    ILOAD = 0x15,
    LLOAD = 0x16,
    FLOAD = 0x17,
    DLOAD = 0x18,
    ISTORE = 0x36,
    LSTORE = 0x37,
    FSTORE = 0x38,
    DSTORE = 0x39,
    POP = 0x57,
    DUP = 0x59,
    IADD = 0x60,
    ISUB = 0x64,
    IMUL = 0x68,
    IDIV = 0x6C,
    INVOKEDYNAMIC = 0xBA
} Bytecode;
void jvm_init(JVM *jvm);
void jvm_load_class(JVM *jvm, const char *class_file);
void jvm_execute(JVM *jvm);

void stack_push(JVMStack *stack, int32_t value);
int32_t stack_pop(JVMStack *stack);

void invoke_method(JVM *jvm, void *method_handle);

#endif // JVM_H