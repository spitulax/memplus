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
 * ## TODO: Changelog
 *
 */

// TODO: Alloc location

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
 * 10. $ FILESYSTEM
 * 11. $ IMPLEMENTATION
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

/*
 * Compilers
 */

#if defined(__GNUC__) || defined(__clang__)
    #define __MP_COMP_GCC_CLANG
#elif defined(_MSC_VER)
    #define __MP_COMP_MSVC
#endif

#if __STDC_VERSION__ >= 202311L
    #define __MP_STD_C23
#elif __STDC_VERSION__ >= 201710L
    #define __MP_STD_C17
#elif __STDC_VERSION__ >= 201112L
    #define __MP_STD_C11
#elif __STDC_VERSION__ >= 199901L
    #define __MP_STD_C99
#else
    #error "You can't compile this"
#endif

#if defined(__MP_STD_C23)
    #define __MP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif defined(__MP_STD_C11)
    #define __MP_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
#else
    #define __MP_STATIC_ASSERT(...) ((void) 0)
#endif

#if defined(__MP_STD_C23)
    #define __MP_NORETURN [[noreturn]]
#elif defined(__MP_STD_C11)
    #define __MP_NORETURN _Noreturn
#else
    #if defined(__MP_COMP_GCC_CLANG)
        #define __MP_NORETURN __attribute__((noreturn))
    #elif defined(__MP_COMP_MSVC)
        #define __MP_NORETURN __declspec(noreturn)
    #else
        #define __MP_NORETURN
    #endif
#endif

#if defined(__MP_STD_C23)
    #define __MP_TYPEOF typeof
#else
    #define __MP_TYPEOF __typeof__
#endif

// Define custom assert by modidying the definition of `__mp_assert_fail()`
#if !(defined(__MP_ASSERT) && defined(__MP_ASSERT_MSG))
    #include <stdio.h>
    #include <stdlib.h>

    #define __MP_NEED_ASSERT
__MP_NORETURN void __mp_assert_fail(const char *assertion, const char *file, const char *func,
                                    size_t line, const char *msg);

    /// \cond
    #ifdef NDEBUG
        #define __MP_ASSERT(expr)
        #define __MP_ASSERT_MSG(expr, msg)
    #else
        #define __MP_ASSERT(expr)                                                                  \
            ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, ""))

        #define __MP_ASSERT_MSG(expr, msg)                                                         \
            ((expr) ? (void) 0 : __mp_assert_fail(#expr, __FILE__, __func__, __LINE__, (msg)))
    #endif
    /// \endcond

#endif

// Assumed have the same behavior as stdlib's `calloc(..., 1)`.
#ifndef __MP_ALLOC
    #include <stdlib.h>
    #define __MP_ALLOC(size) calloc((size), 1)
#endif
// Must have the same signature and behavior as stdlib's `free`.
#ifndef __MP_FREE
    #include <stdlib.h>
    #define __MP_FREE free
#endif

/**
 * \brief The version of the library.
 *
 * Semver encoded in hexadecimal where two digits represent each element.
 * Example: 0.1.0 -> (0x) 00 01 00
 */
#define MEMPLUS_VERSION (0x000100)

// "Private" macros that are used outside of the implementation block.
#define __MP_ZERO(ptr)            memset((ptr), 0, sizeof(*(ptr)))
#define __MP_BOUNDS_CHECK(i, len) __MP_ASSERT_MSG((i) < (len), "Array index out of bounds")
#if defined(__GNUC__) || defined(__clang__)
    #define __MP_PRINTF_FORMAT(fmt_index)                                                          \
        __attribute__((format(printf, (fmt_index), (fmt_index) + 1)))
#else
    #define __MP_PRINTF_FORMAT(fmt_index)
#endif

/**
 * \brief Indicates error return for `size_t`.
 */
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
 * The \ref mp_Alloc_Func "allocator function" is a function that handles the operations requested
 * by the user of your allocator. The function may be given a context, which may contain any data
 * specific to the allocator. For details see \ref mp_Alloc_Func "here".
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
 * void *alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size, void *ptr) {
 *     switch (op) {
 *         case MP_ALLOC_OP_ALLOC:   // do something
 *         case MP_ALLOC_OP_REALLOC: // do something
 *         case MP_ALLOC_OP_FREE:    // do something
 *         case __MP_ALLOC_OP_COUNT: assert(0 & "unreachable");
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

/**
 * \brief Possible operations on \ref mp_Alloc_Func.
 *
 * See the documentation for each operation \ref mp_Alloc_Func "here".
 */
typedef enum {
    MP_ALLOC_OP_ALLOC,
    MP_ALLOC_OP_REALLOC,
    MP_ALLOC_OP_FREE,
    __MP_ALLOC_OP_COUNT,
} mp_Alloc_Op;

/**
 * \brief Function prototype used for allocators.
 *
 * Functions of this type do different things depending on the \a op given.
 * They also use their parameters differently on each type.
 *
 * Operations will ignores parameters that are not listed for them.
 *
 * # Operations
 *
 * - **MP_ALLOC_OP_ALLOC**
 *
 *     Allocates a block of memory and returns the pointer to it.
 *
 *     **Notes**
 *     - If \a new_size == 0, does nothing and returns NULL
 *
 *     **Parameters**
 *     - **context**: allocator context
 *     - **new_size**: size to allocate in bytes
 *
 * - **MP_ALLOC_OP_REALLOC**
 *
 *     Reallocates a block of memory, i.e. allocates new block, copies over the data from the old
 *     block to the new block then frees the old block. Returns the pointer to the new block.
 *
 *     **Notes**
 *     - If \a old_size <= \a new_size, reallocation does not happen and the function just returns
 * \a ptr.
 *     - If \a new_size == 0, does nothing and returns NULL
 *     - If \a old_size == 0 or \a ptr == NULL, skips copying data and freeing the old block,
 * behaving like **MP_ALLOC_OP_ALLOC**
 *
 *     **Parameters**
 *     - **context**: allocator context
 *     - **ptr**: pointer to the old block
 *     - **old_size**: size of old block in bytes
 *     - **new_size**: size of new block in bytes
 *
 * - **MP_ALLOC_OP_FREE**
 *
 *     Frees a block of memory that has been allocated. Always returns NULL.
 *
 *     **Notes**
 *     - If \a ptr == NULL, does nothing
 *
 *     **Parameters**
 *     - **context**: allocator context
 *     - **ptr**: block to be freed
 *     - **new_size**: size of block in bytes
 *
 * \return pointer to newly allocated memory, NULL if allocation failed or with **MP_ALLOC_OP_FREE**
 */
typedef void *(*mp_Alloc_Func)(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                               void *ptr);

/**
 * \brief Inteface to wrap functions to allocate memory.
 */
typedef struct {
    /**
     * \brief Data that is passed to the allocator function.
     *
     * In case of allocator that works with global memory, this can be specified as NULL.
     */
    void *context;

    /**
     * \brief Function that handles the operations requested by the user of the allocator.
     *
     * See \ref mp_Alloc_Func.
     */
    mp_Alloc_Func f;
} mp_Alloc;

/**
 * \defgroup GenericAllocMacros Generic Allocator Macros
 *
 * These macros wrap the operations of \ref mp_Alloc_Func.
 * By passing an \ref mp_Alloc, these macros will call its allocator function and pass the context
 * and the arguments correctly.
 *
 * \{
 */

/**
 * \brief Allocates a block of memory.
 *
 * Calls allocator function with **MP_ALLOC_OP_ALLOC**.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param size (size_t) number of bytes to be allocated
 * \return (void *) pointer to allocated block of memory, NULL if allocation failed
 */
#define /* void* */ mp_alloc(/* mp_Alloc */ alloc, /* size_t */ size)                              \
    ((alloc).f(MP_ALLOC_OP_ALLOC, (alloc).context, (size), 0, NULL))

/**
 * \brief Reallocates a block of memory.
 *
 * Calls allocator function with **MP_ALLOC_OP_REALLOC**.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param old_ptr (void *) pointer to the block to be reallocated
 * \param old_size (size_t) size of block in bytes
 * \param new_size (size_t) size of new allocated block in bytes
 * \return (void *) pointer to newly allocated block of memory, NULL if allocation failed
 */
#define /* void* */ mp_realloc(/* mp_Alloc */ alloc, /* void* */ old_ptr, /* size_t */ old_size,   \
                               /* size_t */ new_size)                                              \
    ((alloc).f(MP_ALLOC_OP_REALLOC, (alloc).context, (new_size), (old_size), (old_ptr)))

/**
 * \brief Frees a block of memory that has been allocated.
 *
 * Calls allocator function with **MP_ALLOC_OP_FREE**.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param ptr (void *) pointer to block to be freed (nullable)
 * \param size (size_t) size of block in bytes
 */
#define mp_free(/* mp_Alloc */ alloc, /* void* */ ptr, /* size_t */ size)                          \
    (void) ((alloc).f(MP_ALLOC_OP_FREE, (alloc).context, (size), 0, (ptr)))

/**
 * \brief Allocates a block of memory that can hold a value of \a type.
 *
 * Free with \ref mp_deinit.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param type ("Type") type name of data
 * \returns (Type *) pointer to newly allocated block, NULL if allocation failed
 */
#define /* Type* */ mp_make(/* mp_Alloc */ alloc, /* "Type" */ type) mp_alloc((alloc), sizeof(type))

/**
 * \brief Allocates clone of a block of memory that holds a value of \a type.
 *
 * Free with \ref mp_deinit.
 *
 * \param alloc (\ref mp_Alloc) allocator
 * \param type ("Type") type name of data
 * \param src (Type *) source of data
 * \return (Type *) pointer to cloned data, NULL if allocation failed
 */
#define /* Type* */ mp_clone(/* mp_Alloc */ alloc, /* "Type" */ type, /* Type* */ src)             \
    mp_dup((alloc), (src), sizeof(type))

/**
 * \brief Frees a block of memory that holds a value of \a type.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param type ("Type") type name of data
 * \param ptr (Type *) pointer to block to be freed (nullable)
 */
#define mp_deinit(/* mp_Alloc */ alloc, /* "Type" */ type, /* Type* */ ptr)                        \
    mp_free((alloc), (ptr), sizeof(type))

/**
 * \brief Allocates a block of memory that can hold an array of \a type.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param type ("Type") type name of data
 * \param len (size_t) length of allocated array
 * \returns (Type *) pointer to newly allocated block, NULL if allocation failed
 */
#define /* Type* */ mp_make_array(/* mp_Alloc */ alloc, /* "Type" */ type, /* size_t */ len)       \
    mp_alloc((alloc), sizeof(type) * (len))

/**
 * \brief Allocates clone of a block of memory that holds an array of \a type.
 *
 * Free with \ref mp_deinit_array.
 *
 * \param alloc (\ref mp_Alloc) allocator
 * \param type ("Type") type name of data
 * \param src (Type *) source of data
 * \param len (size_t) length of array
 * \return (Type *) pointer to cloned data, NULL if allocation failed
 */
#define mp_clone_array(/* mp_Alloc */ alloc, /* "Type" */ type, /* Type* */ src, /* size_t */ len) \
    mp_dup((alloc), (src), sizeof(type) * (len))

/**
 * \brief Frees a block of memory that holds an array of \a type.
 *
 * \param alloc (\ref mp_Alloc) allocator (no side effects)
 * \param type ("Type") type name of data
 * \param ptr (Type *) pointer to block to be freed (nullable)
 * \param len (size_t) length of allocated array
 */
#define mp_deinit_array(/* mp_Alloc */ alloc, /* "Type" */ type, /* Type* */ ptr,                  \
                        /* size_t */ len)                                                          \
    mp_free((alloc), (ptr), sizeof(type) * (len))

/**
 * \brief Allocates a duplicate of \a data.
 *
 * The function allocates a new block of memory with the same size as \a data (i.e. \a size) and
 * copies the data from \a data to the newly allocated block.
 *
 * \param alloc allocator
 * \param data pointer to the block to be cloned
 * \param size size of block
 * \return pointer to allocated clone of \a data
 */
void *mp_dup(mp_Alloc alloc, const void *data, size_t size);

/// \}

/**
 * \brief Create an \ref mp_Alloc from \a ctx and \a func.
 *
 * \param ctx (Any *) allocator context (automatically casted to void *)
 * \param func (\ref mp_Alloc_Func) allocator function
 * \return allocator interface instance working with arguments given
 */
#define /* mp_Alloc */ mp_alloc_new(/* Any* */ ctx, /* mp_Alloc_Func */ func)                      \
    ((mp_Alloc) {                                                                                  \
        .context = (void *) (ctx),                                                                 \
        .f       = (func),                                                                         \
    })

/**
 * \brief Returns an invalid \ref mp_Alloc.
 *
 * An invalid \ref mp_Alloc requires that field \a f == NULL.
 *
 * \return invalid \ref mp_Alloc
 */
#define /* mp_Alloc */ mp_alloc_invalid()                                                          \
    ((mp_Alloc) {                                                                                  \
        .context = NULL,                                                                           \
        .f       = NULL,                                                                           \
    })

/**
 * \brief Handles reallocation for custom allocators.
 *
 * You can call this function in your allocator function as long as alloc and free
 * functionalities are defined.
 *
 * This function already implements realloc operation for an allocator by using its alloc and free
 * operation.
 *
 * # Example
 * \code
 * // ...
 * case MP_ALLOC_OP_REALLOC: {
 *     return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
 * } break;
 * // ...
 * \endcode
 *
 * \param alloc allocator
 * \param old_ptr pointer to the old block
 * \param old_size size of old block in bytes
 * \param new_size size of new block in bytes
 * \return pointer to the new block. If \a new_size == 0, does nothing and returns NULL.
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
 * Throughout the documentation, a generic dynamic array type is written as \a Dyn_Array.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the dynamic
 * array type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_da_typedef(int, Da_Int);
 * \endcode
 *
 * Declare an array then use \ref mp_da_init or \ref mp_da_init_with and pass an allocator to manage
 * the array. \ref mp_da_init does not allocate the data immediately. But only once you append
 * something to the array.
 *
 * \code
 * Da_Int array;
 * mp_da_init(Da_Int, &array, alloc);
 * mp_da_append(&array, 0);
 * mp_da_append_many(&array, 1, 2);
 * \endcode
 *
 * \code
 * Da_Int array;
 * mp_da_init_with(Da_Int, &array, alloc, 0, 1, 2);
 * \endcode
 *
 * By default, arrays start allocating memory for a certain number of elements, and if
 * the array wants more it will reallocate the double of the current capacity.
 *
 * Use \ref mp_da_deinit to free a dynamic array.
 *
 * # Layout
 *
 * \code
 * struct {
 *     mp_Alloc alloc;
 *     size_t   len;
 *     size_t   cap;
 *     size_t   size;
 *     Type     *data;
 * };
 * \endcode
 *
 * **Fields**
 * - **alloc**: The allocator that manages the allocation of the array
 * - **len**: The amount of used data in the array
 * - **cap**: The size of the allocated block holding the data
 * - **size**: The size of an individual datum
 * - **data**: The pointer to the first element of the array (the data are continuous in memory)
 *
 * # Marker
 *
 * Dynamic arrays contain a zero-sized field `__mp_dyn_array_marker`. This field is used as a type
 * checking for function-like macros used for dynamic arrays. These macros will check if this field
 * exists and if it does then the data should be a valid dynamic array and thus can be passed to the
 * implementation function safely.
 *
 * \{
 */

// Starting capacity of a dynamic array.
#ifndef __MP_DARRAY_INIT_CAPACITY
    #define __MP_DARRAY_INIT_CAPACITY 64
#endif

/**
 * \brief Defines a \ref DynamicArray "dynamic array" struct holding data of type \a type.
 *
 * \param type ("Type") array data type name
 * \param name (identifier) name of array struct
 */
#define mp_da_typedef(/* "Type" */ type, /* identifier */ name)                                    \
    typedef struct {                                                                               \
        mp_Alloc alloc;                                                                            \
        size_t   len;                                                                              \
        size_t   cap;                                                                              \
        size_t   size;                                                                             \
        type    *data;                                                                             \
        char     __mp_dyn_array_marker[];                                                          \
    } name

#define __mp_da_struct(type, name)                                                                 \
    struct name {                                                                                  \
        mp_Alloc alloc;                                                                            \
        size_t   len;                                                                              \
        size_t   cap;                                                                              \
        size_t   size;                                                                             \
        type    *data;                                                                             \
        char     __mp_dyn_array_marker[];                                                          \
    }

// Generic dynamic array type.
typedef struct {
    mp_Alloc alloc;
    size_t   len;
    size_t   cap;
    size_t   size;
    void    *data;
} __mp_Dyn_Array;

/**
 * \brief Initializes \a a managed by \a alloc.
 *
 * Deinit with \ref mp_da_deinit.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param type ("Type") array type name
 * \param a (Dyn_Array *) array (initialized by this)
 * \param alloc (\ref mp_Alloc) allocator to manage \a a
 */
#define mp_da_init(/* "Type" */ type, /* Dyn_Array* */ a, /* mp_Alloc */ alloc)                    \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_init((a), (alloc), sizeof(*((type *) 0)->data));                                   \
    } while (0)
