#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MEMPLUS_IMPLEMENTATION
#define __MP_HASH_TABLE_MAX_LOAD 1    // test when any conflict arises
#include "memplus.h"
#include "test.h"

mp_ht_typedef(int, Ht_Int);
mp_da_typedef(int, Da_Int);

int main(void) {
    mp_talloc_s(4096);

    mp_Alloc alloc = mp_heap_alloc();

    Ht_Int ht;
    mp_ht_init(Ht_Int, &ht, alloc);
    expect_eq(ht.__ht_val_size, sizeof(int), "%zu");
    expect_eq(ht.__da_item_size, sizeof(__Ht_Int_Entry), "%zu");

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
        mp_Sb key;
        mp_sb_init_withf(&key, temp_alloc, "key_%d", i);
        mp_ht_set_s(&ht, mp_sb_str(&key), i);
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
        mp_Sb key;
        mp_sb_init_withf(&key, temp_alloc, "key_%d", i);
        val = mp_ht_get_s(&ht, mp_sb_str(&key));
        expect_ne((void *) val, NULL, "%p");
        expect_eq(*val, i, "%d");
    }

    // Iterator test
    Ht_Int_Iter it;
    mp_ht_iter_init(&it, &ht);
    size_t counter = 0;
    while (mp_ht_iter_next(&it)) {
        ++counter;
    }
    expect_eq(counter, ht.len, "%zu");

    // Extract test
    mp_Ht_Keys keys;
    mp_da_init(mp_Ht_Keys, &keys, alloc);
    mp_ht_keys(&ht, &keys);
    expect_eq(keys.len, ht.len, "%zu");
    expect_eq(keys.cap, ht.len, "%zu");

    Da_Int vals;
    mp_da_init(Da_Int, &vals, alloc);
    mp_ht_values(&ht, &vals);
    expect_eq(vals.len, ht.len, "%zu");
    expect_eq(vals.cap, ht.len, "%zu");

    for (size_t i = 0; i < keys.len; ++i) {
        mp_String key = mp_get(&keys, i);
        // TODO: use `mp_str_substr`
        mp_String string_key   = mp_string_from(mp_str_s(key.data + 4, key.len - 4), temp_alloc);
        int       val_from_key = atoi(string_key.data);
        expect_eq(val_from_key, mp_get(&vals, i), "%d");
    }

    mp_da_deinit(&vals);
    mp_ht_keys_deinit(&keys);

    // Clone test
    Ht_Int ht2;
    mp_ht_clone(&ht2, &ht, alloc);
    expect_eq(ht.len, ht2.len, "%zu");
    expect_eq(ht.cap, ht2.cap, "%zu");
    expect_ne((void *) ht.data, (void *) ht2.data, "%p");

    mp_ht_iter_init(&it, &ht);
    while (mp_ht_iter_next(&it)) {
        __Ht_Int_Entry *o = ht.data + (it.__ht_it_i - 1);
        expect_streq_mp(it.key, mp_str_v(o->key));
        expect_eq(*it.val, o->val, "%d");
    }

    mp_ht_deinit(&ht2);

    mp_ht_deinit(&ht);

    return 0;

fail:
    exit(1);
}
