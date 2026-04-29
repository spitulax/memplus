<h2 align="center">Memplus: A library to help with memory allocation in C</h2>

## Progress

- [x] Custom allocator interface
- [x] Arena allocators (growing, static, temp)
- [x] Heap allocator
- [x] Dynamic array
- [x] Sized string
- [x] String builder
- [x] UTF-8 string support
- [x] Hash table
- [x] File IO
- [ ] Logging
- [ ] Tracking allocator
- [ ] Subprocess
- [ ] Other kinds of allocators
- [ ] Socket IO
- [ ] Threading

## Usage

Memplus is a header only library that you can copy to your project as you like.

```c
#define MEMPLUS_IMPLEMENTATION // add this line once
#include "memplus.h"
```
