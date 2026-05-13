#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    {
        mp_Arena arena;
        mp_arena_init(&arena, mp_heap());
        mp_Alloc alloc = mp_arena_alloc(&arena);

        int *i1     = mp_create(alloc, int);
        *i1         = 1;
        size_t mark = mp_arena_mark(&arena);
        int   *i2   = mp_create(alloc, int);
        *i2         = 2;
        mp_arena_rewind(&arena, mark);
        int *i3 = mp_create(alloc, int);
        *i3     = 3;

        expect_eq(*i1, 1, "%d");
        expect_eq(*i3, 3, "%d");
        expect(*i2 == *i3);

        mp_alloc(alloc, __MP_REGION_DEFAULT_SIZE);
        mp_Region *prev_end = arena.end;
        expect_ne((void *) arena.begin, (void *) arena.end, "%p");
        expect_eq((void *) arena.begin->next, (void *) arena.end, "%p");

        mp_arena_rewind(&arena, mark);
        expect_eq((void *) arena.begin, (void *) arena.end, "%p");
        expect_eq((void *) arena.begin->next, (void *) prev_end, "%p");
        expect_eq(*((char *) arena.end->data + arena.end->len), *i3, "%d");

        mp_alloc(alloc, __MP_REGION_DEFAULT_SIZE);
        expect_ne((void *) arena.end, (void *) prev_end, "%p");
    }

    {
        mp_SArena arena;
        mp_sarena_init(&arena, mp_heap(), 1024);
        mp_Alloc alloc = mp_sarena_alloc(&arena);

        int *i1     = mp_create(alloc, int);
        *i1         = 1;
        size_t mark = mp_sarena_mark(&arena);
        int   *i2   = mp_create(alloc, int);
        *i2         = 2;
        mp_sarena_rewind(&arena, mark);
        int *i3 = mp_create(alloc, int);
        *i3     = 3;

        expect_eq(*i1, 1, "%d");
        expect_eq(*i3, 3, "%d");
        expect(*i2 == *i3);
    }

    return 0;

fail:
    exit(1);
}
