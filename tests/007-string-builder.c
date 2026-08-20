#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Alloc alloc = mp_heap_alloc();

    mp_Sb sb;
    mp_sb_init(&sb, alloc);

    mp_sb_append(&sb, mp_str("Hello, World!"));
    expect_eq(sb.len, (size_t) 13, "%zu");
    expect_streq_mp_s(sb, "Hello, World!");

    mp_sb_appendf(&sb, " %d", 67);
    expect_eq(sb.len, (size_t) 16, "%zu");
    expect_streq_mp_s(sb, "Hello, World! 67");

    mp_da_append(&sb, (char) 10);
    expect_eq(sb.len, (size_t) 17, "%zu");
    expect_streq_mp_s(sb, "Hello, World! 67\n");

    // Other functions should work like dynamic arrays, I hope

    mp_Str str = mp_sb_str(&sb);
    expect_eq(str.len, (size_t) strlen(str.data), "%zu");
    expect_streq_mp(str, mp_str("Hello, World! 67\n"));

    mp_sb_deinit(&sb);

    mp_sb_init_with(&sb, mp_str("hello"), alloc);
    expect_eq(sb.len, (size_t) 5, "%zu");
    expect_eq(sb.cap, (size_t) 5, "%zu");
    expect_streq_mp_s(sb, "hello");

    mp_sb_deinit(&sb);

    mp_sb_init_withf(&sb, alloc, "hello %d", 13);
    expect_eq(sb.len, (size_t) 8, "%zu");
    expect_eq(sb.cap, (size_t) 9, "%zu");
    expect_streq_mp_s(sb, "hello 13");

    mp_String string1 = mp_sb_clone_to_string(&sb, alloc);
    mp_String string2 = mp_sb_string(&sb);
    expect_eq(string1.len, (size_t) 8, "%zu");
    expect_eq(string2.len, (size_t) 8, "%zu");
    expect_streq_mp_s(string1, "hello 13");
    expect_streq_mp_s(string2, "hello 13");
    mp_string_deinit(&string2, alloc);
    mp_string_deinit(&string1, alloc);

    return 0;

fail:
    exit(1);
}
