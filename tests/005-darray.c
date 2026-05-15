#include "test.h"

#include <stdio.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

mp_da_create(int32_t, ArrayInt32);

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    ArrayInt32 array;
    mp_da_init(ArrayInt32, &array, alloc);

    // Append test
    mp_da_append(&array, 0);
    expect_eq(array.len, (size_t) 1, "%zu");
    expect_eq(array.cap, (size_t) __MP_DARRAY_INIT_CAPACITY, "%zu");
    expect_eq(mp_da_get(&array, 0), (int32_t) 0, "%d");

    // Realloc test
    for (int32_t i = 1; i < __MP_DARRAY_INIT_CAPACITY + 1; ++i) {
        mp_da_append(&array, i);
    }
    expect_eq(array.len, (size_t) __MP_DARRAY_INIT_CAPACITY + 1, "%zu");
    expect_eq(array.cap, (size_t) __MP_DARRAY_INIT_CAPACITY * 2, "%zu");

    // Pop test
    expect_eq(mp_da_pop(&array), (int32_t) __MP_DARRAY_INIT_CAPACITY, "%d");
    expect_eq(array.len, (size_t) __MP_DARRAY_INIT_CAPACITY, "%zu");

    // Grow/Shrink test
    array.len -= 50;
    expect_eq(array.len, (size_t) __MP_DARRAY_INIT_CAPACITY - 50, "%zu");
    mp_da_grow(&array, __MP_DARRAY_INIT_CAPACITY * 2);
    array.len += __MP_DARRAY_INIT_CAPACITY * 2;
    expect_eq(array.len, (size_t) __MP_DARRAY_INIT_CAPACITY * 3 - 50, "%zu");
    expect_eq(array.cap, (size_t) __MP_DARRAY_INIT_CAPACITY * 2 * 2, "%zu");

    // Reset test
    mp_da_reset(&array);
    mp_da_append(&array, 0);
    expect_eq(array.len, (size_t) 1, "%zu");
    expect_eq(mp_da_get(&array, 0), (int32_t) 0, "%d");
    expect_eq(array.cap, (size_t) __MP_DARRAY_INIT_CAPACITY * 2 * 2, "%zu");

    // Clone test
    ArrayInt32 array2;
    mp_da_clone(&array2, &array, alloc);
    mp_da_deinit(&array);

    expect_eq(array2.len, (size_t) 1, "%zu");
    expect_eq(mp_da_get(&array2, 0), (int32_t) 0, "%d");
    expect_eq(array2.cap, (size_t) __MP_DARRAY_INIT_CAPACITY + 1, "%zu");

    // Insert test
    mp_da_insert(&array2, 0, 67);
    mp_da_insert(&array2, 1, 69);
    expect_eq(array2.len, (size_t) 3, "%zu");
    expect_eq(mp_da_get(&array2, 0), (int32_t) 67, "%d");
    expect_eq(mp_da_get(&array2, 1), (int32_t) 69, "%d");
    expect_eq(mp_da_get(&array2, 2), (int32_t) 0, "%d");

    // Delete test
    mp_da_delete(&array2, 0);
    expect_eq(array2.len, (size_t) 2, "%zu");
    expect_eq(mp_da_get(&array2, 0), (int32_t) 69, "%d");
    expect_eq(mp_da_get(&array2, 1), (int32_t) 0, "%d");

    mp_da_quick_delete(&array2, 0);
    expect_eq(array2.len, (size_t) 1, "%zu");
    expect_eq(mp_da_get(&array2, 0), (int32_t) 0, "%d");

    mp_da_deinit(&array2);

    // Append many test
    ArrayInt32 array3;
    mp_da_init(ArrayInt32, &array3, alloc);
    mp_da_append_many(&array3, 69, 420, 67, 13, 37);
    mp_da_insert_many(&array3, 2, 10, 20);
    expect_eq(array3.len, (size_t) 7, "%zu");

    expect_eq(mp_da_get(&array3, 0), (int32_t) 69, "%d");
    expect_eq(mp_da_get(&array3, 1), (int32_t) 420, "%d");
    expect_eq(mp_da_get(&array3, 2), (int32_t) 10, "%d");
    expect_eq(mp_da_get(&array3, 3), (int32_t) 20, "%d");
    expect_eq(mp_da_get(&array3, 4), (int32_t) 67, "%d");
    expect_eq(mp_da_get(&array3, 5), (int32_t) 13, "%d");
    expect_eq(mp_da_get(&array3, 6), (int32_t) 37, "%d");

    mp_da_reset(&array3);
    int32_t append[] = { 10, 11, 12 };
    mp_da_append_array(&array3, append, 3);
    mp_da_insert_array(&array3, 2, append, 3);
    expect_eq(array3.len, (size_t) 6, "%zu");
    expect_eq(mp_da_get(&array3, 0), (int32_t) 10, "%d");
    expect_eq(mp_da_get(&array3, 1), (int32_t) 11, "%d");
    expect_eq(mp_da_get(&array3, 2), (int32_t) 10, "%d");
    expect_eq(mp_da_get(&array3, 3), (int32_t) 11, "%d");
    expect_eq(mp_da_get(&array3, 4), (int32_t) 12, "%d");
    expect_eq(mp_da_get(&array3, 5), (int32_t) 12, "%d");

    mp_da_deinit(&array3);

    // Quick initialization
    ArrayInt32 array4;
    mp_da_init_with(ArrayInt32, &array4, alloc, 1, 2, 3, 4, 5);
    expect_eq(array4.len, (size_t) 5, "%zu");
    expect_eq(mp_da_get(&array4, 0), (int32_t) 1, "%d");
    expect_eq(mp_da_get(&array4, 1), (int32_t) 2, "%d");
    expect_eq(mp_da_get(&array4, 2), (int32_t) 3, "%d");
    expect_eq(mp_da_get(&array4, 3), (int32_t) 4, "%d");
    expect_eq(mp_da_get(&array4, 4), (int32_t) 5, "%d");

    return 0;

fail:
    exit(1);
}