void __mp_da_init(void *a, mp_Alloc alloc, size_t size);

/**
 * \brief Initializes \a a managed by \a alloc and appends items to it.
 *
 * Deinit with \ref mp_da_deinit.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param type ("Type") array type name
 * \param a (Dyn_Array *) array (initialized by this) (no side effects)
 * \param alloc (\ref mp_Alloc) allocator to manage \a a
 * \param ... (DataType...) values to append
 */
#define mp_da_init_with(/* Type */ type, /* Dyn_Array* */ a, /* mp_Alloc */ alloc,                 \
                        /* DataType... */...)                                                      \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        mp_da_init(type, (a), (alloc));                                                            \
        mp_da_append_many((a), __VA_ARGS__);                                                       \
    } while (0)

/**
 * \brief Deinitializes \a a.
 *
 * \param a (Dyn_Array *) array (deinitialized by this)
 */
#define mp_da_deinit(/* Dyn_Array* */ a)                                                           \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_deinit(a);                                                                         \
    } while (0)
void __mp_da_deinit(void *a);

void __mp_da_append(void *a, const void *items, size_t items_len);

/**
 * \brief Appends \a item to \a a.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param item (Type) item to append to \a a
 */
#define mp_da_append(/* Dyn_Array* */ a, /* Type */ item)                                          \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(item) __it = (item);                                                           \
        __MP_ASSERT_MSG(sizeof(__it) == (a)->size,                                                 \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_append((a), &__it, 1);                                                             \
    } while (0)

/**
 * \brief Appends multiple items to \a a.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param ... (Type...) items to append to \a a
 */
#define mp_da_append_many(/* Dyn_Array* */ a, /* Type... */...)                                    \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*(a)->data) __items[] = { __VA_ARGS__ };                                       \
        size_t __len                      = sizeof(__items) / sizeof(*__items);                    \
        __MP_ASSERT_MSG(sizeof(*__items) == (a)->size,                                             \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_append((a), __items, __len);                                                       \
    } while (0)

/**
 * \brief Appends items from \a items to \a a.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array
 * \param items (Type *) array of items to append to \a a
 * \param items_len (size_t) amount of items in \a items
 */
#define mp_da_append_array(/* Dyn_Array* */ a, /* Type* */ items, /* size_t */ items_len)          \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*items) *__items = (items);                                                    \
        __MP_ASSERT_MSG(sizeof(*__items) == (a)->size,                                             \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_append((a), __items, (items_len));                                                 \
    } while (0)

/**
 * \brief Gets item at \a i.
 *
 * No bounds checking, use \ref mp_da_get_s for that.
 *
 * \param a (const Dyn_Array*) array
 * \param i (size_t) index to item
 * \return (Type) item at \a i
 */
#define /* Type */ mp_da_get(/* const Dyn_Array* */ a, /* size_t */ i)                             \
    ((void) (a)->__mp_dyn_array_marker, (a)->data[i])

/**
 * \brief Gets pointer to item at \a i.
 *
 * No bounds checking, use \ref mp_da_get_s for that.
 *
 * \param a (const Dyn_Array*) array
 * \param i (size_t) index to item
 * \return (Type *) pointer to item at \a i
 */
#define /* Type* */ mp_da_getp(/* const Dyn_Array* */ a, /* size_t */ i)                           \
    ((void) (a)->__mp_dyn_array_marker, (a)->data + i)

/**
 * \brief Gets item at \a i with bounds-checking.
 *
 * Asserts that \a i is not out of bounds.
 *
 * The assert will not trigger if `NDEBUG` is defined.
 *
 * \param a (const Dyn_Array*) array
 * \param i (size_t) index to item
 * \return (Type) item at \a i
 */
#define /* Type */ mp_da_get_s(/* const Dyn_Array */ a, /* size_t */ i)                            \
    ((void) (a)->__mp_dyn_array_marker, __MP_BOUNDS_CHECK((i), (a)->len), (a)->data[i])

/**
 * \brief Gets pointer to item at \a i with bounds-checking.
 *
 * Asserts that \a i is not out of bounds.
 *
 * The assert won't trigger if `NDEBUG` is defined.
 *
 * \param a (const Dyn_Array*) array
 * \param i (size_t) index to item
 * \return (Type *) pointer to item at \a i
 */
#define /* Type* */ mp_da_getp_s(/* const Dyn_Array */ a, /* size_t */ i)                          \
    ((void) (a)->__mp_dyn_array_marker, __MP_BOUNDS_CHECK((i), (a)->len), (a)->data + i)

// Generic dynamic array get function
#define __mp_da_get(type, a, i) (type *) ((char *) (a)->data + (i) * (a)->size)

/**
 * \brief Alias of \ref mp_da_get.
 */
#define mp_get mp_da_get

/**
 * \brief Alias of \ref mp_da_getp.
 */
#define mp_getp mp_da_getp

/**
 * \brief Passes \a a to a function that accepts array as pointer and length.
 *
 * Example usage:
 * \code
 * mp_str_concat(mp_da_arg(strings), NULL, mp_heap());
 * \endcode
 *
 * \param a (const? Dyn_Array *) array (no side effects)
 */
#define mp_da_arg(/* const? Dyn_Array* */ a) (a)->data, (a)->len

/**
 * \brief Gets the last item in \a a.
 *
 * \param a (const Dyn_Array *) array (no side effects)
 * \return (Type) the last item in \a a
 */
#define /* Type */ mp_da_last(/* const Dyn_Array* */ a)                                            \
    ((void) (a)->__mp_dyn_array_marker, (a)->data[(a)->len - 1])

/**
 * \brief Removes the last item in \a a and returns it.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \return (Type) the last item in \a a
 */
#define /* Type */ mp_da_pop(/* Dyn_Array* */ a)                                                   \
    ((void) (a)->__mp_dyn_array_marker, --(a)->len, (a)->data[(a)->len])

/**
 * \brief Sets length of \a a to 0.
 *
 * This resets the dynamic array to "initial condition" but without actually freeing the data.
 *
 * \param a (Dyn_Array *) array
 */
#define mp_da_reset(/* Dyn_Array* */ a)                                                            \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        (a)->len = 0;                                                                              \
    } while (0)

/**
 * \brief Grows \a a to be able to hold \a offset more items from the current length.
 *
 * Does a calculation to determine the new capacity and then calls \ref mp_da_reserve if
 * allocation is needed, which happens when length + offset is greater than the capacity.
 *
 * This function may increase \a a->cap if allocation does happen, but it will not increase \a
 * a->len by itself.
 *
 * If \a a->cap is 0, reserves for a certain number of items.
 *
 * If \a a->cap is not large enough, reserves for double of \a a->cap.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array
 * \param offset (size_t) amount to grow
 */
#define mp_da_grow(/* Dyn_Array* */ a, /* size_t */ offset)                                        \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_grow((a), (offset));                                                               \
    } while (0)
void __mp_da_grow(void *a, size_t offset);

/**
 * \brief Reserve \a a to hold exactly \a offset more items from the current capacity.
 *
 * Increases \a a->cap by \a offset and reallocates \a a->data if \a offset is greater than 0.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array
 * \param offset (size_t) amount to grow
 */
#define mp_da_reserve(/* Dyn_Array* */ a, /* size_t */ offset)                                     \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_reserve((a), (offset));                                                            \
    } while (0)
void __mp_da_reserve(void *a, size_t offset);

/**
 * \brief Clones \a src to \a dest managed by \a alloc.
 *
 * The \a dest array does not inherit the capacity of \a src. Instead it will only
 * allocate for \a src.len + *initial capacity* items.
 *
 * \a dest->data == NULL if allocation failed.
 *
 * \param dest (Dyn_Array *) destination of the clone (initialized by this)
 * \param src (const Dyn_Array *) source array
 * \param alloc (\ref mp_Alloc) allocator to manage \a dest
 */
#define mp_da_clone(/* Dyn_Array* */ dest, /* const Dyn_Array* */ src, /* mp_Alloc */ alloc)       \
    do {                                                                                           \
        (void) (dest)->__mp_dyn_array_marker;                                                      \
        (void) (src)->__mp_dyn_array_marker;                                                       \
        __mp_da_clone((dest), (src), (alloc));                                                     \
    } while (0)
void __mp_da_clone(void *dest, const void *src, mp_Alloc alloc);

void __mp_da_insert(void *a, size_t pos, const void *items, size_t items_len);

/**
 * \brief Inserts item to \a a at \a pos.
 *
 * The item will be exactly at \a pos after insertion. Items after it gets moved.
 *
 * If \a pos > \a a->len, then just puts the item at \a a->len.
 *
 * \a pos must not be negative.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param pos (size_t) insertion position
 * \param item (Type) item to insert
 */
#define mp_da_insert(/* Dyn_Array* */ a, /* size_t */ pos, /* Type */ item)                        \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(item) __it = (item);                                                           \
        __MP_ASSERT_MSG(sizeof(__it) == (a)->size,                                                 \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_insert((a), (pos), &__it, 1);                                                      \
    } while (0)

/**
 * \brief Inserts multiple items to \a a at \a pos.
 *
 * The first item will be exactly at \a pos after insertion and the succeding items follow. Items
 * after them gets moved.
 *
 * If \a pos > \a a->len, then just puts the items at \a a->len.
 *
 * \a pos must not be negative.
 *
 * \a a->data == NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param pos (size_t) insertion position
 * \param ... (Type...) items to insert
 */
#define mp_da_insert_many(/* Dyn_Array* */ a, /* size_t */ pos, /* Type... */...)                  \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*(a)->data) __items[] = { __VA_ARGS__ };                                       \
        size_t __len                      = sizeof(__items) / sizeof(*__items);                    \
        __MP_ASSERT_MSG(sizeof(*__items) == (a)->size,                                             \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_insert((a), (pos), __items, __len);                                                \
    } while (0)

/**
 * \brief Inserts items from \a items to \a a at \a pos.
 *
 * If \a pos > \a a->len, then it just puts the item at \a a->len.
 *
 * \a pos must not be negative.
 *
 * \a a->data becomes NULL if allocation failed.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param pos (size_t) insertion position
 * \param items (Type *) array of items to insert to \a a
 * \param items_len (size_t) amount of items in \a items
 */
#define mp_da_insert_array(/* Dyn_Array* */ a, /* size_t */ pos, /* Type* */ items,                \
                           /* size_t */ items_len)                                                 \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*items) *__items = (items);                                                    \
        __MP_ASSERT_MSG(sizeof(*__items) == (a)->size,                                             \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_insert((a), (pos), (items), (items_len));                                          \
    } while (0)

/**
 * \brief Removes \a len of items from \a a at \a pos.
 *
 * This operation is O(n) in the worst case. This may move items to the deleted slots with items
 * from the slots after it.
 *
 * If you only want to delete one item and do not care about the order of the elements after the
 * delete, use \ref mp_da_quick_delete instead.
 *
 * \param a (Dyn_Array *) array
 * \param pos (size_t) position of the first item to delete
 * \param len (size_t) amount of items to delete
 */
#define mp_da_delete(/* Dyn_Array* */ a, /* size_t */ pos, /* size_t */ len)                       \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_move((a), (pos), NULL, (len));                                                     \
    } while (0)

/**
 * \brief Removes a single item from \a a at \a pos if you do not care about the order of items.
 *
 * This operation is O(1) and a much faster alternative to \ref mp_da_delete if you do not care
 * about the order of items.
 *
 * This works by swapping the item to be deleted with the last item and shrinking the array,
 * effectively making it ignore the last item.
 *
 * \param a (Dyn_Array *) array
 * \param pos (size_t) position of item to delete
 */
#define mp_da_quick_delete(/* Dyn_Array* */ a, /* size_t */ pos)                                   \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __mp_da_quick_move((a), (pos), NULL);                                                      \
    } while (0)

/**
 * \brief Moves \a len of items from \a a at \a pos to \a dest.
 *
 * This operation is O(n) in the worst case. This may move items to the deleted slots with items
 * from the slots after it.
 *
 * If you only want to move one item and do not care about the order of the elements after the
 * delete, use \ref mp_da_quick_move instead.
 *
 * \a dest must not alias \a a->data.
 *
 * \param a (Dyn_Array *) array (no side effects)
 * \param pos (size_t) position of the first item to delete
 * \param len (size_t) amount of items to delete
 * \param dest (Type *) where to copy deleted items (must be at least \a len * sizeof(Type))
 */
#define mp_da_move(/* Dyn_Array* */ a, /* size_t */ pos, /* size_t */ len, /* Type* */ dest)       \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*dest) *__dest = (dest);                                                       \
        __MP_ASSERT_MSG(sizeof(*__dest) == (a)->size,                                              \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_move((a), (pos), __dest, (len));                                                   \
    } while (0)
void __mp_da_move(void *a, size_t pos, void *ret_items, size_t items_len);

/**
 * \brief Moves a single item from \a a at \a pos to \a dest if you do not care about the order of
 * items.
 *
 * This operation is O(1) and a much faster alternative to \ref mp_da_move if you do not care
 * about the order of items.
 *
 * This works by swapping the item to be deleted with the last item and shrinking the array,
 * effectively making it ignore the last item.
 *
 * \a dest must not alias \a a->data.
 *
 * \param a (Dyn_Array *) array
 * \param pos (size_t) position of item to delete
 * \param dest (Type *) where to copy the deleted item (must be at least sizeof(Type))
 */
