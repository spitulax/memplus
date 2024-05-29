#ifndef MEMPLUS_ALLOC_H__
#define MEMPLUS_ALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MEMPLUS_ASSERT
#include <assert.h>
#define _MEMPLUS_ASSERT assert
#endif

#ifndef MP_REGION_DEFAULT_SIZE
#define MP_REGION_DEFAULT_SIZE (8 * 1024)
#endif

typedef struct mp_Region mp_Region;

struct mp_Region {
    mp_Region *next;
    size_t     count;
    size_t     capacity;
    uintptr_t  data[];
};

typedef struct {
    mp_Region *begin, *end;
} mp_Arena;

typedef struct {
    void *context;
    void *(*alloc)(void *context, size_t size);
    void *(*realloc)(void *context, void *old_ptr, size_t old_size, size_t new_size);
    void *(*dup)(void *context, void *data, size_t size);
} mp_Allocator;

mp_Region *mp_region_new(size_t capacity);
void       mp_region_free(mp_Region *self);

mp_Arena     mp_arena_init(void);
void         mp_arena_deinit(mp_Arena *self);
mp_Allocator mp_arena_new_allocator(mp_Arena *arena);

#define mp_allocator_alloc(self, size)  ((self).alloc((self).context, (size)))
#define mp_allocator_create(self, type) ((self).alloc((self).context, (sizeof(type))))
#define mp_allocator_realloc(self, old_ptr, old_size, new_size)                                    \
    ((self).realloc((self).context, (old_ptr), (old_size), (new_size)))
#define mp_allocator_dup(self, data, size) ((self).dup((self).context, (data), (size)))
#define mp_allocator_new(context, alloc_func, realloc_func, dup_func)                              \
    ((mp_Allocator){                                                                               \
        (void *) (context),                                                                        \
        (void *(*) (void *, size_t))(alloc_func),                                                  \
        (void *(*) (void *, void *, size_t, size_t))(realloc_func),                                \
        (void *(*) (void *, void *, size_t))(dup_func),                                            \
    })

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

mp_Arena mp_arena_init(void) {
    return (mp_Arena){ 0 };
}

void mp_arena_deinit(mp_Arena *self) {
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
