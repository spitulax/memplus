#define MEMPLUS_IMPLEMENTATION
#include "memplus.h"
#include "test.h"

int main(void) {
    mp_Path cwd;
    expect_erreq(mp_get_current_dir(&cwd, mp_heap()), MP_ERR_NONE, mp_err_str);

    for (size_t i = 0; i < cwd.comps.len; ++i) {
        logf("%.*s", mp_str_print(mp_get(&cwd.comps, i)));
    }

    mp_path_deinit(&cwd);

    return 0;

fail:
    exit(1);
}
