// This file is to test the testing stuff.

#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main() {
    auto i = 67;
    logf("Hello, World! %d (Memplus %06X)\n", i, MEMPLUS_VERSION);

    return 0;
}
