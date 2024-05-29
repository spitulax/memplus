/* Copyright 2024 Bintang Adiputra Pratama <bintangadiputrapratama@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MEMPLUS_STRING_H__
#define MEMPLUS_STRING_H__

/* #define MEMPLUS_STRING_IMPLEMENTATION */

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef MEMPLUS_STRING_IMPLEMENTATION
#define MEMPLUS_ALLOC_IMPLEMENTATION
#endif
#include "memplus_alloc.h"

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