#define mp_da_quick_move(/* Dyn_Array* */ a, /* size_t */ pos, /* Type* */ dest)                   \
    do {                                                                                           \
        (void) (a)->__mp_dyn_array_marker;                                                         \
        __MP_TYPEOF(*dest) *__dest = (dest);                                                       \
        __MP_ASSERT_MSG(sizeof(*__dest) == (a)->size,                                              \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_da_quick_move((a), (pos), __dest);                                                    \
    } while (0)
void __mp_da_quick_move(void *a, size_t pos, void *ret_item);

/// \}

/***********
 * $ STRING
 ***********/

/**
 * \defgroup String String
 *
 * Holds a pointer to a string and its size (excluding the null-terminator if any).
 *
 * This object is a "view" to a string data and does not manage or allocate the data itself.
 *
 * \{
 */

/**
 * \brief Holds a pointer to a string and its size (excluding the null-terminator if any).
 */
typedef struct {
    /// Length/size of the string (in bytes, excluding the null-terminator).
    size_t len;
    /// Pointer to the first character.
    const char *data;
} mp_Str;

/**
 * \brief Returns invalid \ref mp_Str.
 *
 * An invalid \ref mp_Str requires that field \a data == NULL.
 *
 * \return (\ref mp_Str) invalid string
 */
#define /* mp_Str */ mp_str_invalid()                                                              \
    ((mp_Str) {                                                                                    \
        .len  = 0,                                                                                 \
        .data = NULL,                                                                              \
    })

/**
 * \brief Tests whether \a str is valid.
 *
 * See \ref mp_str_invalid.
 *
 * \param str (\ref mp_Str) string
 * \return (bool) whether \a s is valid
 */
#define /* bool */ mp_str_is_valid(/* mp_Str */ str) ((str).data != NULL)

/**
 * \brief Creates a view to a null-terminated string.
 *
 * Calculates string length with strlen.
 *
 * \param str (const char *) null-terminated string (no side effects)
 * \return (\ref mp_Str) view to \a str
 */
#define /* mp_Str */ mp_str(/* const char* */ str) mp_str_s((str), strlen(str))

/**
 * \brief Creates a view to a string.
 *
 * \param str (const char *) string
 * \param length (size_t) length of \a str
 * \return (\ref mp_Str) view to \a str
 */
#define /* mp_Str */ mp_str_s(/* const char* */ str, /* size_t */ length)                          \
    ((mp_Str) {                                                                                    \
        .len  = (length),                                                                          \
        .data = (str),                                                                             \
    })

/**
 * \brief Shortcut for printing a \ref mp_Utf8_Char_Data, use with `%.*s` format specifier.
 *
 * \param str (\ref mp_Str) string (no side effects)
 */
#define mp_str_print(/* mp_Str */ str) (int) (str).len, (str).data

/**
 * \brief Passes \a str to a function that accepts string as pointer and length.
 *
 * \param str (mp_Str) string (no side effects)
 */
#define mp_str_arg(/* mp_Str */ str) (str).data, (str).len

/**
 * \brief Allocates copy of null-terminated \a str with \a alloc and returns a view to it.
 *
 * \param str null-terminated string
 * \param alloc allocator for the copy
 * \return allocated copy of \a str (not null-terminated), \ref mp_str_invalid "invalid string" if
 * allocation failed
 */
mp_Str mp_str_alloc(const char *str, mp_Alloc alloc);

/**
 * \brief Allocates clone of \a str with \a alloc.
 *
 * \param str string (may or may not be allocated on heap)
 * \param alloc allocator for the clone
 * \return allocated clone of \a str (not null-terminated), \ref mp_str_invalid "invalid string" if
 * allocation failed
 */
mp_Str mp_str_clone(mp_Str str, mp_Alloc alloc);

/**
 * \brief Deinitializes \a str, given it was allocated on the heap with \a alloc.
 *
 * \param str pointer to string (allocated on heap)
 * \param alloc allocator that allocated \a str
 */
void mp_str_deinit(mp_Str *str, mp_Alloc alloc);

/**
 * \brief Creates a clone of \a str that is null-terminated with \a alloc.
 *
 * \param str string to clone
 * \param alloc allocator for the clone
 * \return null-terminated clone of \a str, NULL if allocation failed
 */
char *mp_str_null_terminated_from(mp_Str str, mp_Alloc alloc);

/**
 * \brief Deinitializes null-terminated \a str allocated with \a alloc.
 *
 * \param str pointer null-terminated string to deinitialize
 * \param alloc allocator that allocated \a str
 */
void mp_str_null_terminated_deinit(char **str, mp_Alloc alloc);

/**
 * \brief Tests whether \a a and \a b are equal.
 *
 * \param a string
 * \param b string
 * \return whether \a a and \a b are equal
 */
bool mp_str_eq(mp_Str a, mp_Str b);

// TODO: String functions:
// - mp_str_substr
// - mp_str_starts/ends_with
// - mp_str_split
// - mp_str_trim

/// \}

/***********
 * $ STRING BUILDER
 ***********/

/**
 * \defgroup StringBuilder String Builder
 *
 * Holds and manage a **non null-terminated** string that is resizable.
 * The underlying data type is a \ref DynamicArray "dynamic array" of char.
 *
 * Pass references to \ref mp_Sb with \ref mp_Str type, use \ref mp_sb_str to
 * convert the string.
 *
 * To convert to a C-compatible string which is null-terminated, use \ref
 * mp_str_null_terminated_from which is going to allocate a clone of the string data and use
 * \ref mp_str_null_terminated_deinit to free it.
 *
 * # Initialization
 *
 * Initializing an \ref mp_Sb with \ref mp_sb_init_with or \ref mp_sb_init_withf will allocate for
 * exactly the amount of bytes required. Meanwhile, initializing then appending manually may
 * allocate for more than required to minimize the amount of future reallocations.
 *
 * If you do not plan to append more data to an \ref mp_Sb, initialize with \ref mp_sb_init_with or
 * \ref mp_sb_init_withf instead to be more memory efficient.
 *
 * \{
 */

__mp_da_struct(char, __mp_Sb);

/**
 * \brief Holds a string that is resizable.
 *
 * The underlying data type is a \ref DynamicArray "dynamic array" of char.
 */
typedef struct __mp_Sb mp_Sb;

/**
 * \brief Initializes \a sb managed by \a alloc.
 *
 * Deinit with \ref mp_sb_deinit.
 *
 * \param sb string builder (initialized by this)
 * \param alloc managing allocator
 */
void mp_sb_init(mp_Sb *sb, mp_Alloc alloc);

/**
 * \brief Initializes \a sb managed by \a alloc and append \a str.
 *
 * Deinit with \ref mp_sb_deinit.
 *
 * Will only reserve for exactly the length of \a str.
 *
 * \a sb->data == NULL if allocation failed.
 *
 * \param sb string builder (initialized by this)
 * \param str initial string
 * \param alloc managing allocator
 */
void mp_sb_init_with(mp_Sb *sb, mp_Str str, mp_Alloc alloc);

/**
 * \brief Initializes \a sb managed by \a alloc and append a formatted string.
 *
 * Deinit with \ref mp_sb_deinit.
 *
 * Will only reserve for exactly the length of the resulting formatted string.
 *
 * \a sb->data == NULL if allocation failed.
 *
 * \param sb string builder (initialized by this)
 * \param alloc managing allocator
 * \param fmt formatting string
 * \param ... formatting arguments
 */
void mp_sb_init_withf(mp_Sb *sb, mp_Alloc alloc, const char *fmt, ...) __MP_PRINTF_FORMAT(3);

/**
 * \brief Deinitializes \a sb.
 *
 * \param sb string builder (deinitialized by this)
 */
void mp_sb_deinit(mp_Sb *sb);

/**
 * \brief Appends \a str to \a sb.
 *
 * \a sb->data == NULL if allocation failed.
 *
 * \param sb string builder
 * \param str string to be appended
 */
void mp_sb_append(mp_Sb *sb, mp_Str str);

/**
 * \brief Appends a formatted string to \a sb.
 *
 * \a sb->data == NULL if allocation failed.
 *
 * \param sb string builder
 * \param fmt formatting string
 * \param ... formatting arguments
 */
void mp_sb_appendf(mp_Sb *sb, const char *fmt, ...) __MP_PRINTF_FORMAT(2);

/**
 * \brief Gets a view to \a sb.
 *
 * \param sb string builder
 * \return view to \a sb
 */
mp_Str mp_sb_str(const mp_Sb *sb);

// TODO: Sb functions:
// - mp_sb_concat

/// \}

/***********
 * $ HASH TABLE (STRING KEY)
 ***********/

/**
 * \defgroup HashTableString Hash Table (String Key)
 *
 * Hash table with string key (not null-terminated).
 * This uses the FNV-1a hash algorithm to hash the string.
 *
 * Throughout the documentation, a generic hash table (string key) type is written as \a
 * Str_Hash_Table. Similarly, its iterator is written as \a Str_Hash_Table_Iter.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the hash table
 * type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_ht_typedef(int, Ht_Int);
 * \endcode
 *
 * Declare a hash table then use \ref mp_ht_init and pass an allocator to manage the hash table.
 * This does not allocate the data immediately. But only once you set something on the hash table.
 * \code
 * Ht_Int ht;
 * mp_ht_init(Ht_Int, &ht, alloc);
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
 * Using \ref mp_ht_typedef also defines an iterator type for that hash table type, named by
 * suffixing `_Iter` to the hash table's type name.
 * Example usage:
 * \code
 * Ht_Int_Iter it;
 * mp_ht_iter_init(&it, &ht);
 * while (mp_ht_iter_next(&it)) {
 *     (void) it.key;
 *     (void) it.val;
 * }
 * \endcode
 *
 * It is best to not modify the hash table in the middle of iteration.
 *
 * # Layout
 *
 * ## Hash Table
 *
 * \code
 * struct {
 *     mp_Alloc    alloc;
 *     size_t      len;
 *     size_t      cap;
 *     size_t      size;
 *     Entry_Type *data;
 *     size_t      val_size;
 * };
 * \endcode
 *
 * **Fields**
 * - **alloc**: allocator that manages the allocations of the hash table
 * - **len**: amount of items in the hash table
 * - **cap**: size of the allocated block holding the data in bytes
 * - **size**: size of an entry in bytes
 * - **data**: pointer to the first entry (the entries are continuous in memory)
 * - **val_size**: size of individual value in bytes (0 for hash sets)
 *
 * ## Entry
 *
 * \code
 * struct {
 *     mp_Str     key;
 *     Value_Type val;
 * };
 * \endcode
 *
 * **Fields**
 * - **key**: key indicating the entry
 * - **val**: value at the entry
 *
 * ## Iterator
 *
 * \code
 * struct {
 *     const Str_Hash_Table *_h;
 *     size_t                _i;
 *     mp_Str                key;
 *     Value_Type           *val;
 * };
 * \endcode
 *
 * **Fields**
 * - **_h**: hash table being iterated on
 * - **_i**: index of iteration
 * - **key**: retrieved key
 * - **val** pointer to the retrieved value at \a key
 *
 * # Marker
 *
 * String hash tables contain a zero-sized field `__mp_str_ht_marker`. This field is used as a type
 * checking for function-like macros used for string hash tables. These macros will check if this
 * field exists and if it does then the data should be a valid string hash tables and thus can be
 * passed to the implementation function safely.
 * String hash table iterators also contain similar field `__mp_str_ht_iter_marker`.
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

/**
 * \brief Defines a \ref HashTableString "hash table (string key)" struct with value of type \a
 * value_type.
 *
 * This also defines the hash table's iterator type, named by suffixing `Iter`
 * after the hash table's type name.
 *
 * \param value_type ("Type") value type name
 * \param name (identifier) hash table name
 */
#define mp_ht_typedef(/* "Type" */ value_type, /* identifier */ name)                              \
    typedef struct {                                                                               \
        mp_Str     key;                                                                            \
        value_type val;                                                                            \
    } __##name##_Entry;                                                                            \
    typedef struct {                                                                               \
        mp_Alloc          alloc;                                                                   \
        size_t            len;                                                                     \
        size_t            cap;                                                                     \
        size_t            size;                                                                    \
        __##name##_Entry *data;                                                                    \
        size_t            val_size;                                                                \
        char              __mp_str_ht_marker[];                                                    \
    } name;                                                                                        \
    typedef struct {                                                                               \
        const name *_h;                                                                            \
        size_t      _i;                                                                            \
        mp_Str      key;                                                                           \
        value_type *val;                                                                           \
        char        __mp_str_ht_iter_marker[];                                                     \
    } name##_Iter

#define __mp_ht_struct(entry_type, name)                                                           \
    struct name {                                                                                  \
        mp_Alloc    alloc;                                                                         \
        size_t      len;                                                                           \
        size_t      cap;                                                                           \
        size_t      size;                                                                          \
        entry_type *data;                                                                          \
        size_t      val_size;                                                                      \
        char        __mp_str_ht_marker[];                                                          \
    }

// Generic hash table type.
typedef struct {
    mp_Alloc alloc;
    size_t   len;
    size_t   cap;
    size_t   size;
    void    *data;
    size_t   val_size;
} __mp_Hash_Table;

// Generic string hash table entry type.
typedef struct {
    mp_Str key;
    char   val[];
} __mp_Str_Ht_Entry;

// Generic string hash table iterator type.
typedef struct {
    const __mp_Hash_Table *_h;
    size_t                 _i;
    mp_Str                 key;
    void                  *val;
} __mp_Str_Ht_Iter;

/**
 * \brief Initializes \a ht managed by \a alloc.
 *
 * Deinit with \ref mp_ht_deinit.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param type ("Type") hash table type name
 * \param ht (Str_Hash_Table *) hash table (initialized by this)
 * \param alloc (\ref mp_Alloc) allocator to manage \a ht
 */
#define mp_ht_init(/* "Type" */ type, /* Str_Hash_Table* */ ht, /* mp_Alloc */ alloc)              \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_init((ht), (alloc), sizeof(*((type *) 0)->data),                                   \
                     sizeof((*((type *) 0)->data).val));                                           \
    } while (0)
void __mp_ht_init(void *ht, mp_Alloc alloc, size_t size, size_t val_size);

/**
 * \brief Deinitializes \a ht.
 *
 * \param ht (Str_Hash_Table *) hash table (deinitialized by this)
 */
#define mp_ht_deinit(/* Str_Hash_Table* */ ht)                                                     \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_deinit(ht);                                                                        \
    } while (0)
void __mp_ht_deinit(void *ht);

/**
 * \brief Gets pointer to item at \a k.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param k (const char *) key (non-null)
 * \return (void *) retrieved value, NULL if cannot retrieve
 */
#define /* void* */ mp_ht_get(/* const Str_Hash_Table* */ ht, /* const char* */ k)                 \
    mp_ht_get_s((ht), mp_str(k))

/**
 * \brief Same as \ref mp_ht_get but accepts \ref mp_Str.
 *
 * See \ref mp_ht_get.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param k (\ref mp_Str) key
 * \return (void *) retrieved value, NULL if cannot retrieve
 */
#define /* void* */ mp_ht_get_s(/* const Str_Hash_Table* */ ht, /* mp_Str */ k)                    \
    ((void) (ht)->__mp_str_ht_marker, __mp_ht_get((ht), (k)))
void *__mp_ht_get(const void *ht, mp_Str k);

/**
 * \brief Sets the value at \a k to \a v.
 *
 * When the entry at \a k has not been initialized before, the key is cloned.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param ht (Str_Hash_Table *) hash table (no side effects)
 * \param k (const char *) key
 * \param v (Type) value to be stored
 */
#define mp_ht_set(/* Str_Hash_Table* */ ht, /* const char* */ k, /* Type */ v)                     \
    mp_ht_set_s((ht), mp_str(k), (v))

/**
 * \brief Same as \ref mp_ht_set but accepts \ref mp_Str.
 *
 * See \ref mp_ht_set.
 *
 * \param ht (Str_Hash_Table *) hash table (no side effects)
 * \param k (\ref mp_Str) key
 * \param v (Type) value to be stored
 */
#define mp_ht_set_s(/* Str_Hash_Table* */ ht, /* mp_Str */ k, /* Type */ v)                        \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __MP_TYPEOF(v) __it = (v);                                                                 \
        __MP_ASSERT_MSG((ht)->val_size > 0, "Did you mean to use `mp_hs_set` instead?");           \
        __MP_ASSERT_MSG(                                                                           \
            sizeof(__it) == (ht)->val_size,                                                        \
            "The size of the value provided does not match the hash table's value size");          \
        __mp_ht_set((ht), (k), &__it);                                                             \
    } while (0)
void __mp_ht_set(void *ht, mp_Str k, void *v);

/**
 * \brief Tests whether \a k exists in \a ht.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param k (const char *) key
 * \return (bool) whether \a k exists in \a ht
 */
#define /* bool */ mp_ht_exists(/* const Str_Hash_Table* */ ht, /* const char * */ k)              \
    __mp_ht_exists((ht), mp_str(k))

/**
 * \brief Same as \ref mp_ht_exists but accepts \ref mp_Str.
 *
 * See \ref mp_ht_exists.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param k (\ref mp_Str) key
 * \return (bool) whether \a k exists in \a ht
 */
