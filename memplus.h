/**
 * \mainpage memplus.h
 * \brief A library to help with memory allocation and other useful things in C.
 *
 * # License
 *
 * Copyright 2024 Bintang Adiputra Pratama <bintangadiputra@proton.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * # Changelog
 *
 * ## 0.1.0
 * Initial release.
 */

/**
 * \file memplus.h
 * \brief The one and only header file in the library.
 */

/*
 * For those who prefer to look at source code for documentation.
 * This library is divided into multiple sections.
 * Jump to sections by searching `$ <section name>`.
 *
 * 1.  $ ALLOCATOR INTERFACE
 * 2.  $ DYNAMIC ARRAY
 * 3.  $ STRING
 * 4.  $ STRING BUILDER
 * 5.  $ HASH TABLE (STRING KEY)
 * 6.  $ HASH TABLE (INTEGER KEY)
 * 7.  $ ALLOCATORS
 * 8.  $ UTF-8
 * 9.  $ ERRORS
 * 10. $ IO INTERFACE
 * 11. $ FILE IO
 * 12. $ IMPLEMENTATION
 */

#ifndef __MEMPLUS_H
#define __MEMPLUS_H

/* #define MEMPLUS_IMPLEMENTATION */

/// \cond
#define _POSIX_C_SOURCE 200809l    // also defines X/Open
/// \endcond

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Systems
 */

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

#if __STDC_VERSION__ >= 202311L
    #define __MP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112L
    #define __MP_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
#else
    #define __MP_STATIC_ASSERT(...)
#endif

// Define custom assert by modidying the definition of `__mp_assert_fail()`
#if !(defined(MEMPLUS_ASSERT) && defined(MEMPLUS_ASSERT_MSG))

    #if __STDC_VERSION__ >= 201112L
        #define __MP_NORETURN _Noreturn
    #elif __STDC_VERSION__ >= 202311L
        #define __MP_NORETURN [[noreturn]]
    #endif

    #include <stdio.h>
    #include <stdlib.h>

    #define __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(const char *assertion, const char *file, const char *func,
                                    size_t line, const char *msg);

    #ifdef NDEBUG
        #define MEMPLUS_ASSERT(expr)
        #define MEMPLUS_ASSERT_MSG(expr, msg)
    #else
        #define MEMPLUS_ASSERT(expr)                                                               \
            ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, ""))

        #define MEMPLUS_ASSERT_MSG(expr, msg)                                                      \
            ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, (msg)))
    #endif

#endif

// Assumed have the same behavior as stdlib's `calloc(..., 1)`.
#ifndef MEMPLUS_ALLOC
    #include <stdlib.h>
    #define MEMPLUS_ALLOC(size) calloc((size), 1)
#endif
// Must have the same signature and behavior as stdlib's `free`.
#ifndef MEMPLUS_FREE
    #include <stdlib.h>
    #define MEMPLUS_FREE free
#endif

/// The version of the library.
/**
 * Semver encoded in hexadecimal where two digits represent each element.
 * Example: 0.1.0 -> (0x) 00 01 00
 */
#define MEMPLUS_VERSION (0x000100)

// "Private" macros that are used outside of the implementation block.
#define __MP_ZERO(ptr)            memset((ptr), 0, sizeof(*(ptr)))
#define __MP_BOUNDS_CHECK(i, len) MEMPLUS_ASSERT_MSG((i) < (len), "Array index out of bounds")
#if defined(__GNUC__) || defined(__clang__)
    #define __MP_PRINTF_FORMAT(fmt_index)                                                          \
        __attribute__((format(printf, (fmt_index), (fmt_index) + 1)))
#else
    #define __MP_PRINTF_FORMAT(fmt_index)
#endif

/// Indicates error return for `size_t`.
#define MP_ERROR ((size_t) -1)

/***********
 * $ ALLOCATOR INTERFACE
 *
 * Allocator implementations are defined in other section.
 * Search for `$ ALLOCATORS`
 ***********/

/**
 * \defgroup AllocatorInterface Allocator Interface
 *
 * The allocator interface wraps many kinds of allocators.
 * The implementation of the allocators themselves are located \ref Allocators "here".
 *
 * # Creating Your Own Allocator
 *
 * To create an allocator, all you have to consider is the *allocator function* and the
 * *context*.
 * The \ref mp_AllocFunc "allocator function" is a function that handles the operations requested by
 * the user of your allocator. The function may be given a context, which may contain any data
 * specific to the allocator. For details see \ref mp_AllocFunc "here".
 *
 * You may create a structure for the context. This will be accessible to the allocator function.
 * After that, you can use \ref mp_alloc_new and pass the context and the allocator function to get
 * the allocator interface that works with your allocator.
 *
 * You can request an allocation with \ref GenericAllocMacros "generic allocator macros", which will
 * call the allocator function defined in the interface and pass in the context to the function.
 *
 * ## Example
 *
 * \code
 * void *alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size, void *ptr) {
 *     switch (op) {
 *         case MP_ALLOCOP_ALLOC:   // do something
 *         case MP_ALLOCOP_REALLOC: // do something
 *         case MP_ALLOCOP_FREE:    // do something
 *         case __MP_ALLOCOP_COUNT: assert(0 & "unreachable");
 *     }
 * }
 *
 * int main(void) {
 *     mp_Alloc alloc = mp_alloc_new(NULL, alloc_func);
 *     void *ptr = mp_alloc(alloc, 10);
 *     return 0;
 * }
 * \endcode
 *
 * You can see the implementation of allocators here for more reference.
 *
 * \{
 */

/// Possible operations on \ref mp_AllocFunc.
/**
 * See the documentation for each operation \ref mp_AllocFunc "here".
 */
typedef enum {
    MP_ALLOCOP_ALLOC,
    MP_ALLOCOP_REALLOC,
    MP_ALLOCOP_FREE,
    __MP_ALLOCOP_COUNT,
} mp_AllocOp;

// TODO: Alloc location

/// Function prototype used for allocators.
/**
 * Functions of this type do different things depending on the \a op given.
 * They also use their parameters differently on each type.
 *
 * Operations will ignores parameters that are not listed for them.
 *
 * # Operations
 *
 * - **MP_ALLOCOP_ALLOC**
 *
 *     Allocates a block of memory and returns the pointer to it.
 *
 *     **Notes**
 *     - If \a new_size == 0, does nothing and returns NULL
 *
 *     **Parameters**
 *     - **context**: The allocator context
 *     - **new_size**: The size of the block (in bytes)
 *
 * - **MP_ALLOCOP_REALLOC**
 *
 *     Reallocates a block of memory, i.e. allocates new block, copies over the data from the old
 *     block to the new block then frees the old block. Returns the pointer to the new block.
 *
 *     **Notes**
 *     - If \a old_size <= \a new_size, reallocation does not happen and the function just returns
 * \a ptr.
 *     - If \a new_size == 0, does nothing and returns NULL
 *     - If \a old_size == 0 or \a ptr == NULL., skips copying data and freeing the
 * old block, behaving like **MP_ALLOCOP_ALLOC**
 *
 *     **Parameters**
 *     - **context**: The allocator context
 *     - **ptr**: The pointer to the old block
 *     - **old_size**: The size of the old block
 *     - **new_size**: The new size the new block
 *
 * - **MP_ALLOCOP_FREE**
 *
 *     Frees a block of memory that has been allocated. Always returns NULL.
 *
 *     **Notes**
 *     - If \a ptr == NULL, does nothing
 *
 *     **Parameters**
 *     - **context**: The allocator context
 *     - **ptr**: The block to be freed
 *     - **new_size**: The size of the block
 *
 * \return The pointer to the newly allocated memory. May return NULL if allocation failed. Always
 * returns NULL on **MP_ALLOCOP_FREE**
 */
typedef void *(*mp_AllocFunc)(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                              void *ptr);

/// Inteface to wrap functions to allocate memory.
typedef struct {
    /// Data that is passed to the allocator function.
    /**
     * In case of allocator that works with global memory, this can be specified as NULL.
     */
    void *context;

    /**
     * \brief Function that handles the operations requested by the user of the allocator. See \ref
     * mp_AllocFunc.
     */
    mp_AllocFunc f;
} mp_Alloc;

/**
 * \defgroup GenericAllocMacros Generic Allocator Macros
 *
 * These macros wrap the operations of \ref mp_AllocFunc.
 * By passing an \ref mp_Alloc, these macros will call its allocator function and pass the context
 * and the arguments correctly.
 *
 * \{
 */

/// Calls allocator function with **MP_ALLOCOP_ALLOC**.
/**
 * \param alloc (mp_Alloc) The allocator (NO SIDE EFFECTS)
 * \param size (size_t) The number of bytes that will be allocated
 * \return (void *) The pointer to the allocated block of memory, NULL if allocation failed
 */
#define /* void* */ mp_alloc(/* mp_Alloc */ alloc, /* size_t */ size)                              \
    ((alloc).f(MP_ALLOCOP_ALLOC, (alloc).context, (size), 0, NULL))

/// Calls allocator function with **MP_ALLOCOP_REALLOC**.
/**
 * \param alloc (mp_Alloc) The allocator (NO SIDE EFFECTS)
 * \param old_ptr (void *) The pointer to the block to be reallocated
 * \param old_size (size_t) The size of the block (in bytes)
 * \param new_size (size_t) The size of the new allocated block (in bytes)
 * \return (void *) The pointer to the newly allocated block of memory, NULL if allocation failed
 */
#define /* void* */ mp_realloc(/* mp_Alloc */ alloc, /* void* */ old_ptr, /* size_t */ old_size,   \
                               /* size_t */ new_size)                                              \
    ((alloc).f(MP_ALLOCOP_REALLOC, (alloc).context, (new_size), (old_size), (old_ptr)))

/// Calls allocator function with **MP_ALLOCOP_FREE**.
/**
 * \param alloc (mp_Alloc) The allocator (NO SIDE EFFECTS)
 * \param ptr (void *) The pointer to the block to be freed (nullability depends on the allocator
 * implementation)
 * \param size (size_t) The size of the block (in bytes)
 * \returns (void *) Always NULL
 */
#define /* void* */ mp_free(/* mp_Alloc */ alloc, /* void* */ ptr, /* size_t */ size)              \
    ((alloc).f(MP_ALLOCOP_FREE, (alloc).context, (size), 0, (ptr)))

/// Calls allocator function with **MP_ALLOCOP_ALLOC** with the size of \a type.
/**
 * \param alloc (mp_Alloc) The allocator (NO SIDE EFFECTS)
 * \param type (identifier) The type of the allocated data
 * \returns (\a <Type> *) The pointer to the newly allocated block that has the size of \a type
 */
#define /* <Type>* */ mp_create(/* mp_Alloc */ alloc, /* Type */ type)                             \
    (mp_alloc((alloc), sizeof(type)))

/// Allocates a duplicate of \a data.
/**
 * The function allocates a new block of memory with the same size as \a data (i.e. \a size) and
 * copies the data from \a data to the newly allocated block.
 *
 * \param alloc The allocator
 * \param data The pointer to the block to be cloned
 * \param size The size of the block
 * \return The allocated clone of \a data
 */
void *mp_dup(mp_Alloc alloc, const void *data, size_t size);

/// \}

/// Create an \ref mp_Alloc from \a ctx and \a func.
/**
 * \param ctx (any *) The context passed to the function (automatically casted to void *)
 * \param func (mp_AllocFunc) The allocator function
 * \return An allocator interface that works with the arguments given.
 */
#define /* mp_Alloc */ mp_alloc_new(/* any* */ ctx, /* mp_AllocFunc */ func)                       \
    ((mp_Alloc) {                                                                                  \
        .context = (void *) (ctx),                                                                 \
        .f       = (func),                                                                         \
    })

/// Returns an invalid \ref mp_Alloc.
/**
 * An invalid \ref mp_Alloc requires that field \a f is NULL.
 */
#define /* mp_Alloc */ mp_alloc_invalid()                                                          \
    ((mp_Alloc) {                                                                                  \
        .context = NULL,                                                                           \
        .f       = NULL,                                                                           \
    })

