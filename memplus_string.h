#ifndef MEMPLUS_STRING_H__
#define MEMPLUS_STRING_H__

/* #ifdef MEMPLUS_STRING_IMPLEMENTATION */

#include "memplus_alloc.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

/* Holds a null-terminated string and the size of the string (excluding the null-terminator). */
typedef struct {
    size_t size;
    char  *cstr;
} mp_String;

/* Allocates a new `mp_String` from a null-terminated string. */
mp_String mp_string_new(mp_Allocator allocator, const char *str);
/* Allocates a new `mp_String` from formatted input. */
mp_String mp_string_newf(mp_Allocator allocator, const char *fmt, ...);
/* Allocates duplicate of `str`. */
mp_String mp_string_dup(mp_Allocator allocator, mp_String str);


#ifdef MEMPLUS_STRING_IMPLEMENTATION

mp_String mp_string_new(mp_Allocator allocator, const char *str) {
    int size = snprintf(NULL, 0, "%s", str);
    _MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    char *result      = mp_allocator_alloc(allocator, size + 1);
    int   result_size = snprintf(result, size + 1, "%s", str);
    _MEMPLUS_ASSERT(result_size == size);
    return (mp_String){ result_size, result };
}

mp_String mp_string_newf(mp_Allocator allocator, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    _MEMPLUS_ASSERT(size >= 0 && "failed to count string size");
    va_end(args);

    char *result = mp_allocator_alloc(allocator, size + 1);

    va_start(args, fmt);
    int result_size = vsnprintf(result, size + 1, fmt, args);
    _MEMPLUS_ASSERT(result_size == size);
    va_end(args);

    return (mp_String){ result_size, result };
}

mp_String mp_string_dup(mp_Allocator allocator, mp_String str) {
    int size = snprintf(NULL, 0, "%s", str.cstr);
    _MEMPLUS_ASSERT((size >= 0 || size != str.size) && "failed to count string size");
    char *ptr = mp_allocator_dup(allocator, str.cstr, size);
    return (mp_String){ size, ptr };
}

#endif /* ifdef MEMPLUS_STRING_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_STRING_H__ */
