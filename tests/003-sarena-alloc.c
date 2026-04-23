#include "test.h"
#include <stdint.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

#define align(a)      (DIV_ROUNDUP((a), sizeof(uintptr_t)) * sizeof(uintptr_t))
#define align_down(a) (((a) / sizeof(uintptr_t)) * sizeof(uintptr_t))

void test(mp_Alloc *alloc, mp_Temp *arena) {
    // First allocation
    mp_alloc(alloc, 10);
    expect_eq(arena->len, (size_t) align(10), "%zu");

    // Too big for this
    void *failed = mp_alloc(alloc, 64 * 1024);
    expect_eq(failed, NULL, "%p");
    expect_eq(arena->len, (size_t) align(10), "%zu");

    // Reset arena and allocate again
    mp_temp_reset(arena);
    mp_alloc(alloc, 10);
    expect_eq(arena->len, (size_t) align(10), "%zu");

    // Realloc test
    mp_temp_reset(arena);
    int32_t *mem  = mp_alloc(alloc, 4);
    *mem          = 67;
    int64_t *mem2 = mp_realloc(alloc, mem, 4, 8);
    expect_eq(arena->len, (size_t) align(4 + 8), "%zu");
    expect_eq(*mem2, 67L, "%ld");
    expect_eq(*(int64_t *) mem, *mem2, "%ld");

    // Dup test
    mp_temp_reset(arena);
    mem           = mp_alloc(alloc, 4);
    *mem          = 67;
    int32_t *mem3 = mp_dup(alloc, mem, 4);
    expect_eq(arena->len, (size_t) align(4) + align(4), "%zu");
    expect_eq(*mem3, 67, "%d");
    expect_eq(*(int32_t *) mem, *mem3, "%d");

    return;

fail:
    exit(1);
}

int main(void) {
    mp_Arena parent_arena;
    mp_arena_init(&parent_arena);
    mp_Alloc parent_alloc = mp_arena_alloc(&parent_arena);

    mp_SArena s_arena;
    mp_sarena_init(&s_arena, &parent_alloc, MP_REGION_DEFAULT_SIZE);
    mp_Alloc s_alloc = mp_sarena_alloc(&s_arena);
    expect_eq(s_arena.cap, (size_t) align(MP_REGION_DEFAULT_SIZE), "%zu");
    test(&s_alloc, (mp_Temp *) &s_arena);
    mp_sarena_deinit(&s_arena);
    mp_arena_deinit(&parent_arena);

    mp_Temp t_arena;
    char    buf[1050];
    mp_temp_init(&t_arena, buf, sizeof(buf));
    mp_Alloc t_alloc = mp_temp_alloc(&t_arena);
    expect_eq(t_arena.cap, (size_t) align_down(1050), "%zu");
    test(&t_alloc, &t_arena);

    return 0;

fail:
    exit(1);
}
