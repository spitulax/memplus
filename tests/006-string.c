#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Allocator alloc = mp_heap_allocator();

    mp_String s1 = mp_string_new(&alloc, "Hello, World!");
    expect_streq(s1.cstr, "Hello, World!");
    expect_eq(s1.len, 13uL, "%zu");

    size_t    i  = 69;
    mp_String s2 = mp_string_newf(&alloc, "Hello, World!, %zu", i);
    expect_streq(s2.cstr, "Hello, World!, 69");
    expect_eq(s2.len, 17uL, "%zu");

    mp_String s3 = mp_string_dup(&alloc, &s2);
    expect_streq(s3.cstr, "Hello, World!, 69");
    expect_ne((void *) s3.cstr, (void *) s2.cstr, "%p");
    expect_eq(s3.len, 17uL, "%zu");

    mp_string_deinit(&alloc, &s3);
    mp_string_deinit(&alloc, &s2);
    mp_string_deinit(&alloc, &s1);

    return 0;

fail:
    exit(1);
}
