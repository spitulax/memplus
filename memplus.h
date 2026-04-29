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
 * Changelog:
 *
 */

#ifndef MEMPLUS_H__
#define MEMPLUS_H__

/* #define MEMPLUS_IMPLEMENTATION */

#define _POSIX_C_SOURCE 200809l    // also defines X/Open

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Systems. */
// POSIX
#if defined(__POSIX__) || defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#if defined(_POSIX_VERSION)
#define __MP_SYSTEM_POSIX

// Linux
#if defined(__linux__)
#define __MP_SYSTEM_LINUX
#endif /* if defined(__linux__) */

#endif /* if defined(_POSIX_VERSION) */

// Windows
#elif defined(_WIN32)
#define __MP_SYSTEM_WINDOWS

#else
#error "Unsupported system."

#endif /* if defined(__POSIX__) || defined(__unix__) || (defined(__APPLE__) ... */

#if __STDC_VERSION__ >= 202311L
#define __MP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112L
#define __MP_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
#else
#define __MP_STATIC_ASSERT(...)
#endif

/* Define custom assert macro with `MEMPLUS_ASSERT` and `MEMPLUS_ASSERT_MSG`.
 * The macro must accept the expression and the fail message. See the define below. */
#if !(defined(MEMPLUS_ASSERT) && defined(MEMPLUS_ASSERT_MSG))

#if __STDC_VERSION__ >= 201112L
#define __MP_NORETURN _Noreturn
#elif __STDC_VERSION__ >= 202311L
#define __MP_NORETURN [[noreturn]]
#endif

#include <stdio.h>
#include <stdlib.h>

#define __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(
    const char *assertion, const char *file, const char *func, size_t line, const char *msg);

#ifdef NDEBUG
#define MEMPLUS_ASSERT(expr)
#define MEMPLUS_ASSERT_MSG(expr, msg)
#else
#define MEMPLUS_ASSERT(expr)                                                                       \
    ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, ""))

#define MEMPLUS_ASSERT_MSG(expr, msg)                                                              \
    ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, (msg)))
#endif /* ifdef NDEBUG */

#endif /* if !(defined(MEMPLUS_ASSERT) && defined(MEMPLUS_ASSERT_MSG)) */

/* Please do not use these two defines inside a macro in this library. */
/* Assumed have the same behavior as stdlib's `calloc(..., 1)`. */
#ifndef MEMPLUS_ALLOC
#include <stdlib.h>
#define MEMPLUS_ALLOC(size) calloc((size), 1)
#endif
/* Must have the same signature and behavior as stdlib's `free`. */
#ifndef MEMPLUS_FREE
#include <stdlib.h>
#define MEMPLUS_FREE free
#endif

#define MEMPLUS_VERSION (0x000100)

#define __MP_ZERO(ptr) memset((ptr), 0, sizeof(*(ptr)))
#define __MP_BOUNDS_CHECK(i, len)                                                                  \
    MEMPLUS_ASSERT_MSG((i) < (len) && (i) >= 0, "Array index out of bounds")
#if defined(__GNUC__) || defined(__clang__)
#define __MP_PRINTF_FORMAT(fmt_index) __attribute__((format(printf, (fmt_index), (fmt_index) + 1)))
#else
#define __MP_PRINTF_FORMAT(fmt_index)
#endif

/* Indicates error return for `size_t`. */
#define MP_ERROR ((size_t) -1)

// TODO: Change some `const \S*\*` paramters to non-pointer

/***********
 * ALLOCATORS
 ***********/

/* Default size of a single region in bytes.
 * Will be aligned to the nearest increment of `sizeof(uintptr_t)`. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (64 * 1024)
#endif

/* See `mp_AllocFunc` below. */
typedef enum {
    MP_ALLOCOP_ALLOC,
    MP_ALLOCOP_REALLOC,
    MP_ALLOCOP_FREE,
    __MP_ALLOCOP_COUNT,
} mp_AllocOp;

// TODO: Alloc location

/* Functions of this type do different things depending on the `op` given.
 * They also use their parameters differently on each type.
 *
 * Returns the pointer to the newly allocated memory. May return NULL if allocation failed.
 * Always returns NULL on MP_ALLOCOP_FREE.
 *
 *  Operations:
 *  - MP_ALLOCOP_ALLOC: Allocates
 *    Does nothing and returns NULL if `new_size` == 0.
 *       - `context`: The allocator context
 *       - `new_size`: The size of the allocated memory
 *       - ignores other parameters
 *  - MP_ALLOCOP_REALLOC: Reallocates a data
 *    If `old_size` <= `new_size`, reallocation does not happen and the function just return `ptr`.
 *    Otherwise, allocates with size `new_size` and frees the memory pointed by `ptr`.
 *    Does nothing and returns NULL if `new_size` == 0
 *    Should behave like `MP_ALLOCOP_ALLOC` if `old_size` == 0 or `ptr` == NULL.
 *       - `context`: The allocator context
 *       - `ptr`: The pointer to the data
 *       - `old_size`: The size of that data
 *       - `new_size`: The new size of the data
 *  - MP_ALLOCOP_FREE: Frees a data that has been allocated
 *    Does nothing if `ptr` == NULL.
 *       - `context`: The allocator context
 *       - `ptr`: The data to be freed
 *       - `new_size`: The size of the data (mostly for logging purpose)
 *       - ignores other parameters
 */
typedef void *(*mp_AllocFunc)(
    mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be customized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this can be specified as NULL.
    void *context;

    // The function that does stuff to the memory.
    // See `mp_AllocFunc` for more information.
    mp_AllocFunc f;
} mp_Alloc;

/* Macros that wrap the functions above.
 * See `mp_AllocFunc` for details. */

/* alloc: mp_Alloc (NO SIDE EFFECTS)
 * size: number of bytes
 * Returns void* */
#define mp_alloc(alloc, size) ((alloc).f(MP_ALLOCOP_ALLOC, (alloc).context, (size), 0, NULL))
/* alloc: mp_Alloc (NO SIDE EFFECTS)
 * old_ptr: pointer
 * old_size: number of bytes
 * new_size: number of bytes
 * Returns void* */
#define mp_realloc(alloc, old_ptr, old_size, new_size)                                             \
    ((alloc).f(MP_ALLOCOP_REALLOC, (alloc).context, (new_size), (old_size), (old_ptr)))
/* alloc: mp_Alloc (NO SIDE EFFECTS)
 * ptr: pointer (nullability depends on the allocator implementation)
 * size: number of bytes
 * Returns NULL */
#define mp_free(alloc, ptr, size) ((alloc).f(MP_ALLOCOP_FREE, (alloc).context, (size), 0, (ptr)))
/* Allocate a new chunk of memory for the given type.
 *
 * alloc: mp_Alloc (NO SIDE EFFECTS)
 * type: typename
 * Returns `type`* */
#define mp_create(alloc, type) (mp_alloc((alloc), sizeof(type)))
/* alloc: mp_Alloc (NO SIDE EFFECTS)
 * data: pointer
 * size: number of bytes
 * Returns void* */
void *mp_dup(mp_Alloc alloc, void *data, size_t size);

/* Creates a custom allocator given the context and the allocation function.
 *
 * ctx (context): pointer
 * func: mp_AllocFunc
 * Returns mp_Alloc */
#define mp_alloc_new(ctx, func)                                                                    \
    ((mp_Alloc) {                                                                                  \
        .context = (void *) (ctx),                                                                 \
        .f       = (func),                                                                         \
    })

/* Returns an invalid `mp_Alloc`.
 * Invalid `mp_Alloc` requires that field `f` is NULL. */
#define mp_alloc_invalid()                                                                         \
    ((mp_Alloc) {                                                                                  \
        .context = NULL,                                                                           \
        .f       = NULL,                                                                           \
    })

/* Handles reallocation for custom allocators.
 * You can slot this into your allocator function as long as alloc and free functionalities are
 * defined. For details see the implementation.
 * Does nothing and returns NULL if `new_size` == 0 */
void *mp_alloc_handle_realloc(mp_Alloc alloc, void *old_ptr, size_t old_size, size_t new_size);

typedef struct mp_Region mp_Region;

/* Linked list element that holds certain size of allocated memory. */
struct mp_Region {
    mp_Region *next;      // The next region in linked list if any
    size_t     len;       // The amount of data (in bytes) used
    size_t     cap;       // The amount of data (in bytes) allocated
    uintptr_t  data[];    // The data (aligned to the `sizeof(uintptr_t)`)
};

/* Allocates a new region with `cap` bytes of size with `alloc`.
 * `cap` will be ROUNDED UP to the nearest increment of `sizeof(uintptr_t)`.
 * Deinit with `mp_region_init`. */
mp_Region *mp_region_init(mp_Alloc alloc, size_t cap);
/* Frees a region. */
void mp_region_deinit(mp_Region *r, mp_Alloc alloc);

/* Growing arena allocator.
 * Manages regions in a linked list. */
typedef struct {
    mp_Region *begin, *end;    // Region linked list
    size_t     len;            // The amount of data (in bytes used, aligned to `sizeof(uintptr_t)`)
    mp_Alloc   alloc;          // Backing allocator
    size_t     _def_size;      // Region default size
} mp_Arena;

/* Creates a new, unallocated arena.
 * Each region will be allocated with size `MP_REGION_DEFAULT_SIZE` by default.
 * Deinit with `mp_arena_deinit`. */
#define mp_arena_init(a, alloc) mp_arena_init_s((a), (alloc), MP_REGION_DEFAULT_SIZE)
/* Creates a new, unallocated arena.
 * Each region will be allocated with size `def_size` by default (aligned to the nearest increment
 * of `sizeof(uintptr_t)`).
 * Deinit with `mp_arena_deinit`. */
void mp_arena_init_s(mp_Arena *a, mp_Alloc alloc, size_t def_size);
/* Set arena `len` to 0, but does not free allocated regions. */
void mp_arena_reset(mp_Arena *a);
/* Frees an arena and its regions. */
void mp_arena_deinit(mp_Arena *a);
/* Returns an allocator that works with `mp_Arena`. */
mp_Alloc mp_arena_alloc(mp_Arena *a);

/* Static arena allocator.
 * Allocations are cancelled and return NULL if the requested size is bigger than the remaining
 * capacity. */
typedef struct {
    uintptr_t *buf;    // The arena buffer (of size `cap`)
    size_t     len;    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t     cap;    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
    mp_Alloc   alloc;    // The allocator used to manage `buf`
} mp_SArena;

/* Initializes and allocates a static arena. `cap` in bytes.
 * `cap` will be ROUNDED UP to the nearest increment of `sizeof(uintptr_t)`.
 * Deinit with `mp_sarena_deinit`. */
void mp_sarena_init(mp_SArena *a, mp_Alloc alloc, size_t cap);
/* Resets the size of the arena. */
void mp_sarena_reset(mp_SArena *a);
/* Frees the arena. */
void mp_sarena_deinit(mp_SArena *a);
/* Returns an allocator that works with `mp_SArena`. */
mp_Alloc mp_sarena_alloc(mp_SArena *a);

/* Temp allocator.
 * `mp_SArena` located in the stack. */
typedef struct {
    uintptr_t *buf;    // The arena buffer (of size `cap`)
    size_t     len;    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t     cap;    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
} mp_Temp;

/* Initializes a temp allocator with an array as buf.
 * `cap` should be an increment of `sizeof(uintptr_t)`.
 * If not, the actual `cap` will ROUND DOWN to the nearest increment. */
void mp_temp_init(mp_Temp *t, char *buf, size_t cap);
/* Resets the size of the temp allocator */
void mp_temp_reset(mp_Temp *t);
/* Returns an allocator that works with `mp_Temp`. */
mp_Alloc mp_temp_alloc(mp_Temp *t);

/* Heap allocator. */
mp_Alloc mp_heap_alloc(void);
#define mp_heap() mp_heap_alloc()

/***********
 * DYNAMIC ARRAY
 ***********/

/* Starting capacity of a dynamic array. */
#ifndef MP_DARRAY_INIT_CAPACITY
#define MP_DARRAY_INIT_CAPACITY 64
#endif

/* You can define a dynamic array struct with any type as long as it is in this format. */
/*
    ```c
    typedef mp_da_create(int) ArrayName;
    ```

    Or manually,
    ```c
    struct {
        mp_Alloc alloc;    // The allocator that manages the allocation of the array
        size_t   len;       // The size of the array
        size_t   cap;       // The capacity of the array
        <type>   *data;     // Pointer to the data (points to the first element)
        // The data is continuous in memory.
    };
    ```

    Any struct which has those fields is a valid dynamic array.
    Any additional fields are tolerated.
*/

/* Defines a dynamic array struct given of `type`.
 * Example usage:
 * ```c
 * mp_da_create(int, ArrayInt);
 * ```
 *
 * type: typename
 * name: identifier */
#define mp_da_create(type, name)                                                                   \
    typedef struct {                                                                               \
        mp_Alloc alloc;                                                                            \
        size_t   len;                                                                              \
        size_t   cap;                                                                              \
        type    *data;                                                                             \
    } name

/* Initializes a new dynamic array managed by `allocator`.
 * Deinit with `mp_da_deinit`.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * allocator: mp_Alloc */
#define mp_da_init(a, allocator)                                                                   \
    do {                                                                                           \
        (a)->alloc = (allocator);                                                                  \
        (a)->len   = 0;                                                                            \
        (a)->cap   = 0;                                                                            \
        (a)->data  = NULL;                                                                         \
    } while (0)

/* Frees a dynamic array.
 *
 * a: DArray* (NO SIDE EFFECTS) */
#define mp_da_deinit(a)                                                                            \
    do {                                                                                           \
        mp_free((a)->alloc, (a)->data, (a)->cap * sizeof(*(a)->data));                             \
        __MP_ZERO(a);                                                                              \
    } while (0)

/* Resizes a dynamic array and appends item to the end.
 * `data` becomes NULL if allocation failed.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * item: <item type> */
#define mp_da_append(a, item)                                                                      \
    do {                                                                                           \
        mp_da_grow((a), 1);                                                                        \
        if ((a)->data != NULL) (a)->data[(a)->len - 1] = (item);                                   \
    } while (0)

/* Resizes a dynamic array and appends multiple items to the end.
 * `data` becomes NULL if allocation failed.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * ...: <item type> */
