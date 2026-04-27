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
#endif

#endif

// Windows
#elif defined(_WIN32)
#define __MP_SYSTEM_WINDOWS

#else
#error "Unsupported system."
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
#endif

#endif

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

/***********
 * ALLOCATORS
 ***********/

/* Default size of a single region in bytes.
 * Will be aligned to the nearest increment of `sizeof(uintptr_t)`. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (64 * 1024)
#endif

typedef enum {
    MP_ALLOCOP_ALLOC,
    MP_ALLOCOP_REALLOC,
    MP_ALLOCOP_FREE,
} mp_AllocOp;

// TODO: Alloc location

/*
 * Functions of this type does different things depending on the `type` given.
 * They also use their parameters differently on each type.
 *
 *  Types:
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
 *
 *  Returns the pointer to the newly allocated memory. May return NULL if allocation failed.
 *  Always returns NULL on MP_ALLOCOP_FREE. */
typedef void *(*mp_AllocFunc)(
    mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be customized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this could be specified as NULL.
    void *context;

    // The function that does stuff to the memory.
    // See `mp_AllocFunc` for more information.
    mp_AllocFunc f;
} mp_Alloc;

/* Macros that wrap the functions above */

/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * size: number of bytes
 * Returns void* */
#define mp_alloc(alloc, size) ((alloc)->f(MP_ALLOCOP_ALLOC, (alloc)->context, (size), 0, NULL))
/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * old_ptr: pointer
 * old_size: number of bytes
 * new_size: number of bytes
 * Returns void* */
#define mp_realloc(alloc, old_ptr, old_size, new_size)                                             \
    ((alloc)->f(MP_ALLOCOP_REALLOC, (alloc)->context, (new_size), (old_size), (old_ptr)))
/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * ptr: pointer (nullability depends on the allocator implementation)
 * size: number of bytes
 * Returns NULL */
#define mp_free(alloc, ptr, size) ((alloc)->f(MP_ALLOCOP_FREE, (alloc)->context, (size), 0, (ptr)))
/* Allocate a new chunk of memory for the given type.
 *
 * alloc: mp_Alloc* (NO SIDE EFFECTS)
 * type: typename
 * Returns `type`* */
#define mp_create(alloc, type) (mp_alloc((alloc), sizeof(type)))
/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * data: pointer
 * size: number of bytes
 * Returns void* */
void *mp_dup(mp_Alloc *alloc, void *data, size_t size);

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

/* Handles reallocation for custom allocators.
 * You can slot this into your allocator function as long as alloc and free functionalities are
 * defined. For details see the implementation.
 * Does nothing and returns NULL if `new_size` == 0 */
void *mp_alloc_handle_realloc(mp_Alloc *alloc, void *old_ptr, size_t old_size, size_t new_size);

typedef struct mp_Region mp_Region;

/* Linked list element that holds certain size of allocated memory. */
struct mp_Region {
    mp_Region *next;      // The next region in linked list if any
    size_t     len;       // The amount of data (in bytes) used
    size_t     cap;       // The amount of data (in bytes) allocated
    uintptr_t  data[];    // The data (aligned to the `sizeof(uintptr_t)`)
};

/* Allocates a new region with `cap` bytes of size.
 * `cap` will be ROUNDED UP to the nearest increment of `sizeof(uintptr_t)`. */
mp_Region *mp_region_init(size_t cap);
/* Frees a region. */
void mp_region_deinit(mp_Region *r);

/* GROWING ARENA ALLOCATOR.
 * Manages regions in a linked list. */
typedef struct {
    mp_Region *begin, *end;    // Region linked list
    size_t     len;            // The amount of data (in bytes used, aligned to `sizeof(uintptr_t)`)
} mp_Arena;

/* Creates a new, unallocated arena. */
void mp_arena_init(mp_Arena *a);
/* Set arena `len` to 0, but does not free allocated regions. */
void mp_arena_reset(mp_Arena *a);
/* Frees an arena and its regions. */
void mp_arena_deinit(mp_Arena *a);
/* Returns an allocator that works with `mp_Arena`. */
mp_Alloc mp_arena_alloc(const mp_Arena *a);

/* STATIC ARENA ALLOCATOR.
 * Allocations are cancelled and return NULL if the requested size is bigger than the remaining
 * capacity. */
typedef struct {
    uintptr_t *buf;    // The arena buffer (of size `cap`)
    size_t     len;    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t     cap;    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
    mp_Alloc  *alloc;    // The allocator used to manage `buf`
} mp_SArena;

/* Initializes and allocates a static arena. `cap` in bytes.
 * `cap` will be ROUNDED UP to the nearest increment of `sizeof(uintptr_t)`. */
void mp_sarena_init(mp_SArena *a, mp_Alloc *alloc, size_t cap);
/* Resets the size of the arena. */
void mp_sarena_reset(mp_SArena *a);
/* Frees the arena. */
void mp_sarena_deinit(mp_SArena *a);
/* Returns an allocator that works with `mp_SArena`. */
mp_Alloc mp_sarena_alloc(const mp_SArena *a);