#define /* bool */ mp_ht_exists_s(/* const Str_Hash_Table* */ ht, /* mp_Str */ k)                  \
    ((void) (ht)->__mp_str_ht_marker, __mp_ht_exists((ht), (k)))
bool __mp_ht_exists(const void *ht, mp_Str k);

/**
 * \brief Grows \a ht to be able to hold \a offset more items from the current length.
 *
 * Does a calculation to determine the new capacity. The increase of the new capacity may not be
 * equal to \a offset.
 *
 * If \a ht->cap is 0, reserves for a certain number of items.
 *
 * If \a ht->cap is not large enough, reserves for double of \a ht->cap.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param ht (Str_Hash_Table *) hash table
 * \param offset (size_t) amount to grow
 */
#define mp_ht_grow(/* Str_Hash_Table* */ ht, /* size_t */ offset)                                  \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_grow((ht), (offset));                                                              \
    } while (0)
void __mp_ht_grow(void *ht, size_t offset);

// Invalidates and frees the string keys
void __mp_ht_free_entries(void *entries, mp_Alloc alloc, size_t cap, size_t size);

/**
 * \brief Sets the length of \a ht to 0 and frees its keys.
 *
 * This resets \a ht to "initial condition" but without actually freeing the data.
 *
 * \param ht (Str_Hash_Table *) hash table
 */
#define mp_ht_reset(/* Str_Hash_Table* */ ht)                                                      \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_reset(ht);                                                                         \
    } while (0)
void __mp_ht_reset(void *ht);

/**
 * \brief Deletes an entry at \a k from \a ht.
 *
 * This decreases \a ht->len but does not actually shrink the hash table, but it just
 * marks the entry as "deleted", which may be overridden by subsequent set operations.
 *
 * Does nothing if it cannot find \a k.
 *
 * \param ht (Str_Hash_Table *) hash table
 * \param k (const char *) key
 */
#define mp_ht_delete(/* Str_Hash_Table* */ ht, /* const char* */ k) mp_ht_delete_s((ht), mp_str(k))

/**
 * \brief Same as \ref mp_ht_delete but accepts to \ref mp_Str.
 *
 * See \ref mp_ht_delete.
 *
 * \param ht (Str_Hash_Table *) hash table
 * \param k (\ref mp_Str) key
 */
#define mp_ht_delete_s(/* Str_Hash_Table* */ ht, /* mp_Str */ k)                                   \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_delete((ht), (k));                                                                 \
    } while (0)
void __mp_ht_delete(void *ht, mp_Str k);

/**
 * \brief Clones \a src to \a dest managed by \a alloc.
 *
 * \a dest inherits all fields of \a src.
 * \a dest->data == NULL if allocation failed.
 *
 * \param dest (Str_Hash_Table *) destination of the clone (initialized by this)
 * \param src (const Str_Hash_Table *) source hash table
 * \param alloc (\ref mp_Alloc) allocator to manage \a dest
 */
#define mp_ht_clone(/* Str_Hash_Table* */ dest, /* const Str_Hash_Table* */ src,                   \
                    /* mp_Alloc */ alloc)                                                          \
    do {                                                                                           \
        (void) (dest)->__mp_str_ht_marker;                                                         \
        (void) (src)->__mp_str_ht_marker;                                                          \
        __mp_ht_clone((dest), (src), (alloc));                                                     \
    } while (0)
void __mp_ht_clone(void *dest, const void *src, mp_Alloc alloc);

/**
 * \brief A \ref DynamicArray "dynamic array" of \ref mp_Str to hold the keys of \ref
 * HashTableString "hash tables (string key)".
 */
typedef struct __mp_Ht_Keys mp_Ht_Keys;

__mp_da_struct(mp_Str, __mp_Ht_Keys);

/**
 * \brief Deinitializes \a keys.
 *
 * \param keys key array (deinitialized by this)
 */
void mp_ht_keys_deinit(mp_Ht_Keys *keys);

/**
 * \brief Extracts the keys from \a ht to \a keys.
 *
 * \a keys **must be initialized** with \ref mp_da_init first.
 *
 * Each key will be cloned using the allocator of \a keys, so free \a keys with \ref
 * mp_ht_keys_deinit.
 *
 * \a keys->data == NULL if allocation failed.
 *
 * # Note
 * This function allocates a temporary bit of memory using \a ht->alloc.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param keys (\ref mp_Ht_Keys *) destination of keys (must be initialized first)
 */
#define mp_ht_keys(/* const Str_Hash_Table* */ ht, /* mp_Ht_Keys* */ keys)                         \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_keys((ht), (keys));                                                                \
    } while (0)
void __mp_ht_keys(const void *ht, mp_Ht_Keys *keys);

/**
 * \brief Extracts the values from \a ht to \a values.
 *
 * \a values is a \ref DynamicArray "dynamic array" and its data type **must match** the
 * value type of \a ht.
 *
 * \a values **must be initialized** with \ref mp_da_init first.
 *
 * \a values->data == NULL if allocation failed.
 *
 * # Note
 * This function allocates a temporary bit of memory using \a ht->alloc.
 *
 * \param ht (const Str_Hash_Table *) hash table
 * \param values (Dyn_Array *) destination of values (must be initialized first)
 */
#define mp_ht_values(/* const Str_Hash_Table* */ ht, /* Dyn_Array* */ values)                      \
    do {                                                                                           \
        (void) (ht)->__mp_str_ht_marker;                                                           \
        __mp_ht_values((ht), (values));                                                            \
    } while (0)
void __mp_ht_values(const void *ht, void *values);

/**
 * \brief Initializes \a it to iterate on \a ht.
 *
 * To use hash table iterators, see \ref HashTableString.
 *
 * \param it (Str_Hash_Table_Iter *) iterator (initialized by this)
 * \param ht (const Str_Hash_Table *) hash table to iterate
 */
#define mp_ht_iter_init(/* Str_Hash_Table_Iter* */ it, /* const Str_Hash_Table* */ ht)             \
    do {                                                                                           \
        (void) (it)->__mp_str_ht_iter_marker;                                                      \
        __mp_ht_iter_init((it), (ht));                                                             \
    } while (0)
void __mp_ht_iter_init(void *it, const void *ht);

/**
 * \brief Get the next element of \a it.
 *
 * To use hash table iterators, see \ref HashTableString.
 *
 * \param it (Str_Hash_Table_Iter *) iterator
 * \return (bool) whether it is valid to access the value
 */
#define /* bool */ mp_ht_iter_next(/* Str_Hash_Table_Iter* */ it)                                  \
    ((void) (it)->__mp_str_ht_iter_marker, __mp_ht_iter_next(it))
bool __mp_ht_iter_next(void *it);

// Hashes a string with FNV-1a hash algorithm.
uint64_t __mp_ht_hash_str(const mp_Str *str);

/**
 * \defgroup HashSetString Hash Set (String)
 *
 * Hash set with string key.
 * Represented by a \ref HashTableString "hash table (string key)" with opaque value.
 *
 * See \ref HashTableString for details about the actual representation.
 *
 * # Usage
 *
 * Initialize and deinitialize with \ref mp_hs_init and \ref mp_hs_deinit.
 * \code
 * mp_Str_Set set;
 * mp_hs_init(&set, mp_heap());
 * mp_hs_deinit(&set);
 * \endcode
 *
 * The primary usage of this is for setting keys. Use \ref mp_hs_set to set an element, do not use
 * \ref mp_ht_set.
 * \code
 * mp_hs_set(&set, "foo");
 * \endcode
 *
 * Getting the pointer to the value is a valid way to assess if the key spot is already occupied.
 * But dereferencing the pointer does not give meaningful result.
 * \code
 * void *v = mp_ht_get(&set, "foo");    // v is not NULL if "foo" exists, alternatively...
 * mp_ht_exists(&set, "foo");           // true if "foo" exists
 * \endcode
 *
 * Also, iterators can be constructed from hash sets.
 *
 * \{
 */

/**
 * \brief String hash set, essentially just a \ref HashTableString "hash table (string key)" with
 * dummy value.
 */
typedef struct __mp_Str_Set mp_Str_Set;

__mp_ht_struct(__mp_Str_Ht_Entry, __mp_Str_Set);

/**
 * \brief Initializes \a hs managed by \a alloc.
 *
 * Deinit with \ref mp_hs_deinit.
 *
 * \a hs->data == NULL if allocation failed.
 *
 * \param hs (\ref mp_Str_Set *) hash set (initialized by this)
 * \param alloc (mp_Alloc) allocator to manage \a hs
 */
#define mp_hs_init(/* mp_Str_Set* */ hs, /* mp_Alloc */ alloc)                                     \
    do {                                                                                           \
        (void) (hs)->__mp_str_ht_marker;                                                           \
        __mp_ht_init((hs), (alloc), sizeof(*((mp_Str_Set *) 0)->data), 0);                         \
    } while (0)

/**
 * \brief Deinitializes \a hs.
 *
 * \param hs (\ref mp_Str_Set *) hash set (deinitialized by this)
 */
#define mp_hs_deinit(/* mp_Str_Set* */ hs)                                                         \
    do {                                                                                           \
        (void) (hs)->__mp_str_ht_marker;                                                           \
        __mp_ht_deinit(hs);                                                                        \
    } while (0)

/**
 * \brief Sets the key \a k.
 *
 * When the key has not been set, \a k is cloned.
 *
 * \a hs->data == NULL if allocation failed.
 *
 * \param hs (\ref mp_Str_Set *) hash set
 * \param k (const char *) key
 */
#define mp_hs_set(/* mp_Str_Set* */ hs, /* const char* */ k) mp_hs_set_s((hs), mp_str(k))

/**
 * \brief Same as \ref mp_hs_set but accepts \ref mp_Str.
 *
 * See \ref mp_hs_set.
 *
 * \param hs (\ref mp_Str_Set *) hash set
 * \param k (\ref mp_Str) key
 */
#define mp_hs_set_s(/* mp_Str_Set* */ hs, /* mp_Str */ k)                                          \
    do {                                                                                           \
        (void) (hs)->__mp_str_ht_marker;                                                           \
        __mp_ht_set((hs), (k), NULL);                                                              \
    } while (0)

/**
 * \brief Iterator for \ref HashSetString "hash sets (string key)".
 */
typedef struct __mp_Str_Set_Iter mp_Str_Set_Iter;

struct __mp_Str_Set_Iter {
    const __mp_Hash_Table *_h;
    size_t                 _i;
    mp_Str                 key;
    void                  *val;
    char                   __mp_str_ht_iter_marker[];
};

/// \}

/// \}

/***********
 * $ HASH TABLE (INTEGER KEY)
 ***********/

/**
 * \defgroup HashTableInt Hash Table (Integer Key)
 *
 * Hash table with integer key.
 * The keys are stored as size_t, but any type that can be coerced to size_t should work.
 *
 * Throughout the documentation, a generic hash table (integer key) type is written as \a
 * Int_Hash_Table. Similarly, its iterator is written as \a Int_Hash_Table_Iter.
 *
 * # Usage
 *
 * Becase C does not have generics, to store data of a certain type you must define the hash table
 * type yourself. Luckily there is a macro that does this job.
 * \code
 * mp_hti_typedef(int, Ht_Int);
 * \endcode
 *
 * Declare a hash table then use \ref mp_hti_init and pass an allocator to manage the hash table.
 * This does not allocate the data immediately. But only once you set something on the hash table.
 * \code
 * Ht_Int ht;
 * mp_hti_init(Ht_Int, &ht, alloc);
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
 * Using \ref mp_hti_typedef also defines an iterator type for that hash table type, named by
 * suffixing `_Iter` to the hash table's type name.
 * Example usage:
 * \code
 * Ht_Int_Iter it;
 * mp_hti_iter_init(&it, &ht);
 * while (mp_hti_iter_next(&it)) {
 *     (void) it.key;
 *     (void) it.val;
 * }
 * \endcode
 *
 * It is best to not modify the hash table in the middle of iteration.
 *
 * # Layout
 *
 * ## Hash Table
 *
 * \code
 * struct {
 *     mp_Alloc    alloc;
 *     size_t      len;
 *     size_t      cap;
 *     size_t      size;
 *     Entry_Type *data;
 *     size_t      val_size;
 * };
 * \endcode
 *
 * **Fields**
 * - **alloc**: allocator that manages the allocation of the hash table
 * - **len**: amount of items in the hash table
 * - **cap**: size of the allocated block holding the data in bytes
 * - **size**: size of an entry in bytes
 * - **data**: pointer to the first entry (the data are continuous in memory)
 * - **val_size**: size of individual value (0 for hash sets)
 *
 * ## Entry
 *
 * \code
 * struct {
 *     Int_Key    key;
 *     Value_Type val;
 * };
 * \endcode
 *
 * **Fields**
 * - **key**: key indicating the entry
 * - **val**: value at the entry
 *
 * ## Key
 *
 * \code
 * struct {
 *     size_t key;
 *     bool   valid;
 * };
 * \endcode
 *
 * **Fields**
 * - **key**: key value
 * - **valid**: whether the key is valid
 *
 * ## Iterator
 *
 * \code
 * struct {
 *     const Int_Hash_Table *_h;
 *     size_t                _i;
 *     Int_Key               key;
 *     Value_Type           *val;
 * };
 * \endcode
 *
 * **Fields**
 * - **_h**: hash table being iterated on
 * - **_i**: index of iteration
 * - **key**: retrieved key
 * - **val** pointer to the retrieved value at \a key
 *
 * # Marker
 *
 * Integer hash tables contain a zero-sized field `__mp_int_ht_marker`. This field is used as a type
 * checking for function-like macros used for integer hash tables. These macros will check if this
 * field exists and if it does then the data should be a valid integer hash tables and thus can be
 * passed to the implementation function safely.
 * Integer hash table iterators also contain similar field `__mp_int_ht_iter_marker`.
 *
 * \{
 */

/**
 * \brief Defines an \ref HashTableInt "hash table (integer key)" struct with value of type \a
 * value_type.
 *
 * This also defines the hash table's iterator type, named by suffixing `Iter` after the
 * hash table's type name.
 *
 * \param value_type ("Type") value type name
 * \param name (identifier) hash table name
 */
#define mp_hti_typedef(/* "Type" */ value_type, /* identifier */ name)                             \
    typedef struct {                                                                               \
        __mp_Int_Ht_Key key;                                                                       \
        value_type      val;                                                                       \
    } __##name##_Entry;                                                                            \
    typedef struct {                                                                               \
        mp_Alloc          alloc;                                                                   \
        size_t            len;                                                                     \
        size_t            cap;                                                                     \
        size_t            size;                                                                    \
        __##name##_Entry *data;                                                                    \
        size_t            val_size;                                                                \
        char              __mp_int_ht_marker[];                                                    \
    } name;                                                                                        \
    typedef struct {                                                                               \
        const name     *_h;                                                                        \
        size_t          _i;                                                                        \
        __mp_Int_Ht_Key key;                                                                       \
        value_type     *val;                                                                       \
        char            __mp_int_ht_iter_marker[];                                                 \
    } name##_Iter

#define __mp_hti_struct(entry_type, name)                                                          \
    struct name {                                                                                  \
        mp_Alloc    alloc;                                                                         \
        size_t      len;                                                                           \
        size_t      cap;                                                                           \
        size_t      size;                                                                          \
        entry_type *data;                                                                          \
        size_t      val_size;                                                                      \
        char        __mp_int_ht_marker[];                                                          \
    }

// The key type is wrapped by this struct so it can have 0 as a key.
typedef struct {
    size_t key;
    bool   valid;
} __mp_Int_Ht_Key;

// Generic int hash table entry type.
typedef struct {
    __mp_Int_Ht_Key key;
    char            val[];
} __mp_Int_Ht_Entry;

// Generic string hash table iterator type.
typedef struct {
    const __mp_Hash_Table *_h;
    size_t                 _i;
    __mp_Int_Ht_Key        key;
    void                  *val;
} __mp_Int_Ht_Iter;

