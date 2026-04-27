#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_File f;
    mp_Err  e = mp_file_open(&f, "010-file.c", "r");
    if (!e) {
        logf("Opened file!");

        mp_Io io = mp_file_io(&f);

        // mp_io_flush(&io);
        // char buf[256] = { 0 };
        // mp_io_setbuf(&io, buf, 256, MP_SETBUF_MODE_LINE);
        // size_t n = 0;
        // mp_io_read(&io, buf, 256, &n);
        // mp_io_write(&io, buf, 256, &n);
        // mp_io_getpos(&io, &n);
        // mp_io_setpos(&io, 0, MP_SETPOS_ORIGIN_START);

        char     buf[4096] = { 0 };
        size_t   n         = 0;
        mp_IoErr read_err  = mp_io_read(&io, buf, 1, 4096, &n);
        if (!read_err) {
            logf("Read %zu bytes", n);
            logf("%s", buf);
        }
    }

    mp_file_deinit(&f);

    return 0;

fail:
    exit(1);
}
