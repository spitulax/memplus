#include <stdio.h>
#include <string.h>
#define MEMPLUS_IMPLEMENTATION
#define MP_HASH_TABLE_MAX_LOAD 1    // test when any conflict arises
#include "memplus.h"
#include "test.h"

mp_ht_create(int, HashTableInt);

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    HashTableInt ht;
    mp_ht_init(&ht, alloc);

    // Simple set/get test
    mp_ht_set(&ht, "foo", 69);
    mp_ht_set(&ht, "bar", 420);

    int *val;

    mp_ht_get(&ht, "foo", val);
    expect_eq(*val, 69, "%d");

    // Must not be NULL
    // mp_ht_get(&ht, NULL, val);

    mp_ht_get(&ht, "bar", val);
    expect_eq(*val, 420, "%d");

    mp_ht_set(&ht, "foo", 20);
    mp_ht_get(&ht, "foo", val);
    expect_eq(*val, 20, "%d");

    mp_ht_get(&ht, "nonexistent", val);
    expect_eq((void *) val, NULL, "%p");

    expect_eq(ht.len, (size_t) 2, "%zu");
    expect_eq(ht.cap, (size_t) MP_HASH_TABLE_INIT_CAPACITY, "%zu");

    // Delete test
    mp_ht_delete(&ht, "bar");

    mp_ht_get(&ht, "bar", val);
    expect_eq((void *) val, NULL, "%p");

    mp_ht_set(&ht, "bar", 30);
    mp_ht_get(&ht, "bar", val);
    expect_eq(*val, 30, "%d");

    mp_ht_reset(&ht);

    // Realloc test
    for (int i = 0; i < MP_HASH_TABLE_INIT_CAPACITY * MP_HASH_TABLE_MAX_LOAD + 1; ++i) {
        mp_Str key = mp_str_newf(alloc, "key_%d", i);
        mp_ht_set_s(&ht, &key, i);
        mp_str_deinit(&key, alloc);
    }

    expect_eq(ht.len, (size_t) (MP_HASH_TABLE_INIT_CAPACITY * MP_HASH_TABLE_MAX_LOAD + 1), "%zu");
    expect_eq(ht.cap, (size_t) MP_HASH_TABLE_INIT_CAPACITY * 2, "%zu");

    // for (size_t i = 0; i < ht.cap; ++i) {
    //     __HashTableIntEntry entry = ht.data[i];
    //     printf("%s ", (entry.key.cstr) ? entry.key.cstr : "[]");
    // }
    // printf("\n");

    for (int i = 0; i < (int) ht.len; ++i) {
        mp_Str key = mp_str_newf(alloc, "key_%d", i);
        int   *val;
        mp_ht_get_s(&ht, &key, val);
        expect_ne((void *) val, NULL, "%p");
        expect_eq(*val, i, "%d");
        mp_str_deinit(&key, alloc);
    }

    // Iterator test
    HashTableIntIter it;
    mp_ht_iter_init(&it, &ht);
    size_t counter = 0;
    while (it.ok) {
        ++counter;
        mp_ht_iter_next(&it);
    }
    expect_eq(counter, ht.len, "%zu");

    // Clone test
    HashTableInt ht2;
    mp_ht_clone(alloc, &ht, &ht2);
    expect_eq(ht.len, ht2.len, "%zu");
    expect_eq(ht.cap, ht2.cap, "%zu");
    expect_ne((void *) ht.data, (void *) ht2.data, "%p");

    mp_ht_iter_init(&it, &ht);
    while (it.ok) {
        __HashTableIntEntry *o = ht.data + (it._i - 1);
        expect_streq(it.key.cstr, o->key.cstr);
        expect_eq(it.val, o->val, "%d");
        mp_ht_iter_next(&it);
    }

    mp_ht_deinit(&ht2);

    mp_ht_deinit(&ht);

    return 0;

fail:
    exit(1);
}