/* TEMP ALLOCATOR.
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
mp_Alloc mp_temp_alloc(const mp_Temp *t);

/* HEAP ALLOCATOR */
mp_Alloc mp_heap_alloc(void);

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
        mp_Alloc *alloc;    // The allocator that manages the allocation of the array
        size_t       len;       // The size of the array
        size_t       cap;       // The capacity of the array
        <type>       *data;     // Pointer to the data (points to the first element)
        // The data is continuous in memory.
    };
    ```
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
        mp_Alloc *alloc;                                                                           \
        size_t    len;                                                                             \
        size_t    cap;                                                                             \
        type     *data;                                                                            \
    } name

/* Initializes a new dynamic array managed by `allocator`.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * allocator: mp_Alloc* */
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
 * allocator: mp_Alloc* (NO SIDE EFFECTS)
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
            (dest)->alloc = NULL;                                                                  \
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

/* Returns an invalid `mp_Str`. */
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
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new(mp_Alloc *alloc, const char *str);
/* Allocates and returns a new `mp_Str` from a string.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new_len(mp_Alloc *alloc, const char *str, size_t len);
/* Allocates and returns a new `mp_Str` from formatted input.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_newf(mp_Alloc *alloc, const char *fmt, ...) __MP_PRINTF_FORMAT(2);
/* Allocates and returns a clone of `str`.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_clone(const mp_Str *str, mp_Alloc *alloc);
/* Frees an allocated `mp_Str`. */
void mp_str_deinit(mp_Str *str, mp_Alloc *alloc);

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
mp_Str mp_str_builder_string(const mp_StrBuilder *sb, mp_Alloc *alloc);

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

// TODO: hash table iterator
// TODO: hash table clone

/* Percentage of elements in a hash table before it resizes. */
#ifndef MP_HASH_TABLE_MAX_LOAD
#define MP_HASH_TABLE_MAX_LOAD 0.75
#endif

/* Starting capacity of a hash table. */
#ifndef MP_HASH_TABLE_INIT_CAPACITY
#define MP_HASH_TABLE_INIT_CAPACITY MP_DARRAY_INIT_CAPACITY
#endif

/* Defines a hash table struct with value of type `value_type`.
 * Example usage:
 * ```c
 * mp_ht_create(int, HashTableInt);
 * ```
 *
 * value_type: typename
 * name: identifier */
#define mp_ht_create(value_type, name)                                                             \
    typedef struct {                                                                               \
        mp_Str     key;                                                                            \
        value_type val;                                                                            \
    } __##name##Entry;                                                                             \
    mp_da_create(__##name##Entry, name)

/* Initializes a new hash table managed by `allocator`.
 *
 * ht: HashTable* (NO SIDE EFFECTS)
 * allocator: mp_Alloc* */
#define mp_ht_init(ht, allocator) mp_da_init(ht, allocator)

/* Frees a hash table.
 *
 * ht: HashTable* (NO SIDE EFFECTS) */
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
 * ht: const DArray*
 * k: const char* (NON-NULL)
 * res: <value type>* */
#define mp_ht_get(ht, k, res) mp_ht_get_s((ht), &mp_str(k), (res))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: const DArray*
 * k: mp_Str*
 * res: <value type>* */
#define mp_ht_get_s(ht, k, res)                                                                    \
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
                    (res)   = &(ht)->data[__i].val;                                                \
                    __found = true;                                                                \
                    break;                                                                         \
                }                                                                                  \
                ++__i;                                                                             \
                if (__i >= (ht)->cap) __i = 0;                                                     \
            }                                                                                      \
        }                                                                                          \
        if (!__found) (res) = NULL;                                                                \
    } while (0)

/* Sets the value at key `k` to `v`.
 * When the item at `k` has not been initialized before, the key is cloned.
 * `data` becomes NULL if allocation failed.
 *
 * ht: const DArray*
 * k: const char*
 * res: <value type>* */
#define mp_ht_set(ht, k, v) mp_ht_set_s((ht), &mp_str(k), (v))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: const DArray*
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

/* Resizes a dynamic array to `offset` of the current `len`.
 * If the current capacity is 0, allocates for `MP_DARRAY_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * `data` becomes NULL if allocation failed.
 * `offset` must be POSITIVE.
 *
 * ht: HashTable* (NO SIDE EFFECTS)
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

/* Sets the length of a dynamic array to 0 and frees its keys.
 *
 * ht: HashTable* */
#define mp_ht_reset(ht)                                                                            \
    do {                                                                                           \
        __mp_ht_free_entries((ht)->data, (ht)->alloc, (ht)->cap);                                  \
        mp_da_reset(ht);                                                                           \
    } while (0)

/* Deletes an item at key `k`.
 * This does not shrink the hash table, but it just marks the spot as "deleted", which may be
 * overridden by subsequent sets.
 *
 * ht: HashTable*
 * k: const char* */
#define mp_ht_delete(ht, k) mp_ht_delete_s((ht), &mp_str(k))

/* The same as above but accepts `mp_Str*`.
 *
 * ht: const DArray*
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

/* Hashes a string with FNV-1a hash algorithm. */
uint64_t mp_ht_hash_str(const mp_Str *str);

/***********
 * ERRORS
 ***********/

/* Error names for POSIX & Linux taken from manpage `errno(3)`. */
// Sort this!
typedef enum {
    MP_ERR_NONE    = 0,
    MP_ERR_UNKNOWN = 1,
    MP_ERR_INVALID_WIDE_CHAR,    // EILSEQ
    MP_ERR_OUT_OF_DOMAIN,        // EDOM
    MP_ERR_RESULT_TOO_LARGE,     // ERANGE
#if defined(__MP_SYSTEM_POSIX)
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
    MP_ERR_DISK_QUOTA_EXCEEDED,         // EDQUOT
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
    MP_ERR_MULTIHOP_ATTEMPTED,          // EMULTIHOP
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
    MP_ERR_STALE_FILE_HANDLE,           // ESTALE
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
    MP_ERR_ACCESS_CORRUPT_LIB,          // ELIBBAD
    MP_ERR_ACCESS_TOO_MANY_LIBS,        // ELIBMAX
    MP_ERR_BLOCK_DEVICE_REQUIRED,       // ENOTBLK
    MP_ERR_CANNOT_ACCESS_ATTRIB,        // ENODATA
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
    MP_ERR_RESOURCE_DEADLOCK2,          // EDEADLOCK
    MP_ERR_SEND_AFTER_SHUTDOWN,         // ESHUTDOWN
    MP_ERR_SEND_COMM_ERR,               // ECOMM
    MP_ERR_SOCKET_TYPE_UNSUPPORTED,     // ESOCKTNOSUPPORT
    MP_ERR_STREAM_PIPE_ERR,             // ESTRPIPE
    MP_ERR_STRUCT_NEED_CLEANING,        // EUCLEAN
    MP_ERR_TOO_MANY_REFERENCES,         // ETOOMANYREFS
    MP_ERR_TOO_MANY_USERS,              // EUSERS
    MP_ERR_WRONG_MEDIUM_TYPE,           // EMEDIUMTYPE
#endif
#else
// #error "Unimplemented"
    MP_ERR_TODO
#endif
} mp_Err;

