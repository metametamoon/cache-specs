#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <unistd.h>

#include "page_traverser.h"

#include <assert.h>


using u8 = std::uint8_t;

// do not discard a value
u8 clear_memory(int alloca_size) {
  int mb = 16;
  auto x = calloc(mb << 20, sizeof(u8));
  assert(x);
  free(x);
  return 0;

  // prev
  // std::vector<u8*> memories;
  // int N = 10000;
  // for (int i = 0; i < N; i++) {
  //   auto x = static_cast<u8 *>(calloc(alloca_size, 1));
  //   if (x == nullptr) {
  //     std::cerr << "Failed to allocate memory" << std::endl;
  //   }
  //   memories.push_back(x);
  // }
  // for (int i = 0; i < N; i++) {
  //   free(memories[i]);
  // }
  // return 0;
}

void activate_pages(int pageCount, int pageSize, u8 *heap) {
  u8 sum = 0;
  for (int i = 0; i < pageCount; i++) {
    u8 value = heap[pageSize * i];
    sum += value;
  }
}

u8* create_aligned_heap(u8* unalignedHeap) {
  u8* heap = unalignedHeap;
  while (reinterpret_cast<unsigned long long>(heap) & 0xFFF) {
    heap += 1;
  }
  return heap;
}

double robust_mean(std::vector<double> means) {
  assert(means.size() >= 15);
  std::sort(means.begin(), means.end());
  auto sum = std::accumulate(means.begin() + 5, means.end() - 5, 0.0);
  return sum / (std::ssize(means)  - 10);
}

double traverse_pages(int step, int repNum) {
  // The following variable represents an amount of mem access in a benchmark.
  // We don't want it to be too small so that we could assess the average mem access as a mean
  // with reasonable error.
  // But we also don't want it to be too big so that we could profit from kicking out
  // outliers using the robust mean calculation.
  int pageCount = 400;
  int pageSize = 4096;
  int alloca_size = pageSize * (pageCount + 1);
  std::vector<double> results;
  u8 sum = 0;
  for (int rep = 0; rep < repNum; ++rep) {
    clear_memory(alloca_size);
    u8* unalignedHeap = (u8*)calloc(alloca_size, 1);
    u8* heap = create_aligned_heap(unalignedHeap);

    clear_memory(alloca_size);
    activate_pages(pageCount, pageSize, heap);

    auto const start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < pageCount; i++) {
      u8 value = heap[pageSize * i + step];
      sum += value;
    }
    auto const end = std::chrono::high_resolution_clock::now();

    auto mean = static_cast<double>((end - start).count()) / pageCount;
    results.push_back(mean);
    free(unalignedHeap);
  }
  // std::cout << "sum= " << sum << std::endl;
  return robust_mean(results);
}


// double traverse_large_memory(int step, int nTimes, int repNum) {
//   int pageSize = 4096;
//   int cacheLineSize = 64; // got from previous experiments
//   int alloca_size = step * nTimes + pageSize * 2;
//   std::vector<double> results;
//   u8 sum = 0;
//   for (int rep = 0; rep < repNum; ++rep) {
//     u8* unalignedHeap = (u8*)calloc(alloca_size, 1);
//     u8* heap = create_aligned_heap(unalignedHeap);
//
//     clear_memory(alloca_size);
//     activate_pages(pageCount, pageSize, heap);
//
//     auto const start = std::chrono::high_resolution_clock::now();
//     for (int i = 0; i < pageCount; i++) {
//       u8 value = heap[pageSize * i + step];
//       sum += value;
//     }
//     auto const end = std::chrono::high_resolution_clock::now();
//
//     auto mean = static_cast<double>((end - start).count()) / pageCount;
//     results.push_back(mean);
//     free(unalignedHeap);
//   }
//   // std::cout << "sum= " << sum << std::endl;
//   return robust_mean(results);
// }
