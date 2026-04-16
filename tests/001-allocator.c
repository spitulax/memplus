#include "memplus.h"
#include "test.h"
#include <stdint.h>

void *alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    expect_eq(context, (void *) 69, "%p");
    switch (type) {
        case MP_ALLOCTYPE_ALLOC:   return (void *) new_size;
        case MP_ALLOCTYPE_REALLOC: return (void *) (new_size + old_size + (uintptr_t) ptr);
        case MP_ALLOCTYPE_DUP:     return (void *) (new_size + (uintptr_t) ptr);
        case MP_ALLOCTYPE_FREE:    return (void *) (new_size + (uintptr_t) ptr);
    }
    unr();
}

int main(void) {
    auto allocator = mp_allocator_new((void *) 69, alloc_func);

    void *alloc_res = mp_alloc(&allocator, 10);
    expect_eq(alloc_res, (void *) 10, "%p");

    void *realloc_res = mp_realloc(&allocator, (void *) 100, 10, 20);
    expect_eq(realloc_res, (void *) (100 + 10 + 20), "%p");

    void *dup_res = mp_dup(&allocator, (void *) 100, 10);
    expect_eq(dup_res, (void *) (100 + 10), "%p");

    void *free_res = mp_free(&allocator, (void *) 100, 10);
    expect_eq(free_res, (void *) (100 + 10), "%p");

    return 0;
}
