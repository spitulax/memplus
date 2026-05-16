#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    // File read
    {
        mp_Str text;
        mp_Err err = mp_file_read_file(&text, ".hello.txt", mp_heap());
        expect_eq(err, (mp_Err) MP_ERR_NONE, "%d");
        expect_eq(text.len, (size_t) 14, "%zu");
        expect_streq(text.cstr, "Hello, World!\n");
        mp_str_deinit(&text, mp_heap());
    }

    return 0;

fail:
    exit(1);
}
