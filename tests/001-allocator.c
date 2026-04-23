#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"
#include <stdint.h>

void *alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    expect_eq(context, (void *) 69, "%p");
    switch (op) {
        case MP_ALLOCOP_ALLOC:   return (void *) new_size;
        case MP_ALLOCOP_REALLOC: return (void *) (new_size + old_size + (uintptr_t) ptr);
        case MP_ALLOCOP_FREE:    return (void *) (new_size + (uintptr_t) ptr);
    }
    unr();

fail:
    exit(1);
}

int main(void) {
    auto allocator = mp_alloc_new((void *) 69, alloc_func);

    void *alloc_res = mp_alloc(&allocator, 10);
    expect_eq(alloc_res, (void *) 10, "%p");

    void *realloc_res = mp_realloc(&allocator, (void *) 100, 10, 20);
    expect_eq(realloc_res, (void *) (100 + 10 + 20), "%p");

    void *free_res = mp_free(&allocator, (void *) 100, 10);
    expect_eq(free_res, (void *) (100 + 10), "%p");

    return 0;

fail:
    exit(1);
}
