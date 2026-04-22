#pragma once

#include <assert.h>
#include <stdio.h>

#define RED   "\033[31m"
#define RESET "\033[0m"

#define logf(fmt, ...)                                                                             \
    printf("%s:%s():%d: " fmt "\n", __FILE__, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);
#define elogf(fmt, ...)                                                                            \
    fprintf(stderr,                                                                                \
            RED "%s:%s():%d: " fmt "\n" RESET,                                                     \
            __FILE__,                                                                              \
            __func__,                                                                              \
            __LINE__ __VA_OPT__(, ) __VA_ARGS__);
#define logfn(fmt, ...)  printf(fmt __VA_OPT__(, ) __VA_ARGS__);
#define elogfn(fmt, ...) fprintf(stderr, RED fmt RESET __VA_OPT__(, ) __VA_ARGS__);
#define unr()            assert(0 && "unreachable");

#define expect_binop(op, a, b, fmt)                                                                \
    do {                                                                                           \
        typeof(a) _a = a;                                                                          \
        typeof(b) _b = b;                                                                          \
        if (!((_a) op(_b))) {                                                                      \
            elogf("Expected `a %s b`, got\n\ta = `" fmt "`\n\tb = `" fmt "`", #op, (_a), (_b));    \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

#define expect_streq(a, b)                                                                         \
    do {                                                                                           \
        if (strcmp((a), (b)) != 0) {                                                               \
            elogf("Expected `a == b`, got\n\ta = `%s`\n\tb = `%s`", (a), (b));                     \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

#define expect_memeq(a, b, len)                                                                    \
    do {                                                                                           \
        if (memcmp((a), (b), (len)) != 0) {                                                        \
            elogf("Expected `a == b`, got");                                                       \
            elogfn("\ta = `");                                                                     \
            for (size_t i = 0; i < (len); ++i) {                                                   \
                if (i > 0) elogfn(" ");                                                            \
                elogfn("%02X", ((char *) (a))[i]);                                                 \
            }                                                                                      \
            elogfn("`\n");                                                                         \
            elogfn("\tb = `");                                                                     \
            for (size_t i = 0; i < (len); ++i) {                                                   \
                if (i > 0) elogfn(" ");                                                            \
                elogfn("%02X", ((char *) (b))[i]);                                                 \
            }                                                                                      \
            elogfn("`\n");                                                                         \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

#define expect_eq(a, b, fmt) expect_binop(==, a, b, fmt)
#define expect_ne(a, b, fmt) expect_binop(!=, a, b, fmt)
