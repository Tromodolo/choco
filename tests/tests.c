#include <stdio.h>
#include "tests.h"

#include "cpu-addressing.h"

void run_tests() {
    // fprintf(stderr, "asdasd");
    printf("Starting tests:\n");
    printf("Starting CPU addressing tests...\n");
    if (!test_cpu_addressing()) {
        fprintf(stderr, "CPU addressing tests failed!\n");
        return;
    }
}
