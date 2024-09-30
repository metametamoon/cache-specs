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
i64 find_cache_line_size() {
    double prev = 100.0;
    for (int i = 1; i < 4096; i = i << 1) {
        double next = traverse_pages(i, 1000);
#ifdef VERBOSE
        std::cout << "step=" << i << " mean=" << next << std::endl;
#endif
        if (next > prev * 1.75) {
            std::cout << "assessed cache line size: " << i << std::endl;
            return i;
        }
        prev = next;
    }
    return -1;
}

i64 find_first_tag_index(i64 cacheline_size) {
    auto prev = 10000000000.0;
    for (int maybeTag = 16; maybeTag <= 24; ++maybeTag) {
        int tagStep = 1 << maybeTag;
        double next = check_tag_index(tagStep);
#ifdef VERBOSE
        std::cout << "maybeTag=" << maybeTag << " mean=" << next << std::endl;
#endif
        if (prev * 1.3 < next) {
            std::cout << "assessed first tag index: " << maybeTag << std::endl;
            return maybeTag;
        }
        prev = next;
    }
    return -1;
}

i64 find_assoc(i64 first_tag_index) {
    // this pre-search allows us to narrow down the search to [8, 16]
    // for (int maybeAssoc = 2; maybeAssoc <= 64; maybeAssoc <<= 1) {
        // double next = check_assoc(maybeAssoc);
        // std::cout << "maybeAssoc=" << maybeAssoc << " mean=" << next << std::endl;
    // }
    double prev = 1000000.;
    for (int maybeAssoc = 8; maybeAssoc <= 16; ++maybeAssoc) {
        double next = check_assoc(maybeAssoc, first_tag_index);
#ifdef VERBOSE
        std::cout << "maybeAssoc=" << maybeAssoc << " mean=" << next << std::endl;
#endif
        if (next / prev >= 1.04) {
            std::cout << "assessed associativity: " << maybeAssoc - 1 << std::endl;
            return maybeAssoc - 1; // the performance will degrade further, we are only interested in the first spike
        }
        prev = next;
    }
    return -1;
}

int main() {
    bind_to_one_core();
    // i64 cacheline_size = find_cache_line_size();
    // if (cacheline_size < 0) {
    //     std::cout << "Failed to calculate cache line size.\n";
    //     return -1;
    // }
    // i64 first_tag_index = find_first_tag_index(cacheline_size);
    // if (first_tag_index < 0) {
    //     std::cout << "Failed to calculate first index pointing to tag in address.\n";
    //     return -1;
    // }
    // i64 assoc = find_assoc(first_tag_index);
    // if (assoc == -1) {
    //     std::cout << "Failed to calculate associativity of the cache.\n";
    // }
    i64 logPageSize = 12;
    for (i64 i = logPageSize + 1; i <= 18; i += 1) {
        printf("size=%4ldKb time=%f\n", (1l << (i - 10)), check_size(i));
    }
    return 0;
}
