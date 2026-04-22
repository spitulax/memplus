#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Allocator alloc = mp_heap_allocator();

    mp_String s1 = mp_string_new(&alloc, "Hello, World!");
    expect_streq(s1.cstr, "Hello, World!");
    expect_eq(s1.len, 13ul, "%zu");

    size_t    i  = 69;
    mp_String s2 = mp_string_newf(&alloc, "Hello, World!, %zu", i);
    expect_streq(s2.cstr, "Hello, World!, 69");
    expect_eq(s2.len, 17ul, "%zu");

    mp_String s3 = mp_string_clone(&alloc, &s2);
    expect_streq(s3.cstr, "Hello, World!, 69");
    expect_ne((void *) s3.cstr, (void *) s2.cstr, "%p");
    expect_eq(s3.len, 17ul, "%zu");

    char      non_null[] = { 'h', 'e', 'l', 'l', 'o' };
    mp_String s4         = mp_string_new_len(&alloc, non_null, 5);
    expect_streq(s4.cstr, "hello");
    expect_eq(s4.len, 5ul, "%zu");

    mp_string_deinit(&alloc, &s4);
    mp_string_deinit(&alloc, &s3);
    mp_string_deinit(&alloc, &s2);
    mp_string_deinit(&alloc, &s1);

    return 0;

fail:
    exit(1);
}
