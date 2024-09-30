#include "clear_memory.h"


u8* clear_memory() {
    int mb = 16;
    auto x = calloc(mb << 20, 1);
    assert(x);
    return (u8*)x;
}