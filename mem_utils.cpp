#include "mem_utils.h"

#include <cassert>
#include <cstdlib>


u8* trash_cpu_caches() {
    int mb = 1;
    auto x = calloc(mb << 20, 1);
    assert(x);
    return (u8*)x;
}

void do_not_optimize(u8 arg) {
}

void do_not_optimize_ptr(void** arg) {
}
