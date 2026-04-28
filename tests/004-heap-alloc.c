#include "test.h"
#include <stdint.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

int main(void) {
    mp_Alloc *alloc = mp_heap_alloc();

    uint32_t *p1 = mp_alloc(alloc, sizeof(*p1));
    *p1          = 67;
    expect_eq(*p1, 67u, "%u");

    uint64_t *p2 = mp_realloc(alloc, p1, sizeof(*p1), sizeof(*p2));
    *p2          = 69;
    expect_eq(*p2, (size_t) 69, "%zu");

    uint64_t *p3 = mp_dup(alloc, p2, sizeof(*p2));
    expect_eq(*p3, (size_t) 69, "%zu");
    mp_free(alloc, p2, sizeof(*p2));
    mp_free(alloc, p3, sizeof(*p3));

    return 0;

fail:
    exit(1);
}