/// Handles reallocation for custom allocators.
/**
 * You can call this function in your allocator function as long as alloc and free
 * functionalities are defined.
 *
 * This function already implements realloc operation for an allocator by using its alloc and free
 * operation.
 *
 * If \a new_size == 0, does nothing and returns NULL.
 *
 * # Example
 * \code
 * // ...
 * case MP_ALLOCOP_REALLOC: {
 *     return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
 * } break;
 * // ...
 * \endcode
 *
 * \param alloc The allocator
 */
void *mp_alloc_handle_realloc(mp_Alloc alloc, void *old_ptr, size_t old_size, size_t new_size);

/// \}

/***********
 * $ DYNAMIC ARRAY
 ***********/

/**
 * \defgroup DynamicArray Dynamic Array
 *
 * Array with dynamic size (e.g. resizable).
 * The elements of dynamic array are stored on the heap and may grow in size by reallocating the
 * memory.
 *
 * Throughout the documentation, a generic dynamic array type is written as \a DynArray.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the dynamic
 * array type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_da_create(int, ArrayInt);
 * \endcode
 *
 * Declare an array then use \ref mp_da_init and pass an allocator to manage the array. This does
 * not allocate the data immediately. But only once you append something to the array.
 * \code
 * ArrayInt array;
 * mp_da_init(&array, alloc);
 * mp_da_append(&array, 0);
 * \endcode
 *
 * By default, arrays start allocating memory for a certain number of elements, and if
 * the array wants more it will reallocate the double of the current capacity.
 *
 * Use \ref mp_da_deinit to free a dynamic array.
 *
 * # Layout
 *
 * Dynamic arrays are structs that have this layout.
 * Any struct which has these fields is a valid dynamic array.
 * Any additional fields after these fields are tolerated.
 *
 * \code
 * struct {
 *     mp_Alloc alloc;
 *     size_t   len;
 *     size_t   cap;
 *     size_t   size;
 *     <Type>   *data;
 * };
 * \endcode
 *
 * **Fields**
 * - **alloc**: The allocator that manages the allocation of the array
 * - **len**: The size of the used data of the array
 * - **cap**: The size of the allocated block holding the data
 * - **size**: The size of an individual datum
 * - **data**: The pointer to the first element of the array (the data are continuous in memory)
 *
 * \{
 */

// Starting capacity of a dynamic array.
#ifndef __MP_DARRAY_INIT_CAPACITY
    #define __MP_DARRAY_INIT_CAPACITY 64
#endif

/// Defines a \ref DynamicArray "dynamic array" struct that holds data of type \a type.
/**
 * \param type (identifier) The type of the data
 * \param name (identifier) The name of the array struct
 */
#define mp_da_create(/* Type */ type, /* identifier */ name)                                       \
    typedef struct {                                                                               \
        mp_Alloc alloc;                                                                            \
        size_t   len;                                                                              \
        size_t   cap;                                                                              \
        size_t   size;                                                                             \
        type    *data;                                                                             \
    } name

// Generic dynamic array type.
typedef struct {
    mp_Alloc alloc;
    size_t   len;
    size_t   cap;
    size_t   size;
    void    *data;
} __mp_DynArray;

/// Initializes a new dynamic array managed by \a allocator.
/**
 * Deinit with \ref mp_da_deinit.
 *
 * \a a should not have been already initialized.
 *
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param type (Type) The type of the array
 * \param a (DynArray *) The array
 * \param alloc(mp_Alloc) The allocator to manage the array
 */
#define mp_da_init(/* Type */ type, /* DynArray* */ a, /* mp_Alloc */ alloc)                       \
    __mp_da_init((a), (alloc), sizeof(*((type *) 0)->data))
void __mp_da_init(void *a, mp_Alloc alloc, size_t size);

/// Frees a dynamic array.
/**
 * \param a (DynArray *) The array
 */
#define mp_da_deinit(/* DynArray* */ a) __mp_da_deinit(a)
void __mp_da_deinit(void *a);

void __mp_da_append(void *a, const void *items, size_t items_len);

/// Appends \a item to a dynamic array.
/**
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param item (<Type>) The item to append to the array
 */
#define mp_da_append(/* DynArray* */ a, /* <Type> */ item)                                         \
    do {                                                                                           \
        __typeof__(item) __it = (item);                                                            \
        __mp_da_append((a), &__it, 1);                                                             \
    } while (0)

/// Appends multiple items to a dynamic array.
/**
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param ... (<Type>...) The items to append to the array
 */
#define mp_da_append_many(/* DynArray* */ a, /* <Type>... */...)                                   \
    do {                                                                                           \
        __typeof__(*(a)->data) __items[] = { __VA_ARGS__ };                                        \
        size_t                 __len     = sizeof(__items) / sizeof(*__items);                     \
        __mp_da_append((a), __items, __len);                                                       \
    } while (0)

/// Appends items in an array to a dynamic array.
/**
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param items (<Type>[]) The array of items to append to the array
 * \param items_len (size_t) The amount of items in the array
 */
#define mp_da_append_array(/* DynArray* */ a, /* <Type>[] */ items, /* size_t */ items_len)        \
    __mp_da_append((a), (items), (items_len))

/// Gets an item at index \a i.
/**
 * No bounds checking, use \ref mp_da_get_s for that.
 *
 * \param a (const DynArray*) The array
 * \param i (size_t) The index to the item
 * \return (<Type>) The item at index \a i
 */
#define /* <Type> */ mp_da_get(/* const DynArray* */ a, /* size_t */ i) (a)->data[i]

/// Gets a pointer to an item at index \a i.
/**
 * No bounds checking, use \ref mp_da_get_s for that.
 *
 * \param a (const DynArray*) The array
 * \param i (size_t) The index to the item
 * \return (<Type> *) The pointer to the item at index \a i
 */
#define /* <Type>* */ mp_da_getp(/* const DynArray* */ a, /* size_t */ i) ((a)->data + i)

/// Gets an item at index \a i with bounds-checking.
/**
 * Asserts that \a i is not out of bounds.
 *
 * The assert won't trigger if `NDEBUG` is defined.
 *
 * \param a (const DynArray*) The array
 * \param i (size_t) The index to the item
 * \return (<Type>) The item at index \a i
 */
#define /* <Type> */ mp_da_get_s(/* const DynArray */ a, /* size_t */ i)                           \
    (__MP_BOUNDS_CHECK((i), (a)->len), (a)->data[i])

/// Gets an a pointer to an item at index \a i with bounds-checking.
/**
 * Asserts that \a i is not out of bounds.
 *
 * The assert won't trigger if `NDEBUG` is defined.
 *
 * \param a (const DynArray*) The array
 * \param i (size_t) The index to the item
 * \return (<Type> *) The pointer to the item at index \a i
 */
#define /* <Type>* */ mp_da_getp_s(/* const DynArray */ a, /* size_t */ i)                         \
    (__MP_BOUNDS_CHECK((i), (a)->len), (a)->data + i)

// Generic dynamic array get function
#define __mp_da_get(type, a, i) (type *) ((char *) (a)->data + (i) * (a)->size)

/// Gets the last item in a dynamic array.
/**
 * \param a (const DynArray *) The array (NO SIDE EFFECTS)
 * \return (<Type>) The the last item
 */
#define /* <Type> */ mp_da_last(/* const DynArray* */ a) (a)->data[(a)->len - 1]

/// Deletes the last item in a dynamic array and returns it.
/**
 * \param a (DynArray *) The array (NO SIDE EFFECTS)
 * \return (<Type>) The last item
 */
#define /* <Type> */ mp_da_pop(/* DynArray* */ a) (--(a)->len, (a)->data[(a)->len])

/// Sets the length of a dynamic array to 0.
/**
 * This resets the dynamic array to "initial condition" but without actually freeing the data.
 *
 * \param a (DynArray *) The array
 */
#define mp_da_reset(/* DynArray* */ a)                                                             \
    do {                                                                                           \
        (a)->len = 0;                                                                              \
    } while (0)

/// Grows a dynamic array by \a offset of the current length.
/**
 * Increases \a DynArray::len by \a offset and does other things if necessary.
 *
 * If \a DynArray::cap is 0, allocates for a certain number of items.
 *
 * If \a DynArray::cap is not large enough, allocates for double the current capacity.
 *
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param offset (size_t) The amount to grow
 */
#define mp_da_grow(/* DynArray* */ a, /* size_t */ offset) __mp_da_grow((a), (offset))
void __mp_da_grow(void *a, size_t offset);

/// Shrinks a dynamic array by \a offset of the current length.
/**
 * Decreases \a DynArray::len by \a offset and does other things if necessary.
 *
 * Asserts if \a offset is too large (i.e. length - offset < 0).
 *
 * \param a (DynArray *) The array
 * \param offset (size_t) The amount to shrink
 */
#define mp_da_shrink(/* DynArray* */ a, /* size_t */ offset) __mp_da_shrink((a), (offset))
void __mp_da_shrink(void *a, size_t offset);

/// Clones a dynamic array to \a dest to be managed by \a allocator.
/**
 * The \a dest array does not inherit the capacity of \a src. Instead it will only
 * allocate for \a src.len + *initial capacity* items.
 *
 * \a dest should not have been already initialized.
 *
 * \a dest->data becomes NULL if allocation failed.
 *
 * \param dest (DynArray *) The destination of the clone
 * \param src (const DynArray *) The source array
 * \param alloc (mp_Alloc) The allocator to manage \a dest
 */
#define mp_da_clone(/* DynArray* */ dest, /* const DynArray* */ src, /* mp_Alloc */ alloc)         \
    __mp_da_clone((dest), (src), (alloc))
void __mp_da_clone(void *dest, const void *src, mp_Alloc alloc);

void __mp_da_insert(void *a, size_t pos, const void *items, size_t items_len);

/// Inserts an item at \a pos.
/**
 * If \a pos > \a DynArray::len, then it just puts the item at \a DynArray::len.
 *
 * \a pos must not be negative.
 *
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param pos (size_t) The position of the item
 * \param item (<Type>) The item to insert
 */
#define mp_da_insert(/* DynArray* */ a, /* size_t */ pos, /* <Type> */ item)                       \
    do {                                                                                           \
        __typeof__(item) __it = (item);                                                            \
        __mp_da_insert((a), (pos), &__it, 1);                                                      \
    } while (0)

/// Inserts multiple items at \a pos.
/**
 * If \a pos > \a DynArray::len, then it just puts the item at \a DynArray::len.
 *
 * \a pos must not be negative.
 *
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param pos (size_t) The position of the item
 * \param ... (<Type>...) The items to insert
 */
#define mp_da_insert_many(/* DynArray* */ a, /* size_t */ pos, /* <Type>... */...)                 \
    do {                                                                                           \
        __typeof__(*(a)->data) __items[] = { __VA_ARGS__ };                                        \
        size_t                 __len     = sizeof(__items) / sizeof(*__items);                     \
        __mp_da_insert((a), (pos), __items, __len);                                                \
    } while (0)

/// Inserts items in an array at \a pos.
/**
 * If \a pos > \a DynArray::len, then it just puts the item at \a DynArray::len.
 *
 * \a pos must not be negative.
 *
 * \a DynArray::data becomes NULL if allocation failed.
 *
 * \param a (DynArray *) The array
 * \param pos (size_t) The position of the item
 * \param items (<Type>[]) The array of items to inserts to the array
 * \param items_len (size_t) The amount of items in the array
 */
#define mp_da_insert_array(/* DynArray* */ a, /* size_t */ pos, /* <Type>[] */ items,              \
                           /* size_t */ items_len)                                                 \
    __mp_da_insert((a), (pos), (items), (items_len))

/// Deletes an item at \a pos.
/**
 * This operation is O(n) in the worst case.
 * If you do not care about the order of the elements after delete, use \ref mp_da_quick_delete
 * instead.
 *
 * \param a (DynArray *) The array
 * \param pos (size_t) The position of the item to delete
 */
#define mp_da_delete(/* DynArray* */ a, /* size_t */ pos) __mp_da_delete((a), (pos))
void __mp_da_delete(void *a, size_t pos);

/// Deletes an item at \a pos if you do not care about the order of items.
/**
 * This operation is O(1) and a much faster alternative to \ref mp_da_delete if you do not care
 * about the order of items.
 *
 * This works by swapping the item to be deleted with the last item an shrinking the array,
 * effectively making it "ignore" the last item.
 *
 * \param a (DynArray *) The array
 * \param pos (size_t) The position of the item to delete
 */