#define mp_da_append_many(a, ...)                                                                  \
    do {                                                                                           \
        __typeof__(*(a)->data) __items[]  = { __VA_ARGS__ };                                       \
        size_t                 __len      = sizeof(__items) / sizeof(*__items);                    \
        size_t                 __prev_len = (a)->len;                                              \
        mp_da_grow((a), __len);                                                                    \
        if ((a)->data != NULL) memcpy((a)->data + __prev_len, __items, sizeof(__items));           \
    } while (0)

/* Resizes a dynamic array and appends items in an array to the end.
 * `data` becomes NULL if allocation failed.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * items: <array of item type>
 * items_len: size_t */
#define mp_da_append_array(a, items, items_len)                                                    \
    do {                                                                                           \
        size_t __prev_len = (a)->len;                                                              \
        mp_da_grow((a), (items_len));                                                              \
        if ((a)->data != NULL)                                                                     \
            memcpy((a)->data + __prev_len, (items), (items_len) * sizeof(*(a)->data));             \
    } while (0)

/* Gets an item or a pointer to an item at index `i`.
 * No bounds checking, use `mp_da_get_safe` for that.
 *
 * a: const DArray*
 * i: integer */
#define mp_da_get(a, i)  (a)->data[i]
#define mp_da_getp(a, i) ((a)->data + i)

/* Gets an item or a pointer to an item at index `i`.
 * Asserts that `i` is not out of bounds.
 *
 * a: const DArray*
 * i: integer */
#define mp_da_get_safe(a, i)  (__MP_BOUNDS_CHECK((i), (a)->len), (a)->data[i])
#define mp_da_getp_safe(a, i) (__MP_BOUNDS_CHECK((i), (a)->len), (a)->data + i)

/* Gets the first or the last item in a dynamic array.
 *
 * a: const DArray* (NO SIDE EFFECTS) */
#define mp_da_first(a) (a)->data[0]
#define mp_da_last(a)  (a)->data[(a)->len - 1]

/* Deletes the last item in a dynamic array and returns it.
 *
 * a: DArray* (NO SIDE EFFECTS) */
#define mp_da_pop(a) (--(a)->len, (a)->data[(a)->len])

/* Sets the length of a dynamic array to 0.
 *
 * a: DArray* */
#define mp_da_reset(a)                                                                             \
    do {                                                                                           \
        (a)->len = 0;                                                                              \
    } while (0)

/* Resizes a dynamic array to `offset` of the current `len`.
 * If the current capacity is 0, allocates for `MP_DARRAY_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * `data` becomes NULL if allocation failed.
 *
 * `mp_da_grow` grows the array.
 * `mp_da_shrink` shrinks the array (positive offset).
 *
 * a: DArray* (NO SIDE EFFECTS)
 * offset: size_t */
#define mp_da_grow(a, offset)                                                                      \
    do {                                                                                           \
        size_t __off = (offset);                                                                   \
        if ((a)->len + __off > (a)->cap && __off > 0) {                                            \
            size_t __old_cap = (a)->cap;                                                           \
            if ((a)->cap == 0) (a)->cap = MP_DARRAY_INIT_CAPACITY;                                 \
            while ((a)->len + __off > (a)->cap)                                                    \
                (a)->cap *= 2;                                                                     \
            (a)->data = mp_realloc((a)->alloc,                                                     \
                                   (a)->data,                                                      \
                                   __old_cap * sizeof(*(a)->data),                                 \
                                   (a)->cap * sizeof(*(a)->data));                                 \
        }                                                                                          \
        if ((a)->data != NULL) (a)->len += __off;                                                  \
    } while (0)
#define mp_da_shrink(a, offset)                                                                    \
    do {                                                                                           \
        MEMPLUS_ASSERT_MSG((offset) <= (a)->len, "`offset` is  out of bounds");                    \
        size_t __off = (offset);                                                                   \
        if ((a)->len - __off > (a)->cap && __off > 0) {                                            \
            size_t __old_cap = (a)->cap;                                                           \
            if ((a)->cap == 0) (a)->cap = MP_DARRAY_INIT_CAPACITY;                                 \
            (a)->data = mp_realloc((a)->alloc,                                                     \
                                   (a)->data,                                                      \
                                   __old_cap * sizeof(*(a)->data),                                 \
                                   (a)->cap * sizeof(*(a)->data));                                 \
        }                                                                                          \
        if ((a)->data != NULL) (a)->len -= __off;                                                  \
    } while (0)

/* Clones a dynamic array to `dest` to be managed by `allocator`.
 * The `dest` array does not inherit the capacity of `src`. Instead it will only allocate with size
 * `len` + `MP_DARRAY_INIT_CAPACITY`.
 * `dest.data` becomes NULL if allocation failed.
 *
 * allocator: mp_Alloc (NO SIDE EFFECTS)
 * src: DArray* (NO SIDE EFFECTS)
 * dest: DArray* (NO SIDE EFFECTS) */
#define mp_da_clone(allocator, src, dest)                                                          \
    do {                                                                                           \
        (dest)->data = mp_dup((allocator), (src)->data, (src)->cap * sizeof(*(src)->data));        \
        if ((dest)->data != NULL) {                                                                \
            (dest)->alloc = (allocator);                                                           \
            (dest)->len   = (src)->len;                                                            \
            (dest)->cap   = (src)->len + MP_DARRAY_INIT_CAPACITY;                                  \
        } else {                                                                                   \
            (dest)->alloc = mp_alloc_invalid();                                                    \
            (dest)->len   = 0;                                                                     \
            (dest)->cap   = 0;                                                                     \
        }                                                                                          \
    } while (0)

/* Inserts an item at the given `pos`.
 * If `pos > len`, then it just puts the item at `len`.
 * `data` becomes NULL if allocation failed.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * pos: size_t
 * item: <item type> */
#define mp_da_insert(a, pos, item)                                                                 \
    do {                                                                                           \
        MEMPLUS_ASSERT_MSG((pos) >= 0, "`pos` is negative");                                       \
        size_t __p        = (pos);                                                                 \
        size_t __actual_p = __p > (a)->len ? (a)->len : __p;                                       \
        mp_da_grow((a), 1);                                                                        \
        if ((a)->data != NULL) {                                                                   \
            for (size_t __i = (a)->len - 2; __i > __actual_p; --__i)                               \
                (a)->data[__i + 1] = (a)->data[__i];                                               \
            (a)->data[__actual_p + 1] = (a)->data[__actual_p];                                     \
            (a)->data[__actual_p]     = (item);                                                    \
        }                                                                                          \
    } while (0)

/* Deletes an item at the given `pos`. This operation is O(n) on the worst case.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * pos: size_t */
#define mp_da_delete(a, pos)                                                                       \
    do {                                                                                           \
        __MP_BOUNDS_CHECK((pos), (a)->len);                                                        \
        size_t __p = (pos);                                                                        \
        mp_da_shrink((a), 1);                                                                      \
        for (size_t __i = __p + 1; __i < (a)->len + 1; ++__i)                                      \
            (a)->data[__i - 1] = (a)->data[__i];                                                   \
    } while (0)

/* Deletes an item at the given `pos`. This operation is O(1).
 *
 * a: Vector* (NO SIDE EFFECTS)
 * pos: size_t */
#define mp_da_unordered_delete(a, pos)                                                             \
    do {                                                                                           \
        __MP_BOUNDS_CHECK((pos), (a)->len);                                                        \
        size_t __p = (pos);                                                                        \
        mp_da_shrink((a), 1);                                                                      \
        if (__p != (a)->len) (a)->data[__p] = (a)->data[(a)->len];                                 \
    } while (0)

/***********
 * STRING
 ***********/

/* Holds a pointer to NULL-TERMINATED string and the size of the string (excluding the
 * null-terminator). */
typedef struct {
    size_t len;
    char  *cstr;
} mp_Str;

/* Returns an invalid `mp_Str`.
 * Invalid `mp_Str` requires that field `cstr` is NULL. */
#define mp_str_invalid()                                                                           \
    ((mp_Str) {                                                                                    \
        .len  = 0,                                                                                 \
        .cstr = NULL,                                                                              \
    })

/* Tests if `s` is invalid (i.e. `cstr` == NULL).
 * Returns true if valid.
 *
 * s: const mp_Str* */
#define mp_str_is_valid(s) ((s)->cstr != NULL)

/* Create a `mp_Str` from a C-string.
 *
 * str: const char* (NULL-TERMINATED) */
#define mp_str(str)                                                                                \
    ((mp_Str) {                                                                                    \
        .len  = strlen(str),                                                                       \
        .cstr = (str),                                                                             \
    })

/* Allocates and returns a new `mp_Str` from a NULL-TERMINATED string.
 * Deinit with `mp_str_deinit`.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new(mp_Alloc alloc, const char *str);
/* Allocates and returns a new `mp_Str` from a string.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new_len(mp_Alloc alloc, const char *str, size_t len);
/* Allocates and returns a new `mp_Str` from formatted input.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_newf(mp_Alloc alloc, const char *fmt, ...) __MP_PRINTF_FORMAT(2);
/* Frees an allocated `mp_Str`. */
void mp_str_deinit(mp_Str *str, mp_Alloc alloc);
/* Allocates and returns a clone of `str`.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_clone(const mp_Str *str, mp_Alloc alloc);

/***********
 * STRING BUILDER
 ***********/

/* Holds a NON NULL-TERMINATED string that is resizable.
 * The underlying data type is a dynamic array of char.
 * Use `mp_da_init()` to initialize a builder first.
 * Use `mp_da_deinit()` to deinitialize.
 *
 * To convert `mp_StrBuilder` to C string, use `mp_str_builder_string()` which returns a
 * null-terminated `mp_Str`. */
mp_da_create(char, mp_StrBuilder);

/* Appends a NULL-TERMINATED string to a `mp_StrBuilder`.
 * `data` becomes NULL if allocation failed. */
void mp_str_builder_append(mp_StrBuilder *sb, const char *str);
/* Appends a formatted string to a `mp_StrBuilder`.
 * `data` becomes NULL if allocation failed. */
void mp_str_builder_appendf(mp_StrBuilder *sb, const char *fmt, ...) __MP_PRINTF_FORMAT(2);
/* Copies the buffer of a `mp_StrBuilder` into a null-terminated `mp_Str`.
 * Returns invalid `mp_Str` if allocation failed. */
mp_Str mp_str_builder_string(const mp_StrBuilder *sb, mp_Alloc alloc);

/***********
 * UTF-8
 ***********/

/* Calculate the length of a UTF-8 string.
 * The string is NULL-TERMINATED.
 * Returns `MP_ERROR` if `str` is not a valid UTF-8 string. */
size_t mp_utf8_len(const char *str);
/* Calculate the length of a UTF-8 string with size (in bytes) parameter.
 * Returns `MP_ERROR` if `str` is not a valid UTF-8 string. */
size_t mp_utf8_len_s(const char *str, size_t size);

/* Iterator for UTF-8 string.
 *
 * `c` and `c_len` can be accessed to get the current character's information.
 *
 * Example usage:
 * ```c
 *  const char *utf8 = "魈くんは大好きです　⸜(｡˃ ᵕ ˂ )⸝♡􏾀";
 *  mp_Utf8Iter iter  = mp_utf8_iter_new(utf8);
 *  while (mp_utf8_iter_next(&iter)) {
 *      (void) iter.c;      // The current character (in char[4])
 *      (void) iter.c_len;  // The current character size (in bytes)
 *  }
 * ```
 * */
typedef struct {
    char c[4];     // Holds current character in iteration
    char c_len;    // Holds current character's size (in bytes)

    const char *_str;     // The UTF-8 string being iterated on
    size_t      _size;    // The size of the string (in bytes)
    size_t      _i;       // The current index of the iteration (in bytes)
} mp_Utf8Iter;

/* Creates a new `mp_Utf8Iter` that iterates over a UTF-8 string.
 * The string is NULL-TERMINATED. */
mp_Utf8Iter mp_utf8_iter_new(const char *str);
/* Creates a new `mp_Utf8Iter` that iterates over a UTF-8 string with size parameter (in bytes). */
mp_Utf8Iter mp_utf8_iter_new_s(const char *str, size_t size);
/* See `mp_Utf8Iter`. */
bool mp_utf8_iter_next(mp_Utf8Iter *it);

/***********
 * HASH TABLE (STRING KEY)
 ***********/

/* Percentage of elements in a hash table before it resizes. */
#ifndef MP_HASH_TABLE_MAX_LOAD
#define MP_HASH_TABLE_MAX_LOAD 0.75
#endif

/* Starting capacity of a hash table. */
#ifndef MP_HASH_TABLE_INIT_CAPACITY
#define MP_HASH_TABLE_INIT_CAPACITY MP_DARRAY_INIT_CAPACITY
#endif

/* Defines a hash table struct with string key and value of type `value_type`.
 * Example usage:
 * ```c
 * mp_ht_create(int, HashTableInt);
 * ```
 *
 * This also defines the hash table's iterator type, named by suffixing `Iter` after the hash
 * table's type name.
 * Example usage:
 * ```c
 * HashTableIntIter it;
 * mp_ht_iter_init(&it, &ht);
 * while (it.ok) {
 *     (void) it.key;
 *     (void) it.val;
 *     mp_ht_iter_next(&it);
 * }
 * ```
 *
 * value_type: typename
 * name: identifier */
