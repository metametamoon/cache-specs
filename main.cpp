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
        double next = traverse_pages(i, 1000);
        // std::cout << "step=" << i << " mean=" << next << std::endl;
        if (next > prev * 1.75) {
            std::cout << "assessed cache line size: " << i << std::endl;
        }
        prev = next;
    }
}

void find_first_tag_index() {
    auto prev = 10000000000.0;
    for (int maybeTag = 16; maybeTag <= 24; ++maybeTag) {
        int tagStep = 1 << maybeTag;
        double next = check_tag_index(tagStep);
        // std::cout << "maybeTag=" << maybeTag << " mean=" << next << std::endl;
        if (prev * 2 < next) {
            std::cout << "assessed first tag index: " << maybeTag << std::endl;
        }
        prev = next;
    }
}

void find_assoc() {
    // this pre-search allows us to narrow down the search to [8, 16]
    // for (int maybeAssoc = 2; maybeAssoc <= 64; maybeAssoc <<= 1) {
        // double next = check_assoc(maybeAssoc);
        // std::cout << "maybeAssoc=" << maybeAssoc << " mean=" << next << std::endl;
    // }
    double prev = 1000000.;
    for (int maybeAssoc = 8; maybeAssoc <= 16; ++maybeAssoc) {
        double next = check_assoc(maybeAssoc);
        // std::cout << "maybeAssoc=" << maybeAssoc << " mean=" << next << std::endl;
        if (next / prev >= 1.04) {
            std::cout << "assessed associativity: " << maybeAssoc - 1 << std::endl;
            break; // the performance will degrade further, we are only interested in the first spike
        }
        prev = next;
    }
}

int main() {
    bind_to_one_core();
    find_cache_line_size();
    find_first_tag_index();
    find_assoc();
}
