#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

#ifdef __MP_SYSTEM_WINDOWS

#include <tchar.h>
#include <windows.h>

int main(void) {
    TCHAR *tstr   = _T("Хэлло, Ворлд!");
    mp_Str mp_str = mp_str_from_tchar(tstr, mp_heap());
    expect_eq(mp_str.len, (size_t) 23, "%zu");
    mp_str_deinit(&mp_str, mp_heap());

    return 0;

fail:
    exit(1);
}

#else

int main(void) {
    return 0;
}

#endif
