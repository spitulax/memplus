<h2 align="center">Memplus: A library to help with memory allocation in C</h2>

Memplus currently implements:
- Arena allocator
- Sized string
- Dynamic array (vector)

## Usage

Memplus is comprised of several header files that you can copy to your project as you like.
Typically a header file contains implementation that should be included once in your project.

**NOTE: all header files depend on `memplus_alloc.h`**

```c
#define MEMPLUS_ALLOC_IMPLEMENTATION
#include "memplus_alloc.h"

#define MEMPLUS_STRING_IMPLEMENTATION
#include "memplus_string.h"

#define MEMPLUS_VECTOR_IMPLEMENTATION
#include "memplus_vector.h"
```

## TODO
- [ ] Resizable string
- [ ] Hash map
- [ ] Other kinds of allocator