#define mp_da_quick_delete(/* DynArray* */ a, /* size_t */ pos) __mp_da_quick_delete((a), (pos))
void __mp_da_quick_delete(void *a, size_t pos);

/// \}

/***********
 * $ STRING
 ***********/

// TODO: mp_str_eq, mp_str_starts/ends_with, mp_str_concat, mp_str_split, mp_str_trim and other
// helper funcs

/**
 * \defgroup String String
 *
 * Holds a pointer to a **null-terminated** string and its size (excluding the
 * null-terminator).
 *
 * \{
 */

/// Holds a pointer to a **null-terminated** string and its size (excluding the
/// null-terminator).
typedef struct {
    size_t len;
    char  *cstr;
} mp_Str;

/// Returns an invalid \ref mp_Str.
/**
 * An invalid \ref mp_Str requires that field \a cstr is NULL.
 *
 * \return (mp_Str) An invalid string
 */
#define /* mp_Str */ mp_str_invalid()                                                              \
    ((mp_Str) {                                                                                    \
        .len  = 0,                                                                                 \
        .cstr = NULL,                                                                              \
    })

/// Tests if an \ref mp_Str is valid (i.e. field \a cstr is not NULL).
/**
 * Returns true if \a s is valid.
 *
 * \param s (mp_Str) The string
 * \return (bool) Whether \a s is valid
 */
#define /* bool */ mp_str_is_valid(/* mp_Str */ s) ((s).cstr != NULL)

/// Creates a `mp_Str` from a **null-terminated** string.
/**
 * \param str (const char *) The null-terminated string
 * \return (mp_Str) Contains the string and its length
 */
#define /* mp_Str */ mp_str(/* const char* */ str)                                                 \
    ((mp_Str) {                                                                                    \
        .len  = strlen(str),                                                                       \
        .cstr = (str),                                                                             \
    })

/// Allocates and returns a new \ref mp_Str from a **null-terminated** string.
/**
 * Deinit with \ref mp_str_deinit.
 * \ref mp_str is the non-allocating variant of this function.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param alloc The allocator handling the allocation
 * \param str The null-terminated string
 * \return The new \ref mp_Str
 */
mp_Str mp_str_new(mp_Alloc alloc, const char *str);

/// Allocates and returns a new \ref mp_Str from a string.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param alloc The allocator handling the allocation
 * \param str The string
 * \param len The length of the new string
 * \return The new \ref mp_Str
 */
mp_Str mp_str_new_len(mp_Alloc alloc, const char *str, size_t len);

/// Allocates and returns a new \ref mp_Str from formatted input.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param alloc The allocator handling the allocation
 * \param fmt The formatting string
 * \param ... The formatting arguments
 * \return The new \ref mp_Str
 */
mp_Str mp_str_newf(mp_Alloc alloc, const char *fmt, ...) __MP_PRINTF_FORMAT(2);

/// Frees an allocated \ref mp_Str.
/**
 * Be careful to not use this function on an unallocated \ref mp_Str.
 *
 * \param str The string
 * \param alloc The allocator that allocated the string
 */
void mp_str_deinit(mp_Str *str, mp_Alloc alloc);

/// Allocates and returns a clone of an \ref mp_Str.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param str The string to be cloned
 * \param alloc The allocator handling the allocation
 * \return The cloned \ref mp_Str
 */
mp_Str mp_str_clone(const mp_Str *str, mp_Alloc alloc);

/// \}

/***********
 * $ STRING BUILDER
 ***********/

/**
 * \defgroup StringBuilder String Builder
 *
 * Holds a **non null-terminated** string that is resizable.
 * The underlying data type is a \ref DynamicArray "dynamic array" of char.

 * To convert \ref mp_StrBuilder to C string, use \ref mp_str_builder_string which
 * returns a **null-terminated** \ref mp_Str.
 *
 * \{
 */

/// Holds a **non null-terminated** string that is resizable.
/**
 * The underlying data type is a \ref DynamicArray "dynamic array" of char.
 *
 * To convert \ref mp_StrBuilder to C string, use \ref mp_str_builder_string which
 * returns a **null-terminated** \ref mp_Str.
 */
typedef struct {
    mp_Alloc alloc;
    size_t   len;
    size_t   cap;
    size_t   size;
    char    *data;
} mp_StrBuilder;

/// Initializes an \ref mp_StrBuilder to be managed by \a alloc.
/**
 * Deinit with \ref mp_str_builder_deinit.
 *
 * \param sb The string builder
 * \param alloc The managing allocator
 */
void mp_str_builder_init(mp_StrBuilder *sb, mp_Alloc alloc);

/// Frees an \ref mp_StrBuilder.
/**
 * \param sb The string builder
 */
void mp_str_builder_deinit(mp_StrBuilder *sb);

/// Appends a **null-terminated** string to an \ref mp_StrBuilder.
/**
 * \a mp_StrBuilder::data becomes NULL if allocation failed.
 *
 * \param sb The string builder
 * \param str The **null-terminated** string to be appended
 */
void mp_str_builder_append(mp_StrBuilder *sb, const char *str);

/// Appends a formatted string to an \ref mp_StrBuilder.
/**
 * \a mp_StrBuilder::data becomes NULL if allocation failed.
 *
 * \param sb The string builder
 * \param fmt The formatting string
 * \param ... The formatting arguments
 */
void mp_str_builder_appendf(mp_StrBuilder *sb, const char *fmt, ...) __MP_PRINTF_FORMAT(2);

/// Copies the buffer of an \ref mp_StrBuilder into a null-terminated \ref mp_Str.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param sb The string builder
 * \param alloc The allocator that allocates the \ref mp_Str
 * \return The **null-terminated** copy of \a sb
 */
mp_Str mp_str_builder_string(const mp_StrBuilder *sb, mp_Alloc alloc);

/// Same as \ref mp_str_builder_string but also deinitializes \a sb.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * Returns an invalid \ref mp_Str if allocation failed.
 *
 * \param sb The string builder
 * \param alloc The allocator that allocates the \ref mp_Str
 * \return The **null-terminated** copy of \a sb
 */
mp_Str mp_str_builder_string_take(mp_StrBuilder *sb, mp_Alloc alloc);

/// \}

/***********
 * $ HASH TABLE (STRING KEY)
 ***********/

/**
 * \defgroup HashTableString Hash Table (String Key)
 *
 * Hash table with string (**null-terminated**) key.
 * This uses the FNV-1a hash algorithm to hash the string.
 *
 * Throughout the documentation, a generic hash table (string key) type is written as \a
 * StrHashTable. Similarly, its iterator is written as \a StrHashTableIter.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the hash table
 * type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_ht_create(int, StrHashTableInt);
 * \endcode
 *
 * Declare a hash table then use \ref mp_ht_init and pass an allocator to manage the hash table.
 * This does not allocate the data immediately. But only once you set something on the hash table.
 * \code
 * StrHashTableInt ht;
 * mp_ht_init(StrHashTableInt, &ht, alloc);
 * mp_ht_set(&ht, "key", 10);
 * \endcode
 *
 * By default, hash tables start allocating memory for a certain number of elements, and if
 * the hash table wants more it will reallocate the double of the current capacity.
 *
 * Note that the hash table is already reallocated once it hits a fraction of the total capacity.
 * This is to reduce the number of key conflicts which can slow down access.
 *
 * Use \ref mp_ht_deinit to free a string hash table.
 *
 * # Iterator
 *
 * Using \ref mp_ht_create also defines an iterator type for that hash table type, named by
 * suffixing `Iter` to the hash table's type name.
 * Example usage:
 * \code
 * StrHashTableIntIter it;
 * mp_ht_iter_init(&it, &ht);
 * while (mp_ht_iter_next(&it)) {
 *     (void) it.key;
 *     (void) it.val;
 * }
 * \endcode
 *
 * \{
 */

// Percentage of elements in a hash table before it resizes.
#ifndef __MP_HASH_TABLE_MAX_LOAD
    #define __MP_HASH_TABLE_MAX_LOAD 0.75
#endif

// Starting capacity of a hash table.
#ifndef __MP_HASH_TABLE_INIT_CAPACITY
    #define __MP_HASH_TABLE_INIT_CAPACITY __MP_DARRAY_INIT_CAPACITY
#endif

/// Defines a \ref HashTableString "string hash table" struct with value of type \a value_type.
/**
 * This also defines the hash table's iterator type, named by suffixing `Iter`
 * after the hash table's type name.
 *
 * \param value_type (idendifier) The type of the value
 * \param name (identifier) The name of the hash table
 */
