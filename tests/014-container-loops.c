#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
// #define QUIET
#include "test.h"

mp_da_typedef(int, DaInt);
mp_ht_typedef(int, HtInt);
mp_hti_typedef(int, HtiInt);

int main(void) {
    {
        mp_talloc();

        DaInt arr;
        mp_da_with(DaInt, &arr, talloc, 0, 1, 1, 2, 3, 5);

        MP_DA_BEGIN(&arr, i, num)
            expect_eq(mp_get(&arr, i), *num, "%d");
            logfn("%d ", *num);

            MP_DA_BEGIN(&arr, j, num2)
                expect_eq(mp_get(&arr, j), *num2, "%d");
                logfn("%d", *num2);
            MP_END()

            logfn("\n", NULL);
        MP_END()

        logfn("----------\n", NULL);
    }

    {
        mp_talloc();

        HtInt ht;
        mp_ht_init(HtInt, &ht, talloc);
        mp_hset(&ht, "foo", 0);
        mp_hset(&ht, "bar", 1);
        mp_hset(&ht, "baz", 2);

        size_t i = 0;
        MP_HT_BEGIN(HtInt, &ht, ht_iter, k, v)
            expect_eq(*(int *) mp_hgets(&ht, k), *v, "%d");
            logf("%.*s: %d", mp_strp(k), *v);
            ++i;
        MP_END()
        expect_eq(i, ht.len, "%zu");

        logfn("----------\n", NULL);
    }

    {
        mp_talloc();

        mp_Str_Set hs;
        mp_hs_init(&hs, talloc);
        mp_hsset(&hs, "foo");
        mp_hsset(&hs, "bar");
        mp_hsset(&hs, "baz");

        size_t i = 0;
        MP_HS_BEGIN(&hs, hs_iter, k)
            logf("%.*s", mp_strp(k));
            ++i;
        MP_END()
        expect_eq(i, hs.len, "%zu");

        logfn("----------\n", NULL);
    }

    return 0;

fail:
    exit(1);
}
