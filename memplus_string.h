#ifndef MEMPLUS_STRING_H__
#define MEMPLUS_STRING_H__

#include "memplus_alloc.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#ifndef MEMPLUS_ASSERT
#include <assert.h>
#define MEMPLUS_ASSERT assert
#endif

typedef struct {
    size_t size;
    char  *cstr;
} mp_String;

mp_String mp_string_new(mp_Allocator allocator, const char *str);
mp_String mp_string_newf(mp_Allocator allocator, const char *fmt, ...);
mp_String mp_string_dup(mp_Allocator allocator, mp_String str);


#ifdef MEMPLUS_STRING_IMPLEMENTATION

mp_String mp_string_new(mp_Allocator allocator, const char *str) {
    int size = snprintf(NULL, 0, "%s", str);
    MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    char *result      = mp_allocator_alloc(allocator, size + 1);
    int   result_size = snprintf(result, size + 1, "%s", str);
    MEMPLUS_ASSERT(result_size == size);
    return (mp_String){ result_size, result };
}

mp_String mp_string_newf(mp_Allocator allocator, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    va_end(args);

    char *result = mp_allocator_alloc(allocator, size + 1);

    va_start(args, fmt);
    int result_size = vsnprintf(result, size + 1, fmt, args);
    MEMPLUS_ASSERT(result_size == size);
    va_end(args);

    return (mp_String){ result_size, result };
}

mp_String mp_string_dup(mp_Allocator allocator, mp_String str) {
    int size = snprintf(NULL, 0, "%s", str.cstr);
    MEMPLUS_ASSERT((size >= 0 || size != str.size) && "failed to count string size");
    char *ptr = mp_allocator_dup(allocator, str.cstr, size);
    return (mp_String){ size, ptr };
}

#endif /* ifdef MEMPLUS_STRING_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_STRING_H__ */
