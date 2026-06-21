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

int main(void) {
    {
        mp_Path path;
        mp_path_init(&path, false, mp_heap());
        // mp_path_init_from_array(&path, false, (const char *[]) { "014-path" }, 1, mp_heap());
        // expect_eq(path.comps.len, (size_t) 2, "%zu");
        // expect_eq(path.comps.cap, (size_t) 2, "%zu");
        // expect_streq_mp_s(mp_get(&path.comps, 0), "014-path");
        // expect_streq_mp_s(mp_get(&path.comps, 1), "foo.txt");

        // TODO: Use the path for filesystem functions

        mp_path_deinit(&path);
    }

    // {
    // size_t path_tests_len = sizeof(path_tests) / sizeof(*path_tests);
    // for (size_t i = 0; i < path_tests_len; ++i) {
    //     mp_Path path;
    //     mp_path_init_parse(mp_Path *path, mp_Str str_path, mp_Alloc alloc)
    //     logf("%s", path_tests[i].path_str_posix);
    // }
    // }

    return 0;

fail:
    exit(1);
}
