#include <stdio.h>
#include <string.h>
#define MEMPLUS_IMPLEMENTATION
#define __MP_HASH_TABLE_MAX_LOAD 1    // test when any conflict arises
#include "memplus.h"
#include "test.h"

mp_ht_create(int, HashTableInt);

int main(void) {
    mp_talloc_s(4096);

    mp_Alloc alloc = mp_heap_alloc();

    HashTableInt ht;
    mp_ht_init(HashTableInt, &ht, alloc);

    expect_eq(mp_ht_get(&ht, "foo"), NULL, "%p");

    // Simple set/get test
    mp_ht_set(&ht, "foo", 69);
    mp_ht_set(&ht, "bar", 420);

    int *val;

    expect(mp_ht_exists(&ht, "foo"));

    val = mp_ht_get(&ht, "foo");
    expect_eq(*val, 69, "%d");

    // Must not be NULL
    // val = mp_ht_get(&ht, NULL);

    val = mp_ht_get(&ht, "bar");
    expect_eq(*val, 420, "%d");

    mp_ht_set(&ht, "foo", 20);
    val = mp_ht_get(&ht, "foo");
    expect_eq(*val, 20, "%d");

    val = mp_ht_get(&ht, "nonexistent");
    expect_eq((void *) val, NULL, "%p");

    expect_eq(ht.len, (size_t) 2, "%zu");
    expect_eq(ht.cap, (size_t) __MP_HASH_TABLE_INIT_CAPACITY, "%zu");

    // Delete test
    mp_ht_delete(&ht, "bar");
    expect_eq(ht.len, (size_t) 1, "%zu");

    expect(!mp_ht_exists(&ht, "bar"));

    val = mp_ht_get(&ht, "bar");
    expect_eq((void *) val, NULL, "%p");

    mp_ht_set(&ht, "bar", 30);
    val = mp_ht_get(&ht, "bar");
    expect_eq(*val, 30, "%d");

    mp_ht_reset(&ht);

    // Realloc test
    for (int i = 0; i < __MP_HASH_TABLE_INIT_CAPACITY * __MP_HASH_TABLE_MAX_LOAD + 1; ++i) {
        mp_Str key = mp_str_newf(temp_alloc, "key_%d", i);
        mp_ht_set_s(&ht, key, i);
    }

    expect_eq(ht.len, (size_t) (__MP_HASH_TABLE_INIT_CAPACITY * __MP_HASH_TABLE_MAX_LOAD + 1),
              "%zu");
    expect_eq(ht.cap, (size_t) __MP_HASH_TABLE_INIT_CAPACITY * 2, "%zu");

    // for (size_t i = 0; i < ht.cap; ++i) {
    //     __HashTableIntEntry entry = ht.data[i];
    //     printf("%s ", (entry.key.cstr) ? entry.key.cstr : "[]");
    // }
    // printf("\n");

    for (int i = 0; i < (int) ht.len; ++i) {
        mp_Str key = mp_str_newf(temp_alloc, "key_%d", i);
        int   *val = mp_ht_get_s(&ht, key);
        expect_ne((void *) val, NULL, "%p");
        expect_eq(*val, i, "%d");
    }

    // Iterator test
    HashTableIntIter it;
    mp_ht_iter_init(&it, &ht);
    size_t counter = 0;
    while (mp_ht_iter_next(&it)) {
        ++counter;
    }
    expect_eq(counter, ht.len, "%zu");

    // Clone test
    HashTableInt ht2;
    mp_ht_clone(&ht2, &ht, alloc);
    expect_eq(ht.len, ht2.len, "%zu");
    expect_eq(ht.cap, ht2.cap, "%zu");
    expect_ne((void *) ht.data, (void *) ht2.data, "%p");

    mp_ht_iter_init(&it, &ht);
    while (mp_ht_iter_next(&it)) {
        __HashTableIntEntry *o = ht.data + (it._i - 1);
        expect_streq(it.key.cstr, o->key.cstr);
        expect_eq(it.val, o->val, "%d");
    }

    mp_ht_deinit(&ht2);

    mp_ht_deinit(&ht);

    return 0;

fail:
    exit(1);
}
