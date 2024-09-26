#include <iostream>
#include <vector>
#include <array>
#include <assert.h>
#include <chrono>
#include <fstream>
#include <numeric>
#include <unistd.h>
#include <sched.h>
#include "page_traverser.h"


// hopefully clear all reasonable caches



void bind_to_one_core() {
    auto pid = getpid();
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    assert(CPU_COUNT(&set) == 1);
    sched_setaffinity(pid, 1, &set);
}

// prints relatively
void find_cache_line_size() {
    double prev = 100.0;
    for (int i = 1; i < 4096; i = i << 1) {
        double next = traverse_pages(i, 30);
        std::cout << "step=" << i << " mean=" << next << std::endl;
        if (next > prev * 1.25) {
            std::cout << "possible cache line size: " << i << std::endl;
        }
        prev = next;
    }
}

int main() {
    bind_to_one_core();
    find_cache_line_size();
}
