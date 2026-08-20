#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Path cwd;
    expect_erreq(mp_get_current_dir(&cwd, mp_heap()), MP_ERR_NONE, mp_err_str);
    expect(cwd.absolute);

    mp_String cwd_str = mp_path_string(&cwd, mp_heap());
    logf("%.*s", mp_str_print(cwd_str));

    mp_string_deinit(&cwd_str, mp_heap());
    mp_path_deinit(&cwd);

    return 0;

fail:
    exit(1);
}
