#ifndef MEMPLUS_ALLOC_H__
#define MEMPLUS_ALLOC_H__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef MEMPLUS_ASSERT
#include <assert.h>
#define MEMPLUS_ASSERT assert
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
    void (*free)(void *context);
} mp_Allocator;

#define REGION_DEFAULT_CAP (8 * 1024)
mp_Region *mp_region_new(size_t capacity);
void       mp_region_free(mp_Region *self);

void *mp_arena_alloc(mp_Arena *self, size_t size);
void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size);
void *mp_arena_dup(mp_Arena *self, void *data, size_t size);
void  mp_arena_free(mp_Arena *self);

#define mp_allocator_alloc(self, size) ((self).alloc((self).context, (size)))
#define mp_allocator_realloc(self, old_ptr, old_size, new_size)                                    \
    ((self).realloc((self).context, (old_ptr), (old_size), (new_size)))
#define mp_allocator_dup(self, data, size) ((self).dup((self).context, (data), (size)))
#define mp_allocator_free(self)            ((self).free((self).context))

mp_Allocator mp_arena_allocator_new(mp_Arena *arena);


#ifdef MEMPLUS_ALLOC_IMPLEMENTATION

mp_Region *mp_region_new(size_t capacity) {
    size_t     bytes  = sizeof(mp_Region) + sizeof(uintptr_t) * capacity;
    mp_Region *region = malloc(bytes);
    MEMPLUS_ASSERT(region != NULL);
    region->next     = NULL;
    region->count    = 0;
    region->capacity = capacity;
    return region;
}

void mp_region_free(mp_Region *self) {
    free(self);
}

void *mp_arena_alloc(mp_Arena *self, size_t size) {
    // size in word/qword (64 bits)
    size_t size_word = (size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    if (self->end == NULL) {
        MEMPLUS_ASSERT(self->begin == NULL);
        size_t capacity = REGION_DEFAULT_CAP;
        if (capacity < size_word) capacity = size_word;
        self->end   = mp_region_new(capacity);
        self->begin = self->end;
    }

    while (self->end->count + size_word > self->end->capacity && self->end->next != NULL) {
        self->end = self->end->next;
    }

    if (self->end->count + size_word > self->end->capacity) {
        MEMPLUS_ASSERT(self->end->next == NULL);
        size_t capacity = REGION_DEFAULT_CAP;
        if (capacity < size_word) capacity = size_word;
        self->end->next = mp_region_new(capacity);
        self->end       = self->end->next;
    }

    void *result = &self->end->data[self->end->count];
    self->end->count += size_word;
    return result;
}

void *mp_arena_realloc(mp_Arena *self, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size <= old_size) return old_ptr;
    void    *new_ptr      = mp_arena_alloc(self, new_size);
    uint8_t *new_ptr_byte = (uint8_t *)new_ptr;
    uint8_t *old_ptr_byte = (uint8_t *)old_ptr;
    for (size_t i = 0; i < old_size; ++i) {
        new_ptr_byte[i] = old_ptr_byte[i];
    }
    return new_ptr;
}

void *mp_arena_dup(mp_Arena *self, void *data, size_t size) {
    return memcpy(mp_arena_alloc(self, size), data, size);
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

mp_Allocator mp_arena_allocator_new(mp_Arena *arena) {
    mp_Allocator allocator = {
        (void *)arena,
        (void *(*)(void *, size_t))mp_arena_alloc,
        (void *(*)(void *, void *, size_t, size_t))mp_arena_realloc,
        (void *(*)(void *, void *, size_t))mp_arena_dup,
        (void (*)(void *))mp_arena_free,
    };
    return allocator;
}

#endif /* ifdef MEMPLUS_ALLOC_IMPLEMENTATION */

#endif /* ifndef MEMPLUS_ALLOC_H__ */
