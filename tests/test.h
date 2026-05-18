#pragma once

#include <assert.h>
#include <stdio.h>

#define RED   "\033[31m"
#define RESET "\033[0m"

#ifdef QUIET
    #define logf(fmt, ...)
    #define logfn(fmt, ...)
#else
    #define logf(fmt, ...)                                                                         \
        printf("%s:%s():%d: " fmt "\n", __FILE__, __func__, __LINE__, __VA_ARGS__);
    #define logs(str)       logf("%s", (str))
    #define logfn(fmt, ...) printf(fmt, __VA_ARGS__);
    #define logsn(str)      logfn("%s", (str))
#endif

#define elogf(fmt, ...)                                                                            \
    fprintf(stderr, RED "%s:%s():%d: " fmt "\n" RESET, __FILE__, __func__, __LINE__, __VA_ARGS__);
#define elogs(str)       elogf("%s", (str))
#define elogfn(fmt, ...) fprintf(stderr, RED fmt RESET, __VA_ARGS__);
#define elogsn(str)      elogfn("%s", (str))

#define unr() assert(0 && "unreachable");

#define typeof __typeof__

#define expect(expr)                                                                               \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            elogf("Expected `%s`", #expr);                                                         \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

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
            elogs("Expected `a == b`, got");                                                       \
            elogsn("\ta = `");                                                                     \
            for (size_t __i = 0; __i < (len); ++__i) {                                             \
                if (__i > 0)                                                                       \
                    elogsn(" ");                                                                   \
                elogfn("%02hhX", ((char *) (a))[__i]);                                             \
            }                                                                                      \
            elogsn("`\n");                                                                         \
            elogsn("\tb = `");                                                                     \
            for (size_t __i = 0; __i < (len); ++__i) {                                             \
                if (__i > 0)                                                                       \
                    elogsn(" ");                                                                   \
                elogfn("%02hhX", ((char *) (b))[__i]);                                             \
            }                                                                                      \
            elogsn("`\n");                                                                         \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

#define expect_eq(a, b, fmt) expect_binop(==, a, b, fmt)
#define expect_ne(a, b, fmt) expect_binop(!=, a, b, fmt)

#define expect_erreq(a, b, f) expect_eq(f(a), f(b), "%s")
