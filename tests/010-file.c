#include <stdlib.h>
#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

void del(void) {
#ifdef __MP_SYSTEM_POSIX
    int _ = system("rm .foo");
#elifdef __MP_SYSTEM_WINDOWS
    int _ = system("del .foo");
#endif
    (void) _;
}

int main(void) {
    // Don't test on Windows
#ifndef __MP_SYSTEM_WINDOWS
    mp_File   f;
    mp_Err    e;
    mp_Io     io;
    mp_Io_Err ie;
    e = mp_file_open(&f, ".foo", "r");
    expect_erreq(e, MP_ERR_NO_FILE_OR_DIR, mp_err_str);

    {
        // IO test
        e = mp_file_open(&f, ".foo", "w");
        expect_erreq(e, MP_ERR_NONE, mp_err_str);
        io = mp_file_io(&f, MP_IO_TYPE_READ);
        expect(!mp_io_is_valid(io));
        io = mp_file_io(&f, MP_IO_TYPE_WRITE);
        expect(mp_io_is_valid(io));

        // Reopen test
        e = mp_file_reopen(&f, ".foo", "w+");
        expect_erreq(e, MP_ERR_NONE, mp_err_str);
        io = mp_file_io(&f, MP_IO_TYPE_WRITE);
        expect(mp_io_is_valid(io));
        io = mp_file_io(&f, MP_IO_TYPE_READ);
        expect(mp_io_is_valid(io));
    }

    // Write test
    {
        e = mp_file_reopen(&f, ".foo", "w");
        expect_erreq(e, MP_ERR_NONE, mp_err_str);
        io = mp_file_io(&f, MP_IO_TYPE_WRITE);
        expect(mp_io_is_valid(io));
        // Write test
        ie = mp_io_putc(&io, 'f');
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);
        const char m[] = "oobar\nbaz\n";    // don't forget this has the NUL-terminator
        size_t     n   = 0;
        ie             = mp_io_write(&io, m, 1, sizeof(m) - 1, &n);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);
        expect_eq(n, (size_t) 10, "%zu");
        ie = mp_io_flush(&io);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);
    }

    char buf[1] = { 0 };
    // Read, setpos, getpos, setbuf test
    {
        mp_io_setbuf(&io, buf, sizeof(buf), MP_SETBUF_MODE_LINE);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);

    #ifdef __GLIBC__
        #define _IO_USER_BUF 1
        expect((f.file->_flags & _IO_USER_BUF) != 0);
        #undef _IO_USER_BUF
    #endif

        e = mp_file_reopen(&f, ".foo", "r");
        expect_erreq(e, MP_ERR_NONE, mp_err_str);
        io = mp_file_io(&f, MP_IO_TYPE_READ);
        expect(mp_io_is_valid(io));
        size_t c = 0;
        ie       = mp_io_getc(&io, &c);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);
        expect_eq(c, (size_t) 'f', "%zu");

        size_t pos = 0;
        ie         = mp_io_getpos(&io, &pos);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);
        expect_eq(pos, (size_t) 1, "%zu");
        ie = mp_io_setpos(&io, 0, MP_SETPOS_ORIGIN_START);
        expect_erreq(ie, MP_IO_ERR_NONE, mp_ioerr_str);

        char   m[512] = { 0 };
        size_t n      = 0;
        ie            = mp_io_read(&io, &m, 1, sizeof(m), &n);
        expect_erreq(ie, MP_IO_ERR_EOF, mp_ioerr_str);
        expect_eq(n, (size_t) 11, "%zu");
        expect_streq(m, "foobar\nbaz\n");
    }

    mp_file_deinit(&f);

    del();
    return 0;

fail:
    del();
    exit(1);

#else
    return 0;
#endif
}
