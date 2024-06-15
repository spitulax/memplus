#include "test.h"

int main(void) {
    mp_Arena arena;
    mp_arena_init(&arena);
    mp_Allocator alloc = mp_arena_allocator(&arena);

    mp_String myhome = mp_string_new(&alloc, getenv("HOME"));
    prn(myhome.cstr);

    mp_String greeting = mp_string_newf(&alloc, "HELLO! My name is %s", getenv("USER"));
    prn(greeting.cstr);

    mp_String mynewhome = mp_string_dup(&alloc, myhome);
    prnf("My old home is at %p, but now I live at %p", myhome.cstr, mynewhome.cstr);

    mp_arena_destroy(&arena);
}
