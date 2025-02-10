#ifndef JVM_H
#define JVM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define ARRAY_TYPE_INT    10
#define ARRAY_TYPE_LONG   11
#define ARRAY_TYPE_FLOAT  6
#define ARRAY_TYPE_DOUBLE 7
// Array Type Constants
#define T_BOOLEAN   4
#define T_CHAR      5
#define T_FLOAT     6
#define T_DOUBLE    7
#define T_BYTE      8
#define T_SHORT     9
#define T_INT       10
#define T_LONG      11


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
    int32_t length;
    uint8_t type;
    void *elements;
    size_t element_size;  // Add element size field
} Array;

typedef struct {
    uint8_t *heap;
    size_t heap_size;
    size_t heap_top;
    Array **arrays;         // Array of pointers
    uint32_t array_count;   // Current number of arrays
    uint32_t array_cap;     // Capacity of the array
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
    GETSTATIC = 0xB2,

    // Load operations
    ALOAD = 0x19,        // Load reference from local variable
    ALOAD_0 = 0x2A,      // Load reference from local variable 0
    ALOAD_1 = 0x2B,      // Load reference from local variable 1
    ALOAD_2 = 0x2C,      // Load reference from local variable 2
    ALOAD_3 = 0x2D,      // Load reference from local variable 3

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
    
    // Load constant operations
    LDC = 0x12,          // Push item from constant pool
    LDC_W = 0x13,        // Push item from constant pool (wide index)
    LDC2_W = 0x14,       // Push long or double from constant pool

    // Loads
    ILOAD = 0X15,
    ILOAD_0 = 0x1A,
    ILOAD_1 = 0x1B,
    ILOAD_2 = 0x1C,
    ILOAD_3 = 0x1D,
    LLOAD = 0x16,
    LLOAD_0 = 0x1E,
    LLOAD_1 = 0x1F,
    LLOAD_2 = 0x20,
    LLOAD_3 = 0x21,
    FLOAD = 0x17,
    DLOAD = 0x18,
    FLOAD_0 = 0X22,
    FLOAD_1 = 0X23,
    FLOAD_2 = 0X24,
    FLOAD_3 = 0X25,
    DLOAD_0 = 0x26,
    DLOAD_1 = 0x27,
    DLOAD_2 = 0x28,
    DLOAD_3 = 0x29,

    // Array load operations
    IALOAD = 0x2E,       // Load int from array
    LALOAD = 0x2F,       // Load long from array
    FALOAD = 0x30,       // Load float from array
    DALOAD = 0x31,       // Load double from array
    AALOAD = 0x32,       // Load reference from array
    BALOAD = 0x33,       // Load byte/boolean from array
    CALOAD = 0x34,       // Load char from array
    SALOAD = 0x35,       // Load short from array

    // Store operations
    ASTORE = 0x3A,       // Store reference into local variable
    ASTORE_0 = 0x4B,     // Store reference into local variable 0
    ASTORE_1 = 0x4C,     // Store reference into local variable 1
    ASTORE_2 = 0x4D,     // Store reference into local variable 2
    ASTORE_3 = 0x4E,     // Store reference into local variable 3

    ISTORE = 0x36,
    ISTORE_0 = 0x3B,
    ISTORE_1 = 0x3C,
    ISTORE_2 = 0x3D,
    ISTORE_3 = 0x3E,

    LSTORE = 0x37,
    LSTORE_0 = 0x3F,
    LSTORE_1 = 0x40,
    LSTORE_2 = 0x41,
    LSTORE_3 = 0x42,
    FSTORE = 0x38,
    FSTORE_0 = 0x43,
    FSTORE_1 = 0x44,
    FSTORE_2 = 0x45,
    FSTORE_3 = 0x46,
    DSTORE = 0x39,
    DSTORE_0 = 0x47,
    DSTORE_1 = 0x48,
    DSTORE_2 = 0x49,
    DSTORE_3 = 0x4a,

    // Array store operations
    IASTORE = 0x4F,      // Store into int array
    LASTORE = 0x50,      // Store into long array
    FASTORE = 0x51,      // Store into float array
    DASTORE = 0x52,      // Store into double array
    AASTORE = 0x53,      // Store into reference array
    BASTORE = 0x54,      // Store into byte/boolean array
    CASTORE = 0x55,      // Store into char array
    SASTORE = 0x56,      // Store into short array
    
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
    DSUB = 0x67,
    DMUL = 0x6B,
    DDIV = 0x6F,
    DREM = 0x73,
    DNEG = 0x77,

    LADD = 0x61,
    LSUB = 0x65,
    LMUL = 0x69,
    LDIV = 0x6D,
    LREM = 0x71,
    LNEG = 0x74,
    LAND = 0x7e,

    // Comparison
    LCMP = 0x94,
    DCMPL = 0X97,
    DCMPG = 0X98,

    // Conversion
    D2F = 0X90,
    D2I = 0X8E,
    D2L = 0X8F,

    // Branch
    IFEQ = 0x99,
    IFNE = 0x9A,
    IFLT = 0x9B,
    IFGE = 0x9C,
    IFGT = 0x9D,
    IFLE = 0x9E,
    IF_ICMPEQ = 0x9F,

    // Return operations
    RETURN = 0xB1,       // Return void from method
    IRETURN = 0xAC,      // Return int from method
    LRETURN = 0xAD,      // Return long from method
    FRETURN = 0xAE,      // Return float from method
    DRETURN = 0xAF,      // Return double from method
    ARETURN = 0xB0,      // Return reference from method

    // Method invocation
    INVOKEVIRTUAL = 0xB6,    // Invoke instance method
    INVOKESPECIAL = 0xB7,    // Invoke instance method (special handling)
    INVOKESTATIC = 0xB8,     // Invoke static method 
    INVOKEINTERFACE = 0xB9,  // Invoke interface method
    INVOKEDYNAMIC = 0xBA,    // Invoke dynamic method

    // Object operations
    NEW = 0xBB,          // Create new object
    NEWARRAY = 0xBC,     // Create new array
    ANEWARRAY = 0xBD,    // Create new array of reference
    ARRAYLENGTH = 0xBE,  // Get array length

    INSTANCEOF = 0xBF,
    CHECKCAST = 0xC0

} Bytecode;

// unions for bytecode operands
typedef union {
    struct{
        uint32_t low;
        uint32_t high;
    };
	int32_t int_;
    uint64_t    bytes;
    int64_t  long_;
    double    double_;
} Cat2;

typedef struct {
    uint64_t *values;  // Change from int32_t to uint64_t
    int size;
    int capacity;
} OperandStack;

void jvm_init(JVM *jvm);
void jvm_load_class(JVM *jvm, const char *class_file);
void jvm_execute(JVM *jvm);
bool operand_stack_push(OperandStack *stack, uint64_t value);
bool operand_stack_pop(OperandStack *stack, uint64_t *value);
void operand_stack_push_cat2(OperandStack *stack, uint64_t value);
uint64_t operand_stack_pop_cat2(OperandStack *stack);
void operand_stack_init(OperandStack *stack, int capacity);
bool validate_constant_pool_index(ClassFile *class_file, uint16_t index);
void print_stack_state(OperandStack *stack);

typedef void (*instruction_handler)(JVM *jvm, uint8_t *bytecode, uint32_t *pc, OperandStack *stack, int32_t *locals);

void stack_push(JVMStack *stack, int32_t value);
int32_t stack_pop(JVMStack *stack);

void invoke_method(JVM *jvm, void *method_handle);

#endif // JVM_H