/**
 * \brief Initializes \a ht managed by \a alloc.
 *
 * Deinit with \ref mp_hti_deinit.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param type ("Type") hash table type name
 * \param ht (Int_Hash_Table *) hash table (initialized by this)
 * \param alloc (\ref mp_Alloc) allocator to manage \a ht
 */
#define mp_hti_init(/* "Type" */ type, /* Int_Hash_Table* */ ht, /* mp_Alloc */ alloc)             \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_ht_init((ht), (alloc), sizeof(*((type *) 0)->data),                                   \
                     sizeof((*((type *) 0)->data).val));                                           \
    } while (0)

/**
 * \brief Deinitializes \a ht.
 *
 * \param ht (Int_Hash_Table *) hash table (deinitialized by this)
 */
#define mp_hti_deinit(/* Int_Hash_Table* */ ht)                                                    \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_da_deinit(ht);                                                                        \
    } while (0)

/**
 * \brief Gets pointer to item at \a k.
 *
 * \param ht (const Int_Hash_Table *) hash table
 * \param k (size_t) key
 * \return (void *) retrieved value, NULL if cannot retrieve
 */
#define /* void* */ mp_hti_get(/* const Int_Hash_Table* */ ht, /* size_t */ k)                     \
    ((void) (ht)->__mp_int_ht_marker, __mp_hti_get((ht), (k)))
void *__mp_hti_get(const void *ht, size_t k);

/**
 * \brief Sets the value at \a k to \a v.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param ht (Int_Hash_Table *) hash table (no side effects)
 * \param k (size_t) key
 * \param v (Type) value to be stored
 */
#define mp_hti_set(/* Int_Hash_Table* */ ht, /* size_t */ k, /* Type */ v)                         \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __MP_TYPEOF(v) __it = (v);                                                                 \
        __MP_ASSERT_MSG((ht)->val_size > 0, "Did you mean to use `mp_hsi_set` instead?");          \
        __MP_ASSERT_MSG(sizeof(__it) == (ht)->val_size,                                            \
                        "The size of each item(s) provided does not match the array's item size"); \
        __mp_hti_set((ht), (k), &__it);                                                            \
    } while (0)
void __mp_hti_set(void *ht, size_t k, void *v);

/**
 * \brief Tests whether \a k exists in \a ht.
 *
 * \param ht (const Int_Hash_Table *) hash table
 * \param k (size_t) key
 * \return (bool) whether \a k exists in \a ht
 */
#define /* bool */ mp_hti_exists(/* const Int_Hash_Table* */ ht, /* size_t */ k)                   \
    ((void) (ht)->__mp_int_ht_marker, __mp_hti_exists((ht), (k)))
bool __mp_hti_exists(const void *ht, size_t k);

/**
 * \brief Grows \a ht to be able to hold \a offset more items from the current length.
 *
 * Does a calculation to determine the new capacity. The increase of the new capacity may not be
 * equal to \a offset.
 *
 * If \a ht->cap is 0, reserves for a certain number of items.
 *
 * If \a ht->cap is not large enough, reserves for double of \a ht->cap.
 *
 * \a ht->data == NULL if allocation failed.
 *
 * \param ht (Str_Hash_Table *) hash table
 * \param offset (size_t) amount to grow
 */
#define mp_hti_grow(/* Int_Hash_Table* */ ht, /* size_t */ offset)                                 \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_hti_grow((ht), (offset));                                                             \
    } while (0)
void __mp_hti_grow(void *ht, size_t offset);

/**
 * \brief Sets the length of \a ht to 0 and frees its keys.
 *
 * This resets the hash table to "initial condition" but without actually freeing the data.
 *
 * \param ht (Int_Hash_Table *) hash table
 */
#define mp_hti_reset(/* Int_Hash_Table* */ ht)                                                     \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_hti_reset(ht);                                                                        \
    } while (0)
void __mp_hti_reset(void *ht);

/**
 * \brief Deletes an entry at \a k from \a ht.
 *
 * This decreases \a ht->len but does not actually shrink the hash table, but it just
 * marks the entry as "deleted", which may be overridden by subsequent set operations.
 *
 * Does nothing if it cannot find \a k.
 *
 * \param ht (Int_Hash_Table *) hash table
 * \param k (size_t) key
 */
#define mp_hti_delete(/* Int_Hash_Table* */ ht, /* size_t */ k)                                    \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_hti_delete((ht), (k));                                                                \
    } while (0)
void __mp_hti_delete(void *ht, size_t k);

/**
 * \brief Clones \a src to \a dest managed by \a alloc.
 *
 * \a dest inherits all fields of \a src.
 * \a dest->data == NULL if allocation failed.
 *
 * \param dest (Str_Hash_Table *) destination of the clone (initialized by this)
 * \param src (const Str_Hash_Table *) source hash table
 * \param alloc (\ref mp_Alloc) allocator to manage \a dest
 */
#define mp_hti_clone(/* Int_Hash_Table* */ dest, /* const Int_Hash_Table* */ src,                  \
                     /* mp_Alloc */ alloc)                                                         \
    do {                                                                                           \
        (void) (dest)->__mp_int_ht_marker;                                                         \
        (void) (src)->__mp_int_ht_marker;                                                          \
        __mp_hti_clone((dest), (src), (alloc));                                                    \
    } while (0)
void __mp_hti_clone(void *dest, const void *src, mp_Alloc alloc);

/**
 * \brief A \ref DynamicArray "dynamic array" of size_t to hold the keys of \ref
 * HashTableInt "hash tables (integer key)".
 */
typedef struct __mp_Hti_Keys mp_Hti_Keys;

__mp_da_struct(size_t, __mp_Hti_Keys);

/**
 * \brief Extracts the keys from \a ht to \a keys.
 *
 * \a keys **must be initialized** with \ref mp_da_init first.
 *
 * Deinit \a keys with \ref mp_da_deinit.
 *
 * \a keys->data == NULL if allocation failed.
 *
 * # Note
 * This function allocates a temporary bit of memory using \a ht->alloc.
 *
 * \param ht (const Int_Hash_Table *) hash table
 * \param keys (\ref mp_Hti_Keys *) destination of keys (must be initialized first)
 */
#define mp_hti_keys(/* const IntHashTable* */ ht, /* mp_Hti_Keys* */ keys)                         \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_hti_keys((ht), (keys));                                                               \
    } while (0)
void __mp_hti_keys(const void *ht, mp_Hti_Keys *keys);

/**
 * \brief Extracts the values from \a ht to \a values.
 *
 * \a values is a \ref DynamicArray "dynamic array" and its data type **must match** the
 * value type of \a ht.
 *
 * \a values **must be initialized** with \ref mp_da_init first.
 *
 * \a values->data == NULL if allocation failed.
 *
 * # Note
 * This function allocates a temporary bit of memory using \a ht->alloc.
 *
 * \param ht (const Int_Hash_Table *) hash table
 * \param values (Dyn_Array *) destination of values (must be initialized first)
 */
#define mp_hti_values(/* const Int_Hash_Table* */ ht, /* Dyn_Array* */ values)                     \
    do {                                                                                           \
        (void) (ht)->__mp_int_ht_marker;                                                           \
        __mp_hti_values((ht), (values));                                                           \
    } while (0)
void __mp_hti_values(const void *ht, void *values);

/**
 * \brief Initializes \a it to iterate on \a ht.
 *
 * To use hash table iterators, see \ref HashTableInt.
 *
 * \param it (Int_Hash_Table_Iter *) iterator (initialized by this)
 * \param ht (const Int_Hash_Table *) hash table to iterate
 */
#define mp_hti_iter_init(/* Int_Hash_Table_Iter* */ it, /* const Int_Hash_Table* */ ht)            \
    do {                                                                                           \
        (void) (it)->__mp_int_ht_iter_marker;                                                      \
        __mp_hti_iter_init((it), (ht));                                                            \
    } while (0)
void __mp_hti_iter_init(void *it, const void *ht);

/**
 * \brief Get the next element of \a it.
 *
 * To use hash table iterators, see \ref HashTableInt.
 *
 * \param it (Int_Hash_Table_Iter *) iterator
 * \return (bool) whether it is valid to access the value
 */
#define /* bool */ mp_hti_iter_next(/* Int_Hash_Table_Iter* */ it)                                 \
    ((void) (it)->__mp_int_ht_iter_marker, __mp_hti_iter_next(it))
bool __mp_hti_iter_next(void *it);

/** \defgroup HashSetInt Hash Set (Integer)
 *
 * Hash set with integer key.
 * Represented by a \ref HashTableInt "hash table (integer key)" with opaque value.
 *
 * See \ref HashTableInt for details about the actual representation.
 *
 * # Usage
 *
 * Initialize and deinitialize with \ref mp_hsi_init and \ref mp_hsi_deinit.
 * \code
 * mp_Int_Set set;
 * mp_hsi_init(&set, mp_heap());
 * mp_hsi_deinit(&set);
 * \endcode
 *
 * The primary usage of this is for setting keys. Use \ref mp_hsi_set to set an element, do not use
 * \ref mp_hti_set.
 * \code
 * mp_hsi_set(&set, 0);
 * \endcode
 *
 * Getting the pointer to the value is a valid way to assess if the key spot is already occupied.
 * But dereferencing the pointer does not give meaningful result.
 * \code
 * void *v = mp_hti_get(&set, 0);   // v is not NULL if key 0 exists, alternatively...
 * mp_hti_exists(&set, 0);      // true if key 0 exists
 * \endcode
 *
 * Also, iterators can be constructed from hash sets.
 *
 * \{
 */

/**
 * \brief Integer hash set, essentially just a \ref HashTableInt "hash table (integer key)" with
 * dummy value.
 */
typedef struct __mp_Int_Set mp_Int_Set;

__mp_hti_struct(__mp_Int_Ht_Entry, __mp_Int_Set);

/**
 * \brief Initializes \a hs managed by \a alloc.
 *
 * Deinit with \ref mp_hsi_deinit.
 *
 * \a hs->data == NULL if allocation failed.
 *
 * \param hs (\ref mp_Int_Set *) hash set (initialized by this)
 * \param alloc (mp_Alloc) allocator to manage \a hs
 */
#define mp_hsi_init(/* mp_Int_Set* */ hs, /* mp_Alloc */ alloc)                                    \
    do {                                                                                           \
        (void) (hs)->__mp_int_ht_marker;                                                           \
        __mp_ht_init((hs), (alloc), sizeof(*((mp_Int_Set *) 0)->data), 0);                         \
    } while (0)

/**
 * \brief Deinitializes \a hs.
 *
 * \param hs (\ref mp_Int_Set *) hash set (deinitialized by this)
 */
#define mp_hsi_deinit(/* mp_Int_Set* */ hs)                                                        \
    do {                                                                                           \
        (void) (hs)->__mp_int_ht_marker;                                                           \
        __mp_ht_deinit(hs);                                                                        \
    } while (0)

/**
 * \brief Sets the key \a k.
 *
 * \a hs->data == NULL if allocation failed.
 *
 * \param hs (\ref mp_Int_Set *) hash set
 * \param k (size_t) key
 */
#define mp_hsi_set(/* mp_Int_Set* */ hs, /* size_t */ k)                                           \
    do {                                                                                           \
        (void) (hs)->__mp_int_ht_marker;                                                           \
        __mp_hti_set((hs), (k), NULL);                                                             \
    } while (0)

/**
 * \brief Iterator for \ref HashSetInt "hash sets (integer key)".
 */
typedef struct __mp_Int_Set_Iter mp_Int_Set_Iter;

struct __mp_Int_Set_Iter {
    const __mp_Hash_Table *_h;
    size_t                 _i;
    __mp_Int_Ht_Key        key;
    void                  *val;
    char                   __mp_int_ht_iter_marker[];
};

/// \}

/// \}

/***********
 * $ ALLOCATORS
 ***********/

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

/*
 * Default size of a single region in bytes.
 *
 * The value will be aligned to the nearest multiple of `sizeof(uintptr_t)`.
 */
#ifndef __MP_REGION_DEFAULT_SIZE
    #define __MP_REGION_DEFAULT_SIZE (64 * 1024)
#endif

/**
 * \brief Forward declaration of \ref mp_Region.
 */
typedef struct mp_Region mp_Region;

/**
 * \brief Linked list element that holds certain size of allocated memory managed by \ref mp_Arena.
 */
struct mp_Region {
    /// Next region in linked list if any.
    mp_Region *next;
    /// Amount of data used in bytes.
    size_t len;
    /// Amount of data allocated in bytes.
    size_t cap;
    /// Data aligned to `sizeof(uintptr_t)`.
    uintptr_t data[];
};

/**
 * \brief Allocates a region with \a cap bytes of size using \a alloc.
 *
 * \a cap will be **rounded up** to the nearest multiple of `sizeof(uintptr_t)`.
 *
 * Deinit with \ref mp_region_deinit.
 *
 * \param alloc backing allocator
 * \param cap how many bytes to allocate
 * \return pointer to the allocated region
 */
mp_Region *mp_region_new(mp_Alloc alloc, size_t cap);

/**
 * \brief Deinitializes a region.
 *
 * \param r region (deinitialized by this)
 * \param alloc backing allocator of \a r
 */
void mp_region_deinit(mp_Region *r, mp_Alloc alloc);

/**
 * \brief The internal context of growing arena allocators, manages regions in a linked list.
 */
typedef struct {
    /// First element of the region linked list.
    mp_Region *begin;
    /// Last element of the region linked list.
    mp_Region *end;
    /// Amount of data used in bytes, aligned to `sizeof(uintptr_t)`.
    size_t len;
    /// Backing allocator, allocator used to allocate the regions.
    mp_Alloc alloc;
    /// Default size of regions allocated by this arena in bytes.
    size_t _def_size;
} mp_Arena;

/**
 * \brief Initializes \a a using \a alloc as the backing allocator.
 *
 * The arena will not allocate anything until the first operation that allocates.
 *
 * Deinit with \ref mp_arena_deinit.
 *
 * \param a (\ref mp_Arena *) arena (initialized by this)
 * \param alloc (\ref mp_Alloc) backing allocator
 */
#define mp_arena_init(/* mp_Arena* */ a, /* mp_Alloc */ alloc)                                     \
    mp_arena_init_s((a), (alloc), __MP_REGION_DEFAULT_SIZE)

/**
 * \brief Same as \ref mp_arena_init but accepts default size for regions.
 *
 * See \ref mp_arena_init.
 *
 * Regions will be allocated with \a def_size bytes of size.
 *
 * \param a arena (initialized by this)
 * \param alloc backing allocator
 * \param def_size size of regions in bytes
 */
void mp_arena_init_s(mp_Arena *a, mp_Alloc alloc, size_t def_size);

/**
 * \brief Sets the length of \a a to 0, but does not free allocated regions.
 *
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param a arena
 */
void mp_arena_reset(mp_Arena *a);

/**
 * \brief Deinitializes \a a and its regions.
 *
 * The freeing will be performed using the arena's backing allocator.
 *
 * \param a arena (deinitialized by this)
 */
void mp_arena_deinit(mp_Arena *a);

/**
 * \brief Returns an allocator that works with \a a.
 *
 * \param a arena
 * \return allocator interface
 */
mp_Alloc mp_arena_alloc(mp_Arena *a);

/**
 * \brief Gets the position after the last element.
 *
 * Used in conjunction with \ref mp_arena_rewind.
 *
 * \param a arena
 * \return pointer (as number) to the end of the last element
 */
uintptr_t mp_arena_mark(const mp_Arena *a);

/**
 * \brief Sets the pointer to the end of the last element to \a mark.
 *
 * \a mark can be obtained via \ref mp_arena_mark.
 *
 * \param a arena
 * \param mark marker (pointer to the end of the last element)
 */
