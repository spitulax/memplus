#include <string.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

mp_da_typedef(const char *, Da_String);

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    mp_Str sv = mp_str("Hello");
    expect_streq_mp(sv, mp_str("Hello"));
    expect_eq(sv.len, (size_t) 5, "%zu");

    mp_String s1 = mp_string_alloc("Hello, World!", alloc);
    expect_streq_mp(s1, mp_str("Hello, World!"));
    expect_eq(s1.len, (size_t) 13, "%zu");

    mp_String s1c = mp_string_from(mp_str_v(s1), alloc);
    expect_eq(s1c.len, s1.len, "%zu");
    expect_streq_mp(s1c, mp_str("Hello, World!"));
    mp_string_deinit(&s1c, alloc);

    mp_String s2 = mp_string_clone(&s1, alloc);
    expect_streq_mp(mp_str_v(s2), mp_str("Hello, World!"));
    expect_ne((void *) s1.data, (void *) s2.data, "%p");
    expect_eq(s2.len, (size_t) 13, "%zu");

    char non_null[] = { 'h', 'e', 'l', 'l', 'o' };
    sv              = mp_str_s(non_null, 5);
    expect_streq_mp(sv, mp_str("hello"));
    expect_eq(sv.len, (size_t) 5, "%zu");

    mp_string_deinit(&s2, alloc);
    mp_string_deinit(&s1, alloc);

    return 0;

fail:
    exit(1);
}
