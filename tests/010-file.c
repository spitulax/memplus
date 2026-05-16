#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    // File read
    {
        mp_talloc();

        mp_Str text;
        mp_Err err = mp_file_read_file(&text, ".hello.txt", temp_alloc);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);
        expect_eq(text.len, (size_t) 14, "%zu");
        expect_streq(text.cstr, "Hello, World!\n");
    }

    // File create, write, delete
    {
        mp_talloc();

        mp_Err      err;
        const char *path = ".foo.txt";
        mp_Str      data = mp_str("Foobarbazquux");

        // TODO: check if file exists

        err = mp_file_create_file(path);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);

        err = mp_file_write_file(path, data.cstr, data.len, false);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);

        mp_Str text;
        err = mp_file_read_file(&text, path, temp_alloc);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);
        expect_streq(text.cstr, "Foobarbazquux");

        err = mp_file_write_file(path, data.cstr, data.len, true);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);

        err = mp_file_read_file(&text, path, temp_alloc);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);
        expect_streq(text.cstr, "FoobarbazquuxFoobarbazquux");

        err = mp_file_delete_file(path);
        expect_erreq(err, MP_ERR_NONE, mp_err_str);
    }

    return 0;

fail:
    exit(1);
}