void mp_arena_rewind(mp_Arena *a, uintptr_t mark);

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
 * mp_Sarena arena;
 * mp_sarena_init(&arena, mp_heap_alloc(), 1024);
 * mp_Alloc alloc = mp_sarena_alloc(&arena);
 * void *ptr = mp_alloc(alloc, 10);
 * \endcode
 *
 * \{
 */

/**
 * \brief The internal context of static arena allocators.
 */
typedef struct {
    /// Arena buffer of size \a cap.
    uintptr_t *buf;
    /// Amount of data in bytes used, aligned to `sizeof(uintptr_t)`.
    size_t len;
    /// Amount of data in bytes allocated, aligned to `sizeof(uintptr_t)`.
    size_t cap;
    /// Backing allocator, allocator used to allocate \a buf.
    mp_Alloc alloc;
} mp_Sarena;

/**
 * \brief Initializes \a a and allocates the buffer of size \a cap bytes.
 *
 * \a cap will be **rounded up** to the nearest multiple of `sizeof(uintptr_t)`.
 *
 * The arena's buffer is immediately allocated.
 *
 * \a a->buf == NULL if allocation failed.
 *
 * Deinit with \ref mp_sarena_deinit.
 *
 * \param a arena (initialized by this)
 * \param alloc backing allocator
 * \param cap how many bytes to allocate
 */
void mp_sarena_init(mp_Sarena *a, mp_Alloc alloc, size_t cap);

/**
 * \brief Sets the length of \a a to 0, but does not free the allocated buffer.
 *
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param a arena
 */
void mp_sarena_reset(mp_Sarena *a);

/**
 * \brief Deinitializes \a a and its buffer.
 *
 * The free will be performed using the arena's backing allocator.
 *
 * \param a arena (deinitialized by this)
 */
void mp_sarena_deinit(mp_Sarena *a);

/**
 * \brief Returns an allocator that works with \a a.
 *
 * \param a arena
 * \return allocator interface, \ref mp_alloc_invalid "invalid allocator" if \a a->buf is NULL
 */
mp_Alloc mp_sarena_alloc(mp_Sarena *a);

/**
 * \brief Gets the position after the last element.
 *
 * Used in conjunction with \ref mp_sarena_rewind.
 *
 * \param a arena
 * \return pointer (as number) to the end of the last element
 */
uintptr_t mp_sarena_mark(const mp_Sarena *a);

/**
 * \brief Sets the pointer to the end of the last element to \a mark.
 *
 * \a mark can be obtained via \ref mp_sarena_mark.
 *
 * \param a arena
 * \param mark marker (pointer to the end of the last element)
 */
void mp_sarena_rewind(mp_Sarena *a, uintptr_t mark);

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
 * Call \ref mp_talloc macro (or \ref mp_talloc_s) to define and initialize a temporary allocator
 * for this scope. The allocator will be accessible as `temp_alloc`.
 *
 * \code
 * mp_talloc();
 * void *ptr = mp_alloc(temp_alloc, 16);
 * \endcode
 *
 * \{
 */

/**
 * \brief The internal context of temp allocators.
 */
typedef struct {
    /// Arena buffer of size \a cap bytes.
    uintptr_t *buf;
    /// Amount of data in bytes used, aligned to `sizeof(uintptr_t)`.
    size_t len;
    /// Amount of data in bytes allocated, aligned to `sizeof(uintptr_t)`.
    size_t cap;
} mp_Temp;

/**
 * \brief Shortcut for defining and initializing a temp allocator.
 *
 * Will initialize a \ref mp_Temp "temp allocator" 1024 bytes in size.
 *
 * Defines `temp_alloc` variable for the current scope.
 */
#define mp_talloc() mp_talloc_s(1024)

/**
 * \brief Shortcut for defining and initializing a temp allocator \a size bytes in size.
 *
 * Will initialize a \ref mp_Temp "temp allocator" \a size bytes in size.
 *
 * Defines `temp_alloc` variable for the current scope.
 *
 * \param size (size_t) size of buffer in bytes
 */
#define mp_talloc_s(/* size_t */ size)                                                             \
    char    __mp_tempbuf[(size)];                                                                  \
    mp_Temp __mp_temp;                                                                             \
    mp_temp_init(&__mp_temp, __mp_tempbuf, (size));                                                \
    mp_Alloc temp_alloc = mp_temp_alloc(&__mp_temp);

/**
 * \brief Initializes \a t with \a buf of size \a cap bytes.
 *
 * \a cap should be an multiple of `sizeof(uintptr_t)`.
 * If not, the actual \a cap will **round up** to the nearest multiple.
 *
 * \param t temp arena (initialized by this)
 * \param buf buffer
 * \param cap size of \a buf in bytes
 */
void mp_temp_init(mp_Temp *t, char *buf, size_t cap);

/**
 * \brief Sets the length of \a t to 0, but does not deinitialize the buffer.
 *
 * This resets the arena to "initial condition" but without actually freeing the data.
 *
 * \param t temp arena
 */
void mp_temp_reset(mp_Temp *t);

/**
 * \brief Returns an allocator that works with \a t.
 *
 * \param t temp arena
 * \return allocator interface, \ref mp_alloc_invalid "invalid allocator" if \a a->buf is NULL
 */
mp_Alloc mp_temp_alloc(mp_Temp *t);

/**
 * \brief Gets the position after the last element.
 *
 * Used in conjunction with \ref mp_temp_rewind.
 *
 * \param t temp arena
 * \return pointer (as number) to the end of the last element
 */
uintptr_t mp_temp_mark(const mp_Temp *t);

/**
 * \brief Sets the pointer to the end of the last element to \a mark.
 *
 * \a mark can be obtained via \ref mp_temp_mark.
 *
 * \param t temp arena
 * \param mark marker (pointer to the end of the last element)
 */
void mp_temp_rewind(mp_Temp *t, uintptr_t mark);

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

/**
 * \brief Alias to \ref mp_heap_alloc.
 */
#define mp_heap() mp_heap_alloc()

/**
 * \brief Returns an allocator that works with the heap.
 *
 * \return allocator interface
 */
mp_Alloc mp_heap_alloc(void);

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

/**
 * \brief Value for invalid Unicode codepoint.
 */
#define MP_UTF8_INVALID_CODEPOINT ((uint32_t) -1)

/**
 * \brief Stores a pointer to a character and its UTF-8 metadata.
 */
typedef struct {
    /// Size of the character in bytes, at most 4 bytes.
    uint8_t size;
    /// Unicode codepoint of the character (\ref MP_UTF8_INVALID_CODEPOINT when invalid).
    uint32_t codepoint;
    /// Pointer to the start of the character.
    const char *c;
} mp_Utf8_Char_Data;

/**
 * \brief Returns an invalid \ref mp_Utf8_Char_Data.
 *
 * An invalid \ref mp_Utf8_Char_Data requires that field \a codepoint is \ref
 * MP_UTF8_INVALID_CODEPOINT.
 *
 * An invalid \ref mp_Utf8_Char_Data only means that it is not a valid UTF-8 character, but the
 * underlying bytes may still exist and are accessible. Invalid does not always mean inacessible, so
 * `c` and `size` may still be usable.
 *
 * \param ch (const char *) start of the error character (nullable)
 * \param sz (uint8_t) size of the error character (should be zero if \a ch is NULL)
 * \return (\ref mp_Utf8_Char_Data) invalid character
 */
#define /* mp_Utf8_Char_Data */ mp_utf8_char_invalid(/* const char* */ ch, /* uint8_t */ sz)       \
    ((mp_Utf8_Char_Data) {                                                                         \
        .size      = (sz),                                                                         \
        .codepoint = (uint32_t) -1,                                                                \
        .c         = (ch),                                                                         \
    })

/**
 * \brief Shortcut for printing a \ref mp_Utf8_Char_Data, use with `%.*s` format specifier.
 *
 * \param ch (\ref mp_Utf8_Char_Data) character data
 */
#define mp_utf8_char_print(/* mp_Utf8_Char_Data */ ch) (ch).size, (ch).c

/**
 * \brief Tests whether \a ch is a valid UTF-8 character.
 *
 * An invalid \ref mp_Utf8_Char_Data requires that field \a codepoint is the maximum possible value
 * of `uint32_t`.
 *
 * The bytes may still be accessible, test with \ref mp_utf8_char_is_accessible.
 *
 * \param ch (\ref mp_Utf8_Char_Data) character data
 * \return (bool) whether \a ch is a valid UTF-8 character.
 */
#define /* bool */ mp_utf8_char_is_valid(/* mp_Utf8_Char_Data */ ch)                               \
    ((ch).codepoint != (uint32_t) -1)

/**
 * \brief Tests whether an \a ch is accessible.
 *
 * \param ch (\ref mp_Utf8_Char_Data) character data
 * \return (bool) whether \a ch is accessible
 */
#define /* bool */ mp_utf8_char_is_accessible(/* mp_Utf8_Char_Data */ ch) ((ch).c != NULL)

/**
 * \brief Takes the first valid UTF-8 character or an error character from a string.
 *
 * After calling this function, \a str will point to the character after the retrieved character and
 * \a size is decremented according to the size of the retrieved character.
 *
 * The returned character may be a valid UTF-8 character or an invalid error character. Check with
 * \ref mp_utf8_char_is_valid.
 *
 * When an error occurs, it will return the error character. The error character may be more than
 * one bytes if the first character expects that some bytes follow it, but the string ends before it
 * gets the required amount of bytes, or one of the bytes is not a valid byte. For more details see
 * <https://en.wikipedia.org/wiki/UTF-8#Error_handling>.
 *
 * \param str pointer to string
 * \param size pointer to size of \a str in bytes
 * \return metadata of retrieved character
 */
mp_Utf8_Char_Data mp_utf8_take(const char **str, size_t *size);

/**
 * \brief Gets a \ref mp_Utf8_Char_Data "character data" from \a c (null-terminated).
 *
 * This function will only read the first UTF-8 character from \a c.
 *
 * \param c null-terminated string
 * \return character data, \ref mp_utf8_char_invalid "invalid character data" if \a c is not a valid
 * UTF-8 character.
 */
mp_Utf8_Char_Data mp_utf8_char(const char *c);

/**
 * \brief Gets a \ref mp_Utf8_Char_Data "character data" from \a c.
 *
 * See \ref mp_utf8_char.
 *
 * \param c string
 * \param size size of \a str in bytes
 * \return character data, \ref mp_utf8_char_invalid "invalid character data" if \a c is not a valid
 * UTF-8 character.
 */
mp_Utf8_Char_Data mp_utf8_char_s(const char *c, size_t size);

/**
 * \brief Calculates the amount of Unicode characters in \a str (null-terminated).
 *
 * This operation is O(n).
 *
 * Use \ref mp_utf8_len_s for non-null-terminated strings.
 *
 * \param str null-terminated string
 * \return amount of Unicode characters in \a str
 */
size_t mp_utf8_len(const char *str);

/**
 * \brief Calculates the amount of Unicode characters in \a str.
 *
 * See \ref mp_utf8_len.
 *
 * \param str string
 * \param size size of \a str in bytes
 * \return amount of Unicode characters in \a str
 */
size_t mp_utf8_len_s(const char *str, size_t size);

/**
 * \brief Gets a UTF-8 character from \a str (null-terminated) at \a index.
 *
 * This operation is O(n).
 *
 * \param str null-terminated string
 * \param index index
 * \return character data at \a index, \ref mp_utf8_char_invalid "invalid character data" when out
 * of bounds.
 */
mp_Utf8_Char_Data mp_utf8_get(const char *str, size_t index);

/**
 * \brief Gets a UTF-8 character from \a str at \a index.
 *
 * See \ref mp_utf8_get.
 *
 * \param str string
 * \param size size of \a str in bytes
 * \param index index
 * \return character data at \a index, \ref mp_utf8_char_invalid "invalid character data" when out
 * of bounds.
 */
mp_Utf8_Char_Data mp_utf8_get_s(const char *str, size_t size, size_t index);

/**
 * \brief Iterator for UTF-8 strings.
 *
 * \a c can be accessed to get the current character's information.
 *
 * # Usage
 *
 * \code
 * const char *utf8 = "魈くんは大好きです　⸜(｡˃ ᵕ ˂)⸝♡􏾀";
 * mp_Utf8_Iter iter = mp_utf8_iter_new(utf8);
 * while (mp_utf8_iter_next(&iter)) {
 *     (void) iter.c;      // The current character (mp_Utf8_Char_Data)
 * }
 * \endcode
 *
 * It is best to not modify the string in the middle of iteration.
 */
typedef struct {
    /// Holds the current character in iteration.
    mp_Utf8_Char_Data c;

    /// The string being iterated on.
    const char *_str;
    /// Remaining size of the string in bytes.
    size_t _size;
} mp_Utf8_Iter;

/**
 * \brief Creates a \ref mp_Utf8_Iter "UTF-8 iterator" that iterates over \a str (null-terminated).
 *
 * Use \ref mp_utf8_iter_new_s for non-null-terminated strings.
 *
 * See \ref mp_Utf8_Iter for usage.
 *
 * \param str null-terminated UTF-8 string
 * \return iterator over \a str
 */
mp_Utf8_Iter mp_utf8_iter_new(const char *str);

/**
 * \brief Creates a \ref mp_Utf8_Iter "UTF-8 iterator" that iterates over \a str.
 *
 * Use \ref mp_utf8_iter_new for null-terminated strings.
 *
 * See \ref mp_Utf8_Iter for usage.
 *
 * \param str UTF-8 string
 * \param size size of \a str in bytes
 * \return iterator over \a str
 */
mp_Utf8_Iter mp_utf8_iter_new_s(const char *str, size_t size);

/**
 * \brief Continues iterating with \a it.
 *
 * See \ref mp_Utf8_Iter for usage.
 *
 * This function consumes \a it->_str.
 *
 * \param it iterator
 * \return whether it is valid to access the value
 */
bool mp_utf8_iter_next(mp_Utf8_Iter *it);

/// \}

/***********
 * $ ERRORS
 ***********/

/**
 * \defgroup Errors Errors
 *
 * \{
 */

// Don't forget `mp_err()` and `mp_err_str()`!
// Sort this!
/**
 * Errors from errno. The available error varies by operating systems.
 *
 * Error names for POSIX & Linux are taken from Linux manpage
 * [errno(3)](https://man7.org/linux/man-pages/man3/errno.3.html).
 *
 * Error names for Windows are taken from
 * <https://learn.microsoft.com/en-us/cpp/c-runtime-library/errno-constants>.
 *
 * For error messages see the definition of \ref mp_err_str.
 */
typedef enum {
    MP_ERR_NONE    = 0,
    MP_ERR_UNKNOWN = 1,

    // Errors thrown by memplus instead of from errno
    MP_ERR_CANNOT_ALLOC,

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

/**
 * \brief Converts errno into \ref mp_Err.
 *
 * \param errnum errno number
 * \return \ref mp_Err representation of the errno
 */
mp_Err mp_err(int errnum);

/**
 * \brief Returns the description of \a e.
 *
 * \param e error
 * \return description \a e
 */
const char *mp_err_str(mp_Err e);

/// \}

/***********
 * $ FILESYSTEM
 ***********/

/**
 * \defgroup Filesystem Filesystem
 *
 * Miscellaneous functions that works with the filesystem.
 *
 * \{
 */

// MAYBE: mp_Path?

// TODO: File functions
// - File iterator (custom separator)
// - mp_file_copy
// - mp_stat, mp_exists

// TODO: Directory functions
// - mp_dir_create
// - mp_dir_delete_recursive
// - mp_dir_copy_recursive
// - Directory iterator

/// Reads the contents of a file at \a file_path to \a out_str.
/**
 * Deinit with \ref mp_str_deinit.
 *
 * \param[out] out_str The contents of the file (initialized by this)
 * \param file_path The path to the file
 * \param alloc The allocator allocating \a out_str
 * \return The error if occurs, MP_ERR_NONE if successful
 */
mp_Err mp_file_read(mp_Str *out_str, const char *file_path, mp_Alloc alloc);

/// Writes data into a file at \a file_path.
/**
 * \param file_path The path to the file
 * \param data The data
 * \param data_size The size of \a data (in bytes)
 * \param append Whether to write from the end (append) or from the beginning (truncate)
 * \return The error if occurs, MP_ERR_NONE if successful
 */
mp_Err mp_file_write(const char *file_path, const char *data, size_t data_size, bool append);

/// Creates a file at \a file_path.
/**
 * The created file will be readable and writeable if permitted.
 *
 * \param file_path The path to the file
 * \return The error if occurs, MP_ERR_NONE if successful
 */
mp_Err mp_file_create(const char *file_path);

/// Deletes a file at \a file_path.
/**
 * Also works on symbolic links and empty directory.
 *
 * \param file_path The path to the file
 * \return The error if occurs, MP_ERR_NONE if successful
 */
mp_Err mp_file_delete(const char *file_path);

/// \}

/***********
 * $ IMPLEMENTATION
 ***********/

#ifdef MEMPLUS_IMPLEMENTATION

    #include <errno.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <string.h>

    #define __MP_UNREACHABLE()      __MP_ASSERT_MSG(0, "Unreachable")
    #define __MP_TODO(msg)          __MP_ASSERT_MSG(0, "todo: " msg)
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
                __MP_ASSERT_MSG(0, "Memory overlaps");                                             \
            }                                                                                      \
        } while (0)

static void *mp_arena_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                 void *ptr);
static void *mp_sarena_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                  void *ptr);
static void *mp_heap_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                void *ptr);

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
    __mp_Dyn_Array *self = a;
    __MP_ZERO(self);
    self->alloc = alloc;
    self->len   = 0;
    self->cap   = 0;
    self->size  = size;
    self->data  = NULL;
}

