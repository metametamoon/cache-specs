#include <iostream>
#include <cassert>
#include <chrono>
#include <fstream>
#include <optional>
#include <unistd.h>
#include <sched.h>
#include "page_traverser.h"


void bind_to_one_core() {
    auto pid = getpid();
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    assert(CPU_COUNT(&set) == 1);
    sched_setaffinity(pid, 1, &set);
}

#define VERBOSE

i64 find_cacheline_size() {
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


void size_eval_loop(i64 cacheline_size) {
    // std::ofstream f("cachesize-to-time.txt");
    double prev = 10000000000.0;
    std::optional<i64> ansKb = std::nullopt;
    for (i64 sizeKb = 8; sizeKb <= 256; sizeKb *= 2) {
        i64 sizeB = sizeKb * 1024;
        auto start = std::chrono::high_resolution_clock::now();
        double measureTime = check_size(sizeB, cacheline_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto passedTime = std::chrono::duration_cast<std::chrono::seconds>((end - start));
        // f << sizeKb << ',' << measureTime << std::endl;
        printf("size=%3ldKb time=%f (took %lds)\n", sizeKb, measureTime, passedTime.count());
        if (measureTime > prev) {
            double relativeIncrease = (measureTime - prev) / prev;
            if (relativeIncrease > 0.08 && !ansKb.has_value()) {
                ansKb = sizeKb / 2;
            }
        }
        prev = measureTime;
    }
    if (ansKb.has_value()) {
        printf("evaluated cache size is %ldKb\n", ansKb.value());
    }
}

void associativity_eval_loop(i64 cacheline_size) {
    double prev = 100000.0;
    std::optional<i64> ansAssoc = std::nullopt;
    for (i64 i = 2; i <= 20; i += 2) {
        auto start = std::chrono::high_resolution_clock::now();
        double measureTime = check_assoc(i, cacheline_size);
        auto end = std::chrono::high_resolution_clock::now();
        auto passedTime = std::chrono::duration_cast<std::chrono::seconds>((end - start));
        printf("assoc=%2ld time=%f (took %lds)\n", i, measureTime, passedTime.count());
        if (measureTime / prev >= 1.7 && !ansAssoc.has_value()) {
            ansAssoc = i - 2;
        }
        prev = measureTime;
    }
    if (ansAssoc.has_value()) {
        printf("evaluated associativity is %ld\n", ansAssoc.value());
    } else {
        printf("failed to evaluate associativity :(");
    }
}

int main() {
    bind_to_one_core();
    auto cacheline_size = find_cacheline_size();
    if (cacheline_size != -1) {
        size_eval_loop(cacheline_size);
        associativity_eval_loop(cacheline_size);
    }
    return 0;
}
