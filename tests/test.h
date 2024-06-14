#ifndef TEST_H
#define TEST_H

#define MEMPLUS_IMPLEMENTATION
#include "../memplus.h"

#include <stdbool.h>
#include <stdio.h>

#define prn(str)                                                                                   \
    do {                                                                                           \
        printf("%s\n", str);                                                                       \
    } while (0)
#define prnf(fmt, ...)                                                                             \
    do {                                                                                           \
        printf(fmt "\n", __VA_ARGS__);                                                             \
    } while (0)

#define expects(cond, str)                                                                         \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("\033[0;31m");                                                                  \
            printf("%s\n", str);                                                                   \
            printf("\033[0m");                                                                     \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)
#define expectf(cond, fmt, ...)                                                                    \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("\033[0;31m");                                                                  \
            printf(fmt "\n", __VA_ARGS__);                                                         \
            printf("\033[0m");                                                                     \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)

#endif /* ifndef TEST_H */
