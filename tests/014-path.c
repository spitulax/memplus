#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

#define expect_dastr_arrstr_eq(a, b)                                                               \
    do {                                                                                           \
        for (size_t __i = 0; __i < (a).len; ++__i) {                                               \
            if (!mp_str_eq(mp_get(&(a), __i), (b)[__i])) {                                         \
                elogs("Expected `a == b`, got");                                                   \
                elogsn("\ta = ");                                                                  \
                for (size_t __j = 0; __j < (a).len; ++__j) {                                       \
                    if (__j > 0)                                                                   \
                        elogsn(" ");                                                               \
                    mp_Str s = mp_get(&(a), __j);                                                  \
                    elogfn("`%.*s`", mp_str_print(s));                                             \
                }                                                                                  \
                elogsn("\n");                                                                      \
                elogsn("\tb = ");                                                                  \
                for (size_t __j = 0; __j < (a).len; ++__j) {                                       \
                    if (__j > 0)                                                                   \
                        elogsn(" ");                                                               \
                    mp_Str s = (b)[__j];                                                           \
                    elogfn("`%.*s`", mp_str_print(s));                                             \
                }                                                                                  \
                elogsn("\n");                                                                      \
                goto fail;                                                                         \
            }                                                                                      \
        }                                                                                          \
    } while (0)

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
                     .path_str_posix    = "foo/bar/",
                     .path_str_win      = "foo\\bar\\",
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

    {
        for (size_t i = 0; i < mp_arr_len(path_tests); ++i) {
            const Path_Test *t = &path_tests[i];

            mp_Path path;
            #ifdef __MP_SYSTEM_WINDOWS
            mp_path_init_parse(&path, mp_str(t->path_str_win), mp_heap());
            #else
            mp_path_init_parse(&path, mp_str(t->path_str_posix), mp_heap());
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

    return 0;

fail:
    exit(1);
}