#define mp_ht_create(value_type, name)                                                             \
    typedef struct {                                                                               \
        mp_Str     key;                                                                            \
        value_type val;                                                                            \
    } __##name##Entry;                                                                             \
    mp_da_create(__##name##Entry, name);                                                           \
    typedef struct {                                                                               \
        mp_Str      key;                                                                           \
        value_type  val;                                                                           \
        bool        ok;                                                                            \
        const name *_h;                                                                            \
        size_t      _i;                                                                            \
    } name##Iter

/* Initializes a new string hash table managed by `allocator`.
 * Deinit with `mp_ht_deinit`.
 *
 * ht: StrHashTable* (NO SIDE EFFECTS)
 * allocator: mp_Alloc */
#define mp_ht_init(ht, allocator) mp_da_init(ht, allocator)

/* Frees a string hash table.
 *
 * ht: StrHashTable* (NO SIDE EFFECTS) */
#define mp_ht_deinit(ht)                                                                           \
    do {                                                                                           \
        for (size_t __i = 0; __i < (ht)->cap; __i++) {                                             \
            if (mp_str_is_valid(&(ht)->data[__i].key))                                             \
                mp_str_deinit(&(ht)->data[__i].key, (ht)->alloc);                                  \
        }                                                                                          \
        mp_da_deinit(ht);                                                                          \
    } while (0)

/* Gets a pointer to an item with key `k` and put it into `res`.
 * `res` becomes NULL if it could not retrieve the item.
 *
 * ht: const StrHashTable*
 * k: const char* (NON-NULL)
 * ret: <value type>* */
#define mp_ht_get(ht, k, ret) mp_ht_get_s((ht), &mp_str(k), (ret))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: const StrHashTable*
 * k: mp_Str*
 * ret: <value type>* */
#define mp_ht_get_s(ht, k, ret)                                                                    \
    do {                                                                                           \
        bool __found = false;                                                                      \
        if ((k) != NULL) {                                                                         \
            mp_Str   __key  = *(k);                                                                \
            uint64_t __hash = mp_ht_hash_str(&__key);                                              \
            size_t   __i    = (size_t) (__hash % (uint64_t) ((ht)->cap - 1));                      \
            while (__i < (ht)->cap && (mp_str_is_valid(&(ht)->data[__i].key) ||                    \
                                       *(char *) &(ht)->data[__i].val == 1)) {                     \
                if (mp_str_is_valid(&(ht)->data[__i].key) &&                                       \
                    strcmp(__key.cstr, (ht)->data[__i].key.cstr) == 0) {                           \
                    (ret)   = &(ht)->data[__i].val;                                                \
                    __found = true;                                                                \
                    break;                                                                         \
                }                                                                                  \
                ++__i;                                                                             \
                if (__i >= (ht)->cap) __i = 0;                                                     \
            }                                                                                      \
        }                                                                                          \
        if (!__found) (ret) = NULL;                                                                \
    } while (0)

/* Sets the value at key `k` to `v`.
 * When the item at `k` has not been initialized before, the key is cloned.
 * `data` becomes NULL if allocation failed.
 *
 * ht: StrHashTable*
 * k: const char*
 * res: <value type>* */
#define mp_ht_set(ht, k, v) mp_ht_set_s((ht), &mp_str(k), (v))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: StrHashTable*
 * k: mp_Str*
 * v: <value type> */
#define mp_ht_set_s(ht, k, v)                                                                      \
    do {                                                                                           \
        mp_Str __key = *(k);                                                                       \
        mp_ht_grow((ht), 1);                                                                       \
        if ((ht)->data != NULL) {                                                                  \
            uint64_t __hash = mp_ht_hash_str(&__key);                                              \
            size_t   __i    = (size_t) (__hash % (uint64_t) ((ht)->cap - 1));                      \
            for (;;) {                                                                             \
                if (!mp_str_is_valid(&(ht)->data[__i].key)) {                                      \
                    (ht)->data[__i].key = mp_str_clone(&__key, (ht)->alloc);                       \
                    (ht)->data[__i].val = (v);                                                     \
                    break;                                                                         \
                } else if (strcmp((ht)->data[__i].key.cstr, __key.cstr) == 0) {                    \
                    (ht)->data[__i].val = (v);                                                     \
                    --(ht)->len;                                                                   \
                    break;                                                                         \
                } else {                                                                           \
                    ++__i;                                                                         \
                }                                                                                  \
                if (__i >= (ht)->cap) __i = 0;                                                     \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/* Resizes a string hash table to `offset` of the current `len`.
 * If the current capacity is 0, allocates for `MP_HASH_TABLE_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * `data` becomes NULL if allocation failed.
 * `offset` must be POSITIVE.
 * Recalculates the positions of every entry if resized.
 *
 * ht: StrHashTable* (NO SIDE EFFECTS)
 * offset: size_t */
#define mp_ht_grow(ht, offset)                                                                     \
    do {                                                                                           \
        size_t __off = (offset);                                                                   \
        if ((ht)->len + __off > (size_t) ((double) (ht)->cap * MP_HASH_TABLE_MAX_LOAD) &&          \
            __off > 0) {                                                                           \
            size_t __old_cap = (ht)->cap;                                                          \
            if ((ht)->cap == 0) (ht)->cap = MP_HASH_TABLE_INIT_CAPACITY;                           \
            while ((ht)->len + __off > (size_t) ((double) (ht)->cap * MP_HASH_TABLE_MAX_LOAD))     \
                (ht)->cap *= 2;                                                                    \
            __typeof__((ht)->data) __new_data =                                                    \
                mp_alloc((ht)->alloc, (ht)->cap * sizeof(*(ht)->data));                            \
            for (size_t __i = 0; __i < __old_cap; ++__i) {                                         \
                if (mp_str_is_valid(&(ht)->data[__i].key)) {                                       \
                    uint64_t __hash  = mp_ht_hash_str(&(ht)->data[__i].key);                       \
                    size_t   __new_i = (size_t) (__hash % (uint64_t) ((ht)->cap - 1));             \
                    for (;;) {                                                                     \
                        if (!mp_str_is_valid(&__new_data[__new_i].key)) {                          \
                            __new_data[__new_i].key =                                              \
                                mp_str_clone(&(ht)->data[__i].key, (ht)->alloc);                   \
                            __new_data[__new_i].val = (ht)->data[__i].val;                         \
                            break;                                                                 \
                        } else {                                                                   \
                            ++__new_i;                                                             \
                        }                                                                          \
                        if (__new_i >= (ht)->cap) __new_i = 0;                                     \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
            __mp_ht_free_entries((ht)->data, (ht)->alloc, __old_cap);                              \
            mp_free((ht)->alloc, (ht)->data, __old_cap * sizeof(*(ht)->data));                     \
            (ht)->data = __new_data;                                                               \
        }                                                                                          \
        if ((ht)->data != NULL) (ht)->len += __off;                                                \
    } while (0)

#define __mp_ht_free_entries(entries, alloc, cap)                                                  \
    do {                                                                                           \
        for (size_t __i = 0; __i < (cap); ++__i) {                                                 \
            if (mp_str_is_valid(&(entries)[__i].key)) {                                            \
                mp_str_deinit(&(entries)[__i].key, (alloc));                                       \
                MEMPLUS_ASSERT(!mp_str_is_valid(&(entries)[__i].key));                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/* Sets the length of a string hash table to 0 and frees its keys.
 *
 * ht: StrHashTable* */
#define mp_ht_reset(ht)                                                                            \
    do {                                                                                           \
        __mp_ht_free_entries((ht)->data, (ht)->alloc, (ht)->cap);                                  \
        mp_da_reset(ht);                                                                           \
    } while (0)

/* Deletes an item at key `k`.
 * This does not shrink the hash table, but it just marks the spot as "deleted", which may be
 * overridden by subsequent sets.
 *
 * ht: StrHashTable*
 * k: const char* */
#define mp_ht_delete(ht, k) mp_ht_delete_s((ht), &mp_str(k))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: const StrHashTable*
 * k: mp_Str* */
#define mp_ht_delete_s(ht, k)                                                                      \
    do {                                                                                           \
        if ((k) != NULL) {                                                                         \
            mp_Str   __key  = *(k);                                                                \
            uint64_t __hash = mp_ht_hash_str(&__key);                                              \
            size_t   __i    = (size_t) (__hash % (uint64_t) ((ht)->cap - 1));                      \
            while (__i < (ht)->cap && mp_str_is_valid(&(ht)->data[__i].key)) {                     \
                if (strcmp(__key.cstr, (ht)->data[__i].key.cstr) == 0) {                           \
                    mp_str_deinit(&(ht)->data[__i].key, (ht)->alloc);                              \
                    MEMPLUS_ASSERT(!mp_str_is_valid(&(ht)->data[__i].key));                        \
                    memset(&(ht)->data[__i].val, 1, 1);                                            \
                    break;                                                                         \
                }                                                                                  \
                ++__i;                                                                             \
                if (__i >= (ht)->cap) __i = 0;                                                     \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/* Clones a string hash table to `dest` to be managed by `allocator`.
 * `dest` inherits all fields of `src`.
 * `dest.data` becomes NULL if allocation failed.
 *
 * allocator: mp_Alloc (NO SIDE EFFECTS)
 * src: StrHashTable* (NO SIDE EFFECTS)
 * dest: StrHashTable* (NO SIDE EFFECTS) */
#define mp_ht_clone(allocator, src, dest)                                                          \
    do {                                                                                           \
        (dest)->data = mp_dup((allocator), (src)->data, (src)->cap * sizeof(*(src)->data));        \
        if ((dest)->data != NULL) {                                                                \
            (dest)->alloc = (allocator);                                                           \
            (dest)->len   = (src)->len;                                                            \
            (dest)->cap   = (src)->cap;                                                            \
            for (size_t __i = 0; __i < (src)->cap; ++__i) {                                        \
                if (mp_str_is_valid(&(src)->data[__i].key)) {                                      \
                    (dest)->data[__i].key = mp_str_clone(&(src)->data[__i].key, (allocator));      \
                    MEMPLUS_ASSERT((dest)->data[__i].key.cstr != (src)->data[__i].key.cstr);       \
                }                                                                                  \
            }                                                                                      \
        } else {                                                                                   \
            (dest)->alloc = mp_alloc_invalid();                                                    \
            (dest)->len   = 0;                                                                     \
            (dest)->cap   = 0;                                                                     \
        }                                                                                          \
    } while (0)

/* Initializes an iterator on a string hash table.
 * To use the iterator, see `mp_ht_create`.
 *
 * it: StrHashTableIter* (NO SIDE EFFECTS)
 * ht: const StrHashTable* */
#define mp_ht_iter_init(it, ht)                                                                    \
    do {                                                                                           \
        __MP_ZERO(it);                                                                             \
        (it)->_h = (ht);                                                                           \
        mp_ht_iter_next(it);                                                                       \
    } while (0)

/* Get next element in a string hash table iterator.
 * To use the iterator, see `mp_ht_create`.
 *
 * it: StrHashTableIter* (NO SIDE EFFECTS) */
#define mp_ht_iter_next(it)                                                                        \
    do {                                                                                           \
        (it)->ok = false;                                                                          \
        while ((it)->_i < (it)->_h->cap) {                                                         \
            __typeof__((it)->_h->data) __entry = (it)->_h->data + (it)->_i;                        \
            if (mp_str_is_valid(&__entry->key)) {                                                  \
                (it)->key = __entry->key;                                                          \
                (it)->val = __entry->val;                                                          \
                (it)->ok  = true;                                                                  \
                ++(it)->_i;                                                                        \
                break;                                                                             \
            }                                                                                      \
            ++(it)->_i;                                                                            \
        }                                                                                          \
    } while (0)

/* Hashes a string with FNV-1a hash algorithm. */
uint64_t mp_ht_hash_str(const mp_Str *str);

/***********
 * HASH TABLE (INTEGER KEY)
 ***********/

/* Defines a hash table struct with integer key and value of type `value_type`.
 * See `mp_ht_create`.
 *
 * value_type: typename
 * name: identifier */
#define mp_hti_create(value_type, name)                                                            \
    typedef struct {                                                                               \
        __mp_IntHtKey key;                                                                         \
        value_type    val;                                                                         \
    } __##name##Entry;                                                                             \
    mp_da_create(__##name##Entry, name);                                                           \
    typedef struct {                                                                               \
        __mp_IntHtKey key;                                                                         \
        value_type    val;                                                                         \
        bool          ok;                                                                          \
        const name   *_h;                                                                          \
        size_t        _i;                                                                          \
    } name##Iter

/* The key type is wrapped by this struct so we can have 0 as a key. */
typedef struct {
    size_t key;
    bool   valid;
} __mp_IntHtKey;

/* Initializes a new integer hash table managed by `allocator`.
 * Deinit with `mp_hti_deinit`.
 *
 * ht: IntHashTable* (NO SIDE EFFECTS)
 * allocator: mp_Alloc */
#define mp_hti_init(ht, allocator) mp_da_init(ht, allocator)

/* Frees an integer hash table.
 *
 * ht: IntHashTable* (NO SIDE EFFECTS) */
#define mp_hti_deinit(ht) mp_da_deinit(ht)

/* Gets a pointer to an item with key `k` and put it into `res`.
 * `res` becomes NULL if it could not retrieve the item.
 *
 * ht: const IntHashTable*
 * k: size_t
 * ret: <value type>* */
#define mp_hti_get(ht, k, ret)                                                                     \
    do {                                                                                           \
        bool   __found = false;                                                                    \
        size_t __i     = (size_t) ((k) % (uint64_t) ((ht)->cap - 1));                              \
        while (__i < (ht)->cap && (((ht)->data[__i].key.valid) || (ht)->data[__i].key.key == 1)) { \
            if ((ht)->data[__i].key.valid && (k) == (ht)->data[__i].key.key) {                     \
                (ret)   = &(ht)->data[__i].val;                                                    \
                __found = true;                                                                    \
                break;                                                                             \
            }                                                                                      \
            ++__i;                                                                                 \
            if (__i >= (ht)->cap) __i = 0;                                                         \
        }                                                                                          \
        if (!__found) (ret) = NULL;                                                                \
    } while (0)

/* Sets the value at key `k` to `v`.
 * `data` becomes NULL if allocation failed.
 *
 * ht: IntHashTable*
 * k: size_t
 * res: <value type>* */
#define mp_hti_set(ht, k, v)                                                                       \
    do {                                                                                           \
        mp_hti_grow((ht), 1);                                                                      \
        if ((ht)->data != NULL) {                                                                  \
            size_t __i = (size_t) ((k) % (uint64_t) ((ht)->cap - 1));                              \
            for (;;) {                                                                             \
                if (!(ht)->data[__i].key.valid) {                                                  \
                    (ht)->data[__i].key = (__mp_IntHtKey) {                                        \
                        .key   = (k),                                                              \
                        .valid = true,                                                             \
                    };                                                                             \
                    (ht)->data[__i].val = (v);                                                     \
                    break;                                                                         \
                } else if ((ht)->data[__i].key.key == (k)) {                                       \
                    (ht)->data[__i].val = (v);                                                     \
                    --(ht)->len;                                                                   \
                    break;                                                                         \
                } else {                                                                           \
                    ++__i;                                                                         \
                }                                                                                  \
                if (__i >= (ht)->cap) __i = 0;                                                     \
            }                                                                                      \
        }                                                                                          \
    } while (0)

/* Resizes an integer hash table to `offset` of the current `len`.
 * If the current capacity is 0, allocates for `MP_HASH_TABLE_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * `data` becomes NULL if allocation failed.
 * `offset` must be POSITIVE.
 * Recalculates the positions of every entry if resized.
 *
 * ht: IntHashTable* (NO SIDE EFFECTS)
 * offset: size_t */
#define mp_hti_grow(ht, offset)                                                                    \
    do {                                                                                           \
        size_t __off = (offset);                                                                   \
        if ((ht)->len + __off > (size_t) ((double) (ht)->cap * MP_HASH_TABLE_MAX_LOAD) &&          \
            __off > 0) {                                                                           \
            size_t __old_cap = (ht)->cap;                                                          \
            if ((ht)->cap == 0) (ht)->cap = MP_HASH_TABLE_INIT_CAPACITY;                           \
            while ((ht)->len + __off > (size_t) ((double) (ht)->cap * MP_HASH_TABLE_MAX_LOAD))     \
                (ht)->cap *= 2;                                                                    \
            __typeof__((ht)->data) __new_data =                                                    \
                mp_alloc((ht)->alloc, (ht)->cap * sizeof(*(ht)->data));                            \
            for (size_t __i = 0; __i < __old_cap; ++__i) {                                         \
                if ((ht)->data[__i].key.valid) {                                                   \
                    size_t __new_i =                                                               \
                        (size_t) ((ht)->data[__i].key.key % (uint64_t) ((ht)->cap - 1));           \
                    for (;;) {                                                                     \
                        if (!__new_data[__new_i].key.valid) {                                      \
                            __new_data[__new_i].key = (ht)->data[__i].key;                         \
                            __new_data[__new_i].val = (ht)->data[__i].val;                         \
                            break;                                                                 \
                        } else {                                                                   \
                            ++__new_i;                                                             \
                        }                                                                          \
                        if (__new_i >= (ht)->cap) __new_i = 0;                                     \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
            mp_free((ht)->alloc, (ht)->data, __old_cap * sizeof(*(ht)->data));                     \
            (ht)->data = __new_data;                                                               \
        }                                                                                          \
        if ((ht)->data != NULL) (ht)->len += __off;                                                \
    } while (0)

/* Sets the length of an integer hash table to 0 and invalidates its keys.
 *
 * ht: IntHashTable* */
#define mp_hti_reset(ht)                                                                           \
    do {                                                                                           \
        for (size_t __i = 0; __i < (ht)->cap; ++__i) {                                             \
            if (&(ht)->data[__i].key.valid) {                                                      \
                __MP_ZERO(&(ht)->data[__i].key);                                                   \
            }                                                                                      \
        }                                                                                          \
        mp_da_reset(ht);                                                                           \
    } while (0)

/* Deletes an item at key `k`.
 * This does not shrink the hash table, but it just marks the spot as "deleted", which may be
 * overridden by subsequent sets.
 *
 * ht: IntHashTable*
 * k: const char* */
#define mp_hti_delete(ht, k)                                                                       \
    do {                                                                                           \
        size_t __i = (size_t) ((k) % (uint64_t) ((ht)->cap - 1));                                  \
        while (__i < (ht)->cap && (ht)->data[__i].key.valid) {                                     \
            if ((k) == (ht)->data[__i].key.key) {                                                  \
                (ht)->data[__i].key.valid = false;                                                 \
                (ht)->data[__i].key.key   = 1;                                                     \
                break;                                                                             \
            }                                                                                      \
            ++__i;                                                                                 \
            if (__i >= (ht)->cap) __i = 0;                                                         \
        }                                                                                          \
    } while (0)

/* Clones an integer hash table to `dest` to be managed by `allocator`.
 * `dest` inherits all fields of `src`.
 * `dest.data` becomes NULL if allocation failed.
 *
 * allocator: mp_Alloc (NO SIDE EFFECTS)
 * src: IntHashTable* (NO SIDE EFFECTS)
 * dest: IntHashTable* (NO SIDE EFFECTS) */
#define mp_hti_clone(allocator, src, dest)                                                         \
    do {                                                                                           \
        (dest)->data = mp_dup((allocator), (src)->data, (src)->cap * sizeof(*(src)->data));        \
        if ((dest)->data != NULL) {                                                                \
            (dest)->alloc = (allocator);                                                           \
            (dest)->len   = (src)->len;                                                            \
            (dest)->cap   = (src)->cap;                                                            \
        } else {                                                                                   \
            (dest)->alloc = mp_alloc_invalid();                                                    \
            (dest)->len   = 0;                                                                     \
            (dest)->cap   = 0;                                                                     \
        }                                                                                          \
    } while (0)

/* Initializes an iterator on an integer hash table.
 * To use the iterator, see `mp_ht_create`.
 *
 * it: IntHashTableIter* (NO SIDE EFFECTS)
 * ht: const IntHashTable* */
#define mp_hti_iter_init(it, ht)                                                                   \
    do {                                                                                           \
        __MP_ZERO(it);                                                                             \
        (it)->_h = (ht);                                                                           \
        mp_hti_iter_next(it);                                                                      \
    } while (0)

/* Get next element in an integer hash table iterator.
 * To use the iterator, see `mp_ht_create`.
 *
 * it: IntHashTableIter* (NO SIDE EFFECTS) */
#define mp_hti_iter_next(it)                                                                       \
    do {                                                                                           \
        (it)->ok = false;                                                                          \
        while ((it)->_i < (it)->_h->cap) {                                                         \
            __typeof__((it)->_h->data) __entry = (it)->_h->data + (it)->_i;                        \
            if (__entry->key.valid) {                                                              \
                (it)->key = __entry->key;                                                          \
                (it)->val = __entry->val;                                                          \
                (it)->ok  = true;                                                                  \
                ++(it)->_i;                                                                        \
                break;                                                                             \
            }                                                                                      \
            ++(it)->_i;                                                                            \
        }                                                                                          \
    } while (0)

/***********
 * ERRORS
 ***********/

/* Errors from errno.
 *
 * Error names for POSIX & Linux taken from Linux manpage `errno(3)`.
 * Error names for Windows taken from
 * https://learn.microsoft.com/en-us/cpp/c-runtime-library/errno-constants
 *
 * For error messages see `mp_err_str()`.
 *
 * Don't forget `mp_err()` and `mp_err_str()`! */
// Sort this!
typedef enum {
    MP_ERR_NONE    = 0,
    MP_ERR_UNKNOWN = 1,

    MP_ERR_INVALID_WIDE_CHAR,    // EILSEQ
    MP_ERR_OUT_OF_DOMAIN,        // EDOM
    MP_ERR_RESULT_TOO_LARGE,     // ERANGE

#if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS)
    MP_ERR_ADDR_IN_USE,                 // EADDRINUSE
    MP_ERR_ADDR_UNAVAILABLE,            // EADDRNOTAVAIL
    MP_ERR_AF_NOT_SUPPORTED,            // EAFNOSUPPORT
    MP_ERR_ARG_TOO_LONG,                // E2BIG
    MP_ERR_BAD_ADDR,                    // EFAULT
    MP_ERR_BAD_FD,                      // EBADF
    MP_ERR_BAD_MSG,                     // EBADMSG
    MP_ERR_BROKEN_PIPE,                 // EPIPE
    MP_ERR_BUSY,                        // EBUSY
    MP_ERR_CANCELED,                    // ECANCELED
    MP_ERR_CONNECTION_ABORTED,          // ECONNABORTED
    MP_ERR_CONNECTION_IN_PROGRESS,      // EALREADY
    MP_ERR_CONNECTION_REFUSED,          // ECONNREFUSED
    MP_ERR_CONNECTION_RESET,            // ECONNRESET
    MP_ERR_CONNECTION_TIMED_OUT,        // ETIMEDOUT
    MP_ERR_DEST_ADDR_REQUIRED,          // EDESTADDRREQ
    MP_ERR_DIR_NOT_EMPTY,               // ENOTEMPTY
    MP_ERR_EXEC_FORMAT_ERR,             // ENOEXEC
    MP_ERR_FILE_EXISTS,                 // EEXIST
    MP_ERR_FILENAME_TOO_LONG,           // ENAMETOOLONG
    MP_ERR_FILE_TOO_LARGE,              // EFBIG
    MP_ERR_FUNCTION_UNIMPLEMENTED,      // ENOSYS
    MP_ERR_HOST_IS_UNREACHABLE,         // EHOSTUNREACH
    MP_ERR_ID_REMOVED,                  // EIDRM
    MP_ERR_INAPPROPRIATE_IO_CONTROL,    // ENOTTY
    MP_ERR_IN_PROGRESS,                 // EINPROGRESS
    MP_ERR_INTERRUPTED_CALL,            // EINTR
    MP_ERR_INVALID_ARG,                 // EINVAL
    MP_ERR_INVALID_CROSSDEVICE_LINK,    // EXDEV
    MP_ERR_INVALID_SEEK,                // ESPIPE
    MP_ERR_IO_ERR,                      // EIO
    MP_ERR_IS_DIR,                      // EISDIR
    MP_ERR_LINK_SEVERED,                // ENOLINK
    MP_ERR_LOCK_UNAVAILABLE,            // ENOLCK
    MP_ERR_MESSAGE_TOO_LONG,            // EMSGSIZE
    MP_ERR_NET_CONNECTION_ABORTED,      // ENETRESET
    MP_ERR_NET_IS_DOWN,                 // ENETDOWN
    MP_ERR_NET_UNREACHABLE,             // ENETUNREACH
    MP_ERR_NO_BUFFER_SPACE,             // ENOBUFS
    MP_ERR_NO_CHILD,                    // ECHILD
    MP_ERR_NO_DEVICE,                   // ENODEV
    MP_ERR_NO_DEVICE_OR_ADDR,           // ENXIO
    MP_ERR_NO_FILE_OR_DIR,              // ENOENT
    MP_ERR_NO_MSG_OF_DESIRED_TYPE,      // ENOMSG
    MP_ERR_NO_PROCESS,                  // ESRCH
    MP_ERR_NO_SPACE_LEFT,               // ENOSPC
    MP_ERR_NO_STREAM_RESOURCES,         // ENOSR
    MP_ERR_NOT_DIR,                     // ENOTDIR
    MP_ERR_NOT_ENOUGH_MEM,              // ENOMEM
    MP_ERR_NOT_PERMITTED,               // EPERM
    MP_ERR_NOT_SOCKET,                  // ENOTSOCK
    MP_ERR_NOT_STREAM,                  // ENOSTR
    MP_ERR_NOT_SUPPORTED,               // ENOTSUP
    MP_ERR_NOT_SUPPORTED_ON_SOCKET,     // EOPNOTSUPP
    MP_ERR_OWNER_DIED,                  // EOWNERDEAD
    MP_ERR_PERM_DENIED,                 // EACCES
    MP_ERR_PROTOCOL_ERR,                // EPROTO
    MP_ERR_PROTOCOL_NOT_SUPPORTED,      // EPROTONOSUPPORT
    MP_ERR_PROTOCOL_UNAVAILABLE,        // ENOPROTOOPT
    MP_ERR_READ_ONLY_FILESYSTEM,        // EROFS
    MP_ERR_RESOURCE_DEADLOCK,           // EDEADLK
    MP_ERR_SOCKET_IS_CONNECTED,         // EISCONN
    MP_ERR_SOCKET_NOT_CONNECTED,        // ENOTCONN
    MP_ERR_STATE_UNRECOVERABLE,         // ENOTRECOVERABLE
    MP_ERR_SYMLINK_TOO_DEEP,            // ELOOP
    MP_ERR_TEMPORARILY_UNAVAILABLE,     // EAGAIN
    MP_ERR_TEXT_FILE_BUSY,              // ETXTBSY
    MP_ERR_TIMER_EXPIRED,               // ETIME
    MP_ERR_TOO_MANY_LINKS,              // EMLINK
    MP_ERR_TOO_MANY_OPEN_FILES,         // EMFILE
    MP_ERR_TOO_MANY_OPEN_FILES_SYS,     // ENFILE
    MP_ERR_VALUE_OVERFLOW,              // EOVERFLOW
    MP_ERR_WOULD_BLOCK,                 // EWOULDBLOCK
    MP_ERR_WRONG_PROTOCOL_TYPE,         // EPROTOTYPE

#if defined(__MP_SYSTEM_LINUX)
    MP_ERR_CANNOT_ACCESS_ATTRIB,    // ENODATA
    MP_ERR_RESOURCE_DEADLOCK2,      // EDEADLOCK

#endif /* if defined(__MP_SYSTEM_LINUX) */

#endif /* if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS) */

#if defined(__MP_SYSTEM_POSIX)
    MP_ERR_DISK_QUOTA_EXCEEDED,    // EDQUOT
    MP_ERR_MULTIHOP_ATTEMPTED,     // EMULTIHOP
    MP_ERR_STALE_FILE_HANDLE,      // ESTALE

#if defined(__MP_SYSTEM_LINUX)
    MP_ERR_ACCESS_CORRUPT_LIB,          // ELIBBAD
    MP_ERR_ACCESS_TOO_MANY_LIBS,        // ELIBMAX
    MP_ERR_BLOCK_DEVICE_REQUIRED,       // ENOTBLK
    MP_ERR_CANNOT_ACCESS_LIB,           // ELIBACC
    MP_ERR_CANNOT_EXEC_LIB,             // ELIBEXEC
    MP_ERR_CHANNEL_NUM_OUT_OF_RANGE,    // ECHRNG
    MP_ERR_EXCHANGE_FULL,               // EXFULL
    MP_ERR_HOST_IS_DOWN,                // EHOSTDOWN
    MP_ERR_INTERRUPTED_SYSCALL,         // ERESTART
    MP_ERR_INVALID_EXCHANGE,            // EBADE
    MP_ERR_INVALID_FD,                  // EBADFD
    MP_ERR_INVALID_REQUEST_CODE,        // EBADRQC
    MP_ERR_INVALID_REQUEST,             // EBADR
    MP_ERR_INVALID_SLOT,                // EBADSLT
    MP_ERR_IS_NAMED_TYPE_FILE,          // EISNAM
    MP_ERR_KEY_EXPIRED,                 // EKEYEXPIRED
    MP_ERR_KEY_REJECTED,                // EKEYREJECTED
    MP_ERR_KEY_REVOKED,                 // EKEYREVOKED
    MP_ERR_KEY_UNAVAILABLE,             // ENOKEY
    MP_ERR_LEVEL_2_HALTED,              // EL2HLT
    MP_ERR_LEVEL_2_NOT_SYNC,            // EL2NSYNC
    MP_ERR_LEVEL_3_HALTED,              // EL3HLT
    MP_ERR_LEVEL_3_RESET,               // EL3RST
    MP_ERR_LIB_SECTION_CORRUPT,         // ELIBSCN
    MP_ERR_LINK_NUM_OUT_OF_RANGE,       // ELNRNG
    MP_ERR_MEM_PAGE_HARDWARE_ERR,       // EHWPOISON
    MP_ERR_NAME_NOT_UNIQUE,             // ENOTUNIQ
    MP_ERR_NO_ANODE,                    // ENOANO
    MP_ERR_NO_MEDIUM_FOUND,             // ENOMEDIUM
    MP_ERR_NOT_ON_NETWORK,              // ENONET
    MP_ERR_NOT_POSSIBLE_BY_RFKILL,      // ERFKILL
    MP_ERR_OBJECT_IS_REMOTE,            // EREMOTE
    MP_ERR_PACKAGE_NOT_INSTALLED,       // ENOPKG
    MP_ERR_PROTO_DRIVER_UNATTACHED,     // EUNATCH
    MP_ERR_PROTO_FAMILY_UNSUPPORTED,    // EPFNOSUPPORT
    MP_ERR_REMOTE_ADDR_CHANGED,         // EREMCHG
    MP_ERR_REMOTE_IO_ERR,               // EREMOTEIO
    MP_ERR_SEND_AFTER_SHUTDOWN,         // ESHUTDOWN
    MP_ERR_SEND_COMM_ERR,               // ECOMM
    MP_ERR_SOCKET_TYPE_UNSUPPORTED,     // ESOCKTNOSUPPORT
    MP_ERR_STREAM_PIPE_ERR,             // ESTRPIPE
    MP_ERR_STRUCT_NEED_CLEANING,        // EUCLEAN
    MP_ERR_TOO_MANY_REFERENCES,         // ETOOMANYREFS
    MP_ERR_TOO_MANY_USERS,              // EUSERS
    MP_ERR_WRONG_MEDIUM_TYPE,           // EMEDIUMTYPE

#endif /* if defined(__MP_SYSTEM_LINUX) */

#elif defined(__MP_SYSTEM_WINDOWS)
    MP_ERR_OTHER,               // EOTHER
    MP_ERR_TRUNCATED_STRING,    // STRUNCATE

#else
#error "Unimplemented"

#endif /* if defined(__MP_SYSTEM_POSIX) */

    __MP_ERR_COUNT,
} mp_Err;

/* Converts errno into `mp_Err`. */
mp_Err mp_err(int errnum);
/* Returns the message of an error. */
const char *mp_err_str(mp_Err e);

/***********
 * IO INTERFACE
 ***********/

/* See `mp_IoFunc` below. */
typedef enum {
    MP_IOOP_FLUSH,
    MP_IOOP_SETBUF,
    MP_IOOP_READ,    // ret < count, if successful, it is EOF
    MP_IOOP_WRITE,
    MP_IOOP_GETPOS,
    MP_IOOP_SETPOS,
    MP_IOOP_GETC,
    MP_IOOP_PUTC,
    __MP_IOOP_COUNT,
} mp_IoOp;

/* Errors that may occur when using IO functions.
 *
 * For error messages see `mp_ioerr_str()`.
 * */
typedef enum {
    MP_IOERR_NONE = 0,
    MP_IOERR_UNSUPPORTED,
    MP_IOERR_EOF,
    MP_IOERR_CANNOT_FLUSH,
    MP_IOERR_CANNOT_SET_BUF,
    MP_IOERR_CANNOT_READ,
    MP_IOERR_CANNOT_WRITE,
    MP_IOERR_CANNOT_GET_POS,
    MP_IOERR_CANNOT_SET_POS,
    __MP_IOERR_COUNT,
} mp_IoErr;

/* Returns the message of an error. */
const char *mp_ioerr_str(mp_IoErr e);

/* Modes given to `setvbuf()`.  */
typedef enum {
    MP_SETBUFMODE_NONE,    // no buffering (_IONBF)
    MP_SETBUFMODE_FULL,    // full buffering (_IOFBF)
    MP_SETBUFMODE_LINE,    // line buffering (_IOLBF)
} mp_SetbufMode;

/* Position from which to apply the offset of the seek. */
typedef enum {
    MP_SETPOSORIGIN_START,      // SEEK_SET
    MP_SETPOSORIGIN_CURRENT,    // SEEK_CUR
    MP_SETPOSORIGIN_END,        // SEEK_END
} mp_SetposOrigin;

/* The type of a stream. Stream of a certain type may only call certain functions.
 * A stream may be both read and write.
 * If a stream calls to a function outside of its domain, MP_IOERR_UNSUPPORTED will be returned.
 * MP_IOTYPE_NONE is only used for invalid `mp_Io` object. */
typedef enum {
    MP_IOTYPE_NONE  = 0,
    MP_IOTYPE_READ  = 1 << 0,
    MP_IOTYPE_WRITE = 1 << 1,
} mp_IoType;

typedef struct mp_Io mp_Io;

/* Functions of this type do different things and utilize the parameters differently depending on
 * the `op` given.
 *
 * Returns `mp_IoErr` type. MP_IOERR_NONE if successful.
 * Not all operations can be done to all streams.
 * If the operation does not allow to be done on the type it will return MP_IOERR_UNSUPPORTED.
 *
 * Operations:
 * - MP_IOOP_FLUSH: Flushes the stream
 *   For MP_IOTYPE_WRITE.
 *   For output streams, writes unwritten data from buffer to the output device.
 *      - `io`: The IO object
 *      - ignores other parameters
 *  - MP_IOOP_SETBUF: Changes the buffering mode of a stream.
 *    For MP_IOTYPE_WRITE and MP_IOTYPE_READ.
 *    Changes the buffering mode or/and the size of the internal buffer.
 *    Can also instruct the stream to use use-provided buffer if `ptr` != NULL.
 *    The stream must be closed before the lifetime of the buffer ends.
 *      - `io`: The IO object
 *      - `ptr`: The buffer to use (if NULL, only resizes the existing buffer to `n1`)
 *      - `n1`: Size of the buffer
 *      - `n2`: `mp_SetbufMode`
 *      - ignores other parameters
 *  - MP_IOOP_READ: Reads objects into given buffer from the stream.
 *    For MP_IOTYPE_READ.
 *    If an error or EOF occurs, `ret` may be less than `n2` and returns MP_IOERR_CANNOT_READ or
 *    MP_IOERR_EOF respsectively.
 *    If `n1` or `n2` is zero, does nothing and `ret` will be set to zero.
 *      - `io`: The IO object
 *      - `ptr`: The buffer which the data will be stored
 *      - `n1`: Size of each object (in bytes)
 *      - `n2`: The number of objects (the total size of the data will be `n1 * n2`)
 *      - `ret`: Stores the number of objects read successfully
 *  - MP_IOOP_WRITE: Writes objects from given buffer to the stream.
 *    For MP_IOTYPE_WRITE.
 *    If an error occurs, `ret` may be less than `n2` and returns MP_IOERR_CANNOT_WRITE.
 *    If `n1` or `n2` is zero, does nothing and `ret` will be set to zero.
 *      - `io`: The IO object
 *      - `ptr`: The buffer of the data to be written
 *      - `n1`: Size of each object (in bytes)
 *      - `n2`: The number of objects (the total size of the data will be `n1 * n2`)
 *      - `ret`: Stores the number of objects written successfully
 *  - MP_IOOP_GETPOS: Gets the file position indicator of a stream.
 *    For MP_IOTYPE_WRITE and MP_IOTYPE_READ.
 *      - `io`: The IO object
 *      - `ret`: Stores the file position indicator (in bytes)
 *      - ignores other parameters
 *  - MP_IOOP_SETPOS: Sets the file position indicator of a stream.
 *    For MP_IOTYPE_WRITE and MP_IOTYPE_READ.
 *    Binary streams may not support MP_SETPOSORIGIN_END or SEEK_END.
 *    For text streams, offset may only be zero or the result of earlier `MP_IOOP_GETPOS` (for
 *    MP_SETPOSORIGIN_START or SEEK_SET only).
 *    For wide-oriented streams, the restrictions of both binary and text streams apply.
 *      - `io`: The IO object
 *      - `n1`: The offset (in bytes)
 *      - `n2`: `mp_SetposOrigin`
 *      - ignores other parameters
 *  - MP_IOOP_GETC: Reads the next character from a stream.
 *    For MP_IOTYPE_READ.
 *      - `io`: The IO object
 *      - `ret`: Stores the character (actually `unsigned char`)
 *      - ignores other parameters
 *  - MP_IOOP_PUTC: Writes a character to a stream.
 *    For MP_IOTYPE_WRITE.
 *      - `io`: The IO object
 *      - `n1`: The character (converted to `unsigned char` before write)
 *      - ignores other parameters
 * */
typedef mp_IoErr (*mp_IoFunc)(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2, size_t *ret);

/* Interface to wrap IO functions.
 * The IO functions can be customized. */
struct mp_Io {
    // The object which contains the underlying data about the stream.
    // This can be specified as NULL if context is not needed.
    void *context;
    // See `mp_IoType`.
    mp_IoType type;

    // See `mp_IoFunc`.
    mp_IoFunc f;
};

/* Returns an invalid `mp_Io`.
 * Invalid `mp_Io` requires that field `type` is MP_IOTYPE_NONE. */
#define mp_io_invalid()                                                                            \
    ((mp_Io) {                                                                                     \
        .context = NULL,                                                                           \
        .type    = MP_IOTYPE_NONE,                                                                 \
        .f       = NULL,                                                                           \
    })

/* Tests if `io` is invalid.
 * Returns true if valid.
 *
 * io: const mp_Io* */
#define mp_io_is_valid(io) ((io)->type != MP_IOTYPE_NONE)

/* Macros that wrap the functions above.
 * See `mp_IoFunc` for details. */

/* io: mp_Io* (NO SIDE EFFECTS)
 * Returns mp_IoErr */
#define mp_io_flush(io) ((io)->f(MP_IOOP_FLUSH, (io), NULL, 0, 0, NULL))
/* io: mp_Io* (NO SIDE EFFECTS)
 * buf: char*
 * bufsize: size_t
 * mode: `mp_SetbufMode`
 * Returns mp_IoErr */
#define mp_io_setbuf(io, buf, bufsize, mode)                                                       \
    ((io)->f(MP_IOOP_SETBUF, (io), (buf), (bufsize), (mode), NULL))
/* io: mp_Io* (NO SIDE EFFECTS)
 * buf: pointer
 * size: size_t
 * count: size_t
 * ret_n: size_t*
 * Returns mp_IoErr */
#define mp_io_read(io, buf, size, count, ret_n)                                                    \
    ((io)->f(MP_IOOP_READ, (io), (buf), (size), (count), (ret_n)))
/* io: mp_Io* (NO SIDE EFFECTS)
 * buf: pointer (may be const)
 * size: size_t
 * count: size_t
 * ret_n: size_t*
 * Returns mp_IoErr */
#define mp_io_write(io, buf, size, count, ret_n)                                                   \
    ((io)->f(MP_IOOP_WRITE, (io), (void *) (buf), (size), (count), (ret_n)))
/* io: mp_Io* (NO SIDE EFFECTS)
 * ret_n: size_t*
 * Returns mp_IoErr */
#define mp_io_getpos(io, ret_n) ((io)->f(MP_IOOP_GETPOS, (io), NULL, 0, 0, (ret_n)))
/* io: mp_Io* (NO SIDE EFFECTS)
 * offset: size_t
 * origin: `mp_SetposOrigin`
 * Returns mp_IoErr */
#define mp_io_setpos(io, offset, origin)                                                           \
    ((io)->f(MP_IOOP_SETPOS, (io), NULL, (offset), (origin), NULL))
/* io: mp_Io* (NO SIDE EFFECTS)
 * ret_c: char*
 * Returns mp_IoErr */
#define mp_io_getc(io, ret_c) ((io)->f(MP_IOOP_GETC, (io), NULL, 0, 0, (size_t *) (ret_c)))
/* io: mp_Io* (NO SIDE EFFECTS)
 * c: char
 * Returns mp_IoErr */
#define mp_io_putc(io, c) ((io)->f(MP_IOOP_PUTC, (io), NULL, (size_t) (c), 0, NULL))

/* Creates a custom IO object given the context and the function.
 *
 * ctx (context): pointer
 * type: mp_IoType
 * func: mp_IoFunc
 * Returns mp_Io */
#define mp_io_new(ctx, type, func)                                                                 \
    ((mp_Io) {                                                                                     \
        .context = (void *) (ctx),                                                                 \
        .type    = (type),                                                                         \
        .f       = (func),                                                                         \
    })

/***********
 * FILE IO
 ***********/

/* Context of file IO. Stores a FILE object corresponding to the open file. */
typedef struct {
    FILE     *file;
    mp_IoType supported_type;
} mp_File;

/* Opens a file at `filename`. See `fopen()` for possible `mode`.
 * Close with `mp_file_deinit`. */
mp_Err mp_file_open(mp_File *f, const char *filename, const char *mode);
/* Opens a file at `filename` and closes the old file.
 * If `filename` is NULL, changes the mode of the existing file (NOT SUPPORTED FOR ALL PLATFORMS)
 * If `f->file` is NULL, returns MP_ERR_BAD_FD. */
mp_Err mp_file_reopen(mp_File *f, const char *filename, const char *mode);
// TODO: mp_file_open_from_fd
/* Closes an open file. Does nothing if mp_File.file == NULL. */
void mp_file_deinit(mp_File *f);
/* Returns an IO object that works with `f` of given `type`.
 * The `type` may not be supported, depends on the mode when opening the file.
 * Returns an invalid `mp_Io` if failed. */
mp_Io mp_file_io(mp_File *f, mp_IoType type);

/***********
 * IMPLEMENTATION
 ***********/

#ifdef MEMPLUS_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define __MP_UNREACHABLE()      MEMPLUS_ASSERT_MSG(0, "Unreachable")
#define __MP_TODO(msg)          MEMPLUS_ASSERT_MSG(0, "todo: " msg)
#define __MP_DIV_ROUNDUP(a, b)  (((a) + (b) - 1) / (b))
#define __MP_ALIGN(a, inc)      (__MP_DIV_ROUNDUP((a), (inc)) * (inc))
#define __MP_ALIGN_DOWN(a, inc) (((a) / (inc)) * (inc))
#define __MP_MAX(a, b)          ((a) > (b) ? (a) : (b))
#define __MP_MIN(a, b)          ((a) < (b) ? (a) : (b))
#define __MP_ASSERT_OVERLAP(a, a_len, b, b_len)                                                    \
    do {                                                                                           \
        auto _a = (uintptr_t) a;                                                                   \
        auto _b = (uintptr_t) b;                                                                   \
        if (__MP_MAX((_a), (_b)) < __MP_MIN((_a) + (a_len), (_b) + (b_len))) {                     \
            MEMPLUS_ASSERT_MSG(0, "Memory overlaps");                                              \
        }                                                                                          \
    } while (0)

static void *
mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);

static mp_IoErr
mp_file_io_func(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2, size_t *ret);

#ifdef __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(
    const char *assertion, const char *file, const char *func, size_t line, const char *msg) {
    fprintf(stderr, "%s:%s():%zu: [memplus] %s. `%s` failed.\n", file, func, line, msg, assertion);
    abort();
}
#endif

void *mp_dup(mp_Alloc alloc, void *data, size_t size) {
    void *buf = mp_alloc(alloc, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

void *mp_alloc_handle_realloc(mp_Alloc alloc, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        return NULL;
    }
    if (new_size <= old_size) return old_ptr;
    void *new_ptr = mp_alloc(alloc, new_size);
    if (new_ptr == NULL) return NULL;
    __MP_ASSERT_OVERLAP(old_ptr, old_size, new_ptr, new_size);
    memcpy(new_ptr, old_ptr, old_size);
    mp_free(alloc, old_ptr, old_size);
    return new_ptr;
}

mp_Region *mp_region_init(mp_Alloc alloc, size_t cap) {
    size_t     bytes  = __MP_ALIGN(cap, sizeof(uintptr_t));
    mp_Region *region = mp_alloc(alloc, bytes);
    region->next      = NULL;
    region->len       = 0;
    region->cap       = bytes;
    return region;
}

void mp_region_deinit(mp_Region *r, mp_Alloc alloc) {
    mp_free(alloc, r, r->cap);
}

void mp_arena_init_s(mp_Arena *a, mp_Alloc alloc, size_t def_size) {
    a->len       = 0;
    a->begin     = NULL;
    a->end       = NULL;
    a->alloc     = alloc;
    a->_def_size = def_size;
}

void mp_arena_reset(mp_Arena *a) {
    a->len = 0;
    for (mp_Region *region = a->begin; region; region = region->next) {
        region->len = 0;
    }
    a->end = a->begin;
}

void mp_arena_deinit(mp_Arena *a) {
    mp_Region *region = a->begin;
    while (region) {
        mp_Region *region_temp = region;
        region                 = region->next;
        mp_region_deinit(region_temp, a->alloc);
    }
    __MP_ZERO(a);
}

mp_Alloc mp_arena_alloc(mp_Arena *a) {
    return mp_alloc_new(a, mp_arena_alloc_func);
}

static void *
mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_Arena *ctx   = context;
    mp_Alloc  alloc = mp_alloc_new(ctx, mp_arena_alloc_func);

    __MP_STATIC_ASSERT(__MP_ALLOCOP_COUNT == 3);
    switch (op) {
        case MP_ALLOCOP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            size_t alloc_size = __MP_ALIGN(new_size, sizeof(uintptr_t));

            if (ctx->end == NULL) {
                MEMPLUS_ASSERT(ctx->begin == NULL);
                size_t capacity = ctx->_def_size;
                if (capacity < alloc_size) capacity = alloc_size;
                ctx->end = mp_region_init(ctx->alloc, capacity);
                if (ctx->end == NULL) return NULL;
                ctx->begin = ctx->end;
            }

            while (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap &&
                   ctx->end->next != NULL) {
                ctx->end = ctx->end->next;
            }

            if (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap) {
                MEMPLUS_ASSERT(ctx->end->next == NULL);
                size_t capacity = ctx->_def_size;
                if (capacity < alloc_size) capacity = alloc_size;
                ctx->end->next = mp_region_init(ctx->alloc, capacity);
                if (ctx->end->next == NULL) return NULL;
                ctx->end = ctx->end->next;
            }

            MEMPLUS_ASSERT(ctx->end->len % sizeof(uintptr_t) == 0);
            size_t len_words = __MP_DIV_ROUNDUP(ctx->end->len, sizeof(uintptr_t));
            void  *result    = &ctx->end->data[len_words];
            ctx->end->len += alloc_size;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCOP_REALLOC: {
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCOP_FREE: {
            (void) old_size;

            return NULL;
        } break;
        case __MP_ALLOCOP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

void mp_sarena_init(mp_SArena *a, mp_Alloc alloc, size_t cap) {
    size_t     bytes  = __MP_ALIGN(cap, sizeof(uintptr_t));
    uintptr_t *buffer = mp_alloc(alloc, bytes);
    a->alloc          = alloc;
    a->buf            = buffer;
    a->len            = 0;
    a->cap            = bytes;
}

void mp_sarena_reset(mp_SArena *a) {
    // memset(a->buf, 0, a->cap);
    a->len = 0;
}

void mp_sarena_deinit(mp_SArena *a) {
    mp_free(a->alloc, a->buf, a->cap * sizeof(*(a)->buf));
    __MP_ZERO(a);
}

mp_Alloc mp_sarena_alloc(mp_SArena *a) {
    return mp_alloc_new(a, mp_sarena_alloc_func);
}

static void *
mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_SArena *ctx   = context;
    mp_Alloc   alloc = mp_alloc_new(ctx, mp_sarena_alloc_func);

    __MP_STATIC_ASSERT(__MP_ALLOCOP_COUNT == 3);
    switch (op) {
        case MP_ALLOCOP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            size_t alloc_size = __MP_ALIGN(new_size, sizeof(uintptr_t));

            MEMPLUS_ASSERT(ctx->len % sizeof(uintptr_t) == 0);
            if (ctx->len + alloc_size > ctx->cap) return NULL;

            void *result = ctx->buf + ctx->len;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCOP_REALLOC: {
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCOP_FREE: {
            (void) old_size;

            return NULL;
        } break;
        case __MP_ALLOCOP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

void mp_temp_init(mp_Temp *t, char *buf, size_t cap) {
    memset(buf, 0, cap);
    t->buf = (uintptr_t *) buf;
    t->len = 0;
    t->cap = __MP_ALIGN_DOWN(cap, sizeof(uintptr_t));
}

void mp_temp_reset(mp_Temp *t) {
    t->len = 0;
}

mp_Alloc mp_temp_alloc(mp_Temp *t) {
    return mp_alloc_new(t, mp_sarena_alloc_func);
}

mp_Alloc mp_heap_alloc(void) {
    return mp_alloc_new(NULL, mp_heap_alloc_func);
}

static void *
mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    (void) context;
    mp_Alloc alloc = mp_alloc_new(NULL, mp_heap_alloc_func);

    __MP_STATIC_ASSERT(__MP_ALLOCOP_COUNT == 3);
    switch (op) {
        case MP_ALLOCOP_ALLOC: {
            (void) old_size;
            (void) ptr;
            if (new_size == 0) {
                return NULL;
            }
            return MEMPLUS_ALLOC(new_size);
        } break;
        case MP_ALLOCOP_REALLOC: {
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCOP_FREE: {
            (void) old_size;
            if (ptr == NULL) {
                return NULL;
            }
            // `new_size` is unused
            MEMPLUS_FREE(ptr);
            return NULL;
        } break;
        case __MP_ALLOCOP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

mp_Str mp_str_new(mp_Alloc alloc, const char *str) {
    int len = snprintf(NULL, 0, "%s", str);
    MEMPLUS_ASSERT_MSG(len >= 0, "Failed to count string length");
    return mp_str_new_len(alloc, str, (size_t) len);
}

mp_Str mp_str_new_len(mp_Alloc alloc, const char *str, size_t len) {
    char *result = mp_alloc(alloc, (size_t) (len + 1));
    if (result == NULL) return mp_str_invalid();
    int result_len = snprintf(result, (size_t) (len + 1), "%.*s", (int) len, str);
    MEMPLUS_ASSERT((size_t) result_len == len);
    return (mp_Str) { .len = (size_t) result_len, .cstr = result };
}

mp_Str mp_str_newf(mp_Alloc alloc, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    MEMPLUS_ASSERT_MSG(len >= 0, "Failed to count string length");
    va_end(args);

    char *result = mp_alloc(alloc, (size_t) (len + 1));
    if (result == NULL) return mp_str_invalid();

    va_start(args, fmt);
    int result_len = vsnprintf(result, (size_t) (len + 1), fmt, args);
    MEMPLUS_ASSERT(result_len == len);
    va_end(args);

    return (mp_Str) { .len = (size_t) result_len, .cstr = result };
}

mp_Str mp_str_clone(const mp_Str *str, mp_Alloc alloc) {
    int len = snprintf(NULL, 0, "%s", str->cstr);
    MEMPLUS_ASSERT_MSG(len >= 0 || (size_t) len != str->len, "Failed to count string length");
    char *ptr = mp_dup(alloc, str->cstr, (size_t) len + 1);
    if (ptr == NULL) return mp_str_invalid();
    return (mp_Str) { .len = (size_t) len, .cstr = ptr };
}

void mp_str_deinit(mp_Str *str, mp_Alloc alloc) {
    mp_free(alloc, str->cstr, str->len + 1);
    __MP_ZERO(str);
}

void mp_str_builder_append(mp_StrBuilder *sb, const char *str) {
    mp_da_append_array(sb, str, (size_t) strlen(str));
}

void mp_str_builder_appendf(mp_StrBuilder *sb, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    MEMPLUS_ASSERT_MSG(len >= 0, "Failed to count string length");
    va_end(args);

    size_t prev_len = sb->len;
    mp_da_grow(sb, (size_t) (len + 1));

    if (sb->data != NULL) {
        va_start(args, fmt);
        int result_len = vsnprintf(sb->data + prev_len, (size_t) (len + 1), fmt, args);
        mp_da_shrink(sb, 1ul);
        MEMPLUS_ASSERT(result_len == len);
        va_end(args);
    }
}

mp_Str mp_str_builder_string(const mp_StrBuilder *sb, mp_Alloc alloc) {
    return mp_str_new_len(alloc, sb->data, sb->len);
}

size_t mp_utf8_len(const char *str) {
    return mp_utf8_len_s(str, strlen(str));
}

size_t mp_utf8_len_s(const char *str, size_t size) {
    size_t len         = 0;
    char   bytes_taken = 0;
    for (size_t i = 0; i < size; ++i) {
        char byte = str[i];
        if (bytes_taken == 0) {
            for (size_t j = 0; j < 4; ++j) {
                char bit = (byte >> (7 - j)) & 0x1;
                if (bit == 0) {
                    break;
                } else {
                    ++bytes_taken;
                }
            }
            if (bytes_taken == 0) {
                bytes_taken = 1;
            } else if (bytes_taken == 1) {
                return MP_ERROR;
            }
        } else {
            if ((byte & 0xC0) != 0x80) {
                return MP_ERROR;
            }
        }

        --bytes_taken;
        if (bytes_taken == 0) {
            ++len;
        }
    }

    if (bytes_taken > 0) {
        return MP_ERROR;
    }

    return len;
}

mp_Utf8Iter mp_utf8_iter_new(const char *str) {
    return (mp_Utf8Iter) {
        ._str  = str,
        ._size = strlen(str),
        ._i    = 0,
    };
}

mp_Utf8Iter mp_utf8_iter_new_s(const char *str, size_t size) {
    return (mp_Utf8Iter) {
        ._str  = str,
        ._size = size,
        ._i    = 0,
    };
}

bool mp_utf8_iter_next(mp_Utf8Iter *it) {
    if (it->_i >= it->_size) {
        return false;
    }

    char byte = it->_str[it->_i];

    char bytes_taken = 0;
    for (size_t j = 0; j < 4; ++j) {
        char bit = (byte >> (7 - j)) & 0x1;
        if (bit == 0) {
            break;
        } else {
            ++bytes_taken;
        }
    }
    if (bytes_taken == 0) {
        bytes_taken = 1;
    } else if (bytes_taken == 1) {
        return false;
    }

    // We don't check if the following bytes is valid

    if (it->_i + (size_t) bytes_taken > it->_size) {
        return false;
    }

    memcpy(it->c, it->_str + it->_i, (size_t) bytes_taken);
    it->c_len = bytes_taken;

    it->_i += (size_t) bytes_taken;

    return true;
}

#define __FNV_OFFSET 14695981039346656037UL
#define __FNV_PRIME  1099511628211UL

uint64_t mp_ht_hash_str(const mp_Str *str) {
    uint64_t hash = __FNV_OFFSET;
    for (const char *p = str->cstr; *p; p++) {
        hash ^= (uint64_t) (unsigned char) (*p);
        hash *= __FNV_PRIME;
    }
    return hash;
}

mp_Err mp_err(int errnum) {
    // Sort this! (by MP_ERR_*)
    switch (errnum) {
        case 0:      return MP_ERR_NONE;
        case EILSEQ: return MP_ERR_INVALID_WIDE_CHAR;
        case EDOM:   return MP_ERR_OUT_OF_DOMAIN;
        case ERANGE: return MP_ERR_RESULT_TOO_LARGE;

#if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS)
        case EADDRINUSE:      return MP_ERR_ADDR_IN_USE;
        case EADDRNOTAVAIL:   return MP_ERR_ADDR_UNAVAILABLE;
        case EAFNOSUPPORT:    return MP_ERR_AF_NOT_SUPPORTED;
        case E2BIG:           return MP_ERR_ARG_TOO_LONG;
        case EFAULT:          return MP_ERR_BAD_ADDR;
        case EBADF:           return MP_ERR_BAD_FD;
        case EBADMSG:         return MP_ERR_BAD_MSG;
        case EPIPE:           return MP_ERR_BROKEN_PIPE;
        case EBUSY:           return MP_ERR_BUSY;
        case ECANCELED:       return MP_ERR_CANCELED;
        case ECONNABORTED:    return MP_ERR_CONNECTION_ABORTED;
        case EALREADY:        return MP_ERR_CONNECTION_IN_PROGRESS;
        case ECONNREFUSED:    return MP_ERR_CONNECTION_REFUSED;
        case ECONNRESET:      return MP_ERR_CONNECTION_RESET;
        case ETIMEDOUT:       return MP_ERR_CONNECTION_TIMED_OUT;
        case EDESTADDRREQ:    return MP_ERR_DEST_ADDR_REQUIRED;
        case ENOTEMPTY:       return MP_ERR_DIR_NOT_EMPTY;
        case ENOEXEC:         return MP_ERR_EXEC_FORMAT_ERR;
        case EEXIST:          return MP_ERR_FILE_EXISTS;
        case ENAMETOOLONG:    return MP_ERR_FILENAME_TOO_LONG;
        case EFBIG:           return MP_ERR_FILE_TOO_LARGE;
        case ENOSYS:          return MP_ERR_FUNCTION_UNIMPLEMENTED;
        case EHOSTUNREACH:    return MP_ERR_HOST_IS_UNREACHABLE;
        case EIDRM:           return MP_ERR_ID_REMOVED;
        case ENOTTY:          return MP_ERR_INAPPROPRIATE_IO_CONTROL;
        case EINPROGRESS:     return MP_ERR_IN_PROGRESS;
        case EINTR:           return MP_ERR_INTERRUPTED_CALL;
        case EXDEV:           return MP_ERR_INVALID_CROSSDEVICE_LINK;
        case ESPIPE:          return MP_ERR_INVALID_SEEK;
        case EIO:             return MP_ERR_IO_ERR;
        case EISDIR:          return MP_ERR_IS_DIR;
        case ENOLINK:         return MP_ERR_LINK_SEVERED;
        case ENOLCK:          return MP_ERR_LOCK_UNAVAILABLE;
        case EMSGSIZE:        return MP_ERR_MESSAGE_TOO_LONG;
        case ENETRESET:       return MP_ERR_NET_CONNECTION_ABORTED;
        case ENETDOWN:        return MP_ERR_NET_IS_DOWN;
        case ENETUNREACH:     return MP_ERR_NET_UNREACHABLE;
        case ENOBUFS:         return MP_ERR_NO_BUFFER_SPACE;
        case ECHILD:          return MP_ERR_NO_CHILD;
        case ENODEV:          return MP_ERR_NO_DEVICE;
        case ENXIO:           return MP_ERR_NO_DEVICE_OR_ADDR;
        case ENOENT:          return MP_ERR_NO_FILE_OR_DIR;
        case ENOMSG:          return MP_ERR_NO_MSG_OF_DESIRED_TYPE;
        case ESRCH:           return MP_ERR_NO_PROCESS;
        case ENOSPC:          return MP_ERR_NO_SPACE_LEFT;
        case ENOSR:           return MP_ERR_NO_STREAM_RESOURCES;
        case ENOTDIR:         return MP_ERR_NOT_DIR;
        case ENOMEM:          return MP_ERR_NOT_ENOUGH_MEM;
        case EPERM:           return MP_ERR_NOT_PERMITTED;
        case ENOTSOCK:        return MP_ERR_NOT_SOCKET;
        case ENOTSUP:         return MP_ERR_NOT_SUPPORTED;
        case EOWNERDEAD:      return MP_ERR_OWNER_DIED;
        case EACCES:          return MP_ERR_PERM_DENIED;
        case EPROTO:          return MP_ERR_PROTOCOL_ERR;
        case EPROTONOSUPPORT: return MP_ERR_PROTOCOL_NOT_SUPPORTED;
        case ENOPROTOOPT:     return MP_ERR_PROTOCOL_UNAVAILABLE;
        case EROFS:           return MP_ERR_READ_ONLY_FILESYSTEM;
        case EDEADLK:         return MP_ERR_RESOURCE_DEADLOCK;
        case EISCONN:         return MP_ERR_SOCKET_IS_CONNECTED;
        case ENOTCONN:        return MP_ERR_SOCKET_NOT_CONNECTED;
        case ENOTRECOVERABLE: return MP_ERR_STATE_UNRECOVERABLE;
        case ELOOP:           return MP_ERR_SYMLINK_TOO_DEEP;
        case EAGAIN:          return MP_ERR_TEMPORARILY_UNAVAILABLE;
        case ETXTBSY:         return MP_ERR_TEXT_FILE_BUSY;
        case ETIME:           return MP_ERR_TIMER_EXPIRED;
        case EMLINK:          return MP_ERR_TOO_MANY_LINKS;
        case EMFILE:          return MP_ERR_TOO_MANY_OPEN_FILES;
        case ENFILE:          return MP_ERR_TOO_MANY_OPEN_FILES_SYS;
        case EOVERFLOW:       return MP_ERR_VALUE_OVERFLOW;
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK: return MP_ERR_WOULD_BLOCK;
#endif
        case EPROTOTYPE: return MP_ERR_WRONG_PROTOCOL_TYPE;

#if defined(__MP_SYSTEM_LINUX)
        case ENODATA: return MP_ERR_CANNOT_ACCESS_ATTRIB;
#if EDEADLOCK != EDEADLK
        case EDEADLOCK: return MP_ERR_RESOURCE_DEADLOCK2;
#endif

#endif /* if defined(__MP_SYSTEM_LINUX) */

#endif /* if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS) */

#if defined(__MP_SYSTEM_POSIX)
        case EDQUOT:    return MP_ERR_DISK_QUOTA_EXCEEDED;
        case EINVAL:    return MP_ERR_INVALID_ARG;
        case EMULTIHOP: return MP_ERR_MULTIHOP_ATTEMPTED;
        case ENOSTR:    return MP_ERR_NOT_STREAM;
#if EOPNOTSUPP != ENOTSUP
        case EOPNOTSUPP: return MP_ERR_NOT_SUPPORTED_ON_SOCKET;
#endif
        case ESTALE: return MP_ERR_STALE_FILE_HANDLE;

#if defined(__MP_SYSTEM_LINUX)
        case ELIBBAD:         return MP_ERR_ACCESS_CORRUPT_LIB;
        case ELIBMAX:         return MP_ERR_ACCESS_TOO_MANY_LIBS;
        case ENOTBLK:         return MP_ERR_BLOCK_DEVICE_REQUIRED;
        case ELIBACC:         return MP_ERR_CANNOT_ACCESS_LIB;
        case ELIBEXEC:        return MP_ERR_CANNOT_EXEC_LIB;
        case ECHRNG:          return MP_ERR_CHANNEL_NUM_OUT_OF_RANGE;
        case EXFULL:          return MP_ERR_EXCHANGE_FULL;
        case EHOSTDOWN:       return MP_ERR_HOST_IS_DOWN;
        case ERESTART:        return MP_ERR_INTERRUPTED_SYSCALL;
        case EBADE:           return MP_ERR_INVALID_EXCHANGE;
        case EBADFD:          return MP_ERR_INVALID_FD;
        case EBADRQC:         return MP_ERR_INVALID_REQUEST_CODE;
        case EBADR:           return MP_ERR_INVALID_REQUEST;
        case EBADSLT:         return MP_ERR_INVALID_SLOT;
        case EISNAM:          return MP_ERR_IS_NAMED_TYPE_FILE;
        case EKEYEXPIRED:     return MP_ERR_KEY_EXPIRED;
        case EKEYREJECTED:    return MP_ERR_KEY_REJECTED;
        case EKEYREVOKED:     return MP_ERR_KEY_REVOKED;
        case ENOKEY:          return MP_ERR_KEY_UNAVAILABLE;
        case EL2HLT:          return MP_ERR_LEVEL_2_HALTED;
        case EL2NSYNC:        return MP_ERR_LEVEL_2_NOT_SYNC;
        case EL3HLT:          return MP_ERR_LEVEL_3_HALTED;
        case EL3RST:          return MP_ERR_LEVEL_3_RESET;
        case ELIBSCN:         return MP_ERR_LIB_SECTION_CORRUPT;
        case ELNRNG:          return MP_ERR_LINK_NUM_OUT_OF_RANGE;
        case EHWPOISON:       return MP_ERR_MEM_PAGE_HARDWARE_ERR;
        case ENOTUNIQ:        return MP_ERR_NAME_NOT_UNIQUE;
        case ENOANO:          return MP_ERR_NO_ANODE;
        case ENOMEDIUM:       return MP_ERR_NO_MEDIUM_FOUND;
        case ENONET:          return MP_ERR_NOT_ON_NETWORK;
        case ERFKILL:         return MP_ERR_NOT_POSSIBLE_BY_RFKILL;
        case EREMOTE:         return MP_ERR_OBJECT_IS_REMOTE;
        case ENOPKG:          return MP_ERR_PACKAGE_NOT_INSTALLED;
        case EUNATCH:         return MP_ERR_PROTO_DRIVER_UNATTACHED;
        case EPFNOSUPPORT:    return MP_ERR_PROTO_FAMILY_UNSUPPORTED;
        case EREMCHG:         return MP_ERR_REMOTE_ADDR_CHANGED;
        case EREMOTEIO:       return MP_ERR_REMOTE_IO_ERR;
        case ESHUTDOWN:       return MP_ERR_SEND_AFTER_SHUTDOWN;
        case ECOMM:           return MP_ERR_SEND_COMM_ERR;
        case ESOCKTNOSUPPORT: return MP_ERR_SOCKET_TYPE_UNSUPPORTED;
        case ESTRPIPE:        return MP_ERR_STREAM_PIPE_ERR;
        case EUCLEAN:         return MP_ERR_STRUCT_NEED_CLEANING;
        case ETOOMANYREFS:    return MP_ERR_TOO_MANY_REFERENCES;
        case EUSERS:          return MP_ERR_TOO_MANY_USERS;
        case EMEDIUMTYPE:     return MP_ERR_WRONG_MEDIUM_TYPE;

#endif /* if defined(__MP_SYSTEM_LINUX) */

#elif defined(__MP_SYSTEM_WINDOWS)
        case EOTHER:    return MP_ERR_OTHER;
        case STRUNCATE: return MP_ERR_TRUNCATED_STRING;

#else
#error "Unimplemented"

#endif /* if defined(__MP_SYSTEM_POSIX) */

        default: return MP_ERR_UNKNOWN;
    }
}

// Error messages taken from Linux manpage `errno(3)`
const char *mp_err_str(mp_Err e) {
    // Sort this!
    switch (e) {
        case MP_ERR_NONE:    return "Success";
        case MP_ERR_UNKNOWN: return "Unknown error";

        case MP_ERR_INVALID_WIDE_CHAR: return "Invalid or incomplete multibyte or wide character";
        case MP_ERR_OUT_OF_DOMAIN:     return "Mathematics argument out of domain of function";
        case MP_ERR_RESULT_TOO_LARGE:  return "Result too large";

#if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS)
        case MP_ERR_ADDR_IN_USE:              return "Address already in use";
        case MP_ERR_ADDR_UNAVAILABLE:         return "Address not available";
        case MP_ERR_AF_NOT_SUPPORTED:         return "Address family not supported";
        case MP_ERR_ARG_TOO_LONG:             return "Argument list too long";
        case MP_ERR_BAD_ADDR:                 return "Bad address";
        case MP_ERR_BAD_FD:                   return "Bad file descriptor";
        case MP_ERR_BAD_MSG:                  return "Bad message";
        case MP_ERR_BROKEN_PIPE:              return "Broken pipe";
        case MP_ERR_BUSY:                     return "Device or resource busy";
        case MP_ERR_CANCELED:                 return "Operation canceled";
        case MP_ERR_CONNECTION_ABORTED:       return "Connection aborted";
        case MP_ERR_CONNECTION_IN_PROGRESS:   return "Connection already in progress";
        case MP_ERR_CONNECTION_REFUSED:       return "Connection refused";
        case MP_ERR_CONNECTION_RESET:         return "Connection reset";
        case MP_ERR_CONNECTION_TIMED_OUT:     return "Connection timed out";
        case MP_ERR_DEST_ADDR_REQUIRED:       return "Destination address required";
        case MP_ERR_DIR_NOT_EMPTY:            return "Directory not empty";
        case MP_ERR_EXEC_FORMAT_ERR:          return "Exec format error";
        case MP_ERR_FILE_EXISTS:              return "File exists";
        case MP_ERR_FILENAME_TOO_LONG:        return "Filename too long";
        case MP_ERR_FILE_TOO_LARGE:           return "File too large";
        case MP_ERR_FUNCTION_UNIMPLEMENTED:   return "Function not implemented";
        case MP_ERR_HOST_IS_UNREACHABLE:      return "Host is unreachable";
        case MP_ERR_ID_REMOVED:               return "Identifier removed";
        case MP_ERR_INAPPROPRIATE_IO_CONTROL: return "Inappropriate I/O control operation";
        case MP_ERR_IN_PROGRESS:              return "Operation in progress";
        case MP_ERR_INTERRUPTED_CALL:         return "Interrupted function call";
        case MP_ERR_INVALID_ARG:              return "Invalid argument";
        case MP_ERR_INVALID_CROSSDEVICE_LINK: return "Invalid cross-device link";
        case MP_ERR_INVALID_SEEK:             return "Invalid seek";
        case MP_ERR_IO_ERR:                   return "Input/output error";
        case MP_ERR_IS_DIR:                   return "Is a directory";
        case MP_ERR_LINK_SEVERED:             return "Link has been severed";
        case MP_ERR_LOCK_UNAVAILABLE:         return "No locks available";
        case MP_ERR_MESSAGE_TOO_LONG:         return "Message too long";
        case MP_ERR_NET_CONNECTION_ABORTED:   return "Connection aborted by network";
        case MP_ERR_NET_IS_DOWN:              return "Network is down";
        case MP_ERR_NET_UNREACHABLE:          return "Network unreachable";
        case MP_ERR_NO_BUFFER_SPACE:          return "No buffer space available";
        case MP_ERR_NO_CHILD:                 return "No child processes";
        case MP_ERR_NO_DEVICE:                return "No such device";
        case MP_ERR_NO_DEVICE_OR_ADDR:        return "No such device or address";
        case MP_ERR_NO_FILE_OR_DIR:           return "No such file or directory";
        case MP_ERR_NO_MSG_OF_DESIRED_TYPE:   return "No message of the desired type";
        case MP_ERR_NO_PROCESS:               return "No such process";
        case MP_ERR_NO_SPACE_LEFT:            return "No space left on device";
        case MP_ERR_NO_STREAM_RESOURCES:      return "No STREAM resources";
        case MP_ERR_NOT_DIR:                  return "Not a directory";
        case MP_ERR_NOT_ENOUGH_MEM:           return "Not enough space/cannot allocate memory";
        case MP_ERR_NOT_PERMITTED:            return "Operation not permitted";
        case MP_ERR_NOT_SOCKET:               return "Not a socket";
        case MP_ERR_NOT_STREAM:               return "Not a STREAM";
        case MP_ERR_NOT_SUPPORTED:            return "Operation not supported";
        case MP_ERR_NOT_SUPPORTED_ON_SOCKET:  return "Operation not supported on socket";
        case MP_ERR_OWNER_DIED:               return "Owner died";
        case MP_ERR_PERM_DENIED:              return "Permission denied";
        case MP_ERR_PROTOCOL_ERR:             return "Protocol error";
        case MP_ERR_PROTOCOL_NOT_SUPPORTED:   return "Protocol not supported";
        case MP_ERR_PROTOCOL_UNAVAILABLE:     return "Protocol not available";
        case MP_ERR_READ_ONLY_FILESYSTEM:     return "Read-only filesystem";
        case MP_ERR_RESOURCE_DEADLOCK:        return "Resource deadlock avoided";
        case MP_ERR_SOCKET_IS_CONNECTED:      return "Socket is connected";
        case MP_ERR_SOCKET_NOT_CONNECTED:     return "The socket is not connected";
        case MP_ERR_STATE_UNRECOVERABLE:      return "State not recoverable";
        case MP_ERR_SYMLINK_TOO_DEEP:         return "Too many levels of symbolic links";
        case MP_ERR_TEMPORARILY_UNAVAILABLE:  return "Resource temporarily unavailable";
        case MP_ERR_TEXT_FILE_BUSY:           return "Text file busy";
        case MP_ERR_TIMER_EXPIRED:            return "Timer expired";
        case MP_ERR_TOO_MANY_LINKS:           return "Too many links";
        case MP_ERR_TOO_MANY_OPEN_FILES:      return "Too many open files";
        case MP_ERR_TOO_MANY_OPEN_FILES_SYS:  return "Too many open files in system";
        case MP_ERR_VALUE_OVERFLOW:           return "Value too large to be stored in data type";
        case MP_ERR_WOULD_BLOCK:              return "Operation would block";
        case MP_ERR_WRONG_PROTOCOL_TYPE:      return "Protocol wrong type for socket";

#if defined(__MP_SYSTEM_LINUX)
        case MP_ERR_CANNOT_ACCESS_ATTRIB:
            return "The named attribute does not exist, or the process has no access to this attribute";
        case MP_ERR_RESOURCE_DEADLOCK2: return "File locking dead‐lock error";

#endif /* if defined(__MP_SYSTEM_LINUX) */

#endif /* if defined(__MP_SYSTEM_POSIX) || defined(__MP_SYSTEM_WINDOWS) */

#if defined(__MP_SYSTEM_POSIX)
        case MP_ERR_DISK_QUOTA_EXCEEDED: return "Disk quota exceeded";
        case MP_ERR_MULTIHOP_ATTEMPTED:  return "Multihop attempted";
        case MP_ERR_STALE_FILE_HANDLE:   return "Stale file handle";

#if defined(__MP_SYSTEM_LINUX)
        case MP_ERR_ACCESS_CORRUPT_LIB:       return "Accessing a corrupted shared library";
        case MP_ERR_ACCESS_TOO_MANY_LIBS:     return "Attempting to link in too many shared libraries";
        case MP_ERR_BLOCK_DEVICE_REQUIRED:    return "Block device required";
        case MP_ERR_CANNOT_ACCESS_LIB:        return "Cannot access a needed shared library";
        case MP_ERR_CANNOT_EXEC_LIB:          return "Cannot exec a shared library directly";
        case MP_ERR_CHANNEL_NUM_OUT_OF_RANGE: return "Channel number out of range";
        case MP_ERR_EXCHANGE_FULL:            return "Exchange full";
        case MP_ERR_HOST_IS_DOWN:             return "Host is down";
        case MP_ERR_INTERRUPTED_SYSCALL:      return "Interrupted system call should be restarted";
        case MP_ERR_INVALID_EXCHANGE:         return "Invalid exchange";
        case MP_ERR_INVALID_FD:               return "File descriptor in bad state";
        case MP_ERR_INVALID_REQUEST_CODE:     return "Invalid request code";
        case MP_ERR_INVALID_REQUEST:          return "Invalid request descriptor";
        case MP_ERR_INVALID_SLOT:             return "Invalid slot";
        case MP_ERR_IS_NAMED_TYPE_FILE:       return "Is a named type file";
        case MP_ERR_KEY_EXPIRED:              return "Key has expired";
        case MP_ERR_KEY_REJECTED:             return "Key was rejected by service";
        case MP_ERR_KEY_REVOKED:              return "Key has been revoked";
        case MP_ERR_KEY_UNAVAILABLE:          return "Required key not available";
        case MP_ERR_LEVEL_2_HALTED:           return "Level 2 halted";
        case MP_ERR_LEVEL_2_NOT_SYNC:         return "Level 2 not synchronized";
        case MP_ERR_LEVEL_3_HALTED:           return "Level 3 halted";
        case MP_ERR_LEVEL_3_RESET:            return "Level 3 reset";
        case MP_ERR_LIB_SECTION_CORRUPT:      return ".lib section in a.out corrupted";
        case MP_ERR_LINK_NUM_OUT_OF_RANGE:    return "Link number out of range";
        case MP_ERR_MEM_PAGE_HARDWARE_ERR:    return "Memory page has hardware error";
        case MP_ERR_NAME_NOT_UNIQUE:          return "Name not unique on network";
        case MP_ERR_NO_ANODE:                 return "No anode";
        case MP_ERR_NO_MEDIUM_FOUND:          return "No medium found";
        case MP_ERR_NOT_ON_NETWORK:           return "Machine is not on the network";
        case MP_ERR_NOT_POSSIBLE_BY_RFKILL:   return "Operation not possible due to RF-kill";
        case MP_ERR_OBJECT_IS_REMOTE:         return "Object is remote";
        case MP_ERR_PACKAGE_NOT_INSTALLED:    return "Package not installed";
        case MP_ERR_PROTO_DRIVER_UNATTACHED:  return "Protocol driver not attached";
        case MP_ERR_PROTO_FAMILY_UNSUPPORTED: return "Protocol family not supported";
        case MP_ERR_REMOTE_ADDR_CHANGED:      return "Remote address changed";
        case MP_ERR_REMOTE_IO_ERR:            return "Remote I/O error";
        case MP_ERR_SEND_AFTER_SHUTDOWN:      return "Cannot send after transport endpoint shutdown";
        case MP_ERR_SEND_COMM_ERR:            return "Communication error on send";
        case MP_ERR_SOCKET_TYPE_UNSUPPORTED:  return "Socket type not supported";
        case MP_ERR_STREAM_PIPE_ERR:          return "Streams pipe error";
        case MP_ERR_STRUCT_NEED_CLEANING:     return "Structure needs cleaning";
        case MP_ERR_TOO_MANY_REFERENCES:      return "Too many references: cannot splice";
        case MP_ERR_TOO_MANY_USERS:           return "Too many users";
        case MP_ERR_WRONG_MEDIUM_TYPE:        return "Wrong medium type";

#endif /* if defined(__MP_SYSTEM_LINUX) */

#elif defined(__MP_SYSTEM_WINDOWS)
        case MP_ERR_OTHER: return "Other";
        case MP_ERR_TRUNCATED_STRING:
            return "A string copy or concatenation resulted in a truncated string";

#else
#error "Unimplemented"

#endif /* if defined(__MP_SYSTEM_POSIX) */

        case __MP_ERR_COUNT: __MP_UNREACHABLE();
    }

    __MP_UNREACHABLE();
}

const char *mp_ioerr_str(mp_IoErr e) {
    switch (e) {
        case MP_IOERR_NONE:           return "Success";
        case MP_IOERR_UNSUPPORTED:    return "Unsupported operation";
        case MP_IOERR_EOF:            return "Reached end-of-file";
        case MP_IOERR_CANNOT_FLUSH:   return "Cannot flush";
        case MP_IOERR_CANNOT_SET_BUF: return "Cannot set internal buffer";
        case MP_IOERR_CANNOT_READ:    return "Cannot read from stream";
        case MP_IOERR_CANNOT_WRITE:   return "Cannot write to stream";
        case MP_IOERR_CANNOT_GET_POS: return "Cannot get file position indicator";
        case MP_IOERR_CANNOT_SET_POS: return "Cannot set file position indicator";
        case __MP_IOERR_COUNT:        __MP_UNREACHABLE();
    }

    __MP_UNREACHABLE();
}

mp_Err mp_file_open(mp_File *f, const char *filename, const char *mode) {
    __MP_ZERO(f);
    FILE *file;
    if ((file = fopen(filename, mode)) != NULL) {
        f->file           = file;
        f->supported_type = MP_IOTYPE_NONE;

        if (strchr(mode, '+') != NULL) {
            f->supported_type = MP_IOTYPE_WRITE | MP_IOTYPE_READ;
        } else if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL) {
            f->supported_type = MP_IOTYPE_WRITE;
        } else if (strchr(mode, 'r') != NULL) {
            f->supported_type = MP_IOTYPE_READ;
        }

        return MP_ERR_NONE;
    } else {
        f->file = NULL;
        return mp_err(errno);
    }
}

mp_Err mp_file_reopen(mp_File *f, const char *filename, const char *mode) {
    if (f->file == NULL) {
        return MP_ERR_BAD_FD;
    }

    if ((f->file = freopen(filename, mode, f->file)) != NULL) {
        f->supported_type = MP_IOTYPE_NONE;

        if (strchr(mode, '+') != NULL) {
            f->supported_type = MP_IOTYPE_WRITE | MP_IOTYPE_READ;
        } else if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL) {
            f->supported_type = MP_IOTYPE_WRITE;
        } else if (strchr(mode, 'r') != NULL) {
            f->supported_type = MP_IOTYPE_READ;
        }

        return MP_ERR_NONE;
    } else {
        f->file = NULL;
        return mp_err(errno);
    }
}

void mp_file_deinit(mp_File *f) {
    if (f->file != NULL) {
        fclose(f->file);
    }
    __MP_ZERO(f);
}

mp_Io mp_file_io(mp_File *f, mp_IoType type) {
    if (type == MP_IOTYPE_READ && (f->supported_type & MP_IOTYPE_READ) == 0) {
        return mp_io_invalid();
    }
    if (type == MP_IOTYPE_WRITE && (f->supported_type & MP_IOTYPE_WRITE) == 0) {
        return mp_io_invalid();
    }
    return mp_io_new(f, type, mp_file_io_func);
}

static mp_IoErr
mp_file_io_func(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2, size_t *ret) {
    mp_File *ctx = io->context;

    __MP_STATIC_ASSERT(__MP_IOOP_COUNT == 8);
    switch (op) {
        case MP_IOOP_FLUSH: {
            (void) ptr;
            (void) n1;
            (void) n2;
            (void) ret;

            if ((io->type & MP_IOTYPE_WRITE) == 0) {
                return MP_IOERR_UNSUPPORTED;
            }

            if (fflush(ctx->file) == EOF && ferror(ctx->file)) {
                return MP_IOERR_CANNOT_FLUSH;
            };
        } break;
        case MP_IOOP_SETBUF: {
            (void) ret;

            int           mode    = 0;
            mp_SetbufMode mp_mode = n2;
            switch (mp_mode) {
                case MP_SETBUFMODE_NONE: {
                    mode = _IONBF;
                } break;
                case MP_SETBUFMODE_FULL: {
                    mode = _IOFBF;
                } break;
                case MP_SETBUFMODE_LINE: {
                    mode = _IOLBF;
                } break;
            }
            if (setvbuf(ctx->file, ptr, (int) mode, n1)) {
                return MP_IOERR_CANNOT_SET_BUF;
            };
        } break;
        case MP_IOOP_READ: {
            if ((io->type & MP_IOTYPE_READ) == 0) {
                return MP_IOERR_UNSUPPORTED;
            }

            size_t res = fread(ptr, n1, n2, ctx->file);
            if (ret != NULL) {
                *ret = res;
            }
            if (res < n2) {
                if (feof(ctx->file)) {
                    return MP_IOERR_EOF;
                }
                if (ferror(ctx->file)) {
                    return MP_IOERR_CANNOT_READ;
                }
            }
        } break;
        case MP_IOOP_WRITE: {
            if ((io->type & MP_IOTYPE_WRITE) == 0) {
                return MP_IOERR_UNSUPPORTED;
            }

            size_t res = fwrite(ptr, n1, n2, ctx->file);
            if (ret != NULL) {
                *ret = res;
            }
            if (res < n2) {
                return MP_IOERR_CANNOT_WRITE;
            }
        } break;
        case MP_IOOP_GETPOS: {
            (void) ptr;
            (void) n1;
            (void) n2;

            long res = ftell(ctx->file);
            if (res == -1l) {
                return MP_IOERR_CANNOT_GET_POS;
            }
            *ret = (size_t) res;
        } break;
        case MP_IOOP_SETPOS: {
            (void) ptr;
            (void) ret;

            int             origin    = 0;
            mp_SetposOrigin mp_origin = n2;
            switch (mp_origin) {
                case MP_SETPOSORIGIN_START: {
                    origin = SEEK_SET;
                } break;
                case MP_SETPOSORIGIN_CURRENT: {
                    origin = SEEK_CUR;
                } break;
                case MP_SETPOSORIGIN_END: {
                    origin = SEEK_END;
                } break;
            }
            if (fseek(ctx->file, (long) n1, origin)) {
                return MP_IOERR_CANNOT_SET_POS;
            }
        } break;
        case MP_IOOP_GETC: {
            (void) ptr;
            (void) n1;
            (void) n2;

            if ((io->type & MP_IOTYPE_READ) == 0) {
                return MP_IOERR_UNSUPPORTED;
            }

            int res = fgetc(ctx->file);
            if (res == EOF) {
                if (feof(ctx->file)) {
                    return MP_IOERR_EOF;
                }
                if (ferror(ctx->file)) {
                    return MP_IOERR_CANNOT_READ;
                }
            }
            *ret = (size_t) res;
        } break;
        case MP_IOOP_PUTC: {
            (void) ptr;
            (void) n2;
            (void) ret;

            if ((io->type & MP_IOTYPE_WRITE) == 0) {
                return MP_IOERR_UNSUPPORTED;
            }

            if (fputc((int) n1, ctx->file) == EOF) {
                return MP_IOERR_CANNOT_WRITE;
            }
        } break;
        case __MP_IOOP_COUNT: __MP_UNREACHABLE();
    }

    return MP_IOERR_NONE;
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_H__ */
