#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Allocator alloc = mp_heap_allocator();

    mp_StringBuilder sb;
    mp_da_init(&sb, &alloc);

    mp_string_builder_append(&sb, "Hello, World!");
    expect_eq(sb.len, 13uL, "%zu");
    expect_memeq(sb.data, "Hello, World!", 13);

    mp_string_builder_appendf(&sb, " %d", 67);
    expect_eq(sb.len, 16uL, "%zu");
    expect_memeq(sb.data, "Hello, World! 67", 16);

    // Other functions should work like dynamic arrays, I hope

    mp_String str = mp_string_builder_string(&sb, &alloc);
    expect_eq(str.len, (size_t) strlen(str.cstr), "%zu");
    expect_streq(str.cstr, "Hello, World! 67");

    mp_string_deinit(&alloc, &str);
    mp_da_deinit(&sb);

    return 0;

fail:
    exit(1);
}
