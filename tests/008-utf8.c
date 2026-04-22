#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    const char *s1 = "魈くんは大好きです　⸜(｡˃ ᵕ ˂ )⸝♡􏾀";
    expect_eq(strlen(s1), 58ul, "%lu");
    expect_eq(mp_utf8_len(s1), 23ul, "%zu");

    // `110` prefix indicates a byte will follow, but nothing follows
    const char s2[] = { (char) 0xC0 /*11000000*/ };
    expect_eq(mp_utf8_len_s(s2, 1), MP_ERROR, "%zu");

    // following bytes must have `10` prefix, but `0` is given
    const char s3[] = { (char) 0xC0 /*11000000*/, (char) 0x0 };
    expect_eq(mp_utf8_len_s(s3, 2), MP_ERROR, "%zu");

    // `10` prefix must follow a "head" byte, but nothing precedes
    const char s4[] = { (char) 0x80 /*10000000*/ };
    expect_eq(mp_utf8_len_s(s4, 1), MP_ERROR, "%zu");

    mp_Utf8Iter iter  = mp_utf8_iter_new(s1);
    size_t      count = 0;
    while (mp_utf8_iter_next(&iter)) {
        ++count;
    }
    expect_eq(count, 23ul, "%zu");

    return 0;

fail:
    exit(1);
}
