#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

void copy_to_mp_str_arr(mp_Str *arr, const char **src, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        arr[i] = mp_str(src[i]);
    }
}

typedef struct {
    const char  *path_str_posix;
    const char  *path_str_win;
    const char **expected_comps;
    char         expected_absolute;
} Path_Test;

int main(void) {
    const Path_Test path_tests[] = {
        (Path_Test) {
                     .path_str_posix    = "foo",
                     .path_str_win      = "foo",
                     .expected_comps    = (const char *[]) { "foo", NULL },
                     .expected_absolute = 0,
                     },
        (Path_Test) {
                     .path_str_posix    = "foo/bar",
                     .path_str_win      = "foo\\bar",
                     .expected_comps    = (const char *[]) { "foo", "bar", NULL },
                     .expected_absolute = 0,
                     },
        (Path_Test) {
                     .path_str_posix    = "foo\\bar\\",
                     .path_str_win      = "foo/bar/",
                     .expected_comps    = (const char *[]) { "foo", "bar", NULL },
                     .expected_absolute = 0,
                     },
        (Path_Test) {
                     .path_str_posix    = "/foo/bar",
                     .path_str_win      = "C:\\foo\\bar",
                     .expected_comps    = (const char *[]) { "foo", "bar", NULL },
                     .expected_absolute = 'C',
                     },
        (Path_Test) {
                     .path_str_posix    = "//foo//bar",
                     .path_str_win      = "C:\\\\foo\\\\bar",
                     .expected_comps    = (const char *[]) { "foo", "bar", NULL },
                     .expected_absolute = 'C',
                     },
        (Path_Test) {
                     .path_str_posix    = "foo//bar////baz",
                     .path_str_win      = "foo\\\\bar\\\\\\\\baz",
                     .expected_comps    = (const char *[]) { "foo", "bar", "baz", NULL },
                     .expected_absolute = 0,
                     },
    };

    {
        mp_Path path;
        mp_path_init(&path, false, mp_heap());

        // TODO: Use the path for filesystem functions

        mp_path_deinit(&path);
    }

    #ifdef __MP_SYSTEM_WINDOWS
    {
        mp_Path path;

        mp_path_parse(&path, mp_str("C:\\"), mp_heap());
        expect(path.absolute);
        expect_eq(path.drive, 'C', "%c");
        mp_path_deinit(&path);

        mp_path_parse(&path, mp_str("\\\\"), mp_heap());
        expect(path.absolute);
        expect_eq(path.drive, 0, "%c");
        mp_path_deinit(&path);

        mp_path_parse(&path, mp_str("\\"), mp_heap());
        expect(path.absolute);
        expect_ne(path.drive, 0, "%c");
        mp_path_deinit(&path);

        mp_path_parse(&path, mp_str("."), mp_heap());
        expect(!path.absolute);
        expect_eq(path.drive, 0, "%c");
        mp_path_deinit(&path);
    }
    #endif

    {
        for (size_t i = 0; i < mp_arr_len(path_tests); ++i) {
            const Path_Test *t = &path_tests[i];

            mp_Path path;
            #ifdef __MP_SYSTEM_WINDOWS
            expect(mp_path_parse(&path, mp_str(t->path_str_win), mp_heap()));
            #else
            expect(mp_path_parse(&path, mp_str(t->path_str_posix), mp_heap()));
            #endif

            expect_eq(path.absolute, (bool) t->expected_absolute, "%d");
            #ifdef __MP_SYSTEM_WINDOWS
            expect_eq(path.drive, t->expected_absolute, "%c");
            #endif

            for (size_t j = 0; j < path.comps.len; ++j) {
                const char *expected = t->expected_comps[j];
                expect_streq_mp_s(mp_get(&path.comps, j), expected);
            }

            mp_path_deinit(&path);
        }
    }

    {
        mp_Path path;
        expect(mp_path_parse(&path, mp_str("././/..///.////tests/./"), mp_heap()));
        expect(mp_path_canonicalize(&path));

        mp_Path cur_dir;
        expect_erreq(mp_get_current_dir(&cur_dir, mp_heap()), MP_OK, mp_err_str);

        expect(mp_path_eq(&path, &cur_dir));

        mp_path_deinit(&cur_dir);
        mp_path_deinit(&path);
    }

    {
        mp_Path path;
        expect(mp_path_parse(&path, mp_str("010-file/.hello.txt"), mp_heap()));
        expect(mp_path_canonicalize(&path));

        mp_Path actual_path;
        expect_erreq(mp_get_current_dir(&actual_path, mp_heap()), MP_OK, mp_err_str);
        expect(mp_path_append_parse(&actual_path, mp_str("010-file/.hello.txt")));

        mp_String path_string        = mp_path_string_deinit(&path, mp_heap());
        mp_String actual_path_string = mp_path_string_deinit(&actual_path, mp_heap());
        expect(mp_path_eq(&path, &actual_path));
        expect_streq_mp(path_string, actual_path_string);

        mp_string_deinit(&actual_path_string, mp_heap());
        mp_string_deinit(&path_string, mp_heap());
    }

    {
        mp_Path path, cloned, cwd;
        expect(mp_path_parse(&path, mp_str("."), mp_heap()));
        expect(mp_path_clone(&cloned, &path, mp_heap()));
        expect_erreq(mp_get_current_dir(&cwd, mp_heap()), MP_OK, mp_err_str);
        expect(mp_path_canonicalize(&path));

        mp_String path_string   = mp_path_string_deinit(&path, mp_heap());
        mp_String cloned_string = mp_path_string_deinit(&cloned, mp_heap());
        mp_String cwd_string    = mp_path_string_deinit(&cwd, mp_heap());

        expect_streq_mp(path_string, cwd_string);
        expect_streq_mp_s(cloned_string, ".");

        mp_string_deinit(&cwd_string, mp_heap());
        mp_string_deinit(&cloned_string, mp_heap());
        mp_string_deinit(&path_string, mp_heap());
    }

    return 0;

fail:
    exit(1);
}