void __mp_da_deinit(void *a) {
    __mp_Dyn_Array *self = a;
    mp_free(self->alloc, self->data, self->cap * self->size);
    __MP_ZERO(self);
}

void __mp_da_append(void *a, const void *items, size_t items_len) {
    __mp_Dyn_Array *self = a;
    __mp_da_grow(self, items_len);
    if (self->data != NULL) {
        memcpy((char *) self->data + self->len * self->size, items, items_len * self->size);
        self->len += items_len;
    }
}

void __mp_da_grow(void *a, size_t offset) {
    __mp_Dyn_Array *self = a;
    if (self->len + offset > self->cap && offset > 0) {
        size_t new_cap = self->cap;
        if (new_cap == 0) {
            new_cap = __MP_DARRAY_INIT_CAPACITY;
        }
        while (self->len + offset > new_cap) {
            new_cap *= 2;
        }
        __mp_da_reserve(self, new_cap - self->cap);
    }
}

void __mp_da_reserve(void *a, size_t offset) {
    if (offset == 0) {
        return;
    }
    __mp_Dyn_Array *self    = a;
    size_t          new_cap = self->cap + offset;
    self->data = mp_realloc(self->alloc, self->data, self->cap * self->size, new_cap * self->size);
    if (self->data != NULL) {
        self->cap = new_cap;
    }
}

void __mp_da_clone(void *dest, const void *src, mp_Alloc alloc) {
    const __mp_Dyn_Array *s = src;
    __mp_Dyn_Array       *d = dest;
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
    __mp_Dyn_Array *self       = a;
    size_t          actual_pos = (pos > self->len) ? self->len : pos;
    __mp_da_grow(self, items_len);
    if (self->data != NULL) {
        memmove((char *) self->data + (actual_pos + items_len) * self->size,
                (char *) self->data + actual_pos * self->size, (items_len + 1) * self->size);
        memcpy((char *) self->data + actual_pos * self->size, items, items_len * self->size);
        self->len += items_len;
    }
}

void __mp_da_move(void *a, size_t pos, void *ret_items, size_t items_len) {
    __mp_Dyn_Array *self = a;
    __MP_BOUNDS_CHECK(pos, self->len);
    size_t moved = self->len - (pos + items_len);
    if (ret_items != NULL) {
        memcpy(ret_items, (char *) self->data + pos * self->size, items_len * self->size);
    }
    if (moved >= 1) {
        memmove((char *) self->data + pos * self->size,
                (char *) self->data + (pos + items_len) * self->size, moved * self->size);
        self->len -= items_len;
    }
}

void __mp_da_quick_move(void *a, size_t pos, void *ret_item) {
    __mp_Dyn_Array *self = a;
    __MP_BOUNDS_CHECK(pos, self->len);
    if (ret_item != NULL) {
        memcpy(ret_item, (char *) self->data + pos * self->size, self->size);
    }
    if (pos != self->len - 1) {
        memcpy((char *) self->data + pos * self->size,
               (char *) self->data + (self->len - 1) * self->size, self->size);
    }
    --self->len;
}

mp_Str mp_str_alloc(const char *str, mp_Alloc alloc) {
    size_t      len  = strlen(str);
    const char *data = mp_dup(alloc, str, len);
    if (data == NULL) {
        return mp_str_invalid();
    }
    return mp_str_s(data, len);
}

mp_Str mp_str_clone(mp_Str str, mp_Alloc alloc) {
    const char *data = mp_dup(alloc, str.data, str.len);
    if (data == NULL) {
        return mp_str_invalid();
    }
    return mp_str_s(data, str.len);
}

void mp_str_deinit(mp_Str *str, mp_Alloc alloc) {
    mp_free(alloc, (void *) str->data, str->len);
    __MP_ZERO(str);
}

char *mp_str_null_terminated_from(mp_Str str, mp_Alloc alloc) {
    char *cstr = mp_alloc(alloc, str.len + 1);
    if (cstr == NULL) {
        return NULL;
    }
    memcpy(cstr, str.data, str.len);
    cstr[str.len] = '\0';
    return cstr;
}

void mp_str_null_terminated_deinit(char **str, mp_Alloc alloc) {
    mp_free(alloc, *str, strlen(*str));
    __MP_ZERO(str);
}

bool mp_str_eq(mp_Str a, mp_Str b) {
    if (a.len != b.len) {
        return false;
    }
    return memcmp(a.data, b.data, a.len) == 0;
}

void mp_sb_init(mp_Sb *sb, mp_Alloc alloc) {
    mp_da_init(mp_Sb, sb, alloc);
}

void mp_sb_init_with(mp_Sb *sb, mp_Str str, mp_Alloc alloc) {
    mp_sb_init(sb, alloc);
    mp_da_reserve(sb, str.len);
    if (sb->data != NULL) {
        mp_sb_append(sb, str);
    }
}

void mp_sb_init_withf(mp_Sb *sb, mp_Alloc alloc, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    __MP_ASSERT_MSG(len >= 0, "Failed to count string length");
    va_end(args);

    mp_sb_init(sb, alloc);
    mp_da_reserve(sb, (size_t) len + 1);

    if (sb->data != NULL) {
        va_start(args, fmt);
        int result_len = vsnprintf(sb->data, (size_t) len + 1, fmt, args);
        va_end(args);
        sb->len += (size_t) result_len;
    }
}

void mp_sb_deinit(mp_Sb *sb) {
    mp_da_deinit(sb);
}

void mp_sb_append(mp_Sb *sb, mp_Str str) {
    mp_da_append_array(sb, str.data, str.len);
}

void mp_sb_appendf(mp_Sb *sb, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    __MP_ASSERT_MSG(len >= 0, "Failed to count string length");
    va_end(args);

    mp_da_grow(sb, (size_t) len + 1);

    if (sb->data != NULL) {
        va_start(args, fmt);
        int result_len = vsnprintf(sb->data + sb->len, (size_t) len + 1, fmt, args);
        va_end(args);
        sb->len += (size_t) result_len;
    }
}

mp_Str mp_sb_str(const mp_Sb *sb) {
    return mp_str_s(sb->data, sb->len);
}

void __mp_ht_init(void *ht, mp_Alloc alloc, size_t size, size_t val_size) {
    __mp_Hash_Table *self = ht;
    __mp_da_init(self, alloc, size);
    self->val_size = val_size;
}

void __mp_ht_deinit(void *ht) {
    __mp_Hash_Table *self = ht;
    for (size_t i = 0; i < self->cap; i++) {
        __mp_Str_Ht_Entry *e = __mp_da_get(__mp_Str_Ht_Entry, self, i);
        if (mp_str_is_valid(e->key)) {
            mp_str_deinit(&e->key, self->alloc);
        }
    }
    __mp_da_deinit(self);
}

