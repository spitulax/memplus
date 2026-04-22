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
 */

#ifndef MEMPLUS_H__
#define MEMPLUS_H__

/* #define MEMPLUS_IMPLEMENTATION */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Define custom assert macro with `MEMPLUS_ASSERT` and `MEMPLUS_ASSERT_MSG`.
 * The macro must accept the expression and the fail message. See the define below. */
#if !(defined(MEMPLUS_ASSERT) && defined(MEMPLUS_ASSERT_MSG))

#if __STDC_VERSION__ >= 201112L
#define ___MP_NORETURN _Noreturn
#elif __STDC_VERSION__ >= 202311L
#define ___MP_NORETURN [[noreturn]]
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef NDEBUG
#define MEMPLUS_ASSERT(expr)
#define MEMPLUS_ASSERT_MSG(expr, msg)
#else
#define MEMPLUS_ASSERT(expr)                                                                       \
    ((expr) ? (void) 0 : ___mp_assert_fail(#expr, __FILE__, __func__, __LINE__, ""))

#define MEMPLUS_ASSERT_MSG(expr, msg)                                                              \
    ((expr) ? (void) 0 : ___mp_assert_fail(#expr, __FILE__, __func__, __LINE__, (msg)))

___MP_NORETURN void ___mp_assert_fail(
    const char *assertion, const char *file, const char *func, size_t line, const char *msg) {
    fprintf(stderr, "%s:%s():%zu: [memplus] %s. `%s` failed.\n", file, func, line, msg, assertion);
    abort();
}
#endif

#endif

/* Assumed have the same behavior as stdlib's `calloc(..., 1)`. */
#ifndef MEMPLUS_ALLOC
#include <stdlib.h>
#define MEMPLUS_ALLOC(size) calloc((size), 1)
#endif

/* Assumed have the same behavior as stdlib's `realloc`. */
#ifndef MEMPLUS_REALLOC
#include <stdlib.h>
#define MEMPLUS_REALLOC(ptr, new_size) realloc((ptr), (new_size))
#endif

/* Must have the same signature and behavior as stdlib's `free`. */
#ifndef MEMPLUS_FREE
#include <stdlib.h>
#define MEMPLUS_FREE free
#endif

#define MEMPLUS_VERSION (0x000100)

#define ___MP_ZERO(ptr) memset((ptr), 0, sizeof(*(ptr)))
#define ___MP_BOUNDS_CHECK(i, len)                                                                 \
    MEMPLUS_ASSERT_MSG((i) < (len) && (i) >= 0, "Array index out of bounds")
#if defined(__GNUC__) || defined(__clang__)
#define ___MP_PRINTF_FORMAT(fmt_index) __attribute__((format(printf, (fmt_index), (fmt_index) + 1)))
#else
#define ___MP_PRINTF_FORMAT(fmt_index)
#endif

/* Indicates error return for `size_t`. */
#define MP_ERROR ((size_t) -1)

/***********
 * ALLOCATORS
 ***********/

/* Default size of a single region in bytes. You can adjust this to your liking.
 * Will be aligned to the nearest increment of `sizeof(uintptr_t)`. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (64 * 1024)
#endif

typedef enum {
    MP_ALLOCTYPE_ALLOC,
    MP_ALLOCTYPE_REALLOC,
    MP_ALLOCTYPE_FREE,
} mp_AllocType;

// TODO: Alloc location

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
 *    If `old_size` <= `new_size`, reallocation does not happen and the function just return `ptr`.
 *    Otherwise, allocates with size `new_size` and frees the memory pointed by `ptr`.
 *       - `context`: The allocator context
 *       - `ptr`: The pointer to the data
 *       - `old_size`: The size of that data
 *       - `new_size`: The new size of the data
 *  - MP_ALLOCTYPE_FREE: Frees a data that has been allocated
 *       - `context`: The allocator context
 *       - `ptr`: The data to be freed
 *       - `new_size`: The size of the data (mostly for logging purpose)
 *       - ignores other parameters
 *
 *  Returns the pointer to the newly allocated memory. May return NULL if allocation failed.
 *  Always returns NULL on MP_ALLOCTYPE_FREE. */
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
} mp_Alloc;

/* Macros that wrap the functions above */

/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * size: number of bytes
 * Returns void* */
#define mp_alloc(alloc, size) ((alloc)->f(MP_ALLOCTYPE_ALLOC, (alloc)->context, (size), 0, NULL))
/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * old_ptr: pointer
 * old_size: number of bytes
 * new_size: number of bytes
 * Returns void* */
#define mp_realloc(alloc, old_ptr, old_size, new_size)                                             \
    ((alloc)->f(MP_ALLOCTYPE_REALLOC, (alloc)->context, (new_size), (old_size), (old_ptr)))
/* alloc: mp_Alloc* (NO SIDE EFFECTS)
 * ptr: pointer (nullability depends on the allocator implementation)
 * size: number of bytes
 * Returns NULL */
#define mp_free(alloc, ptr, size)                                                                  \
    ((alloc)->f(MP_ALLOCTYPE_FREE, (alloc)->context, (size), 0, (ptr)))
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
 * defined. For details see the implementation. */
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

/* Starting capacity of a dynamic array. You can adjust this to your liking. */
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
 * typedef mp_da_create(int) ArrayInt;
 * ```
 *
 * type: typename */
#define mp_da_create(type)                                                                         \
    struct {                                                                                       \
        mp_Alloc *alloc;                                                                           \
        size_t    len;                                                                             \
        size_t    cap;                                                                             \
        type     *data;                                                                            \
    }

/* Initializes a new dynamic array and tell it to use `alloc`.
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
        mp_free((a)->alloc, (a)->data, (a)->cap);                                                  \
        ___MP_ZERO(a);                                                                             \
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
        __typeof__(*(a)->data) items[]  = { __VA_ARGS__ };                                         \
        size_t                 len      = sizeof(items) / sizeof(*items);                          \
        size_t                 prev_len = (a)->len;                                                \
        mp_da_grow((a), len);                                                                      \
        if ((a)->data != NULL) memcpy((a)->data + prev_len, items, sizeof(items));                 \
    } while (0)

/* Resizes a dynamic array and appends items in an array to the end.
 * `data` becomes NULL if allocation failed.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * items: <array of item type>
 * items_len: size_t */
#define mp_da_append_array(a, items, items_len)                                                    \
    do {                                                                                           \
        size_t prev_len = (a)->len;                                                                \
        mp_da_grow((a), (items_len));                                                              \
        if ((a)->data != NULL)                                                                     \
            memcpy((a)->data + prev_len, (items), (items_len) * sizeof(*(a)->data));               \
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
#define mp_da_get_safe(a, i)  (___MP_BOUNDS_CHECK((i), (a)->len), (a)->data[i])
#define mp_da_getp_safe(a, i) (___MP_BOUNDS_CHECK((i), (a)->len), (a)->data + i)

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
        size_t off = (offset);                                                                     \
        if ((a)->len + (off) > (a)->cap && (off) > 0) {                                            \
            size_t old_cap = (a)->cap;                                                             \
            if ((a)->cap == 0) {                                                                   \
                (a)->cap = MP_DARRAY_INIT_CAPACITY;                                                \
            }                                                                                      \
            while ((a)->len + (off) > (a)->cap) {                                                  \
                (a)->cap *= 2;                                                                     \
            }                                                                                      \
            (a)->data = mp_realloc((a)->alloc,                                                     \
                                   (a)->data,                                                      \
                                   old_cap * sizeof(*(a)->data),                                   \
                                   (a)->cap * sizeof(*(a)->data));                                 \
        }                                                                                          \
        if ((a)->data != NULL) (a)->len += (off);                                                  \
    } while (0)
#define mp_da_shrink(a, offset)                                                                    \
    do {                                                                                           \
        MEMPLUS_ASSERT_MSG((offset) <= (a)->len, "`offset` is  out of bounds");                    \
        size_t off = (offset);                                                                     \
        if ((a)->len - (off) > (a)->cap && (off) > 0) {                                            \
            size_t old_cap = (a)->cap;                                                             \
            if ((a)->cap == 0) {                                                                   \
                (a)->cap = MP_DARRAY_INIT_CAPACITY;                                                \
            }                                                                                      \
            (a)->data = mp_realloc((a)->alloc,                                                     \
                                   (a)->data,                                                      \
                                   old_cap * sizeof(*(a)->data),                                   \
                                   (a)->cap * sizeof(*(a)->data));                                 \
        }                                                                                          \
        if ((a)->data != NULL) (a)->len -= (off);                                                  \
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
        size_t p        = (pos);                                                                   \
        size_t actual_p = p > (a)->len ? (a)->len : p;                                             \
        mp_da_grow((a), 1);                                                                        \
        if ((a)->data != NULL) {                                                                   \
            for (size_t i = (a)->len - 2; i > actual_p; --i)                                       \
                (a)->data[i + 1] = (a)->data[i];                                                   \
            (a)->data[actual_p + 1] = (a)->data[actual_p];                                         \
            (a)->data[actual_p]     = (item);                                                      \
        }                                                                                          \
    } while (0)

/* Deletes an item at the given `pos`. This operation is O(n) on the worst case.
 *
 * a: DArray* (NO SIDE EFFECTS)
 * pos: size_t */
#define mp_da_delete(a, pos)                                                                       \
    do {                                                                                           \
        ___MP_BOUNDS_CHECK((pos), (a)->len);                                                       \
        size_t p = (pos);                                                                          \
        mp_da_shrink((a), 1);                                                                      \
        for (size_t i = (p) + 1; i < (a)->len + 1; ++i)                                            \
            (a)->data[i - 1] = (a)->data[i];                                                       \
    } while (0)

/* Deletes an item at the given `pos`. This operation is O(1).
 *
 * a: Vector* (NO SIDE EFFECTS)
 * pos: size_t */
#define mp_da_unordered_delete(a, pos)                                                             \
    do {                                                                                           \
        ___MP_BOUNDS_CHECK((pos), (a)->len);                                                       \
        size_t p = (pos);                                                                          \
        mp_da_shrink((a), 1);                                                                      \
        if ((p) != (a)->len) (a)->data[p] = (a)->data[(a)->len];                                   \
    } while (0)

/***********
 * STRING
 ***********/

/* Holds a NULL-TERMINATED string and the size of the string (excluding the null-terminator).
 * If its allocation has failed, `cstr` == NULL and `len` == 0. */
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

/* Allocates and returns a new `mp_Str` from a NULL-TERMINATED string.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new(mp_Alloc *alloc, const char *str);
/* Allocates and returns a new `mp_Str` from a string.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_new_len(mp_Alloc *alloc, const char *str, size_t len);
/* Allocates and returns a new `mp_Str` from formatted input.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_newf(mp_Alloc *alloc, const char *fmt, ...) ___MP_PRINTF_FORMAT(2);
/* Allocates and returns a clone of `str`.
 * Returns invalid string if allocation failed. */
mp_Str mp_str_clone(mp_Alloc *alloc, const mp_Str *str);
/* Frees an `mp_Str`. */
void mp_str_deinit(mp_Alloc *alloc, mp_Str *str);

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
typedef mp_da_create(char) mp_StrBuilder;

/* Appends a NULL-TERMINATED string to a `mp_StrBuilder`.
 * `data` becomes NULL if allocation failed. */
void mp_str_builder_append(mp_StrBuilder *sb, const char *str);
/* Appends a formatted string to a `mp_StrBuilder`.
 * `data` becomes NULL if allocation failed. */
void mp_str_builder_appendf(mp_StrBuilder *sb, const char *fmt, ...) ___MP_PRINTF_FORMAT(2);
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
 * IMPLEMENTATION
 ***********/

#include <stdarg.h>
#include <string.h>

#ifdef MEMPLUS_IMPLEMENTATION

#define DIV_ROUNDUP(a, b)  (((a) + (b) - 1) / (b))
#define ALIGN(a, inc)      (DIV_ROUNDUP((a), (inc)) * (inc))
#define ALIGN_DOWN(a, inc) (((a) / (inc)) * (inc))
#define UNREACHABLE()      MEMPLUS_ASSERT_MSG(0, "Unreachable")
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
mp_arena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_sarena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_heap_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);

void *mp_dup(mp_Alloc *alloc, void *data, size_t size) {
    void *buf = mp_alloc(alloc, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

void *
mp_allocator_handle_realloc(mp_Alloc *alloc, void *old_ptr, size_t old_size, size_t new_size) {
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
    ___MP_ZERO(a);
}

mp_Alloc mp_arena_alloc(const mp_Arena *a) {
    return mp_alloc_new(a, mp_arena_alloc_func);
}

static void *
mp_arena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_Arena *ctx   = context;
    mp_Alloc  alloc = mp_alloc_new(ctx, mp_arena_alloc_func);

    switch (type) {
        case MP_ALLOCTYPE_ALLOC: {
            (void) old_size;
            (void) ptr;

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
        case MP_ALLOCTYPE_REALLOC: {
            return mp_allocator_handle_realloc(&alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCTYPE_FREE: {
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
    mp_free(a->alloc, a->buf, a->cap);
    ___MP_ZERO(a);
}

mp_Alloc mp_sarena_allocator(const mp_SArena *a) {
    return mp_alloc_new(a, mp_sarena_alloc_func);
}

static void *mp_sarena_alloc_func(
    mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_SArena *ctx   = context;
    mp_Alloc   alloc = mp_alloc_new(ctx, mp_sarena_alloc_func);

    switch (type) {
        case MP_ALLOCTYPE_ALLOC: {
            (void) old_size;
            (void) ptr;

            size_t alloc_size = ALIGN(new_size, sizeof(uintptr_t));

            MEMPLUS_ASSERT(ctx->len % sizeof(uintptr_t) == 0);
            if (ctx->len + alloc_size > ctx->cap) return NULL;

            void *result = ctx->buf + ctx->len;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCTYPE_REALLOC: {
            return mp_allocator_handle_realloc(&alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOCTYPE_FREE: {
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

mp_Alloc mp_heap_allocator(void) {
    return mp_alloc_new(NULL, mp_heap_alloc_func);
}

static void *
mp_heap_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    (void) context;

    switch (type) {
        case MP_ALLOCTYPE_ALLOC: {
            (void) old_size;
            (void) ptr;

            return MEMPLUS_ALLOC(new_size);
        } break;
        case MP_ALLOCTYPE_REALLOC: {
            if (new_size <= old_size) return ptr;
            return MEMPLUS_REALLOC(ptr, new_size);
        } break;
        case MP_ALLOCTYPE_FREE: {
            (void) old_size;
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

mp_Str mp_str_clone(mp_Alloc *alloc, const mp_Str *str) {
    int len = snprintf(NULL, 0, "%s", str->cstr);
    MEMPLUS_ASSERT_MSG(len >= 0 || (size_t) len != str->len, "Failed to count string length");
    char *ptr = mp_dup(alloc, str->cstr, (size_t) len + 1);
    if (ptr == NULL) return mp_str_invalid();
    return (mp_Str) { .len = (size_t) len, .cstr = ptr };
}

void mp_str_deinit(mp_Alloc *alloc, mp_Str *str) {
    mp_free(alloc, str->cstr, str->len + 1);
    ___MP_ZERO(str);
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

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_H__ */
