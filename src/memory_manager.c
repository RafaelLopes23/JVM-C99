#include "jvm.h"
#include <stdio.h>
#include <stdlib.h>

// Function prototypes
void heap_init(Heap *heap);
void stack_init(JVMStack *stack);

void jvm_init(JVM *jvm) {
    // Initialize JVM state
    printf("Initializing JVM\n");

    // Initialize heap
    heap_init(&jvm->heap);

    // Initialize stack
    stack_init(&jvm->jvm_stack);
}

#define HEAP_SIZE 1024 * 1024 // 1 MB heap size

void heap_init(Heap *heap) {
    heap->heap = (uint8_t *)malloc(HEAP_SIZE);
    if (heap->heap == NULL) {
        fprintf(stderr, "Failed to allocate heap memory\n");
        exit(1);
    }
    heap->heap_size = HEAP_SIZE;
    heap->heap_top = 0;
}

void *heap_alloc(Heap *heap, size_t size) {
    if (heap->heap_top + size > heap->heap_size) {
        fprintf(stderr, "Heap overflow\n");
        exit(1);
    }
    void *ptr = heap->heap + heap->heap_top;
    heap->heap_top += size;
    return ptr;
}

void heap_free(Heap *heap) {
    free(heap->heap);
}

#define STACK_SIZE 1024

void stack_init(JVMStack *stack) {
    stack->stack = (int32_t *)malloc(STACK_SIZE * sizeof(int32_t));
    if (stack->stack == NULL) {
        fprintf(stderr, "Failed to allocate stack memory\n");
        exit(1);
    }
    stack->stack_size = STACK_SIZE;
    stack->stack_top = 0;
}

void stack_push(JVMStack *stack, int32_t value) {
    if (stack->stack_top >= stack->stack_size) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    stack->stack[stack->stack_top++] = value;
}

int32_t stack_pop(JVMStack *stack) {
    if (stack->stack_top <= 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return stack->stack[--stack->stack_top];
}

void stack_free(JVMStack *stack) {
    free(stack->stack);
}