#define mp_ht_create(/* Type */ value_type, /* identifier */ name)                                 \
    typedef struct {                                                                               \
        mp_Str     key;                                                                            \
        value_type val;                                                                            \
    } __##name##Entry;                                                                             \
    mp_da_create(__##name##Entry, name);                                                           \
    typedef struct {                                                                               \
        const name *_h;                                                                            \
        size_t      _i;                                                                            \
        mp_Str      key;                                                                           \
        value_type  val;                                                                           \
    } name##Iter

// Generic string hash table entry type.
typedef struct {
    mp_Str key;
    char   val[];
} __mp_StrHashTableEntry;

// Generic string hash table iterator type.
typedef struct {
    const __mp_DynArray *_h;
    size_t               _i;
    mp_Str               key;
    char                 val[];
} __mp_StrHashTableIter;

/// Initializes a new string hash table managed by \a allocator.
/**
 * Deinit with \ref mp_ht_deinit.
 *
 * \a a should not have been already initialized.
 *
 * \a StrHashTable::data becomes NULL if allocation failed.
 *
 * \param type (Type) The type of the data
 * \param ht (StrHashTable *) The hash table
 * \param allocator (mp_Alloc) The allocator to manage the hash table
 */
#define mp_ht_init(/* Type */ type, /* StrHashTable* */ ht, /* mp_Alloc */ allocator)              \
    mp_da_init(type, ht, allocator)

/// Frees a string hash table.
/**
 * \param ht (StrHashTable *) The hash table
 */
#define mp_ht_deinit(/* StrHashTable* */ ht) __mp_ht_deinit(ht)
void __mp_ht_deinit(void *ht);

/// Gets a pointer to an item at key \a k.
/**
 * \a ret becomes NULL if it could not retrieve the item.
 *
 * \param ht (const StrHashTable *) The hash table
 * \param k (const char *) The key (NON-NULL)
 * \return (void *) The retrieved value
 */
#define /* void* */ mp_ht_get(/* const StrHashTable* */ ht, /* const char* */ k)                   \
    mp_ht_get_s((ht), mp_str(k))

/// The same as \ref mp_ht_get but accepts \ref mp_Str.
/**
 * See \ref mp_ht_get.
 *
 * \param ht (const StrHashTable *) The hash table
 * \param k (mp_Str) The key
 * \return (void *) The retrieved value
 */
#define /* void* */ mp_ht_get_s(/* const StrHashTable* */ ht, /* mp_Str */ k) __mp_ht_get((ht), (k))
void *__mp_ht_get(const void *ht, mp_Str k);

/// Sets the value at key \a k to \a v.
/**
 * When the item at \a k has not been initialized before, the key is cloned.
 *
 * \a StrHashTable::data becomes NULL if allocation failed.
 *
 * \param ht (StrHashTable *) The hash table
 * \param k (const char *) The key
 * \param v (<Type>) The value to be stored
 */
#define mp_ht_set(/* StrHashTable* */ ht, /* const char* */ k, /* <Type> */ v)                     \
    mp_ht_set_s((ht), mp_str(k), (v))

/// The same as \ref mp_ht_set but accepts \ref mp_Str.
/**
 * See \ref mp_ht_set.
 *
 * \param ht (StrHashTable *) The hash table
 * \param k (mp_Str) The key
 * \param v (<Type>) The value to be stored
 */
#define mp_ht_set_s(/* StrHashTable* */ ht, /* mp_Str */ k, /* <Type> */ v)                        \
    do {                                                                                           \
        __typeof__(v) __it = (v);                                                                  \
        __mp_ht_set((ht), (k), &__it);                                                             \
    } while (0)
void __mp_ht_set(void *ht, mp_Str k, void *v);

/// Tests if an item at key \a k exists in the given string hash table.
/**
 * \param ht (const StrHashTable *) The hash table
 * \param k (const char *) The key
 * \return (bool) Whether an item at key \a k exists
 */
#define /* bool */ mp_ht_exists(/* const StrHashTable* */ ht, /* const char * */ k)                \
    __mp_ht_exists((ht), mp_str(k))

/// The same as \ref mp_ht_exists but accepts \ref mp_Str.
/**
 * See \ref mp_ht_exists.
 *
 * \param ht (const StrHashTable *) The hash table
 * \param k (mp_Str) The key
 * \return (bool) Whether an item at key \a k exists
 */
#define /* bool */ mp_ht_exists_s(/* const StrHashTable* */ ht, /* mp_Str */ k)                    \
    __mp_ht_exists((ht), (k))
bool __mp_ht_exists(const void *ht, mp_Str k);

/// Grows a string hash table by \a offset of the current length.
/**
 * Increases \a StrHashTable::len by \a offset and does other things if necessary.
 *
 * If \a StrHashTable::cap is 0, allocates for a certain number of items.
 *
 * If \a StrHashTable::cap is not large enough, allocates for double the current capacity.
 *
 * Recalculates the positions of every entry if resized.
 *
 * \a StrHashTable::data becomes NULL if allocation failed.
 *
 * \param ht (StrHashTable *) The hash table
 * \param offset (size_t) The amount to grow
 */
#define mp_ht_grow(/* StrHashTable* */ ht, /* size_t */ offset) __mp_ht_grow((ht), (offset))
void __mp_ht_grow(void *ht, size_t offset);

// Invalidates and frees the string keys
void __mp_ht_free_entries(void *entries, mp_Alloc alloc, size_t cap, size_t size);

/// Sets the length of a string hash table to 0 and frees its keys.
/**
 * This resets the hash table to "initial condition" but without actually freeing the data.
 *
 * \param ht (StrHashTable *) The hash table
 */
#define mp_ht_reset(/* StrHashTable* */ ht) __mp_ht_reset(ht)
void __mp_ht_reset(void *ht);

/// Deletes an item at key \a k.
/**
 * This decreases \a StrHashTable::len but does not actually shrink the hash table, but it just
 * marks the spot as "deleted", which may be overridden by subsequent set operations.
 *
 * \param ht (StrHashTable *) The hash table
 * \param k (const char *) The key
 */
#define mp_ht_delete(/* StrHashTable* */ ht, /* const char* */ k) mp_ht_delete_s((ht), mp_str(k))

/// The same as \ref mp_ht_delete but accepts to \ref mp_Str.
/**
 * See \ref mp_ht_delete.
 *
 * \param ht (StrHashTable *) The hash table
 * \param k (mp_Str) The key
 */
#define mp_ht_delete_s(/* StrHashTable* */ ht, /* mp_Str */ k) __mp_ht_delete((ht), (k))
void __mp_ht_delete(void *ht, mp_Str k);

/// Clones a string hash table to \a dest to be managed by \a allocator.
/**
 * \a dest inherits all fields of \a src.
 * \a dest.data becomes NULL if allocation failed.
 *
 * \param dest (StrHashTable *) Stores the cloned hash table
 * \param src (const StrHashTable *) The hash table to be cloned
 * \param alloc (mp_Alloc) The allocator to manage \a dest
 */
#define mp_ht_clone(/* StrHashTable* */ dest, /* const StrHashTable* */ src, /* mp_Alloc */ alloc) \
    __mp_ht_clone((dest), (src), (alloc))
void __mp_ht_clone(void *dest, const void *src, mp_Alloc alloc);

/// Initializes an iterator on a string hash table.
/**
 * To use hash table iterators, see \ref HashTableString.
 *
 * \param it (StrHashTableIter *) The iterator to initialize
 * \param ht (const StrHashTable *) The hash table to iterate
 */
#define mp_ht_iter_init(/* StrHashTableIter* */ it, /* const StrHashTable* */ ht)                  \
    __mp_ht_iter_init((it), (ht))
void __mp_ht_iter_init(void *it, const void *ht);

/// Get the next element in the iterator.
/**
 * To use hash table iterators, see \ref HashTableString.
 *
 * \param it (StrHashTableIter *) The iterator
 * \return (bool) Whether it is valid to access the data
 */
#define /* bool */ mp_ht_iter_next(/* StrHashTableIter* */ it) __mp_ht_iter_next(it)
bool __mp_ht_iter_next(void *it);

// Hashes a string with FNV-1a hash algorithm.
uint64_t __mp_ht_hash_str(const mp_Str *str);

/// \}

/***********
 * $ HASH TABLE (INTEGER KEY) %
 ***********/

/**
 * \defgroup HashTableInt Hash Table (Integer Key)
 *
 * Hash table with integer key.
 * The keys are stored as \ref size_t, but any type that can be coerced to \ref size_t should work.
 *
 * Throughout the documentation, a generic hash table (integer key) type is written as \a
 * IntHashTable. Similarly, its iterator is written as \a IntHashTableIter.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the hash table
 * type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_hti_create(int, IntHashTableInt);
 * \endcode
 *
 * Declare a hash table then use \ref mp_hti_init and pass an allocator to manage the hash table.
 * This does not allocate the data immediately. But only once you set something on the hash table.
 * \code
 * IntHashTableInt ht;
 * mp_hti_init(IntHashTableInt, &ht, alloc);
 * mp_hti_set(&ht, 0, 10);
 * \endcode
 * Note that zero is a valid key.
 *
 * By default, hash tables start allocating memory for a certain number of elements, and if
 * the hash table wants more it will reallocate the double of the current capacity.
 *
 * Note that the hash table is already reallocated once it hits a fraction of the total capacity.
 * This is to reduce the number of key conflicts which can slow down access.
 *
 * Use \ref mp_hti_deinit to free an integer hash table.
 *
 * # Iterator
 *
 * Using \ref mp_hti_create also defines an iterator type for that hash table type, named by
 * suffixing `Iter` to the hash table's type name.
 * Example usage:
 * \code
 * IntHashTableIntIter it;
 * mp_hti_iter_init(&it, &ht);
 * while (mp_hti_iter_next(&it)) {
 *     (void) it.key;
 *     (void) it.val;
 * }
 * \endcode
 *
 * \{
 */

/// Defines an \ref HashTableInt "integer hash table" struct with value of type \a value_type.
/**
 * This also defines the hash table's iterator type, named by suffixing `Iter`
 * after the hash table's type name.
 *
 * \param value_type (idendifier) The type of the value
 * \param name (identifier) The name of the hash table
 */
#define mp_hti_create(/* Type */ value_type, /* identifier */ name)                                \
    typedef struct {                                                                               \
        __mp_IntHtKey key;                                                                         \
        value_type    val;                                                                         \
    } __##name##Entry;                                                                             \
    mp_da_create(__##name##Entry, name);                                                           \
    typedef struct {                                                                               \
        const name   *_h;                                                                          \
        size_t        _i;                                                                          \
        __mp_IntHtKey key;                                                                         \
        value_type    val;                                                                         \
    } name##Iter

// The key type is wrapped by this struct so it can have 0 as a key.
typedef struct {
    size_t key;
    bool   valid;
} __mp_IntHtKey;

// Generic int hash table entry type.
typedef struct {
    __mp_IntHtKey key;
    char          val[];
} __mp_IntHashTableEntry;

// Generic string hash table iterator type.
typedef struct {
    const __mp_DynArray *_h;
    size_t               _i;
    __mp_IntHtKey        key;
    char                 val[];
} __mp_IntHashTableIter;

/// Initializes a new integer hash table managed by \a allocator.
/**
 * Deinit with \ref mp_hti_deinit.
 *
 * \a a should not have been already initialized.
 *
 * \a IntHashTable::data becomes NULL if allocation failed.
 *
 * \param ht (IntHashTable *) The hash table
 * \param allocator (mp_Alloc) The allocator to manage the hash table
 */
#define mp_hti_init(/* Type */ type, /* IntHashTable* */ ht, /* mp_Alloc */ allocator)             \
    mp_da_init(type, ht, allocator)

/// Frees an integer hash table.
/**
 * \param ht (IntHashTable *) The hash table
 */
#define mp_hti_deinit(/* IntHashTable* */ ht) mp_da_deinit(ht)

/// Gets a pointer to an item at key \a k.
/**
 * \a ret becomes NULL if it could not retrieve the item.
 *
 * \param ht (const IntHashTable *) The hash table (NO SIDE EFFECTS)
 * \param k (size_t) The key
 * \return (void *) The retrieved value
 */
#define /* void* */ mp_hti_get(/* const IntHashTable* */ ht, /* size_t */ k) __mp_hti_get((ht), (k))
void *__mp_hti_get(const void *ht, size_t k);

/// Sets the value at key \a k to \a v.
/**
 * \a IntHashTable::data becomes NULL if allocation failed.
 *
 * \param ht (IntHashTable *) The hash table
 * \param k (size_t) The key
 * \param v (<Type>) The value to be stored
 */
#define mp_hti_set(/* IntHashTable* */ ht, /* size_t */ k, /* <Type> */ v)                         \
    do {                                                                                           \
        __typeof__(v) __it = (v);                                                                  \
        __mp_hti_set((ht), (k), &__it);                                                            \
    } while (0)
void __mp_hti_set(void *ht, size_t k, void *v);

/// Tests if an item at key \a k exists in the given int hash table.
/**
 * \param ht (const IntHashTable *) The hash table
 * \param k (size_t) The key
 * \return (bool) Whether an item at key \a k exists
 */
#define /* bool */ mp_hti_exists(/* const IntHashTable* */ ht, /* size_t */ k)                     \
    __mp_hti_exists((ht), (k))
bool __mp_hti_exists(const void *ht, size_t k);

/// Grows an integer hash table by \a offset of the current length.
/**
 * Increases \a IntHashTable::len by \a offset and does other things if necessary.
 *
 * If \a IntHashTable::cap is 0, allocates for a certain number of items.
 *
 * If \a IntHashTable::cap is not large enough, allocates for double the current capacity.
 *
 * Recalculates the positions of every entry if resized.
 *
 * \a IntHashTable::data becomes NULL if allocation failed.
 *
 * \param ht (IntHashTable *) The hash table
 * \param offset (size_t) The amount to grow
 */
#define mp_hti_grow(/* IntHashTable* */ ht, /* size_t */ offset) __mp_hti_grow((ht), (offset))
void __mp_hti_grow(void *ht, size_t offset);

/// Sets the length of an integer hash table to 0 and invalidate its keys.
/**
 * This resets the hash table to "initial condition" but without actually freeing the data.
 *
 * \param ht (IntHashTable *) The hash table
 */
#define mp_hti_reset(/* IntHashTable* */ ht) __mp_hti_reset(ht)
void __mp_hti_reset(void *ht);

/// Deletes an item at key \a k.
/**
 * This decreases \a IntHashTable::len but does not actually shrink the hash table, but it just
 * marks the spot as "deleted", which may be overridden by subsequent set operations.
 *
 * \param ht (IntHashTable *) The hash table
 * \param k (size_t) The key
 */
#define mp_hti_delete(/* IntHashTable* */ ht, /* size_t */ k) __mp_hti_delete((ht), (k))
void __mp_hti_delete(void *ht, size_t k);

/// Clones an integer hash table to \a dest to be managed by \a allocator.
/**
 * \a dest inherits all fields of \a src.
 * \a dest.data becomes NULL if allocation failed.
 *
 * \param dest (StrHashTable *) Stores the cloned hash table
 * \param src (const StrHashTable *) The hash table to be cloned
 * \param alloc (mp_Alloc) The allocator to manage \a dest
 */
#define mp_hti_clone(/* IntHashTable* */ dest, /* const IntHashTable* */ src,                      \
                     /* mp_Alloc */ alloc)                                                         \
    __mp_hti_clone((dest), (src), (alloc))
void __mp_hti_clone(void *dest, const void *src, mp_Alloc alloc);

/// Initializes an iterator on an integer hash table.
/**
 * To use hash table iterators, see \ref HashTableInt.
 *
 * \param it (IntHashTableIter *) The iterator to initialize
 * \param ht (const IntHashTable *) The hash table to iterate
 */
#define mp_hti_iter_init(/* IntHashTableIter* */ it, /* const IntHashTable* */ ht)                 \
    __mp_hti_iter_init((it), (ht))
void __mp_hti_iter_init(void *it, const void *ht);

/// Get the next element in the iterator.
/**
 * To use hash table iterators, see \ref HashTableInt.
 *
 * \param it (IntHashTableIter *) The iterator
 * \return (bool) Whether it is valid to access the data
 */
#define /* bool */ mp_hti_iter_next(/* IntHashTableIter* */ it) __mp_hti_iter_next(it)
bool __mp_hti_iter_next(void *it);

/// \}

/***********
 * $ ALLOCATORS
 ***********/

// TODO: Arena rewinding

/**
 * \defgroup Allocators Allocators
 *
 * The implementation of some allocators using the \ref AllocatorInterface "allocator interface".
 *
 * Some of these allocators are definitely **not thread-safe**.
 *
 * \{
 */


/**
 * \defgroup GrowingArenaAllocator Growing Arena Allocator
 *
 * An arena allocator that can grow its size by managing its memory using a linked list of blocks.
 *
 * A block of a certain size will be allocated when needed using the arena's backing allocator, and
 * the previous block will hold a link to it. Each block is stored using the \ref mp_Region type and
 * the data is aligned to `sizeof(uintptr_t)`.
 *
 * # Usage
 *
 * \code
 * mp_Arena arena;
 * mp_arena_init(&arena, mp_heap_alloc());
 * mp_Alloc alloc = mp_arena_alloc(&arena);
 * void *ptr = mp_alloc(alloc, 10);
 * \endcode
 *
 * \{
 */

// Default size of a single region in bytes.
/*
 * The value will be aligned to the nearest increment of `sizeof(uintptr_t)`.
 */
#ifndef __MP_REGION_DEFAULT_SIZE
    #define __MP_REGION_DEFAULT_SIZE (64 * 1024)
#endif

typedef struct mp_Region mp_Region;

/// Linked list element that holds certain size of allocated memory managed by \ref mp_Arena.
struct mp_Region {
    /// The next region in linked list if any.
    mp_Region *next;
    /// The amount of data (in bytes) used.
    size_t len;
    /// The amount of data (in bytes) allocated.
    size_t cap;
    /// The data (aligned to the `sizeof(uintptr_t)`).
    uintptr_t data[];
};

/// Allocates a new region with \a cap bytes of size using \a alloc.
/**
 * \a cap will be **rounded up** to the nearest increment of `sizeof(uintptr_t)`.
 *
 * Deinit with \ref mp_region_deinit.
 *
 * \param alloc The backing allocator used to allocate the memory
 * \param cap How many bytes to allocates
 * \return The pointer to the allocated region
 */
mp_Region *mp_region_init(mp_Alloc alloc, size_t cap);
/// Frees a region.
/**
 * \param alloc The backing allocator that allocated the memory
 * \param r The region to free
 */
void mp_region_deinit(mp_Region *r, mp_Alloc alloc);

/// The internal context of growing arena allocators, manages regions in a linked list.
typedef struct {
    /// The first element of the region linked list.
    mp_Region *begin;
    /// The last element of the region linked list.
    mp_Region *end;
    /// The amount of data used (in bytes, aligned to `sizeof(uintptr_t)`).
    size_t len;
    /// The backing allocator, allocator used to allocate the regions.
    mp_Alloc alloc;
    /// The default size of regions allocated by this arena.
    size_t _def_size;
} mp_Arena;

/// Creates a new, unallocated arena using \a alloc as the backing allocator.
/**
 * The arena will not allocate anything until the first operation that allocates.
 *
 * Deinit with \ref mp_arena_deinit.
 *
 * \param a (mp_Arena *) The arena
 * \param alloc (mp_Alloc) The backing allocator
 */
#define mp_arena_init(/* mp_Arena* */ a, /* mp_Alloc */ alloc)                                     \
    mp_arena_init_s((a), (alloc), __MP_REGION_DEFAULT_SIZE)

/// The same as \ref mp_arena_init but accepts a custom default size for regions.
/**
 * See \ref mp_arena_init.
 *
 * Regions will be allocated with \a def_size size.
 *
 * \param a The arena
 * \param alloc The backing allocator
 * \param def_size The size of regions
 */
void mp_arena_init_s(mp_Arena *a, mp_Alloc alloc, size_t def_size);

/// Sets an arena length to 0, but does not free allocated regions.
/**
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param a The arena
 */
void mp_arena_reset(mp_Arena *a);

/// Frees an arena and its regions.
/**
 * The free will be performed using the arena's backing allocator.
 *
 * \param a The arena
 */
void mp_arena_deinit(mp_Arena *a);
/// Returns an allocator that works with \ref mp_Arena.
/**
 * \param a The arena
 * \return The allocator interface
 */
mp_Alloc mp_arena_alloc(mp_Arena *a);

/// \}

/**
 * \defgroup StaticArenaAllocator Static Arena Allocator
 *
 * An arena allocator that cannot grow in size.
 *
 * The arena will allocate a block of the given size and use that to store the data aligned to
 * `sizeof(uintptr_t)`. If the block does not have enough space, allocation operations will not
 * allocate and return NULL.
 *
 * # Usage
 *
 * \code
 * mp_SArena arena;
 * mp_sarena_init(&arena, mp_heap_alloc(), 1024);
 * mp_Alloc alloc = mp_sarena_alloc(&arena);
 * void *ptr = mp_alloc(alloc, 10);
 * \endcode
 *
 * \{
 */

/// The internal context of static arena allocators.
typedef struct {
    // The arena buffer (of size \a cap).
    uintptr_t *buf;
    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t len;
    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
    size_t cap;
    // The backing allocator, allocator used to allocate \a buf.
    mp_Alloc alloc;
} mp_SArena;

/// Initializes and allocates a static arena of size \a cap in bytes.
/**
 * \a cap will be **rounded up** to the nearest increment of `sizeof(uintptr_t)`.
 *
 * The arena's buffer will be immediately allocated.
 *
 * \a a->buf is NULL if allocation failed.
 *
 * Deinit with \ref mp_sarena_deinit.
 *
 * \param a The arena
 * \param alloc The backing allocator
 * \param cap How many bytes to allocate
 */
void mp_sarena_init(mp_SArena *a, mp_Alloc alloc, size_t cap);
/// Sets an arena length to 0, but does not free the allocated buffer.
/**
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param a The arena
 */
void mp_sarena_reset(mp_SArena *a);
/// Frees an arena and its buffer.
/**
 * The free will be performed using the arena's backing allocator.
 *
 * \param a The arena
 */
void mp_sarena_deinit(mp_SArena *a);
/// Returns an allocator that works with \ref mp_SArena.
/**
 * Returns an invalid allocator if \a a->buf is NULL.
 *
 * \param a The arena
 * \return The allocator interface
 */
mp_Alloc mp_sarena_alloc(mp_SArena *a);

/// \}

/**
 * \defgroup TempAllocator Temporary Arena Allocator
 *
 * A \ref StaticArenaAllocator "static arena allocator" that uses buffer allocated on the stack.
 *
 * To initialize this allocator, the user must provide a buffer that is already allocated.
 *
 * # Usage
 *
 * \code
 * char     tempbuf[1024];
 * mp_Alloc talloc;
 * mp_talloc(tempbuf, &talloc);
 * \endcode
 *
 * \{
 */

/// The internal context of temp allocators.
typedef struct {
    // The arena buffer (of size \a cap).
    uintptr_t *buf;
    // The amount of data (in bytes) used (aligned to `sizeof(uintptr_t)`).
    size_t len;
    // The amount of data (in bytes) allocated (aligned to `sizeof(uintptr_t)`).
    size_t cap;
} mp_Temp;

/// A shortcut for initializing a temp allocator.
/**
 * The beginning portion of \a buf will be used to allocate the \a mp_Temp "allocator context".
 *
 * The allocator will **round down** the size to the nearest increment of `sizeof(uintptr_t)`.
 *
 * \param buf (char *) The buffer (at least `sizeof(mp_Temp)` of size)
 * \param ret_alloc (mp_Alloc *) Stores the allocator interface
 */
#define mp_talloc(/* char* */ buf, /* mp_Alloc* */ ret_alloc)                                      \
    do {                                                                                           \
        MEMPLUS_ASSERT_MSG(sizeof(buf) >= sizeof(mp_Temp),                                         \
                           "Buffer size is smaller than `sizeof(mp_Temp)`");                       \
        mp_Temp *__t = (mp_Temp *) (buf);                                                          \
        mp_temp_init(__t, ((char *) (buf)) + sizeof(mp_Temp), sizeof(buf) - sizeof(mp_Temp));      \
        *(ret_alloc) = mp_temp_alloc(__t);                                                         \
    } while (0)

/// Initializes a temp allocator with a \buf of size \a cap (in bytes).
/**
 * \a cap should be an increment of `sizeof(uintptr_t)`.
 * If not, the actual \a cap will **round down** to the nearest increment.
 *
 * \param t The temp arena
 * \param buf The buffer
 * \param cap The size of \a buf (in bytes)
 */
void mp_temp_init(mp_Temp *t, char *buf, size_t cap);

/// Sets an arena length to 0, but does not deinitialize the buffer.
/**
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param t The temp arena
 */
void mp_temp_reset(mp_Temp *t);

/// Returns an allocator that works with \ref mp_Temp.
/**
 * \param t The arena
 * \return The allocator interface
 */
mp_Alloc mp_temp_alloc(mp_Temp *t);

/// \}

/**
 * \defgroup HeapAllocator Heap Allocator
 *
 * An \ref mp_Alloc "allocator interface" that manages heap allocations.
 *
 * The allocations utilize the standard functions `calloc` and `free`.
 *
 * This allocator is context-less.
 *
 * \{
 */

/// Returns an allocator that works with the heap.
/**
 * \return The allocator interface
 */
mp_Alloc mp_heap_alloc(void);

/// Alias to \ref mp_heap_alloc.
#define mp_heap() mp_heap_alloc()

/// \}

/// \}

/***********
 * $ UTF-8
 ***********/

/**
 * \defgroup Utf8 UTF-8
 *
 * Utility functions for dealing with UTF-8 strings.
 *
 * \{
 */

/// Calculate the amount of characters in a **null-terminated** UTF-8 string.
/**
 * Use \ref mp_utf8_len_s for non-null-terminated strings.
 *
 * Returns \ref MP_ERROR if \a str is not a valid UTF-8 string.
 *
 * \param str The UTF-8 string (null-terminated)
 * \return The amount of characters in \a str
 */
size_t mp_utf8_len(const char *str);

/// Calculate the amount of characters in a UTF-8 string with size parameter (in bytes).
/**
 * Returns \ref MP_ERROR if \a str is not a valid UTF-8 string.
 *
 * \param str The UTF-8 string
 * \param size The size of \a str (in bytes)
 * \return The amount of characters in \a str
 */
size_t mp_utf8_len_s(const char *str, size_t size);

/// Iterator for UTF-8 strings.
/**
 * \a c and \a c_len can be accessed to get the current character's information.
 *
 * # Usage
 *
 * \code
 * const char *utf8 = "魈くんは大好きです　⸜(｡˃ ᵕ ˂)⸝♡􏾀";
 * mp_Utf8Iter iter = mp_utf8_iter_new(utf8);
 * while (mp_utf8_iter_next(&iter)) {
 *     (void) iter.c;      // The current character (char[4])
 *     (void) iter.c_len;  // The current character size (in bytes)
 * }
 * \endcode
 */
typedef struct {
    /// Holds the current character in iteration.
    char c[4];
    /// Holds the current character's size (in bytes).
    char c_len;

    /// The UTF-8 string being iterated on.
    const char *_str;
    /// The size of the string (in bytes).
    size_t _size;
    /// The current index of the iteration (in bytes).
    size_t _i;
} mp_Utf8Iter;

/// Creates a new \ref mp_Utf8Iter that iterates over a **null-terminated** UTF-8 string.
/**
 * Use \ref mp_utf8_iter_new_s for non-null-terminated strings.
 *
 * See \ref mp_Utf8Iter for usage.
 *
 * \param str The UTF-8 string (null-terminated)
 * \return The iterator
 */
mp_Utf8Iter mp_utf8_iter_new(const char *str);

/// Creates a new \ref mp_Utf8Iter that iterates over a UTF-8 string with size parameter (in
/// bytes).
/**
 * See \ref mp_Utf8Iter for usage.
 *
 * \param str The UTF-8 string
 * \param size The size of \a str (in bytes)
 * \return The iterator
 */
mp_Utf8Iter mp_utf8_iter_new_s(const char *str, size_t size);

/// Continues iterating an \ref mp_Utf8Iter.
/**
 * See \ref mp_Utf8Iter for usage.
 *
 * \param it The iterator
 * \return Whether it is valid to access the data
 */
bool mp_utf8_iter_next(mp_Utf8Iter *it);

/// \}

/***********
 * $ ERRORS
 ***********/

/**
 * \defgroup Errors Errors
 *
 * Errors from errno. The available error varies by operating systems.
 *
 * Error names for POSIX & Linux are taken from Linux manpage
 * [errno(3)](https://man7.org/linux/man-pages/man3/errno.3.html).
 *
 * Error names for Windows are taken from
 * <https://learn.microsoft.com/en-us/cpp/c-runtime-library/errno-constants>.
 *
 * For error messages see the definition of \ref mp_err_str.
 *
 * \{
 */

// Don't forget `mp_err()` and `mp_err_str()`!
// Sort this!
/// See \ref Errors.
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

/// Converts `errno` into \ref mp_Err.
mp_Err mp_err(int errnum);

/// Returns the message of an error.
const char *mp_err_str(mp_Err e);

/// \}

/***********
 * $ IO INTERFACE
 ***********/

/**
 * \defgroup IoInterface IO Interface
 *
 * The IO interface wraps many kinds of IO implementations.
 * The IO implementations themselves are located \ref IO "here".
 *
 * # Creating Your Own IO Implementation
 *
 * > TODO: Will be done after I'm really sure about things in this interface.
 *
 * > NOTE: This interface may be (or will certainly be) reworked.
 *
 * \{
 */

/// Possible operations on \ref mp_IoFunc.
/**
 * See the documentation for each operation \ref mp_IoFunc "here".
 */
typedef enum {
    MP_IOOP_FLUSH,
    MP_IOOP_SETBUF,
    MP_IOOP_READ,    // When ret < count, if successful, it means EOF
    MP_IOOP_WRITE,
    MP_IOOP_GETPOS,
    MP_IOOP_SETPOS,
    MP_IOOP_GETC,
    MP_IOOP_PUTC,
    __MP_IOOP_COUNT,
} mp_IoOp;

/// Errors that may occur when using IO functions.
/**
 * For error messages see \ref mp_ioerr_str.
 */
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

/// Returns the message of an \ref mp_IoErr "IO error".
const char *mp_ioerr_str(mp_IoErr e);

/// Modes given to \ref MP_IOOP_SETBUF.
typedef enum {
    /// No buffering (_IONBF)
    MP_SETBUFMODE_NONE,
    /// Full buffering (_IOFBF)
    MP_SETBUFMODE_FULL,
    /// Line buffering (_IOLBF)
    MP_SETBUFMODE_LINE,
} mp_SetbufMode;

/// Position from which to apply the offset of the seek.
typedef enum {
    /// Beginning of the stream (SEEK_SET)
    MP_SETPOSORIGIN_START,
    /// The current position in the stream (SEEK_CUR)
    MP_SETPOSORIGIN_CURRENT,
    /// The end of the stream (SEEK_END)
    MP_SETPOSORIGIN_END,
} mp_SetposOrigin;

/// The types of streams.
/**
 * Stream of a certain type may only call certain functions. A stream may be both read and
 * write.
 *
 * If a stream calls to a function outside of its domain, \ref MP_IOERR_UNSUPPORTED will be
 * returned.
 *
 * \ref MP_IOTYPE_NONE is only used for invalid \ref mp_Io.
 */
typedef enum {
    MP_IOTYPE_NONE  = 0,
    MP_IOTYPE_READ  = 1 << 0,
    MP_IOTYPE_WRITE = 1 << 1,
} mp_IoType;

typedef struct mp_Io mp_Io;

// TODO: Move flush, setbuf, getpos and setpos to the implementation

/// Function protoype used for IO implementations.
/**
 * Functions of this type do different things depending on the \a op given.
 * They also use their parameters differently on each type.
 *
 * Returns \a mp_IoErr type. \ref MP_IOERR_NONE if successful.
 *
 * Not all operations can be done to all streams.
 * If the operation does not allow to be done on the type it will return
 * \ref MP_IOERR_UNSUPPORTED.
 *
 * Operations will ignores parameters that are not listed for them.
 *
 * # Operations
 *
 * - **MP_IOOP_FLUSH** (MP_IOTYPE_WRITE)

 *     Flushes the stream
 *
 *     For output streams, writes unwritten data from buffer to the output device.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *
 * - **MP_IOOP_SETBUF** (MP_IOTYPE_WRITE or MP_IOTYPE_READ)
 *
 *     Changes the buffering mode or/and the size of the internal buffer.
 *     Can also instruct the stream to use use-provided buffer if \a ptr is not NULL.
 *     The stream must be closed before the lifetime of the buffer ends.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **ptr**: The buffer to use (if NULL, only resizes the existing buffer to \a n1)
 *     - **n1**: The size of the buffer (in bytes)
 *     - **n2**: The buffering mode (see \ref mp_SetbufMode)
 *
 * - **MP_IOOP_READ** (MP_IOTYPE_READ)
 *
 *     Reads objects into given buffer from the stream.
 *     If an error or EOF occurs, \a ret may be less than \a n2 and returns MP_IOERR_CANNOT_READ
 or
 *     MP_IOERR_EOF respsectively. If \a n1 or \a n2 is zero, does nothing and \a ret will be
 set to
 *     zero.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **ptr**: The buffer which the data will be stored
 *     - **n1**: The size of each object (in bytes)
 *     - **n2**: The number of objects (the total size will be \a n1 * \a n2)
 *     - **ret**: Stores the number of objects read successfully
 *
 * - **MP_IOOP_WRITE** (MP_IOTYPE_WRITE)
 *
 *     Writes objects from given buffer to the stream.
 *     If an error occurs, \a ret may be less than \a n2 and returns MP_IOERR_CANNOT_WRITE. If
 \a n1
 *     or \a n2 is zero, does nothing and \a ret will be set to zero.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **ptr**: The buffer of the data to be written
 *     - **n1**: The size of each object (in bytes)
 *     - **n2**: The number of objects (the total size will be \a n1 * \a n2)
 *     - **ret**: Stores the number of objects written successfully
 *
 * - **MP_IOOP_GETPOS** (MP_IOTYPE_WRITE or MP_IOTYPE_READ)
 *
 *     Gets the file position indicator of a stream.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **ret**: Stores the file position indicator (in bytes)
 *
 * - **MP_IOOP_SETPOS** (MP_IOTYPE_WRITE or MP_IOTYPE_READ)
 *
 *     Sets the file position indicator of a stream.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **n1**: The offset (in bytes)
 *     - **n2**: The origin of the seek (see \ref mp_SetposOrigin)
 *
 * - **MP_IOOP_GETC** (MP_IOTYPE_READ)
 *
 *     Reads the next character from a stream.
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **ret**: Stores the retrieved character
 *
 * - **MP_IOOP_PUTC** (MP_IOTYPE_WRITE)
 *
 *     Writes a character to a stream .
 *
 *     **Parameters**
 *     - **io**: The IO object
 *     - **n1**: The character to write
 *
 * \return \ref MP_IOERR_NONE if successful, \ref MP_IOERR_UNSUPPORTED if the operation is not
 * supported by the type, or other errors.
 */
typedef mp_IoErr (*mp_IoFunc)(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2, size_t *ret);

/// Interface to wrap IO functions.
struct mp_Io {
    /// Data that is passed to the IO function.
    /**
     * This can be specified as NULL if context is not needed.
     */
    void *context;
    /// See \ref mp_IoType.
    mp_IoType type;

    /**
     * \brief Function that handles the operations requested by the user of the interface. See
     * \ref mp_IoFunc.
     */
    mp_IoFunc f;
};

/**
 * \defgroup GenericIoMacros Generic IO Macros
 *
 * These macros wrap the operations of \ref mp_IoFunc.
 * By passing an \ref mp_Io, these macros will call its function and pass the context
 * and the arguments correctly.
 *
 * \{
 */

/// Calls IO function with **MP_IOOP_FLUSH**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_flush(/* mp_Io* */ io) ((io)->f(MP_IOOP_FLUSH, (io), NULL, 0, 0, NULL))

/// Calls IO function with **MP_IOOP_SETBUF**.
/**
 * The stream must be closed before the lifetime \a buf ends.
 *
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param buf (char *) The buffer to use (if NULL, only resizes the existing buffer to \a bufsize)
 * \param bufsize (size_t) The size of the buffer (in bytes)
 * \param mode (mp_SetbufMode) The buffering mode
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_setbuf(/* mp_Io* */ io, /* char* */ buf, /* size_t */ bufsize,        \
                                    /* mp_SetbufMode */ mode)                                      \
    ((io)->f(MP_IOOP_SETBUF, (io), (buf), (bufsize), (mode), NULL))

