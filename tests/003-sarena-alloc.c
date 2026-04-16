#include "test.h"
#include <stdint.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

#define align(a) (DIV_ROUNDUP((a), sizeof(uintptr_t)) * sizeof(uintptr_t))

int main(void) {
    mp_Arena parent_arena;
    mp_arena_init(&parent_arena);
    mp_Allocator parent_alloc = mp_arena_allocator(&parent_arena);

    mp_SArena arena;
    mp_sarena_init(&arena, &parent_alloc, MP_REGION_DEFAULT_SIZE);
    mp_Allocator alloc = mp_sarena_allocator(&arena);
    expect_eq(arena.cap, (size_t) align(MP_REGION_DEFAULT_SIZE), "%zu");

    // First allocation
    mp_alloc(&alloc, 10);
    expect_eq(arena.len, (size_t) align(10), "%zu");

    // Too big for this
    void *failed = mp_alloc(&alloc, 64 * 1024);
    expect_eq(failed, NULL, "%p");
    expect_eq(arena.len, (size_t) align(10), "%zu");

    // Reset arena and allocate again
    mp_sarena_reset(&arena);
    mp_alloc(&alloc, 10);
    expect_eq(arena.cap, (size_t) align(MP_REGION_DEFAULT_SIZE), "%zu");
    expect_eq(arena.len, (size_t) align(10), "%zu");

    // Realloc test
    mp_sarena_reset(&arena);
    int32_t *mem  = mp_alloc(&alloc, 4);
    *mem          = 67;
    int64_t *mem2 = mp_realloc(&alloc, mem, 4, 8);
    expect_eq(arena.len, (size_t) align(4 + 8), "%zu");
    expect_eq(*mem2, (int64_t) 67, "%ld");
    expect_eq(*(int64_t *) mem, *mem2, "%ld");

    // Dup test
    mp_sarena_reset(&arena);
    mem           = mp_alloc(&alloc, 4);
    *mem          = 67;
    int32_t *mem3 = mp_dup(&alloc, mem, 4);
    expect_eq(arena.len, (size_t) align(4) + align(4), "%zu");
    expect_eq(*mem3, (int32_t) 67, "%d");
    expect_eq(*(int32_t *) mem, *mem3, "%d");

    mp_sarena_deinit(&arena);

    return 0;

fail:
    exit(1);
}