void *__mp_ht_get(const void *ht, mp_Str k) {
    const __mp_Hash_Table *self = ht;
    if (self->cap == 0) {
        return NULL;
    }
    if (mp_str_is_valid(k)) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_Str_Ht_Entry *e = __mp_da_get(__mp_Str_Ht_Entry, self, i);
            if (mp_str_is_valid(e->key) && mp_str_eq(k, e->key)) {
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
    __mp_Hash_Table *self = ht;
    __mp_ht_grow(self, 1);
    if (self->data != NULL) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_Str_Ht_Entry *e = __mp_da_get(__mp_Str_Ht_Entry, self, i);
            if (!mp_str_is_valid(e->key)) {
                e->key = mp_str_clone(k, self->alloc);
                memcpy(&e->val, v, self->val_size);
                break;
            } else if (mp_str_eq(e->key, k)) {
                memcpy(&e->val, v, self->val_size);
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
    __mp_Hash_Table *self = ht;
    if (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD)
        && offset > 0) {
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
            __mp_Str_Ht_Entry *e = __mp_da_get(__mp_Str_Ht_Entry, self, i);
            if (mp_str_is_valid(e->key)) {
                uint64_t hash  = __mp_ht_hash_str(&e->key);
                size_t   new_i = (size_t) (hash % (uint64_t) (self->cap - 1));
                for (;;) {
                    __mp_Str_Ht_Entry *new_e =
                        (__mp_Str_Ht_Entry *) ((char *) new_data + new_i * self->size);
                    if (!mp_str_is_valid(new_e->key)) {
                        new_e->key = mp_str_clone(e->key, self->alloc);
                        memcpy(&new_e->val, &e->val, self->val_size);
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
        __mp_Str_Ht_Entry *e = (__mp_Str_Ht_Entry *) ((char *) entries + i * size);
        if (mp_str_is_valid(e->key)) {
            mp_str_deinit(&e->key, alloc);
            __MP_ASSERT(!mp_str_is_valid(e->key));
        }
    }
}

void __mp_ht_reset(void *ht) {
    __mp_Hash_Table *self = ht;
    __mp_ht_free_entries(self->data, self->alloc, self->cap, self->size);
    self->len = 0;
}

// TODO: mp_ht(i)_move
void __mp_ht_delete(void *ht, mp_Str k) {
    __mp_Hash_Table *self = ht;
    if (mp_str_is_valid(k)) {
        uint64_t hash = __mp_ht_hash_str(&k);
        size_t   i    = (size_t) (hash % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_Str_Ht_Entry *e = __mp_da_get(__mp_Str_Ht_Entry, self, i);
            if (mp_str_is_valid(e->key) && mp_str_eq(k, e->key)) {
                mp_str_deinit(&e->key, self->alloc);
                __MP_ASSERT(!mp_str_is_valid(e->key));
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

// DOCS: notice to clone funcs that dest and src must not overlap
void __mp_ht_clone(void *dest, const void *src, mp_Alloc alloc) {
    const __mp_Hash_Table *s = src;
    __mp_Hash_Table       *d = dest;
    memcpy(d, s, sizeof(__mp_Hash_Table));
    d->data = mp_dup(alloc, s->data, s->cap * s->size);
    if (d->data != NULL) {
        for (size_t i = 0; i < s->cap; ++i) {
            __mp_Str_Ht_Entry *s_e = __mp_da_get(__mp_Str_Ht_Entry, s, i);
            __mp_Str_Ht_Entry *d_e = __mp_da_get(__mp_Str_Ht_Entry, d, i);
            if (mp_str_is_valid(s_e->key)) {
                d_e->key = mp_str_clone(s_e->key, alloc);
                __MP_ASSERT(d_e->key.data != s_e->key.data);
            }
        }
    } else {
        __MP_ZERO(d);
    }
}

void mp_ht_keys_deinit(mp_Ht_Keys *keys) {
    for (size_t i = 0; i < keys->len; ++i) {
        mp_str_deinit(mp_getp(keys, i), keys->alloc);
    }
    mp_da_deinit(keys);
}

void __mp_ht_keys(const void *ht, mp_Ht_Keys *keys) {
    const __mp_Hash_Table *self = ht;
    mp_da_reserve(keys, self->len);
    if (keys->data != NULL) {
        size_t            iter_size = sizeof(__mp_Str_Ht_Iter) + self->val_size;
        __mp_Str_Ht_Iter *it        = mp_alloc(self->alloc, iter_size);
        __mp_ht_iter_init(it, self);
        while (__mp_ht_iter_next(it)) {
            mp_da_append(keys, mp_str_clone(it->key, keys->alloc));
        }
        mp_free(self->alloc, it, iter_size);
    }
}

void __mp_ht_values(const void *ht, void *values) {
    const __mp_Hash_Table *self = ht;
    __mp_Hash_Table       *vals = values;
    __MP_ASSERT(self->val_size == vals->size);
    __mp_da_reserve(vals, self->len);
    if (vals->data != NULL) {
        size_t            iter_size = sizeof(__mp_Str_Ht_Iter) + self->val_size;
        __mp_Str_Ht_Iter *it        = mp_alloc(self->alloc, iter_size);
        __mp_ht_iter_init(it, self);
        while (__mp_ht_iter_next(it)) {
            memcpy((char *) vals->data + vals->len * vals->size, it->val, vals->size);
            ++vals->len;
        }
        mp_free(self->alloc, it, iter_size);
    }
}

void __mp_ht_iter_init(void *it, const void *ht) {
    __mp_Str_Ht_Iter      *self = it;
    const __mp_Hash_Table *h    = ht;
    memset(self, 0, sizeof(*self) + h->val_size);
    self->_h = h;
}

bool __mp_ht_iter_next(void *it) {
    __mp_Str_Ht_Iter *self = it;
    while (self->_i < self->_h->cap) {
        __mp_Str_Ht_Entry *entry = __mp_da_get(__mp_Str_Ht_Entry, self->_h, self->_i);
        if (mp_str_is_valid(entry->key)) {
            self->key = entry->key;
            if (self->_h->val_size > 0) {
                self->val = &entry->val;
            }
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
    for (size_t i = 0; i < str->len; ++i) {
        hash ^= (uint64_t) (unsigned char) (str->data[i]);
        hash *= __MP_FNV_PRIME;
    }
    return hash;
}

void *__mp_hti_get(const void *ht, size_t k) {
    const __mp_Hash_Table *self = ht;
    if (self->cap == 0) {
        return NULL;
    }
    size_t i = (size_t) (k % (uint64_t) (self->cap - 1));
    for (;;) {
        __mp_Int_Ht_Entry *e = __mp_da_get(__mp_Int_Ht_Entry, self, i);
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
    __mp_Hash_Table *self = ht;
    __mp_hti_grow(self, 1);
    if (self->data != NULL) {
        size_t i = (size_t) (k % (uint64_t) (self->cap - 1));
        for (;;) {
            __mp_Int_Ht_Entry *e = __mp_da_get(__mp_Int_Ht_Entry, self, i);
            if (!e->key.valid) {
                e->key = (__mp_Int_Ht_Key) {
                    .key   = k,
                    .valid = true,
                };
                memcpy(&e->val, v, self->val_size);
                break;
            } else if (e->key.key == k) {
                memcpy(&e->val, v, self->val_size);
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
    __mp_Hash_Table *self = ht;
    if (self->len + offset > (size_t) ((double) self->cap * __MP_HASH_TABLE_MAX_LOAD)
        && offset > 0) {
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
            __mp_Int_Ht_Entry *e = __mp_da_get(__mp_Int_Ht_Entry, self, i);
            if (e->key.valid) {
                size_t new_i = (size_t) (e->key.key % (uint64_t) (self->cap - 1));
                for (;;) {
                    __mp_Int_Ht_Entry *new_e =
                        (__mp_Int_Ht_Entry *) ((char *) new_data + new_i * self->size);
                    if (!new_e->key.valid) {
                        new_e->key = e->key;
                        memcpy(&new_e->val, &e->val, self->val_size);
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
    __mp_Hash_Table *self = ht;
    for (size_t i = 0; i < self->cap; ++i) {
        __mp_Int_Ht_Entry *e = __mp_da_get(__mp_Int_Ht_Entry, self, i);
        if (e->key.valid) {
            __MP_ZERO(&e->key);
        }
    }
    self->len = 0;
}

void __mp_hti_delete(void *ht, size_t k) {
    __mp_Hash_Table *self = ht;
    size_t           i    = (size_t) (k % (uint64_t) (self->cap - 1));
    for (;;) {
        __mp_Int_Ht_Entry *e = __mp_da_get(__mp_Int_Ht_Entry, self, i);
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
    const __mp_Hash_Table *s = src;
    __mp_Hash_Table       *d = dest;
    memcpy(d, s, sizeof(__mp_Hash_Table));
    d->data = mp_dup(alloc, s->data, s->cap * s->size);
    if (d->data == NULL) {
        __MP_ZERO(d);
    }
}

void __mp_hti_keys(const void *ht, mp_Hti_Keys *keys) {
    const __mp_Hash_Table *self = ht;
    mp_da_reserve(keys, self->len);
    if (keys->data != NULL) {
        size_t            iter_size = sizeof(__mp_Int_Ht_Iter) + self->val_size;
        __mp_Int_Ht_Iter *it        = mp_alloc(self->alloc, iter_size);
        __mp_hti_iter_init(it, self);
        while (__mp_hti_iter_next(it)) {
            mp_da_append(keys, it->key.key);
        }
        mp_free(self->alloc, it, iter_size);
    }
}

void __mp_hti_values(const void *ht, void *values) {
    const __mp_Hash_Table *self = ht;
    __mp_Hash_Table       *vals = values;
    __MP_ASSERT(self->val_size == vals->size);
    __mp_da_reserve(vals, self->len);
    if (vals->data != NULL) {
        size_t            iter_size = sizeof(__mp_Str_Ht_Iter) + self->val_size;
        __mp_Int_Ht_Iter *it        = mp_alloc(self->alloc, iter_size);
        __mp_hti_iter_init(it, self);
        while (__mp_hti_iter_next(it)) {
            memcpy((char *) vals->data + vals->len * vals->size, it->val, vals->size);
            ++vals->len;
        }
        mp_free(self->alloc, it, iter_size);
    }
}

void __mp_hti_iter_init(void *it, const void *ht) {
    __mp_Int_Ht_Iter      *self = it;
    const __mp_Hash_Table *h    = ht;
    memset(self, 0, sizeof(*self) + h->val_size);
    self->_h = h;
}

bool __mp_hti_iter_next(void *it) {
    __mp_Int_Ht_Iter *self = it;
    while (self->_i < self->_h->cap) {
        __mp_Int_Ht_Entry *entry = __mp_da_get(__mp_Int_Ht_Entry, self->_h, self->_i);
        if (entry->key.valid) {
            self->key = entry->key;
            if (self->_h->val_size > 0) {
                self->val = &entry->val;
            }
            ++self->_i;
            return true;
        }
        ++self->_i;
    }
    return false;
}

mp_Region *mp_region_new(mp_Alloc alloc, size_t cap) {
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

uintptr_t mp_arena_mark(const mp_Arena *a) {
    if (a->end == NULL) {
        return 0;
    }
    return (uintptr_t) (char *) a->end->data + a->end->len;
}

void mp_arena_rewind(mp_Arena *a, uintptr_t mark) {
    if (mark == 0) {
        return;
    }
    for (mp_Region *region = a->begin; region; region = region->next) {
        uintptr_t start = (uintptr_t) region->data;
        if (mark >= start + region->cap) {
            region->len = 0;
        } else if (mark >= start) {
            region->len -= mark - start;
            a->end = region;
        }
    }
}

static void *mp_arena_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                 void *ptr) {
    mp_Arena *ctx   = context;
    mp_Alloc  alloc = mp_alloc_new(ctx, mp_arena_alloc_func);

    switch (op) {
        case MP_ALLOC_OP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            size_t alloc_size = __MP_ALIGN(new_size, sizeof(uintptr_t));

            if (ctx->end == NULL) {
                __MP_ASSERT(ctx->begin == NULL);
                size_t capacity = ctx->_def_size;
                if (capacity < alloc_size) {
                    capacity = alloc_size;
                }
                ctx->end = mp_region_new(ctx->alloc, capacity);
                if (ctx->end == NULL) {
                    return NULL;
                }
                ctx->begin = ctx->end;
            }

            while (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap
                   && ctx->end->next != NULL) {
                ctx->end = ctx->end->next;
            }

            if (__MP_ALIGN(ctx->end->len, sizeof(uintptr_t)) + alloc_size > ctx->end->cap) {
                __MP_ASSERT(ctx->end->next == NULL);
                size_t capacity = ctx->_def_size;
                if (capacity < alloc_size) {
                    capacity = alloc_size;
                }
                ctx->end->next = mp_region_new(ctx->alloc, capacity);
                if (ctx->end->next == NULL) {
                    return NULL;
                }
                ctx->end = ctx->end->next;
            }

            __MP_ASSERT(ctx->end->len % sizeof(uintptr_t) == 0);
            size_t len_words = __MP_DIV_ROUNDUP(ctx->end->len, sizeof(uintptr_t));
            void  *result    = &ctx->end->data[len_words];
            ctx->end->len += alloc_size;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOC_OP_REALLOC: {
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOC_OP_FREE: {
            (void) old_size;

            return NULL;
        } break;
        case __MP_ALLOC_OP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

void mp_sarena_init(mp_Sarena *a, mp_Alloc alloc, size_t cap) {
    size_t     bytes  = __MP_ALIGN(cap, sizeof(uintptr_t));
    uintptr_t *buffer = mp_alloc(alloc, bytes);
    a->alloc          = alloc;
    a->buf            = buffer;
    a->len            = 0;
    a->cap            = bytes;
}

void mp_sarena_reset(mp_Sarena *a) {
    // memset(a->buf, 0, a->cap);
    a->len = 0;
}

void mp_sarena_deinit(mp_Sarena *a) {
    mp_free(a->alloc, a->buf, a->cap * sizeof(*(a)->buf));
    __MP_ZERO(a);
}

mp_Alloc mp_sarena_alloc(mp_Sarena *a) {
    if (a->buf == NULL) {
        return mp_alloc_invalid();
    }
    return mp_alloc_new(a, mp_sarena_alloc_func);
}

uintptr_t mp_sarena_mark(const mp_Sarena *a) {
    return (uintptr_t) (char *) a->buf + a->len;
}

void mp_sarena_rewind(mp_Sarena *a, uintptr_t mark) {
    a->len = mark - (uintptr_t) a->buf;
}

static void *mp_sarena_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                  void *ptr) {
    mp_Sarena *ctx   = context;
    mp_Alloc   alloc = mp_alloc_new(ctx, mp_sarena_alloc_func);

    switch (op) {
        case MP_ALLOC_OP_ALLOC: {
            (void) old_size;
            (void) ptr;

            if (new_size == 0) {
                return NULL;
            }

            if (ctx->buf == NULL) {
                return NULL;
            }

            size_t alloc_size = __MP_ALIGN(new_size, sizeof(uintptr_t));

            __MP_ASSERT(ctx->len % sizeof(uintptr_t) == 0);
            if (ctx->len + alloc_size > ctx->cap) {
                return NULL;
            }

            void *result = (char *) ctx->buf + ctx->len;
            ctx->len += alloc_size;
            return result;
        } break;
        case MP_ALLOC_OP_REALLOC: {
            if (ctx->buf == NULL) {
                return NULL;
            }
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOC_OP_FREE: {
            (void) old_size;

            return NULL;
        } break;
        case __MP_ALLOC_OP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

void mp_temp_init(mp_Temp *t, char *buf, size_t cap) {
    memset(buf, 0, cap);
    t->buf = (uintptr_t *) buf;
    t->len = 0;
    t->cap = __MP_ALIGN(cap, sizeof(uintptr_t));
}

void mp_temp_reset(mp_Temp *t) {
    t->len = 0;
}

mp_Alloc mp_temp_alloc(mp_Temp *t) {
    if (t->buf == NULL) {
        return mp_alloc_invalid();
    }
    return mp_alloc_new(t, mp_sarena_alloc_func);
}

uintptr_t mp_temp_mark(const mp_Temp *t) {
    return (uintptr_t) (char *) t->buf + t->len;
}

void mp_temp_rewind(mp_Temp *t, uintptr_t mark) {
    t->len = mark - (uintptr_t) t->buf;
}

mp_Alloc mp_heap_alloc(void) {
    return mp_alloc_new(NULL, mp_heap_alloc_func);
}

static void *mp_heap_alloc_func(mp_Alloc_Op op, void *context, size_t new_size, size_t old_size,
                                void *ptr) {
    (void) context;
    mp_Alloc alloc = mp_alloc_new(NULL, mp_heap_alloc_func);

    switch (op) {
        case MP_ALLOC_OP_ALLOC: {
            (void) old_size;
            (void) ptr;
            if (new_size == 0) {
                return NULL;
            }
            return __MP_ALLOC(new_size);
        } break;
        case MP_ALLOC_OP_REALLOC: {
            return mp_alloc_handle_realloc(alloc, ptr, old_size, new_size);
        } break;
        case MP_ALLOC_OP_FREE: {
            (void) old_size;
            if (ptr == NULL) {
                return NULL;
            }
            // `new_size` is unused
            __MP_FREE(ptr);
            return NULL;
        } break;
        case __MP_ALLOC_OP_COUNT: __MP_UNREACHABLE();
    }
    __MP_UNREACHABLE();
}

// Thanks, Wikipedia! <https://en.wikipedia.org/wiki/UTF-8#Description>
mp_Utf8_Char_Data mp_utf8_take(const char **str, size_t *size) {
    if (!(str != NULL && *str != NULL)) {
        return mp_utf8_char_invalid(NULL, 0);
    }
    if (*size == 0) {
        return mp_utf8_char_invalid(NULL, 0);
    }
    const char *ch    = *str;
    uint8_t     first = (uint8_t) ch[0];

    uint8_t char_size           = 0;
    uint8_t actual_decoded_size = 0;
    if (first <= 0x7F) {
        char_size = 1;
    } else if (first >= 0xC0 && first <= 0xDF) {
        char_size = 2;
    } else if (first >= 0xE0 && first <= 0xEF) {
        char_size = 3;
    } else if (first >= 0xF0) {
        char_size = 4;
    } else {
        goto fail;
    }

    uint32_t codepoint = 0x00;
    for (uint8_t i = 1; i <= char_size; ++i) {
        if (i > *size) {
            goto fail;
        }

        uint8_t byte  = (uint8_t) ch[i - 1];
        uint8_t order = char_size - i;

        if (i == 1) {
            uint8_t second_shift = (char_size == 1) ? char_size : char_size + 1;
            codepoint |= (uint32_t) ((byte & (0xFF >> second_shift)) << 6 * (char_size - 1));
        } else {
            // not a "continuation byte"
            if (!(byte >= 0x80 && byte <= 0xBF)) {
                goto fail;
            }
            codepoint |= (uint32_t) (byte & 0x3F) << 6 * order;
        }
        ++actual_decoded_size;
    }

    __MP_ASSERT(char_size == actual_decoded_size);

    // checking for overlong encoding
    if ((actual_decoded_size == 2 && !(codepoint >= 0x0080 && codepoint <= 0x07FF))
        || (actual_decoded_size == 3 && !(codepoint >= 0x0800 && codepoint <= 0xFFFF))
        || (actual_decoded_size == 4 && !(codepoint >= 0x010000))) {
        goto fail;
    }

    // surrogates
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        goto fail;
    }

    if (codepoint > 0x10FFFF) {
        goto fail;
    }

    *str += char_size;
    *size -= char_size;

    return (mp_Utf8_Char_Data) {
        .size      = char_size,
        .c         = ch,
        .codepoint = codepoint,
    };

fail:
    if (actual_decoded_size == 0) {
        actual_decoded_size = 1;
    }
    *str += actual_decoded_size;
    *size -= actual_decoded_size;
    return mp_utf8_char_invalid(ch, actual_decoded_size);
}

mp_Utf8_Char_Data mp_utf8_char(const char *c) {
    return mp_utf8_char_s(c, strlen(c));
}

mp_Utf8_Char_Data mp_utf8_char_s(const char *c, size_t size) {
    return mp_utf8_take(&c, &size);
}

size_t mp_utf8_len(const char *str) {
    return mp_utf8_len_s(str, strlen(str));
}

size_t mp_utf8_len_s(const char *str, size_t size) {
    size_t            len = 0;
    mp_Utf8_Char_Data c;
    while ((c = mp_utf8_take(&str, &size)).c != NULL) {
        ++len;
    }
    return len;
}

mp_Utf8_Char_Data mp_utf8_get(const char *str, size_t index) {
    return mp_utf8_get_s(str, strlen(str), index);
}

mp_Utf8_Char_Data mp_utf8_get_s(const char *str, size_t size, size_t index) {
    size_t            i = 0;
    mp_Utf8_Char_Data c;
    while ((c = mp_utf8_take(&str, &size)).c != NULL) {
        if (index == i) {
            return c;
        }
        ++i;
    }
    return mp_utf8_char_invalid(NULL, 0);
}

mp_Utf8_Iter mp_utf8_iter_new(const char *str) {
    return (mp_Utf8_Iter) {
        ._str  = str,
        ._size = strlen(str),
    };
}

mp_Utf8_Iter mp_utf8_iter_new_s(const char *str, size_t size) {
    return (mp_Utf8_Iter) {
        ._str  = str,
        ._size = size,
    };
}

bool mp_utf8_iter_next(mp_Utf8_Iter *it) {
    it->c = mp_utf8_take(&it->_str, &it->_size);
    return mp_utf8_char_is_accessible(it->c);
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

        case MP_ERR_CANNOT_ALLOC:      return "Cannot allocate memory";

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

mp_Err mp_file_read(mp_Str *out_str, const char *file_path, mp_Alloc alloc) {
    FILE *f = fopen(file_path, "r");
    if (f == NULL) {
        return mp_err(errno);
    }

    mp_Err err = MP_ERR_NONE;

    fseek(f, 0, SEEK_END);
    size_t size = (size_t) ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = mp_alloc(alloc, size + 1);
    if (buf == NULL) {
        err = MP_ERR_CANNOT_ALLOC;
        goto defer;
    }

    size_t read_size = fread(buf, 1, size, f);
    if (read_size != size) {
        err = mp_err(errno);
        goto defer;
    }

    buf[size] = '\0';

    *out_str = (mp_Str) {
        .cstr = buf,
        .len  = size,
    };

defer:
    fclose(f);
    return err;
}

mp_Err mp_file_write(const char *file_path, const char *data, size_t data_size, bool append) {
    const char *mode = "w";
    if (append) {
        mode = "a";
    }
    FILE *f = fopen(file_path, mode);
    if (f == NULL) {
        return mp_err(errno);
    }

    mp_Err err = MP_ERR_NONE;

    size_t written_size = fwrite(data, 1, data_size, f);
    if (written_size != data_size) {
        err = mp_err(errno);
        goto defer;
    }

defer:
    fclose(f);
    return err;
}

mp_Err mp_file_create(const char *file_path) {
    FILE *f = fopen(file_path, "w");
    if (f == NULL) {
        return mp_err(errno);
    }

    fclose(f);
    return MP_ERR_NONE;
}

mp_Err mp_file_delete(const char *file_path) {
    if (remove(file_path)) {
        return mp_err(errno);
    }
    return MP_ERR_NONE;
}

#endif /* ifdef MEMPLUS_IMPLEMENTATION */

#endif /* ifndef __MEMPLUS_H */