/// Calls IO function with **MP_IOOP_READ**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param buf (char *) The buffer which the data will be stored
 * \param size (size_t) The size of each object (in bytes)
 * \param count (size_t) The number of objects (the total size will be \a size * \a count)
 * \param ret_n (size_t *) Stores the number of objects read successfully
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_read(/* mp_Io* */ io, /* char* */ buf, /* size_t */ size,             \
                                  /* size_t */ count, /* size_t* */ ret_n)                         \
    ((io)->f(MP_IOOP_READ, (io), (buf), (size), (count), (ret_n)))

/// Calls IO function with **MP_IOOP_WRITE**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param buf (char *) The buffer of the data to be written
 * \param size (size_t) The size of each object (in bytes)
 * \param count (size_t) The number of objects (the total size will be \a size * \a count)
 * \param ret_n (size_t *) Stores the number of objects written successfully
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_write(/* mp_Io* */ io, /* char* */ buf, /* size_t */ size,            \
                                   /* size_t */ count, /* size_t* */ ret_n)                        \
    ((io)->f(MP_IOOP_WRITE, (io), (void *) (buf), (size), (count), (ret_n)))

/// Calls IO function with **MP_IOOP_GETPOS**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param ret_n (size_t *) Stores the file position indicator (in bytes)
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_getpos(/* mp_Io* */ io, /* size_t* */ ret_n)                          \
    ((io)->f(MP_IOOP_GETPOS, (io), NULL, 0, 0, (ret_n)))

