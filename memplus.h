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

/*
 *
 * Changelog:
 *
 * # Version 0.1.0
 * - Rewritten the library.
 *
 */

#ifndef MEMPLUS_H__
#define MEMPLUS_H__

/* #define MEMPLUS_IMPLEMENTATION */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MEMPLUS_ASSERT
#include <assert.h>
#define MEMPLUS_ASSERT assert
#endif

#define MEMPLUS_VERSION (0x000100)

/***********
 * ALLOCATORS
 ***********/

typedef enum {
    MP_ALLOCTYPE_ALLOC,
    MP_ALLOCTYPE_REALLOC,
    MP_ALLOCTYPE_DUP,
    MP_ALLOCTYPE_FREE,
} mp_AllocType;

/*
 * Functions of this type does different things depending on the `type` given.
 * They also use their parameters differently on each type.
 *
 *  Types:
 *  - MP_ALLOCTYPE_ALLOC: Allocates
 *       - `context`: The allocator context
 *       - `new_size`: The size of the allocated memory
 *       - ignores other parameters
 *  - MP_ALLOCTYPE_REALLOC: Reallocates a data
 *       - `context`: The allocator context
 *       - `ptr`: The pointer to the data
 *       - `old_size`: The size of that data
 *       - `new_size`: The new size of the data
 *  - MP_ALLOCTYPE_DUP: Duplicates a data
 *       - `context`: The allocator context
 *       - `ptr`: The data
 *       - `new_size`: The size of the data
 *       - ignores other parameters
 *  - MP_ALLOCTYPE_FREE: Frees a data that has been allocated
 *       - `context`: The allocator context
 *       - `ptr`: The data to be freed
 *       - `new_size`: The size of the data (mostly for logging purpose)
 *       - ignores other parameters
 *
 *  Returns the pointer to the newly allocated memory. May return NULL if allocation failed.
 *  Always returns NULL on MP_ALLOCTYPE_FREE.
 */
typedef void *(*mp_AllocFunc)(
    mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be customized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this could be specified as NULL.
    void *context;

    // The function that does stuff to the memory.
    // See `mp_AllocFunc` for more information.
    mp_AllocFunc f;
} mp_Allocator;

/* Macros that wrap the functions above */

/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * size: number of bytes
 * Returns void*
 */
#define mp_alloc(alloc, size) ((alloc)->f(MP_ALLOCTYPE_ALLOC, (alloc)->context, (size), 0, NULL))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * old_ptr: pointer
 * old_size: number of bytes
 * new_size: number of bytes
 * Returns void*
 */
#define mp_realloc(alloc, old_ptr, old_size, new_size)                                             \
    ((alloc)->f(MP_ALLOCTYPE_REALLOC, (alloc)->context, (new_size), (old_size), (old_ptr)))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * data: pointer
 * size: number of bytes
 * Returns void*
 */
#define mp_dup(alloc, data, size)                                                                  \
    ((alloc)->f(MP_ALLOCTYPE_DUP, (alloc)->context, (size), 0, (data)))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * ptr: pointer (nullability depends on the allocator implementation)
 * size: number of bytes
 * Returns NULL
 */
#define mp_free(alloc, ptr, size)                                                                  \
    ((alloc)->f(MP_ALLOCTYPE_FREE, (alloc)->context, (size), 0, (ptr)))

/* Allocate a new chunk of memory for the given type.
 *
 * alloc: mp_Allocator*
 * type: typename
 * Returns `type`*
 */
#define mp_create(alloc, type) (mp_alloc((alloc), sizeof(type)))

/* Creates a custom allocator given the context and the allocation function.
 *
 * ctx (context): pointer
 * func: mp_AllocFunc
 * Returns mp_Allocator
 */
#define mp_allocator_new(ctx, func)                                                                \
    ((mp_Allocator) {                                                                              \
        .context = (void *) (ctx),                                                                 \
        .f       = (func),                                                                         \
    })

#endif /* ifndef MEMPLUS_H__ */
