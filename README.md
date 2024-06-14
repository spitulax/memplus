<h2 align="center">Memplus: A library to help with memory allocation in C</h2>

**WARNING: The library is still in development. Something may be changed drastically.**

Memplus currently implements:
- Customizable allocator interface
- Growing and static arena allocator
- Stack temp allocator
- Sized string
- Dynamic array (vector)

## Usage

Memplus is a header only library that you can copy to your project as you like.

```c
#define MEMPLUS_IMPLEMENTATION // add this line once
#include "memplus.h"
```

## TODO
- [ ] Resizable string/String builder
- [ ] Hash map
- [ ] Other kinds of allocator