/// Calls IO function with **MP_IOOP_SETPOS**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param offset (size_t) The offset (in bytes)
 * \param origin (size_t) The origin of the seek
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_setpos(/* mp_Io* */ io, /* size_t */ offset,                          \
                                    /* mp_SetposOrigin */ origin)                                  \
    ((io)->f(MP_IOOP_SETPOS, (io), NULL, (offset), (origin), NULL))

/// Calls IO function with **MP_IOOP_GETC**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param ret_n (size_t *) Stores the retrieved character
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_getc(/* mp_Io* */ io, /* size_t* */ ret_c)                            \
    ((io)->f(MP_IOOP_GETC, (io), NULL, 0, 0, (ret_c)))

/// Calls IO function with **MP_IOOP_PUTC**.
/**
 * \param io (mp_Io *) The IO object (NO SIDE EFFECTS)
 * \param c (size_t) The character to write
 * \return (mp_IoErr) The error, returns MP_IOERR_NONE if successful
 */
#define /* mp_IoErr */ mp_io_putc(/* mp_Io* */ io, /* size_t */ c)                                 \
    ((io)->f(MP_IOOP_PUTC, (io), NULL, (size_t) (c), 0, NULL))

/// \}

/// Returns an invalid \ref mp_Io.
/**
 * Invalid \ref mp_Io requires field \a type is MP_IOTYPE_NONE.
 */
#define /* mp_Io */ mp_io_invalid()                                                                \
    ((mp_Io) {                                                                                     \
        .context = NULL,                                                                           \
        .type    = MP_IOTYPE_NONE,                                                                 \
        .f       = NULL,                                                                           \
    })

