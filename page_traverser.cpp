#include <chrono>
#include <iostream>
#include <numeric>
#include <vector>
#include <unistd.h>

#include "page_traverser.h"

#include <assert.h>
#include <source_location>

#include "clear_memory.h"


u8* zero_alloc(i64 size, const std::source_location location = std::source_location::current()) {
  auto x = calloc(size, 1);
  if (x == nullptr) {
    printf("zero_alloc failed\n");
    std::cout << "file: "
        << location.file_name() << '('
        << location.line() << ':'
        << location.column() << ") `"
        << location.function_name()
          << " tried allocating (bytes):" << size <<  std::endl;
    exit(-1);
  }
  return static_cast<u8 *>(x);
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
    free(clear_memory());
    u64* heap = (u64*)calloc(alloca_size, sizeof(u8));
    // u8* heap = create_aligned_heap(unalignedHeap);

    free(clear_memory());
    //activate_pages(pageCount, pageSize, heap);

    /*
     * If step is greater than the size of the cache line, each (a simplification) iteration of the loop
     * will result in a cache miss, degrading the performance. The experiments showed that the performance
     * is being halved in such cases.
     */

    for (i64 i = 0; i < pageCount - 1; i++) {
      heap[pageSize * i + step] = reinterpret_cast<u64>(heap + pageSize * (i + 1) + step);
    }
    auto ptr = heap + step;
    free(clear_memory());
    std::cout << "step=" << step << std::endl;
    auto const start = std::chrono::high_resolution_clock::now();
    for (i64 i = 0; i < pageCount; i++) {
      ptr = reinterpret_cast<u64 *>(*ptr);
      std::cout << "i = " << i << " " << ptr - heap << std::endl;
    }
    auto const end = std::chrono::high_resolution_clock::now();
    if (reinterpret_cast<u64>(ptr) != 0) {
      std::cout << "Here!\n";
    }

    auto mean = static_cast<double>((end - start).count()) / pageCount;
    results.push_back(mean);
    free(heap);
  }
  // std::cout << "sum= " << sum << std::endl;
  return robust_mean(results);
}

/*
 * The idea is as follows:
 * we are going reading-writing at address `heap + tag_try * i` for i in [0..1024).
 * If `tag_try` is an index that points to an index field of the address,
 * we will hit a new cache set most of the times, thus, the need to write-back during
 * the overflow of cache will be escaped.
 * If, however, `tag_try` is actually an index pointing into the tag section of the address,
 * at each iteration we will try to access new cache line with trying to put it into the same cache set,
 * thus causing the writeback to some cache line from the cache set, causing additional overhead.
 * The experiments show that this overhead causes the resulting time of the operation to be at least twice
 * as large as the cost without the overhead.
 */
double check_tag_index(i64 tag_try) {
  auto unalignedHeap = zero_alloc(tag_try * 1024 + 4096);
  auto heap = create_aligned_heap(unalignedHeap);
  free(clear_memory());;
  int nTries = 500000;
  std::vector<double> results;

  free(clear_memory());;
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
  free(unalignedHeap);
  return robust_mean(results);
}

/*
 *  We will to access addresses `heap + i * (1 << first_tag_index)` for i in [0..assoc_try) in the loop.
 *  When `assoc_try > real_assoc`, we are hitting the same cache set in the loop more times than there are
 *  entries in a cache set, thus causing the writeback to negatively impact the performance
 */
double check_assoc(i64 assoc_try, i64 first_tag_index) {
  auto unalignedHeap = zero_alloc(64 * (1 << 21) + 4096);
  auto heap = create_aligned_heap(unalignedHeap);
  free(clear_memory());
  int nTries = 1000000;
  std::vector<double> results;

  free(clear_memory());
  int tag = 1 << first_tag_index;

  int portion = 4096 / assoc_try;
  for (i64 nTry = 0; nTry < nTries; nTry++) {
    auto start = std::chrono::high_resolution_clock::now();
    for (i64 p = 0; p < portion; p++) {
      for (i64 i = 0; i < assoc_try; i++) {
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

template <typename T>
T* create_aligned_ptr(T* unalignedHeap) {
  T* heap = unalignedHeap;
  while (reinterpret_cast<unsigned long long>(heap) & 0xFFF) {
    heap += 1;
  }
  return heap;
}


// size must be a power of 2
double check_size(i64 logsize) {
  assert(logsize >= 13);
  void** unalignedHeap = (void**)calloc(1 << 22, sizeof(void*));
  void** heap = create_aligned_ptr(unalignedHeap);
  i64 const step = 1 << 6;
  i64 N = (1 << logsize) / step; // >= 1 << 7
  for (i64 i = 0; i < N; ++i) {
    i64 nextI = (i + 1) % N;
    heap[step * i] = heap + step * nextI;
  }
  {
    i64 n = N * 4;
    auto ptr = heap;
#pragma unroll 128
    while (--n) {
      ptr = (void**)*ptr;
    }
    fprintf(stdin, "%p", ptr);
  }
  i64 repCount = 10'000'000;
  i64 n = N * repCount;
  auto ptr = heap;
  auto start = std::chrono::high_resolution_clock::now();
  while (--n) {
    ptr = (void**)*ptr;
  }
  auto end = std::chrono::high_resolution_clock::now();
  fprintf(stdin, "%p", ptr);
  auto diff = static_cast<double>((end - start).count()) / N;
  free(unalignedHeap);
  return diff;
}