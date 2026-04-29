#define MEMPLUS_IMPLEMENTATION
#define MP_HASH_TABLE_MAX_LOAD 1    // test when any conflict arises
#include "memplus.h"
#include "test.h"

mp_hti_create(int, HashTableInt);

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    HashTableInt ht;
    mp_hti_init(&ht, alloc);

    // Simple set/get test
    mp_hti_set(&ht, 0, 69);
    mp_hti_set(&ht, 1, 420);

    int *val;

    mp_hti_get(&ht, 0, val);
    expect_eq(*val, 69, "%d");

    mp_hti_get(&ht, 1, val);
    expect_eq(*val, 420, "%d");

    mp_hti_set(&ht, 0, 20);
    mp_hti_get(&ht, 0, val);
    expect_eq(*val, 20, "%d");

    mp_hti_get(&ht, 69, val);
    expect_eq((void *) val, NULL, "%p");

    expect_eq(ht.len, (size_t) 2, "%zu");
    expect_eq(ht.cap, (size_t) MP_HASH_TABLE_INIT_CAPACITY, "%zu");

    // Delete test
    mp_hti_delete(&ht, 1);

    mp_hti_get(&ht, 1, val);
    expect_eq((void *) val, NULL, "%p");

    mp_hti_set(&ht, 1, 30);
    mp_hti_get(&ht, 1, val);
    expect_eq(*val, 30, "%d");

    mp_hti_reset(&ht);

    // Realloc test
    for (int i = 0; i < MP_HASH_TABLE_INIT_CAPACITY * MP_HASH_TABLE_MAX_LOAD + 1; ++i) {
        mp_hti_set(&ht, (size_t) i * 2, i);
    }

    expect_eq(ht.len, (size_t) (MP_HASH_TABLE_INIT_CAPACITY * MP_HASH_TABLE_MAX_LOAD + 1), "%zu");
    expect_eq(ht.cap, (size_t) MP_HASH_TABLE_INIT_CAPACITY * 2, "%zu");

    // for (size_t i = 0; i < ht.cap; ++i) {
    //     __HashTableIntEntry entry = ht.data[i];
    //     mp_Str              key   = mp_str_newf(alloc, "%zu", entry.key.key);
    //     printf("%s ", (entry.key.valid) ? key.cstr : "[]");
    //     mp_str_deinit(&key, alloc);
    // }
    // printf("\n");

    for (int i = 0; i < (int) ht.len; ++i) {
        int *val;
        mp_hti_get(&ht, (size_t) i * 2, val);
        expect_ne((void *) val, NULL, "%p");
        expect_eq(*val, i, "%d");
    }

    // Iterator test
    HashTableIntIter it;
    mp_hti_iter_init(&it, &ht);
    size_t counter = 0;
    while (it.ok) {
        ++counter;
        mp_hti_iter_next(&it);
    }
    expect_eq(counter, ht.len, "%zu");

    // Clone test
    HashTableInt ht2;
    mp_hti_clone(alloc, &ht, &ht2);
    expect_eq(ht.len, ht2.len, "%zu");
    expect_eq(ht.cap, ht2.cap, "%zu");
    expect_ne((void *) ht.data, (void *) ht2.data, "%p");

    mp_hti_iter_init(&it, &ht);
    while (it.ok) {
        __HashTableIntEntry *o = ht.data + (it._i - 1);
        expect_eq(it.key.key, o->key.key, "%zu");
        expect_eq(it.val, o->val, "%d");
        mp_hti_iter_next(&it);
    }

    mp_hti_deinit(&ht2);

    mp_hti_deinit(&ht);

    return 0;

fail:
    exit(1);
}
