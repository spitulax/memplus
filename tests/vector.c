#include "test.h"

mp_vector_create(Vector_Int, int);

void print_vector(Vector_Int *vector) {
    printf("{");
    for (size_t i = 0; i < (vector)->len; ++i) {
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
    expectf(
        vec1.len == 10 && vec1.cap == MP_VECTOR_INIT_CAPACITY, "1(%zu;%zu)", vec1.len, vec1.cap);
    printf("mp_append: ");
    print_vector(&vec1);

    size_t cap = vec1.cap;
    mp_reserve(&vec1, 100);
    expectf(vec1.cap == 100, "1(;%zu) -> 1(;%zu)", cap, vec1.cap);

    size_t len           = vec1.len;
    int    many_ints[]   = { 69, 420, 13, 37, 42 };
    int    many_ints_len = sizeof(many_ints) / sizeof(many_ints[0]);
    mp_append_many(&vec1, many_ints, many_ints_len);
    printf("mp_append_many: ");
    print_vector(&vec1);
    expectf(vec1.len == len + many_ints_len, "1(%zu;) -> 1(%zu;)", len, vec1.len);

    len      = vec1.len;
    int last = mp_pop(&vec1);
    expectf(last == 42 && vec1.len == len - 1, "%d : 1(%zu;) -> 1(%zu;)", last, len, vec1.len);

    Vector_Int vec2;
    mp_clone(&vec1, &vec2, &alloc);

    len = vec1.len;
    mp_clear(&vec1);
    expectf(vec1.len == 0 && vec1.cap == 100, "1(%zu;%zu)", vec1.len, vec1.cap);
    expectf(vec2.len == len, "1(%zu;%zu) 2(%zu;%zu)", len, vec1.cap, vec2.len, vec2.cap);

    printf("mp_clear: ");
    print_vector(&vec1);
    printf("mp_pop -> mp_clone: ");
    print_vector(&vec2);

    len = vec2.len;
    mp_insert(&vec2, 3, 777);
    printf("mp_insert 3 777: ");
    print_vector(&vec2);
    expectf(vec2.len == len + 1, "2(%zu;) -> 2(%zu;)", len, vec2.len);

    len = vec2.len;
    mp_insert_many(&vec2, 4, many_ints, many_ints_len);
    printf("mp_insert_many many_ints: ");
    print_vector(&vec2);
    expectf(vec2.len == len + many_ints_len, "2(%zu;) -> 2(%zu;)", len, vec2.len);

    len        = vec2.len;
    int erased = mp_erase_ret(&vec2, 4);
    printf("mp_erase: ");
    print_vector(&vec2);
    expectf(erased == 69 && vec2.len == len - 1,
            "erased: %d, expected: 69 : 2(%zu;) -> 2(%zu;)",
            erased,
            len,
            vec2.len);

    len = vec2.len;
    mp_erase_many(&vec2, 0, 3);
    printf("mp_erase_many 0 3: ");
    print_vector(&vec2);
    expectf(vec2.len == len - 3, "2(%zu;) -> 2(%zu;)", len, vec2.len);

    len = vec2.len;
    int erased_ints[5];
    int erased_ints_len = sizeof(erased_ints) / sizeof(erased_ints[0]);
    mp_erase_many_to_buf(&vec2, 5, erased_ints, erased_ints_len);
    mp_append_many(&vec1, erased_ints, erased_ints_len);
    printf("mp_erase_to_buf -> mp_insert_many: ");
    print_vector(&vec1);
    expectf(vec2.len == len - erased_ints_len && vec1.len == erased_ints_len,
            "2(%zu;) -> 2(%zu;)\n1(0;) -> 1(%zu;)",
            len,
            vec2.len,
            vec1.len);

    len    = vec1.len;
    erased = mp_unordered_erase_ret(&vec1, 0);
    printf("mp_unordered_erase 0: ");
    print_vector(&vec1);
    expectf(erased == 3 && vec1.len == len - 1,
            "erased: %d, expected: 69 : 1(%zu;) -> 1(%zu;)",
            erased,
            len,
            vec1.len);

    Vector_Int vec3;
    mp_vector_init(&vec3, &alloc);
    for (size_t i = 0; i < MP_VECTOR_INIT_CAPACITY + 1; ++i) {
        mp_append(&vec3, 69);
    }
    expectf(vec3.cap == MP_VECTOR_INIT_CAPACITY * 2 && vec3.len == MP_VECTOR_INIT_CAPACITY + 1,
            "3(%zu;%zu)",
            vec3.len,
            vec3.cap);

    mp_arena_destroy(&arena);
}
