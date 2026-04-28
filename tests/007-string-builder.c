#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Alloc *alloc = mp_heap_alloc();

    mp_StrBuilder sb;
    mp_da_init(&sb, alloc);

    mp_str_builder_append(&sb, "Hello, World!");
    expect_eq(sb.len, (size_t) 13, "%zu");
    expect_memeq(sb.data, "Hello, World!", 13);

    mp_str_builder_appendf(&sb, " %d", 67);
    expect_eq(sb.len, (size_t) 16, "%zu");
    expect_memeq(sb.data, "Hello, World! 67", 16);

    // Other functions should work like dynamic arrays, I hope

    mp_Str str = mp_str_builder_string(&sb, alloc);
    expect_eq(str.len, (size_t) strlen(str.cstr), "%zu");
    expect_streq(str.cstr, "Hello, World! 67");

    mp_str_deinit(&str, alloc);
    mp_da_deinit(&sb);

    return 0;

fail:
    exit(1);
}
