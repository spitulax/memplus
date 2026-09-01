#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    const size_t nums = 200;

    mp_talloc_s(4096);

    {
        mp_Str_Set str_set;
        mp_hs_init(&str_set, mp_heap());
        expect_eq(str_set.__ht_val_size, (size_t) 0, "%zu");

        for (size_t i = 0; i < nums; ++i) {
            mp_Sb key;
            mp_sb_withf(&key, temp_alloc, "key_%zu", i / 2);
            if (i % 2 == 0) {
                expect(!mp_ht_exists_s(&str_set, mp_sb_str(&key)));
            } else {
                expect(mp_ht_exists_s(&str_set, mp_sb_str(&key)));
            }
            mp_hs_set_s(&str_set, mp_sb_str(&key));
        }
        expect_eq(str_set.len, nums / 2, "%zu");

        mp_Str_Set_Iter str_set_it;
        mp_ht_iter_init(&str_set_it, &str_set);
        size_t count = 0;
        while (mp_ht_iter_next(&str_set_it)) {
            ++count;
            expect(mp_is_valid(str_set_it.key));
        }
        expect_eq(str_set.len, count, "%zu");

        mp_ht_deinit(&str_set);
    }

    {
        mp_Int_Set int_set;
        mp_hsi_init(&int_set, mp_heap());
        expect_eq(int_set.__hti_val_size, (size_t) 0, "%zu");

        for (size_t i = 0; i < nums; ++i) {
            size_t key = i / 2;
            if (i % 2 == 0) {
                expect(!mp_hti_exists(&int_set, key));
            } else {
                expect(mp_hti_exists(&int_set, key));
            }
            mp_hsi_set(&int_set, key);
        }
        expect_eq(int_set.len, nums / 2, "%zu");

        mp_Int_Set_Iter int_set_it;
        mp_hti_iter_init(&int_set_it, &int_set);
        size_t count = 0;
        while (mp_hti_iter_next(&int_set_it)) {
            ++count;
            expect(int_set_it.key.valid);
        }
        expect_eq(int_set.len, count, "%zu");

        mp_hti_deinit(&int_set);
    }

    return 0;

fail:
    exit(1);
}
