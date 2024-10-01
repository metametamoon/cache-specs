//
// Created by metametamoon on 9/27/24.
//

#ifndef CLEAR_MEMORY_H
#define CLEAR_MEMORY_H
#include <cstdint>

using u8 = uint8_t;

// free me!
u8* trash_cpu_caches();

void do_not_optimize(u8 arg);
void do_not_optimize_ptr(void** arg);

#endif //CLEAR_MEMORY_H
