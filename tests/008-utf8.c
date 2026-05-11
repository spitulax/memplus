#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    const char *s1 = "魈くんは大好きです　⸜(｡˃ ᵕ ˂ )⸝♡􏾀";
    expect_eq((size_t) strlen(s1), (size_t) 58, "%zu");
    expect_eq(mp_utf8_len(s1), (size_t) 23, "%zu");

    // `10` prefix must follow a "head" byte, but nothing precedes
    const char s2[] = { (char) 0x80 /*10000000*/, (char) 0x80 };
    expect_eq(mp_utf8_len_s(s2, 2), (size_t) 2, "%zu");

    // `110` prefix indicates a byte will follow, but nothing follows
    const char s3[] = { (char) 0xC0 /*11000000*/ };
    expect_eq(mp_utf8_len_s(s3, 1), (size_t) 1, "%zu");

    // `110` prefix indicates a byte will follow, and a byte follows
    const char s4[] = { (char) 0xC0 /*11000000*/, (char) 0x80 /*10000000*/ };
    expect_eq(mp_utf8_len_s(s4, 2), (size_t) 1, "%zu");

    // `1110` prefix indicates 2 bytes will follow, but only one follows
    const char s5[] = { (char) 0xE0 /*11100000*/, (char) 0x80 /*10000000*/ };
    expect_eq(mp_utf8_len_s(s5, 2), (size_t) 2, "%zu");

    // `1110` prefix indicates 2 bytes will follow, and two bytes follow
    const char s6[] = { (char) 0xE0 /*11100000*/, (char) 0x80 /*10000000*/,
                        (char) 0x80 /*10000000*/ };
    expect_eq(mp_utf8_len_s(s6, 3), (size_t) 1, "%zu");

    // head byte followed by non `10` prefix byte
    const char s7[] = { (char) 0xC0 /*11000000*/, (char) 0x0 };
    expect_eq(mp_utf8_len_s(s7, 2), (size_t) 2, "%zu");

    mp_Utf8Iter iter  = mp_utf8_iter_new(s1);
    size_t      count = 0;
    while (mp_utf8_iter_next(&iter)) {
        ++count;
    }
    expect_eq(count, (size_t) 23, "%zu");

    return 0;

fail:
    exit(1);
}
