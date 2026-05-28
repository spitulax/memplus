#include <stdio.h>
#include <string.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    // Don't test on Windows
#ifndef __MP_SYSTEM_WINDOWS
    const char *all_kinds =
        " ab국ꦧ𑨀魈􏾀\x80\xC0\xC0\xA0\xE0\xA0\x20\xF4\x90\x90\x90\xED\xA0\x80";

    expect_eq((size_t) strlen(all_kinds), (size_t) 34, "%zu");
    expect_eq(mp_utf8_len(all_kinds), (size_t) 15, "%zu");

    bool              valids[]     = { true,  true,  true,  true,  true, true,  true, true,
                                       false, false, false, false, true, false, false };
    unsigned int      codepoints[] = { 0x20, 0x61, 0x62, 0xAD6D, 0xA9A7, 0x11A00, 0x9B48, 0x10FF80,
                                       0x0,  0x0,  0x0,  0x0,    0x20,   0x0,     0x0 };
    unsigned char     sizes[]      = { 1, 1, 1, 3, 3, 4, 3, 4, 1, 1, 2, 2, 1, 4, 3 };
    size_t            size         = strlen(all_kinds);
    size_t            size_copy    = size;
    mp_Utf8_Char_Data c;
    size_t            i = 0;
    while ((c = mp_utf8_take(&all_kinds, &size)).c != NULL) {
        bool valid = mp_utf8_char_is_valid(c);
        expect_eq(valid, valids[i], "%d");
        expect_eq(c.size, sizes[i], "%d");
        mp_Utf8_Char_Data ch = mp_utf8_char_s(c.c, c.size);
        if (valid) {
            expect_eq(c.codepoint, codepoints[i], "%d");
            expect_eq(c.size, ch.size, "%hhu");
            expect_eq(c.c, ch.c, "%p");
            expect_eq(c.codepoint, ch.codepoint, "%04X");
            expect_eq(mp_utf8_char_is_valid(ch), valid, "%d");
        }
        size_copy -= c.size;
        ++i;
    }
    expect_eq(size_copy, (size_t) 0, "%zu");

    const char *all_valid = "魈くんは大好きです　⸜(｡˃ ᵕ ˂ )⸝♡􏾀";
    expect_eq((size_t) strlen(all_valid), (size_t) 58, "%zu");
    expect_eq(mp_utf8_len(all_valid), (size_t) 23, "%zu");
    expect_eq(mp_utf8_char("大").size, 3, "%u");
    expect_memeq(mp_utf8_get(all_valid, 4).c, "大", mp_utf8_char("大").size);
    expect(!mp_utf8_char_is_valid(mp_utf8_get(all_valid, 23)));

    /* Error checking */
    {
        // `10` prefix (continuation byte) must follow a "head" byte, but nothing precedes
        const char unused_continuation[] = { (char) 0x80 /*10000000*/, (char) 0x80 };
        expect_eq(mp_utf8_len_s(unused_continuation, 2), (size_t) 2, "%zu");

        // `110` prefix indicates a byte will follow, but nothing follows
        const char abruptly_ends[] = { (char) 0xC0 /*11000000*/ };
        expect_eq(mp_utf8_len_s(abruptly_ends, 1), (size_t) 1, "%zu");

        // `110` prefix indicates a byte will follow, but overlong encoding is detected
        const char overlong[] = { (char) 0xC0 /*11000000*/, (char) 0x80 /*10000000*/ };
        expect_eq(mp_utf8_len_s(overlong, 2), (size_t) 1, "%zu");

        // `1110` prefix indicates 2 bytes will follow, but only one follows
        const char abruptly_ends2[] = { (char) 0xE0 /*11100000*/, (char) 0x80 /*10000000*/ };
        expect_eq(mp_utf8_len_s(abruptly_ends2, 2), (size_t) 1, "%zu");

        // `1110` prefix indicates 2 bytes will follow, and two bytes follow
        const char ok2[] = { (char) 0xE0 /*11100000*/, (char) 0x80 /*10000000*/,
                             (char) 0x80 /*10000000*/ };
        expect_eq(mp_utf8_len_s(ok2, 3), (size_t) 1, "%zu");

        // "head" byte followed by non-continuation byte
        const char invalid_continuation[] = { (char) 0xC0 /*11000000*/, (char) 0x0 };
        expect_eq(mp_utf8_len_s(invalid_continuation, 2), (size_t) 2, "%zu");
    }

    mp_Utf8_Iter iter  = mp_utf8_iter_new(all_valid);
    size_t       count = 0;
    while (mp_utf8_iter_next(&iter)) {
        logfn("%.*s", mp_utf8_char_print(iter.c));
        ++count;
    }
    logsn("\n");
    expect_eq(count, mp_utf8_len(all_valid), "%zu");

    return 0;

fail:
    exit(1);

#else
    return 0;
#endif
}
