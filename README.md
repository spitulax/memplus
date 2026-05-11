<h2 align="center">Memplus</h2>
A library to help with memory allocation and other useful things in C.
<hr>

## Features

- [x] Allocator interface
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
- [ ] Testing
- [ ] Subprocess
- [ ] Other kinds of allocators
- [ ] Socket IO
- [ ] Threading

## Usage

Memplus is a header only library that you can copy to your project and modify as
you like.

```c
#define MEMPLUS_IMPLEMENTATION // add this line once in a C file
#include "memplus.h"
```

## Support

Theoretically supports POSIX systems and Windows. Tested on Linux with GCC and
Clang, and on Windows with MSVC.
