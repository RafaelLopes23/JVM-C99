#include "jvm.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <class file>\n", argv[0]);
        return 1;
    }

    JVM jvm;
    jvm_init(&jvm);
    jvm_load_class(&jvm, argv[1]);
    jvm_execute(&jvm);

    return 0;
}