/// Tests if \a io is valid (i.e. field \a type is not MP_IOTYPE_NONE).
/**
 * Returns true if \a io is valid.
 *
 * \param io (mp_Io) The IO object
 * \return (bool) Whether \a io is valid
 */
#define /* bool */ mp_io_is_valid(/* mp_Io */ io) ((io).type != MP_IOTYPE_NONE)

/// Creates an \ref mp_Io given the context, the type and the function.
/**
 * \param ctx (any *) The context passed to the function (automatically casted to void *)
 * \param type (mp_IoType) The type of the interface
 * \param func (mp_IoFunc) The IO function
 * \return An IO interface that works with the arguments given.
 */
#define /* mp_Io */ mp_io_new(/* any* */ ctx, /* mp_IoType */ type, /* mp_IoFunc */ func)          \
    ((mp_Io) {                                                                                     \
        .context = (void *) (ctx),                                                                 \
        .type    = (type),                                                                         \
        .f       = (func),                                                                         \
    })

/// \}

/**
 * \defgroup IO IO Implementations
 *
 * Some IO implementations using the \ref IoInterface "IO interface".
 *
 * \{
 */

/***********
 * $ FILE IO
 ***********/

/**
 * \defgroup FileIO File IO
 *
 * An IO interface that works with files.
 *
 * # Usage
 *
 * \code
 * mp_File f;
 * mp_Err e = mp_file_open(&f, "foo.txt", "r");
 * mp_Io io = mp_file_io(&f, MP_IOTYPE_READ);
 * const char m[] = "foobar";
 * size_t     n   = 0;
 * mp_IoErr   ie  = mp_io_write(&io, m, 1, sizeof(m) - 1, &n);
 * \endcode
 *
 * \{
 */

// TODO: Put this notice somewhere
/* Binary streams may not support MP_SETPOSORIGIN_END or SEEK_END. For text streams, offset may
 * only be zero or the result of earlier `MP_IOOP_GETPOS` (for MP_SETPOSORIGIN_START or SEEK_SET
 * only). For wide-oriented streams, the restrictions of both binary and text streams apply. */

/// The internal context of file IO.
typedef struct {
    /// The handled file object
    FILE *file;
    /// The supported IO type for the file object
    mp_IoType supported_type;
} mp_File;

/// Opens a file at \a filename.
/**
 * Close with \ref mp_file_deinit.
 *
 * \param f The file
 * \param filename The name of the file to open
 * \param mode The mode of the file. See `fopen` for possible modes
 * \return The error, returns MP_ERR_NONE if successful
 */
mp_Err mp_file_open(mp_File *f, const char *filename, const char *mode);

/// Opens a file at \a filename and closes the old file.
/**
 * Close with \ref mp_file_deinit.
 *
 * If \a filename is NULL, changes the mode of the existing file (**not supported for all
 * platforms**).
 *
 * If \a f->file is NULL, returns MP_ERR_BAD_FD.
 *
 * \param f The file
 * \param filename The name of the file to open
 * \param mode The mode of the file. See `fopen` for possible modes
 * \return The error, returns MP_ERR_NONE if successful
 */
mp_Err mp_file_reopen(mp_File *f, const char *filename, const char *mode);

// TODO: mp_file_open_from_fd

/// Closes an open file.
/**
 * Does nothing if \a f->file is NULL.
 *
 * \param f The file
 */
void mp_file_deinit(mp_File *f);

/// Returns an IO object that works with a file.
/**
 * The \a type may not be supported, depends on the mode when opening the file.
 *
 * \param f The file
 * \param type The IO type
 * \return The IO object, returns an invalid \a mp_Io if failed
 */
mp_Io mp_file_io(mp_File *f, mp_IoType type);

/// \}

/// \}