mp_Err      mp_err(int errnum);
const char *mp_err_str(mp_Err e);

/***********
 * IO INTERFACE
 ***********/

typedef enum {
    MP_IOOP_FLUSH,
    MP_IOOP_SETBUF,
    MP_IOOP_READ,
    MP_IOOP_WRITE,
    MP_IOOP_GETPOS,
    MP_IOOP_SETPOS,
} mp_IoOp;

typedef mp_Err (*mp_IoFunc)(mp_IoOp op, void *context, void *ptr, size_t n);

typedef struct {
    void *context;

    mp_IoFunc f;
} mp_Io;

/***********
 * FILE IO
 ***********/

// TODO: For now these functions return `mp_Err`. We will create a separate `mp_FileErr` type after
// we're done with Windows.
// mp_FileErr      mp_file_err(mp_Err e);
// const char *mp_file_err_str(mp_Err e);

typedef struct {
    FILE *file;
} mp_File;

mp_Err mp_file_open(mp_File *f, const char *filename, const char *mode);
// TODO: mp_file_open_from_fd
void mp_file_close(mp_File *f);

/***********
 * IMPLEMENTATION
 ***********/

#ifdef MEMPLUS_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define UNREACHABLE()      MEMPLUS_ASSERT_MSG(0, "Unreachable")
#define TODO(msg)          MEMPLUS_ASSERT_MSG(0, "TODO: " msg)
#define DIV_ROUNDUP(a, b)  (((a) + (b) - 1) / (b))
#define ALIGN(a, inc)      (DIV_ROUNDUP((a), (inc)) * (inc))
#define ALIGN_DOWN(a, inc) (((a) / (inc)) * (inc))
#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define ASSERT_OVERLAP(a, a_len, b, b_len)                                                         \
    do {                                                                                           \
        auto _a = (uintptr_t) a;                                                                   \
        auto _b = (uintptr_t) b;                                                                   \
        if (MAX((_a), (_b)) < MIN((_a) + (a_len), (_b) + (b_len))) {                               \
            MEMPLUS_ASSERT_MSG(0, "Memory overlaps");                                              \
        }                                                                                          \
    } while (0)

static void *
mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr);

#ifdef __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(
    const char *assertion, const char *file, const char *func, size_t line, const char *msg) {
    fprintf(stderr, "%s:%s():%zu: [memplus] %s. `%s` failed.\n", file, func, line, msg, assertion);
    abort();
}
#endif

void *mp_dup(mp_Alloc *alloc, void *data, size_t size) {
    void *buf = mp_alloc(alloc, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

void *mp_alloc_handle_realloc(mp_Alloc *alloc, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        return NULL;
    }
    if (new_size <= old_size) return old_ptr;
    void *new_ptr = mp_alloc(alloc, new_size);
    if (new_ptr == NULL) return NULL;
    ASSERT_OVERLAP(old_ptr, old_size, new_ptr, new_size);
    memcpy(new_ptr, old_ptr, old_size);
    mp_free(alloc, old_ptr, old_size);
    return new_ptr;
}

mp_Region *mp_region_init(size_t cap) {
    size_t     bytes  = ALIGN(cap, sizeof(uintptr_t));
    mp_Region *region = MEMPLUS_ALLOC(bytes);
    region->next      = NULL;
    region->len       = 0;
    region->cap       = bytes;
    return region;
}

void mp_region_deinit(mp_Region *r) {
    MEMPLUS_FREE(r);
}

void mp_arena_init(mp_Arena *a) {
    a->len   = 0;
    a->begin = NULL;
    a->end   = NULL;
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
        mp_region_deinit(region_temp);
    }
    __MP_ZERO(a);
}

mp_Alloc mp_arena_alloc(const mp_Arena *a) {
    return mp_alloc_new(a, mp_arena_alloc_func);
}

