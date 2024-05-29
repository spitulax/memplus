#ifndef MEMPLUS_ALLOC_H__
#define MEMPLUS_ALLOC_H__

/* #ifdef MEMPLUS_ALLOC_IMPLEMENTATION */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MEMPLUS_ASSERT
#include <assert.h>
#define _MEMPLUS_ASSERT assert
#endif

/* Default size of a single region in qwords. You can adjust this to your liking. */
#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (8 * 1024)
#endif

typedef struct mp_Region mp_Region;

/* Holds certain size of allocated memory. */
struct mp_Region {
    mp_Region *next;        // The next region in the linked list if any
    size_t     count;       // The amount of data (in qwords) used
    size_t     capacity;    // The amount of data (in qwords) allocated
    uintptr_t  data[];      // The data (aligned)
};

/* Manages regions in a linked list. */
typedef struct {
    mp_Region *begin, *end;
} mp_Arena;

/* Interface to wrap functions to allocate memory.
 * The method of allocation can be costumized by the user. */
typedef struct {
    // The object that manages or holds the memory.
    // In case of allocator that works with global memory, this could be specified as NULL.
    void *context;
    /* These functions is not meant to be used directly.
     * To call them use the macros defined below. */
    // Allocates the memory.
    void *(*alloc)(void *context, size_t size);
    // Takes a pointer to a data and reallocate it with a new size.
    void *(*realloc)(void *context, void *old_ptr, size_t old_size, size_t new_size);
    // Takes a pointer to a data and allocate its duplicate.
    void *(*dup)(void *context, void *data, size_t size);
} mp_Allocator;

/* Macros that wrap the functions above */
#define mp_allocator_alloc(self, size)  ((self).alloc((self).context, (size)))
#define mp_allocator_create(self, type) ((self).alloc((self).context, (sizeof(type))))
#define mp_allocator_realloc(self, old_ptr, old_size, new_size)                                    \
    ((self).realloc((self).context, (old_ptr), (old_size), (new_size)))
#define mp_allocator_dup(self, data, size) ((self).dup((self).context, (data), (size)))

/* Creates a custom allocator given the context and respective function pointers. */
#define mp_allocator_new(context, alloc_func, realloc_func, dup_func)                              \
    ((mp_Allocator){                                                                               \
        (void *) (context),                                                                        \
        (void *(*) (void *, size_t))(alloc_func),                                                  \
        (void *(*) (void *, void *, size_t, size_t))(realloc_func),                                \
        (void *(*) (void *, void *, size_t))(dup_func),                                            \
    })

/* Allocates a new region with `capacity` * 8  bytes of size. */
mp_Region *mp_region_new(size_t capacity);
/* Frees region from memory. */
void mp_region_free(mp_Region *self);

/* Creates a new, unallocated arena. */
#define mp_arena_new()                                                                             \
    (mp_Arena) {                                                                                   \
        0                                                                                          \
    }
/* Frees the arena and its regions. */
void mp_arena_free(mp_Arena *self);
/* Returns an allocator that works with `arena`. */
mp_Allocator mp_arena_new_allocator(mp_Arena *arena);

/* Functions that are used by `mp_arena_new_allocator` to define the arena allocator. */
static void *mp_arena_alloc(mp_Arena *self, size_t size);
static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size);
static void *mp_arena_dup(mp_Arena *self, void *data, size_t size);


#ifdef MEMPLUS_ALLOC_IMPLEMENTATION

mp_Region *mp_region_new(size_t capacity) {
    size_t     bytes  = sizeof(mp_Region) + sizeof(uintptr_t) * capacity;
    mp_Region *region = calloc(bytes, 1);
    _MEMPLUS_ASSERT(region != NULL);
    region->next     = NULL;
    region->count    = 0;
    region->capacity = capacity;
    return region;
}

void mp_region_free(mp_Region *self) {
    free(self);
}

void mp_arena_free(mp_Arena *self) {
    mp_Region *region = self->begin;
    while (region) {
        mp_Region *region_temp = region;
        region                 = region->next;
        mp_region_free(region_temp);
    }
    self->begin = NULL;
    self->end   = NULL;
}

mp_Allocator mp_arena_new_allocator(mp_Arena *arena) {
    return mp_allocator_new(arena, mp_arena_alloc, mp_arena_realloc, mp_arena_dup);
}

static void *mp_arena_alloc(mp_Arena *self, size_t size) {
    // size in word/qword (64 bits)
    size_t size_word = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (self->end == NULL) {
        _MEMPLUS_ASSERT(self->begin == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end   = mp_region_new(capacity);
        self->begin = self->end;
    }

    while (self->end->count + size_word > self->end->capacity && self->end->next != NULL) {
        self->end = self->end->next;
    }

    if (self->end->count + size_word > self->end->capacity) {
        _MEMPLUS_ASSERT(self->end->next == NULL);
        size_t capacity = MP_REGION_DEFAULT_SIZE;
        if (capacity < size_word) capacity = size_word;
        self->end->next = mp_region_new(capacity);
        self->end       = self->end->next;
    }

    void *result = &self->end->data[self->end->count];
    self->end->count += size_word;
    return result;
}

static void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void    *new_ptr      = mp_arena_alloc(self, new_size);
    uint8_t *new_ptr_byte = (uint8_t *) new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *) old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

static void *mp_arena_dup(mp_Arena *self, void *data, size_t size) {
    return memcpy(mp_arena_alloc(self, size), data, size);
}

#endif /* ifdef MEMPLUS_ALLOC_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_ALLOC_H__ */
