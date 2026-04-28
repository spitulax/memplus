#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Alloc *alloc = mp_heap_alloc();

    mp_Str sv = mp_str("Hello");
    expect_streq(sv.cstr, "Hello");
    expect_eq(sv.len, (size_t) 5, "%zu");

    mp_Str s1 = mp_str_new(alloc, "Hello, World!");
    expect_streq(s1.cstr, "Hello, World!");
    expect_eq(s1.len, (size_t) 13, "%zu");

    size_t i  = 69;
    mp_Str s2 = mp_str_newf(alloc, "Hello, World!, %zu", i);
    expect_streq(s2.cstr, "Hello, World!, 69");
    expect_eq(s2.len, (size_t) 17, "%zu");

    mp_Str s3 = mp_str_clone(&s2, alloc);
    expect_streq(s3.cstr, "Hello, World!, 69");
    expect_ne((void *) s3.cstr, (void *) s2.cstr, "%p");
    expect_eq(s3.len, (size_t) 17, "%zu");

    char   non_null[] = { 'h', 'e', 'l', 'l', 'o' };
    mp_Str s4         = mp_str_new_len(alloc, non_null, 5);
    expect_streq(s4.cstr, "hello");
    expect_eq(s4.len, (size_t) 5, "%zu");

    mp_str_deinit(&s4, alloc);
    mp_str_deinit(&s3, alloc);
    mp_str_deinit(&s2, alloc);
    mp_str_deinit(&s1, alloc);

    return 0;

fail:
    exit(1);
}
