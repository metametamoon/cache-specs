#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <unistd.h>

#include "page_traverser.h"

#include <assert.h>


// do not discard a value
u8 clear_memory() {
  i64 mb = 16;
  auto x = calloc(mb << 20, sizeof(u8));
  assert(x);
  free(x);
  return 0;
}

void activate_pages(i64 pageCount, i64 pageSize, u8 *heap) {
  u8 sum = 0;
  for (i64 i = 0; i < pageCount; i++) {
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
  i64 n = means.size();
  i64 cutoff = n / 5;
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
    clear_memory();
    u8* unalignedHeap = (u8*)calloc(alloca_size, 1);
    u8* heap = create_aligned_heap(unalignedHeap);

    clear_memory();
    activate_pages(pageCount, pageSize, heap);

    auto const start = std::chrono::high_resolution_clock::now();
    for (i64 i = 0; i < pageCount; i++) {
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

double check_index_tag(i64 tag_try) {
  auto unalignedHeap = static_cast<u8 *>(calloc(tag_try * 1024 + 4096, 1));
  auto heap = create_aligned_heap(unalignedHeap);
  clear_memory();
  int nTries = 500000;
  std::vector<double> results;

  clear_memory();
  for (i64 nTry = 0; nTry < nTries; nTry++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (i64 i = 0; i < 1024; i++) {
      i64 baseIndex = 0;
      heap[baseIndex + tag_try * i] = heap[baseIndex + tag_try * i] + 1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = static_cast<double>((end - start).count());
    results.push_back(diff);
  }
  return robust_mean(results);
}

double check_assoc(i64 assoc_try) {
  auto unalignedHeap = static_cast<u8 *>(calloc(64 * (1 << 21) + 4096, 1));
  auto heap = create_aligned_heap(unalignedHeap);
  clear_memory();
  int nTries = 100000;
  std::vector<double> results;

  clear_memory();
  int tag = 1 << 21;

  int portion = 4096 / assoc_try;
  for (i64 nTry = 0; nTry < nTries; nTry++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (i64 p = 0; p < portion; p++) {
      for (i64 i = 0; i < assoc_try; i++) {
        i64 step_index = 1 << 6; // we know it from the experiment above
        i64 baseIndex = 0;
        heap[baseIndex + tag * i] = heap[baseIndex + tag * i] + 1;
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = static_cast<double>((end - start).count());
    results.push_back(diff);
  }
  free(unalignedHeap);
  return robust_mean(results);
}