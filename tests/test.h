#pragma once

#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"

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

#define expect_eq(a, b, fmt)                                                                       \
    do {                                                                                           \
        if ((a) != (b)) {                                                                          \
            elogf("Expected `a == b`, got\n\ta == `" fmt "`\n\tb == `" fmt "`", (a), (b));         \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
