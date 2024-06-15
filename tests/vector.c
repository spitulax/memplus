#include "test.h"

mp_vector_create(Vector_Int, int);

void print_vector(Vector_Int *vector) {
    printf("{");
    for (size_t i = 0; i < (vector)->size; ++i) {
        if (i > 0) printf(", ");
        printf("%d", mp_get(vector, i));
    }
    printf("}\n");
}

int main(void) {
    mp_Arena arena;
    mp_arena_init(&arena);
    mp_Allocator alloc = mp_arena_allocator(&arena);

    Vector_Int vec1;
    mp_vector_init(&vec1, &alloc);
    for (int i = 0; i < 10; ++i)
        mp_append(&vec1, i);
    expectf(vec1.size == 10 && vec1.capacity == MP_VECTOR_INIT_CAPACITY,
            "1(%zu;%zu)",
            vec1.size,
            vec1.capacity);
    printf("mp_append: ");
    print_vector(&vec1);

    size_t cap = vec1.capacity;
    mp_reserve(&vec1, 100);
    expectf(vec1.capacity == 100, "1(;%zu) -> 1(;%zu)", cap, vec1.capacity);

    size_t size          = vec1.size;
    int    many_ints[]   = { 69, 420, 13, 37, 42 };
    int    many_ints_len = sizeof(many_ints) / sizeof(many_ints[0]);
    mp_append_many(&vec1, many_ints, many_ints_len);
    printf("mp_append_many: ");
    print_vector(&vec1);
    expectf(vec1.size == size + many_ints_len, "1(%zu;) -> 1(%zu;)", size, vec1.size);

    size     = vec1.size;
    int last = mp_pop(&vec1);
    expectf(last == 42 && vec1.size == size - 1, "%d : 1(%zu;) -> 1(%zu;)", last, size, vec1.size);

    Vector_Int vec2;
    mp_clone(&vec1, &vec2, &alloc);

    size = vec1.size;
    mp_clear(&vec1);
    expectf(vec1.size == 0 && vec1.capacity == 100, "1(%zu;%zu)", vec1.size, vec1.capacity);
    expectf(
        vec2.size == size, "1(%zu;%zu) 2(%zu;%zu)", size, vec1.capacity, vec2.size, vec2.capacity);

    printf("mp_clear: ");
    print_vector(&vec1);
    printf("mp_pop -> mp_clone: ");
    print_vector(&vec2);

    size = vec2.size;
    mp_insert(&vec2, 3, 777);
    printf("mp_insert 3 777: ");
    print_vector(&vec2);
    expectf(vec2.size == size + 1, "2(%zu;) -> 2(%zu;)", size, vec2.size);

    size = vec2.size;
    mp_insert_many(&vec2, 4, many_ints, many_ints_len);
    printf("mp_insert_many many_ints: ");
    print_vector(&vec2);
    expectf(vec2.size == size + many_ints_len, "2(%zu;) -> 2(%zu;)", size, vec2.size);

    size       = vec2.size;
    int erased = mp_erase_ret(&vec2, 4);
    printf("mp_erase: ");
    print_vector(&vec2);
    expectf(erased == 69 && vec2.size == size - 1,
            "erased: %d, expected: 69 : 2(%zu;) -> 2(%zu;)",
            erased,
            size,
            vec2.size);

    size = vec2.size;
    mp_erase_many(&vec2, 0, 3);
    printf("mp_erase_many 0 3: ");
    print_vector(&vec2);
    expectf(vec2.size == size - 3, "2(%zu;) -> 2(%zu;)", size, vec2.size);

    size = vec2.size;
    int erased_ints[5];
    int erased_ints_len = sizeof(erased_ints) / sizeof(erased_ints[0]);
    mp_erase_many_to_buf(&vec2, 5, erased_ints, erased_ints_len);
    mp_append_many(&vec1, erased_ints, erased_ints_len);
    printf("mp_erase_to_buf -> mp_insert_many: ");
    print_vector(&vec1);
    expectf(vec2.size == size - erased_ints_len && vec1.size == erased_ints_len,
            "2(%zu;) -> 2(%zu;)\n1(0;) -> 1(%zu;)",
            size,
            vec2.size,
            vec1.size);

    size   = vec1.size;
    erased = mp_unordered_erase_ret(&vec1, 0);
    printf("mp_unordered_erase 0: ");
    print_vector(&vec1);
    expectf(erased == 3 && vec1.size == size - 1,
            "erased: %d, expected: 69 : 1(%zu;) -> 1(%zu;)",
            erased,
            size,
            vec1.size);

    mp_arena_destroy(&arena);
}
