#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include "page_traverser.h"
#include <cassert>
#include <fstream>

#include "mem_utils.h"




void activate_pages(i64 pageCount, i64 pageSize, u8 *heap) {
  u8 sum = 0;
  for (i64 i = 0; i < pageCount; i++) {
    u8 value = heap[pageSize * i];
    sum += value;
  }
  do_not_optimize(sum);
}

template <typename T>
T* create_aligned_ptr(T* unalignedHeap) {
  T* heap = unalignedHeap;
  while (reinterpret_cast<unsigned long long>(heap) & 0xFFF) {
    heap += 1;
  }
  return heap;
}

double robust_mean(std::vector<double> means) {
  assert(means.size() >= 25);
  i64 n = means.size();
  i64 cutoff = 10;
  std::sort(means.begin(), means.end());
  auto sum = std::accumulate(means.begin() + cutoff, means.end() - cutoff, 0.0);
  return sum / (std::ssize(means)  - 2 * cutoff);
}

double traverse_pages(i64 step, i64 repNum) {
  // The following variable represents an amount of mem access in a benchmark.
  // We don't want it to be too small so that we could assess the average mem access as a mean
  // with reasonable error.

  // But we also don't want it to be too big so that we could profit from kicking out
  // outliers using the robust mean calculation.
  i64 pageCount = 400;
  i64 pageSize = 4096;
  i64 alloca_size = pageSize * (pageCount + 1);
  std::vector<double> results;
  u8 sum = 0;
  for (i64 rep = 0; rep < repNum; ++rep) {
    free(trash_cpu_caches());
    u8* unalignedHeap = (u8*)calloc(alloca_size, 1);
    u8* heap = create_aligned_ptr(unalignedHeap);

    free(trash_cpu_caches());
    activate_pages(pageCount, pageSize, heap);

    auto const start = std::chrono::high_resolution_clock::now();
    for (i64 i = 0; i < pageCount; i++) {
      u8 value = heap[pageSize * i + step];
      sum += value;
    }
    auto const end = std::chrono::high_resolution_clock::now();
    do_not_optimize(sum);
    auto mean = static_cast<double>((end - start).count()) / pageCount;
    results.push_back(mean);
    free(unalignedHeap);
  }
  std::ofstream of{ "step-" + std::to_string(step) + ".txt" };
  for (auto const &result : results) {
    of << result << ",";
    std::cout << result << ",";
  }
  // std::cout << "sum= " << sum << std::endl;
  return robust_mean(results);
}


double check_size(i64 size, i64 cacheline_size) {
  void** unalignedHeap = (void**)calloc(size * 2 + 4096, sizeof(void*));
  void** heap = create_aligned_ptr(unalignedHeap);
  i64 const step = cacheline_size;
  i64 N = size / step;
  for (i64 i = 0; i < N; ++i) {
    i64 nextI = (i + 1) % N;
    heap[step * i] = heap + step * nextI;
  }
  {
    // setting up memory
    i64 n = 1l << 26;
    auto ptr = heap;
    while (--n) {
      ptr = (void**)*ptr;
    }
    do_not_optimize_ptr(ptr);
  }
  i64 const loopIters = 1l << 31; // 33 is optimal
  i64 n = loopIters;
  auto ptr = heap;
  auto start = std::chrono::high_resolution_clock::now();
#pragma unroll 1024
  while (--n) {
    ptr = (void**)*ptr;
  }
  auto end = std::chrono::high_resolution_clock::now();
  do_not_optimize_ptr(ptr);
  auto diff = static_cast<double>((end - start).count()) / (loopIters);
  free(unalignedHeap);
  return diff;
}

// size must be a power of 2
double check_assoc(i64 maybeAssoc, i64 cacheline_size) {
  void** unalignedHeap = (void**)calloc(1 << 25, sizeof(void*));
  void** heap = create_aligned_ptr(unalignedHeap);
  i64 const bigStep = 1 << 18; // definitely bigger than 2^(index + offset)
  i64 const smallStep = cacheline_size;
  i64 smallStepBound = 4;
  for (i64 i = 0; i < maybeAssoc; ++i) {
    for (int j = 0; j < smallStepBound; ++j) {
      i64 nextJ = (j + 1) % smallStepBound;
      i64 nextI = i;
      if (nextJ == 0) {
        nextI = (nextI + 1) % maybeAssoc;
      }
      // i64 nextI = (i + 1) % maybeAssoc;
      heap[bigStep * i + smallStep * j] = heap + bigStep * nextI + smallStep * nextJ;
    }
  }
  { // seeting up the memory
    i64 n = 1l << 28;
    auto ptr = heap;
    while (--n) {
      ptr = (void**)*ptr;
    }
    do_not_optimize_ptr(ptr);
  }
  constexpr i64 repCount = 1l << 31;
  i64 n = repCount;
  auto ptr = heap;
  auto start = std::chrono::high_resolution_clock::now();
#pragma unroll 1024
  while (--n) {
    ptr = (void**)*ptr;
  }
  auto end = std::chrono::high_resolution_clock::now();
  do_not_optimize_ptr(ptr);
  auto diff = static_cast<double>((end - start).count()) / repCount;
  free(unalignedHeap);
  return diff;
}

double check_cacheline2(i64 maybeCacheline) {
  void** unalignedHeap = (void**)calloc((1 << 27) + 4096, sizeof(void*));
  void** heap = create_aligned_ptr(unalignedHeap);
  i64 const bigStep = 1 << 18;
  i64 const smallStep = maybeCacheline;
  i64 smallStepBound = 2;
  for (i64 i = 0; i < 128; ++i) {
    for (int j = 0; j < smallStepBound; ++j) {
      i64 nextJ = (j + 1) % smallStepBound;
      i64 nextI = i;
      if (nextJ == 0) {
        nextI = (nextI + 1) % 128;
      }
      heap[bigStep * i + smallStep * j] = heap + bigStep * nextI + smallStep * nextJ;
    }
  }
  { // seeting up the memory
    i64 n = 1l << 23;
    auto ptr = heap;
    while (--n) {
      ptr = (void**)*ptr;
    }
    fprintf(stdin, "%p", ptr);
  }
  constexpr i64 repCount = 1l << 29;
  i64 n = repCount;
  auto ptr = heap;
  auto start = std::chrono::high_resolution_clock::now();
#pragma unroll 1024
  while (--n) {
    ptr = (void**)*ptr;
  }
  auto end = std::chrono::high_resolution_clock::now();
  fprintf(stdin, "%p", ptr);
  auto diff = static_cast<double>((end - start).count()) / repCount;
  free(unalignedHeap);
  return diff;
}