/***********
 * $ IMPLEMENTATION
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
    #define __MP_ASSERT_OVERLAP(a, a_len, b, b_len)                                                \
        do {                                                                                       \
            uintptr_t _a = (uintptr_t) a;                                                          \
            uintptr_t _b = (uintptr_t) b;                                                          \
            if (__MP_MAX((_a), (_b)) < __MP_MIN((_a) + (a_len), (_b) + (b_len))) {                 \
                MEMPLUS_ASSERT_MSG(0, "Memory overlaps");                                          \
            }                                                                                      \
        } while (0)

static void *mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                 void *ptr);
static void *mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                  void *ptr);
static void *mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                void *ptr);

static mp_IoErr mp_file_io_func(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2,
                                size_t *ret);

    #ifdef __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(const char *assertion, const char *file, const char *func,
                                    size_t line, const char *msg) {
    fprintf(stderr, "%s:%s():%zu: [memplus] %s. `%s` failed.\n", file, func, line, msg, assertion);
    abort();
}
    #endif

void *mp_dup(mp_Alloc alloc, const void *data, size_t size) {
    void *buf = mp_alloc(alloc, size);
    if (buf == NULL) {
        return NULL;
    }
    return memcpy(buf, data, size);
}

void *mp_alloc_handle_realloc(mp_Alloc alloc, void *old_ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        return NULL;
    }
    if (new_size <= old_size) {
        return old_ptr;
    }
    void *new_ptr = mp_alloc(alloc, new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    __MP_ASSERT_OVERLAP(old_ptr, old_size, new_ptr, new_size);
    memcpy(new_ptr, old_ptr, old_size);
    mp_free(alloc, old_ptr, old_size);
    return new_ptr;
}

void __mp_da_init(void *a, mp_Alloc alloc, size_t size) {
    __mp_DynArray *self = a;
    __MP_ZERO(self);
    self->alloc = alloc;
    self->len   = 0;
    self->cap   = 0;
    self->size  = size;
    self->data  = NULL;
}

void __mp_da_deinit(void *a) {
    __mp_DynArray *self = a;
    mp_free(self->alloc, self->data, self->cap * self->size);
    __MP_ZERO(self);
}

void __mp_da_append(void *a, const void *items, size_t items_len) {
    __mp_DynArray *self     = a;
    size_t         prev_len = self->len;
    mp_da_grow(self, items_len);
    if (self->data != NULL) {
        memcpy((char *) self->data + prev_len * self->size, items, items_len * self->size);
    }
}

void __mp_da_grow(void *a, size_t offset) {
    __mp_DynArray *self = a;
    if (self->len + offset > self->cap && offset > 0) {
        size_t old_cap = self->cap;
        if (self->cap == 0) {
            self->cap = __MP_DARRAY_INIT_CAPACITY;
        }
        while (self->len + offset > self->cap) {
            self->cap *= 2;
        }
        self->data =
            mp_realloc(self->alloc, self->data, old_cap * self->size, self->cap * self->size);
    }
    if (self->data != NULL) {
        self->len += offset;
    }
}

void __mp_da_shrink(void *a, size_t offset) {
    __mp_DynArray *self = a;
    MEMPLUS_ASSERT_MSG(offset < self->len, "`offset` is out of bounds");
    self->len -= offset;
}

void __mp_da_clone(void *dest, const void *src, mp_Alloc alloc) {
    const __mp_DynArray *s = src;
    __mp_DynArray       *d = dest;
    __MP_ZERO(d);
    d->data = mp_dup(alloc, s->data, s->cap * s->size);
    if (d->data != NULL) {
        d->alloc = alloc;
        d->len   = s->len;
        d->cap   = s->len + __MP_DARRAY_INIT_CAPACITY;
        d->size  = s->size;
    }
}

void __mp_da_insert(void *a, size_t pos, const void *items, size_t items_len) {
    __mp_DynArray *self       = a;
    size_t         actual_pos = (pos > self->len) ? self->len : pos;
    mp_da_grow(self, items_len);
    if (self->data != NULL) {
        memmove((char *) self->data + (actual_pos + items_len) * self->size,
                (char *) self->data + actual_pos * self->size, (items_len + 1) * self->size);
        memcpy((char *) self->data + actual_pos * self->size, items, items_len * self->size);
    }
}

void __mp_da_delete(void *a, size_t pos) {
    __mp_DynArray *self = a;
    __MP_BOUNDS_CHECK(pos, self->len);
    mp_da_shrink(self, 1);
    size_t moved = self->len - pos;
    if (moved >= 1) {
        memmove((char *) self->data + pos * self->size,
                (char *) self->data + (pos + 1) * self->size, moved * self->size);
    }
}

void __mp_da_quick_delete(void *a, size_t pos) {
    __mp_DynArray *self = a;
    __MP_BOUNDS_CHECK((pos), self->len);
    mp_da_shrink(self, 1);
    memcpy((char *) self->data + pos * self->size, (char *) self->data + self->len * self->size,
           self->size);
}

mp_Str mp_str_new(mp_Alloc alloc, const char *str) {
    return mp_str_new_len(alloc, str, strlen(str));
}

mp_Str mp_str_new_len(mp_Alloc alloc, const char *str, size_t len) {
    char *result = mp_alloc(alloc, (size_t) (len + 1));
    if (result == NULL) {
        return mp_str_invalid();
    }
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
    if (result == NULL) {
        return mp_str_invalid();
    }

    va_start(args, fmt);
    int result_len = vsnprintf(result, (size_t) (len + 1), fmt, args);
    MEMPLUS_ASSERT(result_len == len);
    va_end(args);

    return (mp_Str) { .len = (size_t) result_len, .cstr = result };
}

void mp_str_deinit(mp_Str *str, mp_Alloc alloc) {
    mp_free(alloc, str->cstr, str->len + 1);
    __MP_ZERO(str);
}

mp_Str mp_str_clone(const mp_Str *str, mp_Alloc alloc) {
    char *ptr = mp_dup(alloc, str->cstr, str->len + 1);
    if (ptr == NULL) {
        return mp_str_invalid();
    }
    return (mp_Str) { .len = str->len, .cstr = ptr };
}

void mp_str_builder_init(mp_StrBuilder *sb, mp_Alloc alloc) {
    mp_da_init(mp_StrBuilder, sb, alloc);
}

void mp_str_builder_deinit(mp_StrBuilder *sb) {
    mp_da_deinit(sb);
}

void mp_str_builder_append(mp_StrBuilder *sb, const char *str) {
    mp_da_append_array(sb, str, strlen(str));
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

mp_Str mp_str_builder_string_take(mp_StrBuilder *sb, mp_Alloc alloc) {
    mp_Str res = mp_str_new_len(alloc, sb->data, sb->len);
    mp_str_builder_deinit(sb);
    return res;
}

void __mp_ht_deinit(void *ht) {
    __mp_DynArray *self = ht;
    for (size_t i = 0; i < self->cap; i++) {
        __mp_StrHashTableEntry *e = __mp_da_get(__mp_StrHashTableEntry, self, i);
        if (mp_str_is_valid(e->key)) {
            mp_str_deinit(&e->key, self->alloc);
        }
    }
    mp_da_deinit(self);
}

void *__mp_ht_get(const void *ht, mp_Str k) {
    const __mp_DynArray *self = ht;
    if (mp_str_is_valid(k)) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_StrHashTableEntry *e = __mp_da_get(__mp_StrHashTableEntry, self, i);
            if (mp_str_is_valid(e->key) && strcmp(k.cstr, e->key.cstr) == 0) {
                return &e->val;
            }
            ++i;
            if (i >= self->cap) {
                i = 0;
            }
            if (!mp_str_is_valid(e->key) && *(char *) &e->val != 1) {
                break;
            }
        }
    }
    return NULL;
}

void __mp_ht_set(void *ht, mp_Str k, void *v) {
    __mp_DynArray *self = ht;
    mp_ht_grow(self, 1);
    if (self->data != NULL) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_StrHashTableEntry *e = __mp_da_get(__mp_StrHashTableEntry, self, i);
            if (!mp_str_is_valid(e->key)) {
                e->key = mp_str_clone(&k, self->alloc);
                memcpy(&e->val, v, self->size - sizeof(e->key));
                break;
            } else if (strcmp(e->key.cstr, k.cstr) == 0) {
                memcpy(&e->val, v, self->size - sizeof(e->key));
                --self->len;
                break;
            } else {
                ++i;
            }
            if (i >= self->cap) {
                i = 0;
            }
        }
    }
}

bool __mp_ht_exists(const void *ht, mp_Str k) {
    return __mp_ht_get(ht, k) != NULL;
}

void __mp_ht_grow(void *ht, size_t offset) {
    __mp_DynArray *self = ht;
    if (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD) &&
        offset > 0) {
        size_t old_cap = self->cap;
        if (self->cap == 0) {
            self->cap = __MP_HASH_TABLE_INIT_CAPACITY;
        }
        while (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD)) {
            self->cap *= 2;
        }
        void *new_data = mp_alloc(self->alloc, self->cap * self->size);
        if (new_data == NULL) {
            self->data = NULL;
            return;
        }
        for (size_t i = 0; i < old_cap; ++i) {
            __mp_StrHashTableEntry *e = __mp_da_get(__mp_StrHashTableEntry, self, i);
            if (mp_str_is_valid(e->key)) {
                uint64_t hash  = __mp_ht_hash_str(&e->key);
                size_t   new_i = (size_t) (hash % (uint64_t) (self->cap - 1));
                for (;;) {
                    __mp_StrHashTableEntry *new_e =
                        (__mp_StrHashTableEntry *) ((char *) new_data + new_i * self->size);
                    if (!mp_str_is_valid(new_e->key)) {
                        new_e->key = mp_str_clone(&e->key, self->alloc);
                        memcpy(&new_e->val, &e->val, self->size - sizeof(e->key));
                        break;
                    } else {
                        ++new_i;
                    }
                    if (new_i >= self->cap) {
                        new_i = 0;
                    }
                }
            }
        }
        __mp_ht_free_entries(self->data, self->alloc, old_cap, self->size);
        mp_free(self->alloc, self->data, old_cap * self->size);
        self->data = new_data;
    }
    self->len += offset;
}

void __mp_ht_free_entries(void *entries, mp_Alloc alloc, size_t cap, size_t size) {
    for (size_t i = 0; i < cap; ++i) {
        __mp_StrHashTableEntry *e = (__mp_StrHashTableEntry *) ((char *) entries + i * size);
        if (mp_str_is_valid(e->key)) {
            mp_str_deinit(&e->key, alloc);
            MEMPLUS_ASSERT(!mp_str_is_valid(e->key));
        }
    }
}

void __mp_ht_reset(void *ht) {
    __mp_DynArray *self = ht;
    __mp_ht_free_entries(self->data, self->alloc, self->cap, self->size);
    mp_da_reset(self);
}

void __mp_ht_delete(void *ht, mp_Str k) {
    __mp_DynArray *self = ht;
    if (mp_str_is_valid(k)) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_StrHashTableEntry *e = __mp_da_get(__mp_StrHashTableEntry, self, i);
            if (mp_str_is_valid(e->key) && strcmp(k.cstr, e->key.cstr) == 0) {
                mp_str_deinit(&e->key, self->alloc);
                MEMPLUS_ASSERT(!mp_str_is_valid(e->key));
                memset(&e->val, 1, sizeof(char));
                --self->len;
                break;
            }
            ++i;
            if (i >= self->cap) {
                i = 0;
            }
            if (!mp_str_is_valid(e->key)) {
                break;
            }
        }
    }
}

void __mp_ht_clone(void *dest, const void *src, mp_Alloc alloc) {
    const __mp_DynArray *s = src;
    __mp_DynArray       *d = dest;
    __MP_ZERO(d);
    d->data = mp_dup(alloc, s->data, s->cap * s->size);
    if (d->data != NULL) {
        d->alloc = alloc;
        d->len   = s->len;
        d->cap   = s->cap;
        d->size  = s->size;
        for (size_t i = 0; i < s->cap; ++i) {
            __mp_StrHashTableEntry *s_e = __mp_da_get(__mp_StrHashTableEntry, s, i);
            __mp_StrHashTableEntry *d_e = __mp_da_get(__mp_StrHashTableEntry, d, i);
            if (mp_str_is_valid(s_e->key)) {
                d_e->key = mp_str_clone(&s_e->key, alloc);
                MEMPLUS_ASSERT(d_e->key.cstr != s_e->key.cstr);
            }
        }
    }
}

void __mp_ht_iter_init(void *it, const void *ht) {
    __mp_StrHashTableIter *self = it;
    const __mp_DynArray   *h    = ht;
    memset(self, 0, sizeof(*self) + h->size);
    self->_h = h;
}

bool __mp_ht_iter_next(void *it) {
    __mp_StrHashTableIter *self = it;
    while (self->_i < self->_h->cap) {
        __mp_StrHashTableEntry *entry = __mp_da_get(__mp_StrHashTableEntry, self->_h, self->_i);
        if (mp_str_is_valid(entry->key)) {
            self->key = entry->key;
            memcpy(&self->val, &entry->val, self->_h->size);
            ++self->_i;
            return true;
        }
        ++self->_i;
    }
    return false;
}

    #define __MP_FNV_OFFSET 14695981039346656037UL
    #define __MP_FNV_PRIME  1099511628211UL

uint64_t __mp_ht_hash_str(const mp_Str *str) {
    uint64_t hash = __MP_FNV_OFFSET;
    for (const char *p = str->cstr; *p; p++) {
        hash ^= (uint64_t) (unsigned char) (*p);
        hash *= __MP_FNV_PRIME;
    }
    return hash;
}

void *__mp_hti_get(const void *ht, size_t k) {
    const __mp_DynArray *self = ht;
    size_t               i    = (size_t) (k % (uint64_t) (self->cap - 1));
    for (;;) {
        __mp_IntHashTableEntry *e = __mp_da_get(__mp_IntHashTableEntry, self, i);
        if (e->key.valid && k == e->key.key) {
            return &e->val;
        }
        ++i;
        if (i >= self->cap) {
            i = 0;
        }
        if (!e->key.valid && e->key.key != 1) {
            break;
        }
    }
    return NULL;
}

bool __mp_hti_exists(const void *ht, size_t k) {
    return __mp_hti_get(ht, k) != NULL;
}

void __mp_hti_set(void *ht, size_t k, void *v) {
    __mp_DynArray *self = ht;
    mp_hti_grow(self, 1);
    if (self->data != NULL) {
        size_t i = (size_t) (k % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_IntHashTableEntry *e = __mp_da_get(__mp_IntHashTableEntry, self, i);
            if (!e->key.valid) {
                e->key = (__mp_IntHtKey) {
                    .key   = k,
                    .valid = true,
                };
                memcpy(&e->val, v, self->size - sizeof(e->key));
                break;
            } else if (e->key.key == k) {
                memcpy(&e->val, v, self->size - sizeof(e->key));
                --self->len;
                break;
            } else {
                ++i;
            }
            if (i >= self->cap) {
                i = 0;
            }
        }
    }
}

void __mp_hti_grow(void *ht, size_t offset) {
    __mp_DynArray *self = ht;
    if (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD) &&
        offset > 0) {
        size_t old_cap = self->cap;
        if (self->cap == 0) {
            self->cap = __MP_HASH_TABLE_INIT_CAPACITY;
        }
        while (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD)) {
            self->cap *= 2;
        }
        void *new_data = mp_alloc(self->alloc, self->cap * self->size);
        if (new_data == NULL) {
            self->data = NULL;
            return;
        }
        for (size_t i = 0; i < old_cap; ++i) {
            __mp_IntHashTableEntry *e = __mp_da_get(__mp_IntHashTableEntry, self, i);
            if (e->key.valid) {
                size_t new_i = (size_t) (e->key.key % (uint64_t) (self->cap - 1));
                for (;;) {
                    __mp_IntHashTableEntry *new_e =
                        (__mp_IntHashTableEntry *) ((char *) new_data + new_i * self->size);
                    if (!new_e->key.valid) {
                        new_e->key = e->key;
                        memcpy(&new_e->val, &e->val, self->size - sizeof(e->key));
                        break;
                    } else {
                        ++new_i;
                    }
                    if (new_i >= self->cap) {
                        new_i = 0;
                    }
                }
            }
        }
        mp_free(self->alloc, self->data, old_cap * self->size);
        self->data = new_data;
    }
    self->len += offset;
}

void __mp_hti_reset(void *ht) {
    __mp_DynArray *self = ht;
    for (size_t i = 0; i < self->cap; ++i) {
        __mp_IntHashTableEntry *e = __mp_da_get(__mp_IntHashTableEntry, self, i);
        if (e->key.valid) {
            __MP_ZERO(&e->key);
        }
    }
    mp_da_reset(self);
}

void __mp_hti_delete(void *ht, size_t k) {
    __mp_DynArray *self = ht;
    size_t         i    = (size_t) (k % (uint64_t) (self->cap - 1));
    for (;;) {
        __mp_IntHashTableEntry *e = __mp_da_get(__mp_IntHashTableEntry, self, i);
        if (e->key.valid && k == e->key.key) {
            e->key.valid = false;
            e->key.key   = 1;
            --self->len;
            break;
        }
        ++i;
        if (i >= self->cap) {
            i = 0;
        }
        if (!e->key.valid) {
            break;
        }
    }
}

void __mp_hti_clone(void *dest, const void *src, mp_Alloc alloc) {
    const __mp_DynArray *s = src;
    __mp_DynArray       *d = dest;
    __MP_ZERO(d);
    d->data = mp_dup(alloc, s->data, s->cap * s->size);
    if (d->data != NULL) {
        d->alloc = alloc;
        d->len   = s->len;
        d->cap   = s->cap;
        d->size  = s->size;
    }
}

void __mp_hti_iter_init(void *it, const void *ht) {
    __mp_IntHashTableIter *self = it;
    const __mp_DynArray   *h    = ht;
    memset(self, 0, sizeof(*self) + h->size);
    self->_h = h;
}

bool __mp_hti_iter_next(void *it) {
    __mp_IntHashTableIter *self = it;
    while (self->_i < self->_h->cap) {
        __mp_IntHashTableEntry *entry = __mp_da_get(__mp_IntHashTableEntry, self->_h, self->_i);
        if (entry->key.valid) {
            self->key = entry->key;
            memcpy(&self->val, &entry->val, self->_h->size);
            ++self->_i;
            return true;
        }
        ++self->_i;
    }
    return false;
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

static void *mp_arena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                 void *ptr) {
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
                if (capacity < alloc_size) {
                    capacity = alloc_size;
                }
                ctx->end = mp_region_init(ctx->alloc, capacity);
                if (ctx->end == NULL) {
                    return NULL;
                }
                ctx->begin = ctx->end;
            }

            while (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap &&
                   ctx->end->next != NULL) {
                ctx->end = ctx->end->next;
            }

            if (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap) {
                MEMPLUS_ASSERT(ctx->end->next == NULL);
                size_t capacity = ctx->_def_size;
                if (capacity < alloc_size) {
                    capacity = alloc_size;
                }
                ctx->end->next = mp_region_init(ctx->alloc, capacity);
                if (ctx->end->next == NULL) {
                    return NULL;
                }
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
    if (a->buf == NULL) {
        return mp_alloc_invalid();
    }
    return mp_alloc_new(a, mp_sarena_alloc_func);
}

static void *mp_sarena_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                  void *ptr) {
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

            if (ctx->buf == NULL) {
                return NULL;
            }

            size_t alloc_size = __MP_ALIGN(new_size, sizeof(uintptr_t));

            MEMPLUS_ASSERT(ctx->len % sizeof(uintptr_t) == 0);
            if (ctx->len + alloc_size > ctx->cap) {
                return NULL;
            }

            void *result = (char *) ctx->buf + ctx->len;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOCOP_REALLOC: {
            if (ctx->buf == NULL) {
                return NULL;
            }
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

static void *mp_heap_alloc_func(mp_AllocOp op, void *context, size_t new_size, size_t old_size,
                                void *ptr) {
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
        case MP_ERR_NONE:              return "Success";
        case MP_ERR_UNKNOWN:           return "Unknown error";

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

static mp_IoErr mp_file_io_func(mp_IoOp op, mp_Io *io, void *ptr, size_t n1, size_t n2,
                                size_t *ret) {
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
            mp_SetbufMode mp_mode = (mp_SetbufMode) n2;
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
            mp_SetposOrigin mp_origin = (mp_SetposOrigin) n2;
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

#endif /* ifndef __MEMPLUS_H */
