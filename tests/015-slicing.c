#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

#include <string.h>

bool test_array(const int *array, size_t array_len, const int *expected, size_t expected_len) {
    if (array_len != expected_len) {
        goto fail;
    }
    if (memcmp(array, expected, array_len * sizeof(*array)) != 0) {
        goto fail;
    }
    return true;

fail:
    elogs("Expected `a == b`, got");
    elogsn("\ta = ");
    for (size_t __i = 0; __i < (array_len); ++__i) {
        if (__i > 0) {
            elogsn(" ");
        }
        elogfn("`%d`", (array)[__i]);
    }
    elogsn("\n");
    elogsn("\tb = ");
    for (size_t __i = 0; __i < (expected_len); ++__i) {
        if (__i > 0) {
            elogsn(" ");
        }
        elogfn("`%d`", (expected)[__i]);
    }
    elogsn("\n");
    return false;
}

mp_da_typedef(int, DaInt);

int main(void) {
    mp_talloc();

    DaInt d_arr;
    mp_da_with(DaInt, &d_arr, talloc, 0, 1, 2, 3, 4);
    int c_arr[] = { 0, 1, 2, 3, 4 };

    expect_da_arr_eq(d_arr, c_arr, "%d");
    expect_eq(mp_lenc(c_arr), (size_t) 5, "%zu");

    expect(test_array(mp_arr(&d_arr), (int[]) { 0, 1, 2, 3, 4 }, 5));
    expect(test_array(mp_arrc(c_arr), (int[]) { 0, 1, 2, 3, 4 }, 5));

    expect(test_array(mp_slice(&d_arr, 1, 4), (int[]) { 1, 2, 3 }, 3));
    expect(test_array(mp_slicec(c_arr, 1, 4), (int[]) { 1, 2, 3 }, 3));

    expect(test_array(mp_slicee(&d_arr, 4), (int[]) { 0, 1, 2, 3 }, 4));
    expect(test_array(mp_sliceec(c_arr, 4), (int[]) { 0, 1, 2, 3 }, 4));

    expect(test_array(mp_sliceb(&d_arr, 2), (int[]) { 2, 3, 4 }, 3));
    expect(test_array(mp_slicec(c_arr, 2, mp_lenc(c_arr)), (int[]) { 2, 3, 4 }, 3));

    return 0;

fail:
    exit(1);
}
