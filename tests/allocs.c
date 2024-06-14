#include "test.h"

void test(mp_Allocator alloc, size_t *size) {
    int32_t *test1, *test2;
    int64_t *test3;

    test1  = mp_allocator_create(alloc, int32_t);
    *test1 = 69;
    if (size) prnf("%zu", *size);
    expectf(*test1 == 69, "1(%d)", *test1);

    test2  = mp_allocator_dup(alloc, test1, sizeof(int32_t));
    *test2 = 420;
    if (size) prnf("%zu", *size);
    expectf(*test1 == 69 && *test2 == 420, "2(%d)\n1(%d) -> %d", *test2, *test1, *test2);

    test3  = mp_allocator_realloc(alloc, test2, sizeof(int32_t), sizeof(int64_t));
    *test3 = INT64_MAX;
    if (size) prnf("%zu", *size);
    expectf(*test3 == INT64_MAX, "3(%ld)", *test3);

    mp_allocator_free(alloc, test1);
    mp_allocator_free(alloc, test3);
}

int main(void) {
    int          result = 0;
    mp_Allocator alloc;

    /* GROWING ARENA ALLOCATOR */

    mp_Arena arena;
    mp_arena_init(&arena);
    alloc = mp_arena_allocator(&arena);
    test(alloc, &arena.size);

    /* STATIC ARENA ALLOCATOR */

    mp_SArena sarena;
    mp_sarena_init(&sarena, 256);
    alloc = mp_sarena_allocator(&sarena);
    test(alloc, &sarena.size);

    /* TEMP ALLOCATOR */

    uint8_t temp_buf[1024];
    mp_Temp temp_arena;
    mp_temp_init(&temp_arena, temp_buf);
    alloc = mp_temp_allocator(&temp_arena);
    test(alloc, &temp_arena.size);

    mp_temp_reset(&temp_arena);
    expects(temp_arena.buf[0] == 0, "mp_temp_reset failed");

    /* HEAP ALLOCATOR */

    alloc = mp_heap_allocator();
    test(alloc, NULL);

defer:
    mp_sarena_destroy(&sarena);
    mp_arena_destroy(&arena);
}
