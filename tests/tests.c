#include <stdio.h>
#include "tests.h"

#define NES_TESTS 0
#define DMG_TESTS 1

#if NES_TESTS
#include "nes/auto-tests.h"
#endif

#if DMG_TESTS
#include "dmg/auto-tests.h"
#endif

void run_tests() {
    printf("Starting automatic tests:\n");
    run_auto_tests();
}