static void *
mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_Arena *ctx   = context;
    mp_Alloc  alloc = mp_alloc_new(ctx, mp_arena_alloc_func);

    switch (op) {
        case MP_ALLOCOP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            size_t alloc_size = ALIGN(new_size, sizeof(uintptr_t));

            if (ctx->end == NULL) {
                MEMPLUS_ASSERT(ctx->begin == NULL);
                size_t capacity = MP_REGION_DEFAULT_SIZE;
                if (capacity < alloc_size) capacity = alloc_size;
                ctx->end = mp_region_init(capacity);
                if (ctx->end == NULL) return NULL;
                ctx->begin = ctx->end;
            }

            while (ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap &&
                   ctx->end->next != NULL) {
                ctx->end = ctx->end->next;
            }

            if (ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap) {
                MEMPLUS_ASSERT(ctx->end->next == NULL);
                size_t capacity = MP_REGION_DEFAULT_SIZE;
                if (capacity < alloc_size) capacity = alloc_size;
                ctx->end->next = mp_region_init(capacity);
                if (ctx->end->next == NULL) return NULL;
                ctx->end = ctx->end->next;
            }

            MEMPLUS_ASSERT(ctx->end->len % sizeof(uintptr_t) == 0);
            size_t len_words = DIV_ROUNDUP(ctx->end->len, sizeof(uintptr_t));
            void  *result    = &ctx->end->data[len_words];
            ctx->end->len += alloc_size;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCOP_REALLOC: {
            return mp_alloc_handle_realloc(&alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCOP_FREE: {
            (void) old_size;

            return NULL;
        } break;
    }
    UNREACHABLE();
}

void mp_sarena_init(mp_SArena *a, mp_Alloc *alloc, size_t cap) {
    size_t     bytes  = ALIGN(cap, sizeof(uintptr_t));
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

mp_Alloc mp_sarena_alloc(const mp_SArena *a) {
    return mp_alloc_new(a, mp_sarena_alloc_func);
}

static void *
mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_SArena *ctx   = context;
    mp_Alloc   alloc = mp_alloc_new(ctx, mp_sarena_alloc_func);

    switch (op) {
        case MP_ALLOCOP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            size_t alloc_size = ALIGN(new_size, sizeof(uintptr_t));

            MEMPLUS_ASSERT(ctx->len % sizeof(uintptr_t) == 0);
            if (ctx->len + alloc_size > ctx->cap) return NULL;

            void *result = ctx->buf + ctx->len;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCOP_REALLOC: {
            return mp_alloc_handle_realloc(&alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCOP_FREE: {
            (void) old_size;

            return NULL;
        } break;
    }
    UNREACHABLE();
}

void mp_temp_init(mp_Temp *t, char *buf, size_t cap) {
    memset(buf, 0, cap);
    t->buf = (uintptr_t *) buf;
    t->len = 0;
    t->cap = ALIGN_DOWN(cap, sizeof(uintptr_t));
}

void mp_temp_reset(mp_Temp *t) {
    t->len = 0;
}

mp_Alloc mp_temp_alloc(const mp_Temp *t) {
    return mp_alloc_new(t, mp_sarena_alloc_func);
}

mp_Alloc mp_heap_alloc(void) {
    return mp_alloc_new(NULL, mp_heap_alloc_func);
}

static void *
mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
    (void) context;
    mp_Alloc alloc = mp_alloc_new(NULL, mp_heap_alloc_func);

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
            return mp_alloc_handle_realloc(&alloc, ptr, old_size, new_size);
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
    }
    UNREACHABLE();
}

mp_Str mp_str_new(mp_Alloc *alloc, const char *str) {
    int len = snprintf(NULL, 0, "%s", str);
    MEMPLUS_ASSERT_MSG(len >= 0, "Failed to count string length");
    return mp_str_new_len(alloc, str, (size_t) len);
}

mp_Str mp_str_new_len(mp_Alloc *alloc, const char *str, size_t len) {
    char *result = mp_alloc(alloc, (size_t) (len + 1));
    if (result == NULL) return mp_str_invalid();
    int result_len = snprintf(result, (size_t) (len + 1), "%.*s", (int) len, str);
    MEMPLUS_ASSERT((size_t) result_len == len);
    return (mp_Str) { .len = (size_t) result_len, .cstr = result };
}

mp_Str mp_str_newf(mp_Alloc *alloc, const char *fmt, ...) {
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

mp_Str mp_str_clone(const mp_Str *str, mp_Alloc *alloc) {
    int len = snprintf(NULL, 0, "%s", str->cstr);
    MEMPLUS_ASSERT_MSG(len >= 0 || (size_t) len != str->len, "Failed to count string length");
    char *ptr = mp_dup(alloc, str->cstr, (size_t) len + 1);
    if (ptr == NULL) return mp_str_invalid();
    return (mp_Str) { .len = (size_t) len, .cstr = ptr };
}

void mp_str_deinit(mp_Str *str, mp_Alloc *alloc) {
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

mp_Str mp_str_builder_string(const mp_StrBuilder *sb, mp_Alloc *alloc) {
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
    // Sort this!
    switch (errnum) {
        case EDOM:   return MP_ERR_OUT_OF_DOMAIN;
        case EILSEQ: return MP_ERR_INVALID_WIDE_CHAR;
        case ERANGE: return MP_ERR_RESULT_TOO_LARGE;
#if defined(__MP_SYSTEM_POSIX)
        case E2BIG:           return MP_ERR_ARG_TOO_LONG;
        case EACCES:          return MP_ERR_PERM_DENIED;
        case EADDRINUSE:      return MP_ERR_ADDR_IN_USE;
        case EADDRNOTAVAIL:   return MP_ERR_ADDR_UNAVAILABLE;
        case EAFNOSUPPORT:    return MP_ERR_AF_NOT_SUPPORTED;
        case EAGAIN:          return MP_ERR_TEMPORARILY_UNAVAILABLE;
        case EALREADY:        return MP_ERR_CONNECTION_IN_PROGRESS;
        case EBADF:           return MP_ERR_BAD_FD;
        case EBADMSG:         return MP_ERR_BAD_MSG;
        case EBUSY:           return MP_ERR_BUSY;
        case ECANCELED:       return MP_ERR_CANCELED;
        case ECHILD:          return MP_ERR_NO_CHILD;
        case ECONNABORTED:    return MP_ERR_CONNECTION_ABORTED;
        case ECONNREFUSED:    return MP_ERR_CONNECTION_REFUSED;
        case ECONNRESET:      return MP_ERR_CONNECTION_RESET;
        case EDEADLK:         return MP_ERR_RESOURCE_DEADLOCK;
        case EDESTADDRREQ:    return MP_ERR_DEST_ADDR_REQUIRED;
        case EDQUOT:          return MP_ERR_DISK_QUOTA_EXCEEDED;
        case EEXIST:          return MP_ERR_FILE_EXISTS;
        case EFAULT:          return MP_ERR_BAD_ADDR;
        case EFBIG:           return MP_ERR_FILE_TOO_LARGE;
        case EHOSTUNREACH:    return MP_ERR_HOST_IS_UNREACHABLE;
        case EIDRM:           return MP_ERR_ID_REMOVED;
        case EINPROGRESS:     return MP_ERR_IN_PROGRESS;
        case EINTR:           return MP_ERR_INTERRUPTED_CALL;
        case EINVAL:          return MP_ERR_INVALID_ARG;
        case EIO:             return MP_ERR_IO_ERR;
        case EISCONN:         return MP_ERR_SOCKET_IS_CONNECTED;
        case EISDIR:          return MP_ERR_IS_DIR;
        case ELOOP:           return MP_ERR_SYMLINK_TOO_DEEP;
        case EMFILE:          return MP_ERR_TOO_MANY_OPEN_FILES;
        case EMLINK:          return MP_ERR_TOO_MANY_LINKS;
        case EMSGSIZE:        return MP_ERR_MESSAGE_TOO_LONG;
        case EMULTIHOP:       return MP_ERR_MULTIHOP_ATTEMPTED;
        case ENAMETOOLONG:    return MP_ERR_FILENAME_TOO_LONG;
        case ENETDOWN:        return MP_ERR_NET_IS_DOWN;
        case ENETRESET:       return MP_ERR_NET_CONNECTION_ABORTED;
        case ENETUNREACH:     return MP_ERR_NET_UNREACHABLE;
        case ENFILE:          return MP_ERR_TOO_MANY_OPEN_FILES_SYS;
        case ENOBUFS:         return MP_ERR_NO_BUFFER_SPACE;
        case ENODEV:          return MP_ERR_NO_DEVICE;
        case ENOENT:          return MP_ERR_NO_FILE_OR_DIR;
        case ENOEXEC:         return MP_ERR_EXEC_FORMAT_ERR;
        case ENOLCK:          return MP_ERR_LOCK_UNAVAILABLE;
        case ENOLINK:         return MP_ERR_LINK_SEVERED;
        case ENOMEM:          return MP_ERR_NOT_ENOUGH_MEM;
        case ENOMSG:          return MP_ERR_NO_MSG_OF_DESIRED_TYPE;
        case ENOPROTOOPT:     return MP_ERR_PROTOCOL_UNAVAILABLE;
        case ENOSPC:          return MP_ERR_NO_SPACE_LEFT;
        case ENOSR:           return MP_ERR_NO_STREAM_RESOURCES;
        case ENOSTR:          return MP_ERR_NOT_STREAM;
        case ENOSYS:          return MP_ERR_FUNCTION_UNIMPLEMENTED;
        case ENOTCONN:        return MP_ERR_SOCKET_NOT_CONNECTED;
        case ENOTDIR:         return MP_ERR_NOT_DIR;
        case ENOTEMPTY:       return MP_ERR_DIR_NOT_EMPTY;
        case ENOTRECOVERABLE: return MP_ERR_STATE_UNRECOVERABLE;
        case ENOTSOCK:        return MP_ERR_NOT_SOCKET;
        case ENOTSUP:         return MP_ERR_NOT_SUPPORTED;
        case ENOTTY:          return MP_ERR_INAPPROPRIATE_IO_CONTROL;
        case ENXIO:           return MP_ERR_NO_DEVICE_OR_ADDR;
#if EOPNOTSUPP != ENOTSUP
        case EOPNOTSUPP: return MP_ERR_NOT_SUPPORTED_ON_SOCKET;
#endif
        case EOVERFLOW:       return MP_ERR_VALUE_OVERFLOW;
        case EOWNERDEAD:      return MP_ERR_OWNER_DIED;
        case EPERM:           return MP_ERR_NOT_PERMITTED;
        case EPIPE:           return MP_ERR_BROKEN_PIPE;
        case EPROTO:          return MP_ERR_PROTOCOL_ERR;
        case EPROTONOSUPPORT: return MP_ERR_PROTOCOL_NOT_SUPPORTED;
        case EPROTOTYPE:      return MP_ERR_WRONG_PROTOCOL_TYPE;
        case EROFS:           return MP_ERR_READ_ONLY_FILESYSTEM;
        case ESPIPE:          return MP_ERR_INVALID_SEEK;
        case ESRCH:           return MP_ERR_NO_PROCESS;
        case ESTALE:          return MP_ERR_STALE_FILE_HANDLE;
        case ETIMEDOUT:       return MP_ERR_CONNECTION_TIMED_OUT;
        case ETIME:           return MP_ERR_TIMER_EXPIRED;
        case ETXTBSY:         return MP_ERR_TEXT_FILE_BUSY;
#if EWOULDBLOCK != EAGAIN
        case EWOULDBLOCK: return MP_ERR_WOULD_BLOCK;
#endif
        case EXDEV: return MP_ERR_INVALID_CROSSDEVICE_LINK;
#if defined(__MP_SYSTEM_LINUX)
        case EBADE:   return MP_ERR_INVALID_EXCHANGE;
        case EBADFD:  return MP_ERR_INVALID_FD;
        case EBADR:   return MP_ERR_INVALID_REQUEST;
        case EBADRQC: return MP_ERR_INVALID_REQUEST_CODE;
        case EBADSLT: return MP_ERR_INVALID_SLOT;
        case ECHRNG:  return MP_ERR_CHANNEL_NUM_OUT_OF_RANGE;
        case ECOMM:   return MP_ERR_SEND_COMM_ERR;
#if EDEADLOCK != EDEADLK
        case EDEADLOCK: return MP_ERR_RESOURCE_DEADLOCK2;
#endif
        case EHOSTDOWN:       return MP_ERR_HOST_IS_DOWN;
        case EHWPOISON:       return MP_ERR_MEM_PAGE_HARDWARE_ERR;
        case EISNAM:          return MP_ERR_IS_NAMED_TYPE_FILE;
        case EKEYEXPIRED:     return MP_ERR_KEY_EXPIRED;
        case EKEYREJECTED:    return MP_ERR_KEY_REJECTED;
        case EKEYREVOKED:     return MP_ERR_KEY_REVOKED;
        case EL2HLT:          return MP_ERR_LEVEL_2_HALTED;
        case EL2NSYNC:        return MP_ERR_LEVEL_2_NOT_SYNC;
        case EL3HLT:          return MP_ERR_LEVEL_3_HALTED;
        case EL3RST:          return MP_ERR_LEVEL_3_RESET;
        case ELIBACC:         return MP_ERR_CANNOT_ACCESS_LIB;
        case ELIBBAD:         return MP_ERR_ACCESS_CORRUPT_LIB;
        case ELIBEXEC:        return MP_ERR_CANNOT_EXEC_LIB;
        case ELIBMAX:         return MP_ERR_ACCESS_TOO_MANY_LIBS;
        case ELIBSCN:         return MP_ERR_LIB_SECTION_CORRUPT;
        case ELNRNG:          return MP_ERR_LINK_NUM_OUT_OF_RANGE;
        case EMEDIUMTYPE:     return MP_ERR_WRONG_MEDIUM_TYPE;
        case ENOANO:          return MP_ERR_NO_ANODE;
        case ENODATA:         return MP_ERR_CANNOT_ACCESS_ATTRIB;
        case ENOKEY:          return MP_ERR_KEY_UNAVAILABLE;
        case ENOMEDIUM:       return MP_ERR_NO_MEDIUM_FOUND;
        case ENONET:          return MP_ERR_NOT_ON_NETWORK;
        case ENOPKG:          return MP_ERR_PACKAGE_NOT_INSTALLED;
        case ENOTBLK:         return MP_ERR_BLOCK_DEVICE_REQUIRED;
        case ENOTUNIQ:        return MP_ERR_NAME_NOT_UNIQUE;
        case EPFNOSUPPORT:    return MP_ERR_PROTO_FAMILY_UNSUPPORTED;
        case EREMCHG:         return MP_ERR_REMOTE_ADDR_CHANGED;
        case EREMOTEIO:       return MP_ERR_REMOTE_IO_ERR;
        case EREMOTE:         return MP_ERR_OBJECT_IS_REMOTE;
        case ERESTART:        return MP_ERR_INTERRUPTED_SYSCALL;
        case ERFKILL:         return MP_ERR_NOT_POSSIBLE_BY_RFKILL;
        case ESHUTDOWN:       return MP_ERR_SEND_AFTER_SHUTDOWN;
        case ESOCKTNOSUPPORT: return MP_ERR_SOCKET_TYPE_UNSUPPORTED;
        case ESTRPIPE:        return MP_ERR_STREAM_PIPE_ERR;
        case ETOOMANYREFS:    return MP_ERR_TOO_MANY_REFERENCES;
        case EUCLEAN:         return MP_ERR_STRUCT_NEED_CLEANING;
        case EUNATCH:         return MP_ERR_PROTO_DRIVER_UNATTACHED;
        case EUSERS:          return MP_ERR_TOO_MANY_USERS;
        case EXFULL:          return MP_ERR_EXCHANGE_FULL;
#endif
#else
// #error "Unimplemented"
#endif
        default: return MP_ERR_UNKNOWN;
    }
}

const char *mp_err_str(mp_Err e) {
    // Sort this!
    switch (e) {
        case MP_ERR_NONE:              return "MP_ERR_NONE";
        case MP_ERR_UNKNOWN:           return "MP_ERR_UNKNOWN";
        case MP_ERR_INVALID_WIDE_CHAR: return "MP_ERR_INVALID_WIDE_CHAR";
        case MP_ERR_OUT_OF_DOMAIN:     return "MP_ERR_OUT_OF_DOMAIN";
        case MP_ERR_RESULT_TOO_LARGE:  return "MP_ERR_RESULT_TOO_LARGE";
#if defined(__MP_SYSTEM_POSIX)
        case MP_ERR_ADDR_IN_USE:              return "MP_ERR_ADDR_IN_USE";
        case MP_ERR_ADDR_UNAVAILABLE:         return "MP_ERR_ADDR_UNAVAILABLE";
        case MP_ERR_AF_NOT_SUPPORTED:         return "MP_ERR_AF_NOT_SUPPORTED";
        case MP_ERR_ARG_TOO_LONG:             return "MP_ERR_ARG_TOO_LONG";
        case MP_ERR_BAD_ADDR:                 return "MP_ERR_BAD_ADDR";
        case MP_ERR_BAD_FD:                   return "MP_ERR_BAD_FD";
        case MP_ERR_BAD_MSG:                  return "MP_ERR_BAD_MSG";
        case MP_ERR_BROKEN_PIPE:              return "MP_ERR_BROKEN_PIPE";
        case MP_ERR_BUSY:                     return "MP_ERR_BUSY";
        case MP_ERR_CANCELED:                 return "MP_ERR_CANCELED";
        case MP_ERR_CONNECTION_ABORTED:       return "MP_ERR_CONNECTION_ABORTED";
        case MP_ERR_CONNECTION_IN_PROGRESS:   return "MP_ERR_CONNECTION_IN_PROGRESS";
        case MP_ERR_CONNECTION_REFUSED:       return "MP_ERR_CONNECTION_REFUSED";
        case MP_ERR_CONNECTION_RESET:         return "MP_ERR_CONNECTION_RESET";
        case MP_ERR_CONNECTION_TIMED_OUT:     return "MP_ERR_CONNECTION_TIMED_OUT";
        case MP_ERR_DEST_ADDR_REQUIRED:       return "MP_ERR_DEST_ADDR_REQUIRED";
        case MP_ERR_DIR_NOT_EMPTY:            return "MP_ERR_DIR_NOT_EMPTY";
        case MP_ERR_DISK_QUOTA_EXCEEDED:      return "MP_ERR_DISK_QUOTA_EXCEEDED";
        case MP_ERR_EXEC_FORMAT_ERR:          return "MP_ERR_EXEC_FORMAT_ERR";
        case MP_ERR_FILE_EXISTS:              return "MP_ERR_FILE_EXISTS";
        case MP_ERR_FILENAME_TOO_LONG:        return "MP_ERR_FILENAME_TOO_LONG";
        case MP_ERR_FILE_TOO_LARGE:           return "MP_ERR_FILE_TOO_LARGE";
        case MP_ERR_FUNCTION_UNIMPLEMENTED:   return "MP_ERR_FUNCTION_UNIMPLEMENTED";
        case MP_ERR_HOST_IS_UNREACHABLE:      return "MP_ERR_HOST_IS_UNREACHABLE";
        case MP_ERR_ID_REMOVED:               return "MP_ERR_ID_REMOVED";
        case MP_ERR_INAPPROPRIATE_IO_CONTROL: return "MP_ERR_INAPPROPRIATE_IO_CONTROL";
        case MP_ERR_IN_PROGRESS:              return "MP_ERR_IN_PROGRESS";
        case MP_ERR_INTERRUPTED_CALL:         return "MP_ERR_INTERRUPTED_CALL";
        case MP_ERR_INVALID_ARG:              return "MP_ERR_INVALID_ARG";
        case MP_ERR_INVALID_CROSSDEVICE_LINK: return "MP_ERR_INVALID_CROSSDEVICE_LINK";
        case MP_ERR_INVALID_SEEK:             return "MP_ERR_INVALID_SEEK";
        case MP_ERR_IO_ERR:                   return "MP_ERR_IO_ERR";
        case MP_ERR_IS_DIR:                   return "MP_ERR_IS_DIR";
        case MP_ERR_LINK_SEVERED:             return "MP_ERR_LINK_SEVERED";
        case MP_ERR_LOCK_UNAVAILABLE:         return "MP_ERR_LOCK_UNAVAILABLE";
        case MP_ERR_MESSAGE_TOO_LONG:         return "MP_ERR_MESSAGE_TOO_LONG";
        case MP_ERR_MULTIHOP_ATTEMPTED:       return "MP_ERR_MULTIHOP_ATTEMPTED";
        case MP_ERR_NET_CONNECTION_ABORTED:   return "MP_ERR_NET_CONNECTION_ABORTED";
        case MP_ERR_NET_IS_DOWN:              return "MP_ERR_NET_IS_DOWN";
        case MP_ERR_NET_UNREACHABLE:          return "MP_ERR_NET_UNREACHABLE";
        case MP_ERR_NO_BUFFER_SPACE:          return "MP_ERR_NO_BUFFER_SPACE";
        case MP_ERR_NO_CHILD:                 return "MP_ERR_NO_CHILD";
        case MP_ERR_NO_DEVICE:                return "MP_ERR_NO_DEVICE";
        case MP_ERR_NO_DEVICE_OR_ADDR:        return "MP_ERR_NO_DEVICE_OR_ADDR";
        case MP_ERR_NO_FILE_OR_DIR:           return "MP_ERR_NO_FILE_OR_DIR";
        case MP_ERR_NO_MSG_OF_DESIRED_TYPE:   return "MP_ERR_NO_MSG_OF_DESIRED_TYPE";
        case MP_ERR_NO_PROCESS:               return "MP_ERR_NO_PROCESS";
        case MP_ERR_NO_SPACE_LEFT:            return "MP_ERR_NO_SPACE_LEFT";
        case MP_ERR_NO_STREAM_RESOURCES:      return "MP_ERR_NO_STREAM_RESOURCES";
        case MP_ERR_NOT_DIR:                  return "MP_ERR_NOT_DIR";
        case MP_ERR_NOT_ENOUGH_MEM:           return "MP_ERR_NOT_ENOUGH_MEM";
        case MP_ERR_NOT_PERMITTED:            return "MP_ERR_NOT_PERMITTED";
        case MP_ERR_NOT_SOCKET:               return "MP_ERR_NOT_SOCKET";
        case MP_ERR_NOT_STREAM:               return "MP_ERR_NOT_STREAM";
        case MP_ERR_NOT_SUPPORTED:            return "MP_ERR_NOT_SUPPORTED";
        case MP_ERR_NOT_SUPPORTED_ON_SOCKET:  return "MP_ERR_NOT_SUPPORTED_ON_SOCKET";
        case MP_ERR_OWNER_DIED:               return "MP_ERR_OWNER_DIED";
        case MP_ERR_PERM_DENIED:              return "MP_ERR_PERM_DENIED";
        case MP_ERR_PROTOCOL_ERR:             return "MP_ERR_PROTOCOL_ERR";
        case MP_ERR_PROTOCOL_NOT_SUPPORTED:   return "MP_ERR_PROTOCOL_NOT_SUPPORTED";
        case MP_ERR_PROTOCOL_UNAVAILABLE:     return "MP_ERR_PROTOCOL_UNAVAILABLE";
        case MP_ERR_READ_ONLY_FILESYSTEM:     return "MP_ERR_READ_ONLY_FILESYSTEM";
        case MP_ERR_RESOURCE_DEADLOCK:        return "MP_ERR_RESOURCE_DEADLOCK";
        case MP_ERR_SOCKET_IS_CONNECTED:      return "MP_ERR_SOCKET_IS_CONNECTED";
        case MP_ERR_SOCKET_NOT_CONNECTED:     return "MP_ERR_SOCKET_NOT_CONNECTED";
        case MP_ERR_STALE_FILE_HANDLE:        return "MP_ERR_STALE_FILE_HANDLE";
        case MP_ERR_STATE_UNRECOVERABLE:      return "MP_ERR_STATE_UNRECOVERABLE";
        case MP_ERR_SYMLINK_TOO_DEEP:         return "MP_ERR_SYMLINK_TOO_DEEP";
        case MP_ERR_TEMPORARILY_UNAVAILABLE:  return "MP_ERR_TEMPORARILY_UNAVAILABLE";
        case MP_ERR_TEXT_FILE_BUSY:           return "MP_ERR_TEXT_FILE_BUSY";
        case MP_ERR_TIMER_EXPIRED:            return "MP_ERR_TIMER_EXPIRED";
        case MP_ERR_TOO_MANY_LINKS:           return "MP_ERR_TOO_MANY_LINKS";
        case MP_ERR_TOO_MANY_OPEN_FILES:      return "MP_ERR_TOO_MANY_OPEN_FILES";
        case MP_ERR_TOO_MANY_OPEN_FILES_SYS:  return "MP_ERR_TOO_MANY_OPEN_FILES_SYS";
        case MP_ERR_VALUE_OVERFLOW:           return "MP_ERR_VALUE_OVERFLOW";
        case MP_ERR_WOULD_BLOCK:              return "MP_ERR_WOULD_BLOCK";
        case MP_ERR_WRONG_PROTOCOL_TYPE:      return "MP_ERR_WRONG_PROTOCOL_TYPE";
#if defined(__MP_SYSTEM_LINUX)
        case MP_ERR_ACCESS_CORRUPT_LIB:       return "MP_ERR_ACCESS_CORRUPT_LIB";
        case MP_ERR_ACCESS_TOO_MANY_LIBS:     return "MP_ERR_ACCESS_TOO_MANY_LIBS";
        case MP_ERR_BLOCK_DEVICE_REQUIRED:    return "MP_ERR_BLOCK_DEVICE_REQUIRED";
        case MP_ERR_CANNOT_ACCESS_ATTRIB:     return "MP_ERR_CANNOT_ACCESS_ATTRIB";
        case MP_ERR_CANNOT_ACCESS_LIB:        return "MP_ERR_CANNOT_ACCESS_LIB";
        case MP_ERR_CANNOT_EXEC_LIB:          return "MP_ERR_CANNOT_EXEC_LIB";
        case MP_ERR_CHANNEL_NUM_OUT_OF_RANGE: return "MP_ERR_CHANNEL_NUM_OUT_OF_RANGE";
        case MP_ERR_EXCHANGE_FULL:            return "MP_ERR_EXCHANGE_FULL";
        case MP_ERR_HOST_IS_DOWN:             return "MP_ERR_HOST_IS_DOWN";
        case MP_ERR_INTERRUPTED_SYSCALL:      return "MP_ERR_INTERRUPTED_SYSCALL";
        case MP_ERR_INVALID_EXCHANGE:         return "MP_ERR_INVALID_EXCHANGE";
        case MP_ERR_INVALID_FD:               return "MP_ERR_INVALID_FD";
        case MP_ERR_INVALID_REQUEST_CODE:     return "MP_ERR_INVALID_REQUEST_CODE";
        case MP_ERR_INVALID_REQUEST:          return "MP_ERR_INVALID_REQUEST";
        case MP_ERR_INVALID_SLOT:             return "MP_ERR_INVALID_SLOT";
        case MP_ERR_IS_NAMED_TYPE_FILE:       return "MP_ERR_IS_NAMED_TYPE_FILE";
        case MP_ERR_KEY_EXPIRED:              return "MP_ERR_KEY_EXPIRED";
        case MP_ERR_KEY_REJECTED:             return "MP_ERR_KEY_REJECTED";
        case MP_ERR_KEY_REVOKED:              return "MP_ERR_KEY_REVOKED";
        case MP_ERR_KEY_UNAVAILABLE:          return "MP_ERR_KEY_UNAVAILABLE";
        case MP_ERR_LEVEL_2_HALTED:           return "MP_ERR_LEVEL_2_HALTED";
        case MP_ERR_LEVEL_2_NOT_SYNC:         return "MP_ERR_LEVEL_2_NOT_SYNC";
        case MP_ERR_LEVEL_3_HALTED:           return "MP_ERR_LEVEL_3_HALTED";
        case MP_ERR_LEVEL_3_RESET:            return "MP_ERR_LEVEL_3_RESET";
        case MP_ERR_LIB_SECTION_CORRUPT:      return "MP_ERR_LIB_SECTION_CORRUPT";
        case MP_ERR_LINK_NUM_OUT_OF_RANGE:    return "MP_ERR_LINK_NUM_OUT_OF_RANGE";
        case MP_ERR_MEM_PAGE_HARDWARE_ERR:    return "MP_ERR_MEM_PAGE_HARDWARE_ERR";
        case MP_ERR_NAME_NOT_UNIQUE:          return "MP_ERR_NAME_NOT_UNIQUE";
        case MP_ERR_NO_ANODE:                 return "MP_ERR_NO_ANODE";
        case MP_ERR_NO_MEDIUM_FOUND:          return "MP_ERR_NO_MEDIUM_FOUND";
        case MP_ERR_NOT_ON_NETWORK:           return "MP_ERR_NOT_ON_NETWORK";
        case MP_ERR_NOT_POSSIBLE_BY_RFKILL:   return "MP_ERR_NOT_POSSIBLE_BY_RFKILL";
        case MP_ERR_OBJECT_IS_REMOTE:         return "MP_ERR_OBJECT_IS_REMOTE";
        case MP_ERR_PACKAGE_NOT_INSTALLED:    return "MP_ERR_PACKAGE_NOT_INSTALLED";
        case MP_ERR_PROTO_DRIVER_UNATTACHED:  return "MP_ERR_PROTO_DRIVER_UNATTACHED";
        case MP_ERR_PROTO_FAMILY_UNSUPPORTED: return "MP_ERR_PROTO_FAMILY_UNSUPPORTED";
        case MP_ERR_REMOTE_ADDR_CHANGED:      return "MP_ERR_REMOTE_ADDR_CHANGED";
        case MP_ERR_REMOTE_IO_ERR:            return "MP_ERR_REMOTE_IO_ERR";
        case MP_ERR_RESOURCE_DEADLOCK2:       return "MP_ERR_RESOURCE_DEADLOCK2";
        case MP_ERR_SEND_AFTER_SHUTDOWN:      return "MP_ERR_SEND_AFTER_SHUTDOWN";
        case MP_ERR_SEND_COMM_ERR:            return "MP_ERR_SEND_COMM_ERR";
        case MP_ERR_SOCKET_TYPE_UNSUPPORTED:  return "MP_ERR_SOCKET_TYPE_UNSUPPORTED";
        case MP_ERR_STREAM_PIPE_ERR:          return "MP_ERR_STREAM_PIPE_ERR";
        case MP_ERR_STRUCT_NEED_CLEANING:     return "MP_ERR_STRUCT_NEED_CLEANING";
        case MP_ERR_TOO_MANY_REFERENCES:      return "MP_ERR_TOO_MANY_REFERENCES";
        case MP_ERR_TOO_MANY_USERS:           return "MP_ERR_TOO_MANY_USERS";
        case MP_ERR_WRONG_MEDIUM_TYPE:        return "MP_ERR_WRONG_MEDIUM_TYPE";
#endif
#else
// #error "Unimplemented"
#endif
    }

    UNREACHABLE();
}

mp_Err mp_file_open(mp_File *f, const char *filename, const char *mode) {
    __MP_ZERO(f);
    FILE *file;
    if ((file = fopen(filename, mode)) != NULL) {
        f->file = file;
        return MP_ERR_NONE;
    } else {
        return mp_err(errno);
    }
}

void mp_file_close(mp_File *f) {
    fclose(f->file);
    __MP_ZERO(f);
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_H__ */
