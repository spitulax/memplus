#include "test.h"
#include <inttypes.h>
#include <stdint.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

#define align(a) (__MP_DIV_ROUNDUP((a), sizeof(uintptr_t)) * sizeof(uintptr_t))

int main(void) {
    mp_Arena arena;
    mp_arena_init(&arena, mp_heap_alloc());
    mp_Alloc alloc = mp_arena_alloc(&arena);

    // First allocation
    mp_alloc(&alloc, 10);
    expect_eq((void *) arena.begin, (void *) arena.end, "%p");
    expect_ne((void *) arena.begin, NULL, "%p");
    expect_ne((void *) arena.end, NULL, "%p");
    expect_eq(arena.len, (size_t) align(10), "%zu");
    expect_eq(arena.begin->cap, (size_t) align(MP_REGION_DEFAULT_SIZE), "%zu");

    // New region allocation
    mp_alloc(&alloc, 64 * 1024);
    mp_Region *prev_end = arena.end;
    expect_ne((void *) arena.begin, (void *) arena.end, "%p");
    expect_eq((void *) arena.begin->next, (void *) arena.end, "%p");
    expect_eq(arena.len, (size_t) align(64 * 1024) + align(10), "%zu");
    expect_eq(arena.begin->len, (size_t) align(10), "%zu");
    expect_eq(arena.end->len, (size_t) align(64 * 1024), "%zu");

    // Reset arena and allocate again
    mp_arena_reset(&arena);
    mp_alloc(&alloc, 10);
    expect_eq((void *) arena.begin, (void *) arena.end, "%p");
    expect_ne((void *) arena.end->next, NULL, "%p");
    expect_eq(arena.len, (size_t) align(10), "%zu");
    expect_eq(arena.begin->len, (size_t) align(10), "%zu");

    // Reuse previous region
    mp_alloc(&alloc, 64 * 1024);
    expect_ne((void *) arena.begin, (void *) arena.end, "%p");
    expect_eq((void *) arena.begin->next, (void *) arena.end, "%p");
    expect_eq((void *) prev_end, (void *) arena.end, "%p");
    expect_eq(arena.len, (size_t) align(64 * 1024) + align(10), "%zu");
    expect_eq(arena.begin->len, (size_t) align(10), "%zu");
    expect_eq(arena.end->len, (size_t) align(64 * 1024), "%zu");

    // Realloc test
    mp_arena_reset(&arena);
    int32_t *mem  = mp_alloc(&alloc, 4);
    *mem          = 67;
    int64_t *mem2 = mp_realloc(&alloc, mem, 4, 8);
    expect_eq(arena.len, (size_t) align(4 + 8), "%zu");
    expect_eq(*mem2, (int64_t) 67, "%" PRId64);
    expect_eq(*(int64_t *) mem, *mem2, "%" PRId64);

    // Dup test
    mp_arena_reset(&arena);
    mem           = mp_alloc(&alloc, 4);
    *mem          = 67;
    int32_t *mem3 = mp_dup(&alloc, mem, 4);
    expect_eq(arena.len, (size_t) align(4) + align(4), "%zu");
    expect_eq(*mem3, 67, "%d");
    expect_eq(*(int32_t *) mem, *mem3, "%d");

    mp_arena_deinit(&arena);

    // Custom default size
    mp_arena_init_s(&arena, mp_heap_alloc(), 2048);
    alloc = mp_arena_alloc(&arena);

    // First allocation
    mp_alloc(&alloc, 10);
    expect_eq((void *) arena.begin, (void *) arena.end, "%p");
    expect_ne((void *) arena.begin, NULL, "%p");
    expect_ne((void *) arena.end, NULL, "%p");
    expect_eq(arena.len, (size_t) align(10), "%zu");
    expect_eq(arena.begin->cap, (size_t) align(2048), "%zu");

    mp_arena_deinit(&arena);

    return 0;

fail:
    exit(1);
}
