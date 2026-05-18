#define MEMPLUS_IMPLEMENTATION
#define __MP_HASH_TABLE_MAX_LOAD 1    // test when any conflict arises
#include "memplus.h"
#include "test.h"

mp_hti_typedef(int, Ht_Int);
mp_da_typedef(int, Da_Int);

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    Ht_Int ht;
    mp_hti_init(Ht_Int, &ht, alloc);
    expect_eq(ht.val_size, sizeof(int), "%zu");
    expect_eq(ht.size, sizeof(__Ht_Int_Entry), "%zu");

    expect_eq(mp_hti_get(&ht, 0), NULL, "%p");

    // Simple set/get test
    mp_hti_set(&ht, 0, 69);
    mp_hti_set(&ht, 1, 420);

    int *val;

    expect(mp_hti_exists(&ht, 0));

    val = mp_hti_get(&ht, 0);
    expect_eq(*val, 69, "%d");

    val = mp_hti_get(&ht, 1);
    expect_eq(*val, 420, "%d");

    mp_hti_set(&ht, 0, 20);
    val = mp_hti_get(&ht, 0);
    expect_eq(*val, 20, "%d");

    val = mp_hti_get(&ht, 69);
    expect_eq((void *) val, NULL, "%p");

    expect_eq(ht.len, (size_t) 2, "%zu");
    expect_eq(ht.cap, (size_t) __MP_HASH_TABLE_INIT_CAPACITY, "%zu");

    // Delete test
    mp_hti_delete(&ht, 1);
    expect_eq(ht.len, (size_t) 1, "%zu");

    expect(!mp_hti_exists(&ht, 1));

    val = mp_hti_get(&ht, 1);
    expect_eq((void *) val, NULL, "%p");

    mp_hti_set(&ht, 1, 30);
    val = mp_hti_get(&ht, 1);
    expect_eq(*val, 30, "%d");

    mp_hti_reset(&ht);

    // Realloc test
    for (int i = 0; i < __MP_HASH_TABLE_INIT_CAPACITY * __MP_HASH_TABLE_MAX_LOAD + 1; ++i) {
        mp_hti_set(&ht, (size_t) i * 2, i);
    }

    expect_eq(ht.len, (size_t) (__MP_HASH_TABLE_INIT_CAPACITY * __MP_HASH_TABLE_MAX_LOAD + 1),
              "%zu");
    expect_eq(ht.cap, (size_t) __MP_HASH_TABLE_INIT_CAPACITY * 2, "%zu");

    // for (size_t i = 0; i < ht.cap; ++i) {
    //     __Ht_IntEntry entry = ht.data[i];
    //     mp_Str              key   = mp_str_newf(alloc, "%zu", entry.key.key);
    //     printf("%s ", (entry.key.valid) ? key.cstr : "[]");
    //     mp_str_deinit(&key, alloc);
    // }
    // printf("\n");

    for (int i = 0; i < (int) ht.len; ++i) {
        val = mp_hti_get(&ht, (size_t) i * 2);
        expect_ne((void *) val, NULL, "%p");
        expect_eq(*val, i, "%d");
    }

    // Iterator test
    Ht_Int_Iter it;
    mp_hti_iter_init(&it, &ht);
    size_t counter = 0;
    while (mp_hti_iter_next(&it)) {
        ++counter;
    }
    expect_eq(counter, ht.len, "%zu");

    // Extract test
    mp_Hti_Keys keys;
    mp_da_init(mp_Hti_Keys, &keys, alloc);
    mp_hti_keys(&ht, &keys);
    expect_eq(keys.len, ht.len, "%zu");
    expect_eq(keys.cap, ht.len, "%zu");

    Da_Int vals;
    mp_da_init(Da_Int, &vals, alloc);
    mp_hti_values(&ht, &vals);
    expect_eq(vals.len, ht.len, "%zu");
    expect_eq(vals.cap, ht.len, "%zu");

    for (size_t i = 0; i < keys.len; ++i) {
        expect_eq(mp_get(&keys, i), (size_t) mp_get(&vals, i) * 2, "%zu");
    }

    mp_da_deinit(&vals);
    mp_da_deinit(&keys);

    // Clone test
    Ht_Int ht2;
    mp_hti_clone(&ht2, &ht, alloc);
    expect_eq(ht.len, ht2.len, "%zu");
    expect_eq(ht.cap, ht2.cap, "%zu");
    expect_ne((void *) ht.data, (void *) ht2.data, "%p");

    mp_hti_iter_init(&it, &ht);
    while (mp_hti_iter_next(&it)) {
        __Ht_Int_Entry *o = ht.data + (it._i - 1);
        expect_eq(it.key.key, o->key.key, "%zu");
        expect_eq(*it.val, o->val, "%d");
        mp_hti_iter_next(&it);
    }

    mp_hti_deinit(&ht2);

    mp_hti_deinit(&ht);

    return 0;

fail:
    exit(1);
}
