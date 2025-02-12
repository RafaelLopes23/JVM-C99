#ifndef JVM_H
#define JVM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARRAY_TYPE_INT    10
#define ARRAY_TYPE_LONG   11
#define ARRAY_TYPE_FLOAT  6
#define ARRAY_TYPE_DOUBLE 7


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


// mb delete down
typedef struct FieldStatic {
    int32_t value;
} FieldStatic;

typedef struct ClassFileElement {
    char *class_name;
    ClassFile *class;
    FieldStatic *static_fields;
    uint16_t static_fields_count;
} ClassFileElement;


// mb delete up


typedef struct {
    JVMStack jvm_stack;
    Heap heap;
    ClassFile class_file; // Add this field to store the parsed class file
    ClassFileElement *loaded_class; // Simplified loaded class representation
    // Add other JVM state and data structures here
} JVM;

typedef enum {

    GETSTATIC = 0xB2,
    INVOKEVIRTUAL = 0xB6,

    LDC = 0x12,
    LDC_W = 0x13,

   // Constants
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
    
    
    // Push values
    BIPUSH = 0x10,
    SIPUSH = 0x11,
    
    // Loads
    ILOAD = 0x15,
    // TODO: test
    ILOAD_0 = 0x1A,
    ILOAD_1 = 0x1B,
    ILOAD_2 = 0x1C,
    ILOAD_3 = 0x1D,

    LLOAD = 0x16,
    FLOAD = 0x17,
    DLOAD = 0x18,
    FLOAD_0 =0X22,
    FLOAD_1 =0X23,
    FLOAD_2 =0X24,
    FLOAD_3 =0X25,
    DLOAD_0 = 0x26,
    DLOAD_1 = 0x27,
    DLOAD_2 = 0x28,
    DLOAD_3 = 0x29,


// longs
    LALOAD = 0x2e,
    LAND = 0x7e,
    LASTORE = 0x4f,
    LCMP = 0x94,
    LLOAD_0 = 0x1a,
    LLOAD_1 = 0x1b,
    LLOAD_2 = 0x1c,
    LLOAD_3 = 0x1d,
    LNEG = 0x74,
    LADD = 0x61, 
    LSUB = 0x65,  
    LMUL = 0x69,  
    LDIV = 0x6d,  
    LREM = 0x71, 

    DREM = 0x73,  
    

    // Stores
    ISTORE = 0x36,
    ISTORE_0 = 0x3B,
    ISTORE_1 = 0x3C,
    ISTORE_2 = 0x3D,
    ISTORE_3 = 0x3E, 


    LSTORE = 0x37,
    FSTORE = 0x38,

    DSTORE = 0x39,
    DSTORE_0 = 0x47,
    DSTORE_1 = 0x48,
    DSTORE_2 = 0x49,
    DSTORE_3 = 0x4a, 
    
    // Stack
    POP = 0x57,
    DUP = 0x59,
    
    // Math operations
    IADD = 0x60,
    ISUB = 0x64,
    IMUL = 0x68,
    IDIV = 0x6C,
    IOR = 0x80,

    DADD = 0x63,
    
    NEW = 0xBB,
    NEWARRAY = 0xBC,
    IASTORE = 0x4F,

    // Method invocation
    INVOKEDYNAMIC = 0xBA,

    
    INSTANCEOF = 0XBF,

    I2F = 0X86,
    IINC = 0X84,
    GOTO_W = 0XC8,
    GOTO = 0XA7,
    
    IFEQ = 0X99,
    IFNE = 0X9A,  
    IFLT = 0X9B,  
    IFGE = 0X9C,
    IFGT = 0X9D,
    IFLE = 0X9E,
    IF_ICMPEQ = 0x9F,
    IF_ICMPNE = 0xA0,
    IF_ICMPLT = 0xA1,
    IF_ICMPGE = 0xA2,
    IF_ICMPGT = 0xA3,
    IF_ICMPLE = 0xA4,
 
    LDC2_W = 0x14,

    DSUB = 0x67,
    DMUL = 0x6B,
    DDIV = 0x6F,
    DNEG = 0x77,

    ASTORE = 0x3A,

    DCMPL = 0X97,
    DCMPG = 0X98,
    D2F = 0X90,
    D2I = 0X8E,
    D2L = 0X8F,

    INVOKESPECIAL = 0xB7,
    CHECKCAST = 0xC0,

    IRETURN = 0xB1,
    RETURN = 0xb1,

} Bytecode;

// unions for bytecode operands
typedef union {
    struct{
        uint32_t low;
        uint32_t high;
    };
	int32_t int_;
    uint64_t    bytes_;
    int64_t  long_;
    double    double_;
} Cat2;



typedef struct {
    uint32_t *values;  // Changed to uint32_t
    int size;          // Number of slots (32 bits)
    int capacity;
} OperandStack;

void jvm_init(JVM *jvm);
void jvm_load_class(JVM *jvm, const char *class_file);
void jvm_execute(JVM *jvm);
bool operand_stack_push(OperandStack *stack, int32_t value);
bool operand_stack_pop(OperandStack *stack, int32_t *value);
void operand_stack_push_cat2(OperandStack *stack, Cat2 val);
Cat2 operand_stack_pop_cat2(OperandStack *stack);
void operand_stack_init(OperandStack *stack, int capacity);
bool validate_constant_pool_index(ClassFile *class_file, uint16_t index);
void print_stack_state(OperandStack *stack);
void print_local_vars(int32_t *local_vars);

typedef void (*instruction_handler)(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals);

void stack_push(JVMStack *stack, int32_t value);
int32_t stack_pop(JVMStack *stack);

void invoke_method(JVM *jvm, void *method_handle);

#endif // JVM_H