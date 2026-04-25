#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_File f;
    mp_Err  e = mp_file_open(&f, "010-file.c", "r");
    logf("%s", mp_err_str(e));
    mp_file_close(&f);

    return 0;

fail:
    exit(1);
}
