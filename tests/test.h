#pragma once

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
#define unr() assert(0 && "unreachable");

#define expect_binop(op, a, b, fmt)                                                                \
    do {                                                                                           \
        typeof(a) _a = a;                                                                          \
        typeof(b) _b = b;                                                                          \
        if (!((_a) op(_b))) {                                                                      \
            elogf("Expected `a %s b`, got\n\ta = `" fmt "`\n\tb = `" fmt "`", #op, (_a), (_b));    \
            goto fail;                                                                             \
        }                                                                                          \
    } while (0)

#define expect_eq(a, b, fmt) expect_binop(==, a, b, fmt)
#define expect_ne(a, b, fmt) expect_binop(!=, a, b, fmt)
