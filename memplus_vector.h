#ifndef MEMPLUS_VECTOR_H__
#define MEMPLUS_VECTOR_H__

#include "memplus_alloc.h"

#ifndef MP_VECTOR_INIT_SIZE
#define MP_VECTOR_INIT_SIZE 256
#endif

// typedef struct {
//     mp_Allocator alloc;
//     size_t       size;
//     size_t       capacity;
//     <type>       *data;
// } Vector;

#define mp_vector_create(name, type)                                                               \
    typedef struct {                                                                               \
        mp_Allocator alloc;                                                                        \
        size_t       size;                                                                         \
        size_t       capacity;                                                                     \
        type        *data;                                                                         \
    } name

#define mp_vector_get(self, i)                                                                     \
    (_MEMPLUS_ASSERT((i) < (self)->size && "index out of bounds"), (self)->data[i])

#define mp_vector_new(allocator)                                                                   \
    { .alloc = (allocator), .size = 0, .capacity = 0, .data = NULL, }

#define mp_vector_resize(self, offset)                                                             \
    do {                                                                                           \
        if ((self)->size + (offset) > (self)->capacity && (offset) > 0) {                          \
            if ((self)->capacity == 0) {                                                           \
                (self)->capacity = MP_VECTOR_INIT_SIZE;                                            \
            }                                                                                      \
            while ((self)->size + (offset) > (self)->capacity) {                                   \
                (self)->capacity *= 2;                                                             \
            }                                                                                      \
            (self)->data = mp_allocator_realloc(                                                   \
                (self)->alloc, (self)->data, 0, (self)->capacity * sizeof(*(self)->data));         \
        }                                                                                          \
        (self)->size += (offset);                                                                  \
    } while (0)

#define mp_vector_append(self, item)                                                               \
    do {                                                                                           \
        mp_vector_resize(self, 1);                                                                 \
        (self)->data[(self)->size - 1] = (item);                                                   \
    } while (0)

#define mp_vector_append_many(self, items_ptr, items_size)                                         \
    do {                                                                                           \
        mp_vector_resize((self), (items_size));                                                    \
        memcpy((self)->data + ((self)->size - (items_size)),                                       \
               (items_ptr),                                                                        \
               (items_size) * sizeof(*(self)->data));                                              \
    } while (0)

#define mp_vector_first(self) (self)->data[0]
#define mp_vector_last(self)  (self)->data[(self)->size - 1]

#define mp_vector_pop(self) (--(self)->size, (self)->data[(self)->size])

#define mp_vector_clear(self)                                                                      \
    do {                                                                                           \
        memset((self)->data, 0, (self)->size * sizeof(*(self)->data));                             \
        (self)->size = 0;                                                                          \
    } while (0)

#define mp_vector_insert(self, item, pos)                                                          \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), 1);                                                               \
        for (ssize_t i = (self)->size - 2; i > actual_pos; --i)                                    \
            (self)->data[i + 1] = (self)->data[i];                                                 \
        (self)->data[actual_pos + 1] = (self)->data[actual_pos];                                   \
        (self)->data[actual_pos]     = (item);                                                     \
    } while (0)

#define mp_vector_insert_many(self, items_ptr, pos, amount)                                        \
    do {                                                                                           \
        size_t actual_pos = (pos) > (self)->size ? (self)->size : (pos);                           \
        mp_vector_resize((self), (amount));                                                        \
        for (ssize_t i = (self)->size - 1 - (amount); i > actual_pos; --i)                         \
            (self)->data[i + amount] = (self)->data[i];                                            \
        (self)->data[actual_pos + amount] = (self)->data[actual_pos];                              \
        memcpy((self)->data + actual_pos, (items_ptr), (amount) * sizeof(*(self)->data));          \
    } while (0)

#define mp_vector_erase(self, pos)                                                                 \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) < (self)->size && "index out of bounds");                            \
        mp_vector_resize((self), -1);                                                              \
        for (size_t i = (pos) + 1; i < (self)->size + 1; ++i)                                      \
            (self)->data[i - 1] = (self)->data[i];                                                 \
    } while (0)
#define mp_vector_erase_ret(self, pos)                                                             \
    (self)->data[pos];                                                                             \
    mp_vector_erase((self), (pos))

#define mp_vector_erase_many(self, pos, amount)                                                    \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)
#define mp_vector_erase_many_to_buf(self, buf, pos, amount)                                        \
    do {                                                                                           \
        _MEMPLUS_ASSERT((pos) + (amount) <= (self)->size && "index out of bounds");                \
        mp_vector_resize((self), -(amount));                                                       \
        for (size_t i = 0; i < (amount); ++i)                                                      \
            (buf)[i] = (self)->data[(pos) + i];                                                    \
        for (size_t i = (pos) + (amount); i < (self)->size + (amount); ++i)                        \
            (self)->data[i - (amount)] = (self)->data[i];                                          \
    } while (0)

#endif /* ifndef MEMPLUS_VECTOR_H__ */
