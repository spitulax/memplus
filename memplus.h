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

#include <stdint.h>

/* Must have the same signature as stdlib's `assert`. */
#ifndef MEMPLUS_ASSERT
#include <assert.h>
#define MEMPLUS_ASSERT assert
#endif

/* Must have the same behavior as stdlib's `calloc(..., 1). */
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
} mp_Allocator;

/* Macros that wrap the functions above */

/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * size: number of bytes
 * Returns void* */
#define mp_alloc(alloc, size) ((alloc)->f(MP_ALLOCTYPE_ALLOC, (alloc)->context, (size), 0, NULL))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * old_ptr: pointer
 * old_size: number of bytes
 * new_size: number of bytes
 * Returns void* */
#define mp_realloc(alloc, old_ptr, old_size, new_size)                                             \
    ((alloc)->f(MP_ALLOCTYPE_REALLOC, (alloc)->context, (new_size), (old_size), (old_ptr)))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * ptr: pointer (nullability depends on the allocator implementation)
 * size: number of bytes
 * Returns NULL */
#define mp_free(alloc, ptr, size)                                                                  \
    ((alloc)->f(MP_ALLOCTYPE_FREE, (alloc)->context, (size), 0, (ptr)))
/* Allocate a new chunk of memory for the given type.
 *
 * alloc: mp_Allocator*
 * type: typename
 * Returns `type`* */
#define mp_create(alloc, type) (mp_alloc((alloc), sizeof(type)))
/* alloc: mp_Allocator* (NO SIDE EFFECTS)
 * data: pointer
 * size: number of bytes
 * Returns void* */
void *mp_dup(mp_Allocator *alloc, void *data, size_t size);

/* Creates a custom allocator given the context and the allocation function.
 *
 * ctx (context): pointer
 * func: mp_AllocFunc
 * Returns mp_Allocator */
#define mp_allocator_new(ctx, func)                                                                \
    ((mp_Allocator) {                                                                              \
        .context = (void *) (ctx),                                                                 \
        .f       = (func),                                                                         \
    })

/* Handles reallocation for custom allocators.
 * You can slot this into your allocator function as long as alloc and free functionalities are
 * defined. For details see the implementation. */
void *
mp_allocator_handle_realloc(mp_Allocator *alloc, void *old_ptr, size_t old_size, size_t new_size);

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
mp_Allocator mp_arena_allocator(const mp_Arena *a);

/* STATIC ARENA ALLOCATOR.
 * Allocations are cancelled and return NULL if the requested size is bigger than the remaining
 * capacity. */
typedef struct {
    uintptr_t *buf;    // The arena buffer (of size `cap`)
    size_t     len;    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t     cap;    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
    mp_Allocator *alloc;    // The allocator used to manage `buf`
} mp_SArena;

/* Initializes and allocates a static arena. `cap` in bytes.
 * `cap` will be ROUNDED UP to the nearest increment of `sizeof(uintptr_t)`. */
void mp_sarena_init(mp_SArena *a, mp_Allocator *alloc, size_t cap);
/* Resets the size of the arena. */
void mp_sarena_reset(mp_SArena *a);
/* Frees the arena. */
void mp_sarena_deinit(mp_SArena *a);
/* Returns an allocator that works with `mp_SArena`. */
mp_Allocator mp_sarena_allocator(const mp_SArena *a);

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
mp_Allocator mp_temp_allocator(const mp_Temp *t);

/***********
 * IMPLEMENTATION
 ***********/

#include <string.h>

#ifdef MEMPLUS_IMPLEMENTATION

#define DIV_ROUNDUP(a, b)  (((a) + (b) - 1) / (b))
#define ALIGN(a, inc)      (DIV_ROUNDUP((a), (inc)) * (inc))
#define ALIGN_DOWN(a, inc) (((a) / (inc)) * (inc))
#define ZERO(ptr)          memset((ptr), 0, sizeof(*(ptr)))
#define UNREACHABLE()      assert(0 && "unreachable")
#define MAX(a, b)          ((a) > (b) ? (a) : (b))
#define MIN(a, b)          ((a) < (b) ? (a) : (b))
#define ASSERT_OVERLAP(a, a_len, b, b_len)                                                         \
    do {                                                                                           \
        auto _a = (uintptr_t) a;                                                                   \
        auto _b = (uintptr_t) b;                                                                   \
        if (MAX((_a), (_b)) < MIN((_a) + (a_len), (_b) + (b_len))) {                               \
            assert(0 && "Memory overlaps");                                                        \
        }                                                                                          \
    } while (0)

static void *
mp_arena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);
static void *
mp_sarena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr);

void *mp_dup(mp_Allocator *alloc, void *data, size_t size) {
    void *buf = mp_alloc(alloc, size);
    if (buf == NULL) return NULL;
    return memcpy(buf, data, size);
}

void *
mp_allocator_handle_realloc(mp_Allocator *alloc, void *old_ptr, size_t old_size, size_t new_size) {
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
    ZERO(a);
}

mp_Allocator mp_arena_allocator(const mp_Arena *a) {
    return mp_allocator_new(a, mp_arena_alloc_func);
}

static void *
mp_arena_alloc_func(mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_Arena    *ctx   = context;
    mp_Allocator alloc = mp_allocator_new(ctx, mp_arena_alloc_func);

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

void mp_sarena_init(mp_SArena *a, mp_Allocator *alloc, size_t cap) {
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
    ZERO(a);
}

mp_Allocator mp_sarena_allocator(const mp_SArena *a) {
    return mp_allocator_new(a, mp_sarena_alloc_func);
}

static void *mp_sarena_alloc_func(
    mp_AllocType type, void *context, size_t new_size, size_t old_size, void *ptr) {
    mp_SArena   *ctx   = context;
    mp_Allocator alloc = mp_allocator_new(ctx, mp_sarena_alloc_func);

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

mp_Allocator mp_temp_allocator(const mp_Temp *t) {
    return mp_allocator_new(t, mp_sarena_alloc_func);
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_H__ */
