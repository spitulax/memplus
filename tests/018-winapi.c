#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

#ifdef __MP_SYSTEM_WINDOWS

#include <tchar.h>
#include <windows.h>

int main(void) {
    {
        TCHAR    *tstr = _T("Хэлло, Ворлд!");
        mp_String mstr = mp_string_from_tchar(tstr, mp_heap());
        expect_eq(mstr.len, (size_t) 23, "%zu");
        mp_string_deinit(&mstr, mp_heap());
    }

    {
        mp_Str mstr = mp_str("Хэлло, Ворлд!");

        LPWSTR wstr = mp_wstr_from_str(mstr, mp_heap());
        expect_ne(wstr, NULL, "%p");
        expect_eq(lstrlenW(wstr), 13, "%d");

        mp_String mstring = mp_wstr_to_string(wstr, mp_heap());
        expect(mp_is_valid(mstring));
        expect_streq_mp(mstring, mstr);
        mp_string_deinit(&mstring, mp_heap());

        mp_deinit(mp_heap(), wstr);
    }

    return 0;

fail:
    exit(1);
}

#else

int main(void) {
    return 0;
}

#endif
