#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    mp_Str_Builder sb;
    mp_str_builder_init(&sb, alloc);

    mp_str_builder_append(&sb, "Hello, World!");
    expect_eq(sb.len, (size_t) 13, "%zu");
    expect_memeq(sb.data, "Hello, World!", 13);

    mp_str_builder_appendf(&sb, " %d", 67);
    expect_eq(sb.len, (size_t) 16, "%zu");
    expect_memeq(sb.data, "Hello, World! 67", 16);

    mp_str_builder_append_byte(&sb, 10);
    expect_eq(sb.len, (size_t) 17, "%zu");
    expect_memeq(sb.data, "Hello, World! 67\n", 16);

    // Other functions should work like dynamic arrays, I hope

    mp_Str str = mp_str_builder_string(&sb, alloc);
    expect_eq(str.len, (size_t) strlen(str.cstr), "%zu");
    expect_streq(str.cstr, "Hello, World! 67\n");

    mp_Str str2 = mp_str_builder_string_take(&sb, alloc);
    expect_eq(str2.len, (size_t) strlen(str.cstr), "%zu");
    expect_streq(str2.cstr, "Hello, World! 67\n");
    expect_streq(str.cstr, str2.cstr);

    mp_str_deinit(&str2, alloc);
    mp_str_deinit(&str, alloc);

    return 0;

fail:
    exit(1);
}
