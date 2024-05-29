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

#ifndef MEMPLUS_VECTOR_H__
#define MEMPLUS_VECTOR_H__

/* #define MEMPLUS_VECTOR_IMPLEMENTATION */

#ifdef MEMPLUS_VECTOR_IMPLEMENTATION
#define MEMPLUS_ALLOC_IMPLEMENTATION
#endif
#include "memplus_alloc.h"

/* Starting capacity of a vector. You can adjust this to your liking. */
#ifndef MP_VECTOR_INIT_CAPACITY
#define MP_VECTOR_INIT_CAPACITY 256
#endif

/* You can define a vector struct with any type as long as it's in this format. */
/*
    typedef struct {
        mp_Allocator alloc;     // The allocator that manages the allocation of the vector
        size_t       size;      // The size of the vector
        size_t       capacity;  // The capacity of the vector
        <type>       *data;     // Pointer to the data (points to the first element)
        // The data is continuous in memory.
    } Vector;
*/

/* Defines a compatible vector struct given `name` and the data `type`. */
#define mp_vector_create(name, type)                                                               \
    typedef struct {                                                                               \
        mp_Allocator alloc;                                                                        \
        size_t       size;                                                                         \
        size_t       capacity;                                                                     \
        type        *data;                                                                         \
    } name

/* Initializes a new vector and tell it to use `allocator`. */
#define mp_vector_new(allocator)                                                                   \
    { .alloc = (allocator), .size = 0, .capacity = 0, .data = NULL, }

/* Gets an item at index `i`. */
#define mp_vector_get(self, i)                                                                     \
    (_MEMPLUS_ASSERT((i) < (self)->size && "index out of bounds"), (self)->data[i])

/* Resizes vector to `offset` of the current size.
 * If the current capacity is 0, allocates for `MP_VECTOR_INIT_CAPACITY` items.
 * If the current capacity is not large enough, allocates for double the current capacity.
 * Positive `offset` grows the vector.
 * Negative `offset` shrinks the vector. */
#define mp_vector_resize(self, offset)                                                             \
    do {                                                                                           \
        if ((self)->size + (offset) > (self)->capacity && (offset) > 0) {                          \
            if ((self)->capacity == 0) {                                                           \
                (self)->capacity = MP_VECTOR_INIT_CAPACITY;                                        \
            }                                                                                      \
            while ((self)->size + (offset) > (self)->capacity) {                                   \
                (self)->capacity *= 2;                                                             \
            }                                                                                      \
            (self)->data = mp_allocator_realloc(                                                   \
                (self)->alloc, (self)->data, 0, (self)->capacity * sizeof(*(self)->data));         \
        }                                                                                          \
        (self)->size += (offset);                                                                  \
    } while (0)

/* Changes the capacity of the vector.
 * Shrinks the vector `capacity` is smaller than the current size.
 * Reallocate the vector if `capacity` is larger than the current capacity. */
#define mp_vector_reserve(self, new_capacity)                                                      \
    do {                                                                                           \
        if ((new_capacity) < (self)->size) {                                                       \
            mp_vector_resize((self), (new_capacity) - (self)->size);                               \
        } else if ((new_capacity) > (self)->capacity) {                                            \
            (self)->data = mp_allocator_realloc(                                                   \
                (self)->alloc, (self)->data, 0, (new_capacity) * sizeof(*(self)->data));           \
        }                                                                                          \
        (self)->capacity = (new_capacity);                                                         \
    } while (0)

/* Resizes the vector and appends item to the end. */
#define mp_vector_append(self, item)                                                               \
    do {                                                                                           \
        mp_vector_resize(self, 1);                                                                 \
        (self)->data[(self)->size - 1] = (item);                                                   \
    } while (0)

/* Resizes the vector and appends items from `items_ptr` to the end. */
#define mp_vector_append_many(self, items_ptr, items_amount)                                       \
    do {                                                                                           \
        mp_vector_resize((self), (items_amount));                                                  \
        memcpy((self)->data + ((self)->size - (items_amount)),                                     \
               (items_ptr),                                                                        \
               (items_amount) * sizeof(*(self)->data));                                            \
    } while (0)

/* Gets the first or the last item in the vector. */
#define mp_vector_first(self) (self)->data[0]
#define mp_vector_last(self)  (self)->data[(self)->size - 1]

/* Deletes the last item in the vector and returns it. */
#define mp_vector_pop(self) (--(self)->size, (self)->data[(self)->size])

/* Sets the vector size to 0. */
#define mp_vector_clear(self)                                                                      \
    do {                                                                                           \
        (self)->size = 0;                                                                          \
    } while (0)

/* Inserts an item in the given `pos`. */
#define mp_vector_insert(self, pos, item)                                                          \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), 1);                                                               \
        for (ssize_t i = (self)->size - 2; i > actual_pos; --i)                                    \
            (self)->data[i + 1] = (self)->data[i];                                                 \
        (self)->data[actual_pos + 1] = (self)->data[actual_pos];                                   \
        (self)->data[actual_pos]     = (item);                                                     \
    } while (0)

/* Inserts items from `items_ptr` in the given `pos`.
 * Items previously in and after `pos` are shifted. */
#define mp_vector_insert_many(self, pos, items_ptr, amount)                                        \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), (amount));                                                        \
        for (ssize_t i = (self)->size - 1 - (amount); i > actual_pos; --i)                         \
            (self)->data[i + amount] = (self)->data[i];                                            \
        (self)->data[actual_pos + amount] = (self)->data[actual_pos];                              \
        memcpy((self)->data + actual_pos, (items_ptr), (amount) * sizeof(*(self)->data));          \
    } while (0)

/* Deletes an item in the given `pos`. */
#define mp_vector_erase(self, pos)                                                                 \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) < (self)->size && "index out of bounds");                            \
        mp_vector_resize((self), -1);                                                              \
        for (size_t i = (pos) + 1; i < (self)->size + 1; ++i)                                      \
            (self)->data[i - 1] = (self)->data[i];                                                 \
    } while (0)

/* Deletes an item in the given `pos` and return that item. */
#define mp_vector_erase_ret(self, pos)                                                             \
    (self)->data[pos];                                                                             \
    mp_vector_erase((self), (pos))

/* Deletes items from `items_ptr` in the given `pos`.
 * Items are shifted to fill the spaces left by deleted items. */
#define mp_vector_erase_many(self, pos, amount)                                                    \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

/* Same as `mp_vector_erase_many`, but also writes the deleted items to `buf`. */
#define mp_vector_erase_many_to_buf(self, pos, buf, amount)                                        \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = 0; i < (amount); ++i)                                                      \
            (buf)[i] = (self)->data[(pos) + i];                                                    \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

#endif /* ifndef MEMPLUS_VECTOR_H__ */
