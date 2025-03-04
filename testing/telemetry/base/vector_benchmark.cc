// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <vector>

#include "base/include/vector.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace lynx {
namespace base {

// These performance test cases are used to compare the performance
// of Vector and std::vector. There is no need to run these test cases
// in CI to prevent misreport on the benchmark platform.

static constexpr size_t kInlineVectorBufferSize = 8;
static const std::array<int, 8> kNumberSmallerThan8 = {1, 2, 7, 6, 5, 4, 3, 2};
static const std::array<int, 8> kNumberLargerThan8 = {15, 9,  12, 14,
                                                      20, 10, 11, 16};

static void BM_InlineVector_CountSmallerThan8(benchmark::State& state) {
  auto get_array = [](int count) {
    InlineVector<int, kInlineVectorBufferSize> small_array;
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberSmallerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_InlineVector_CountLargerThan8(benchmark::State& state) {
  auto get_array = [](int count) {
    InlineVector<int, kInlineVectorBufferSize> small_array;
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberLargerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_InlineVector_CountLargerThan8_Reserve(benchmark::State& state) {
  auto get_array = [](int count) {
    InlineVector<int, kInlineVectorBufferSize> small_array;
    small_array.reserve(count);
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberLargerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_StdVector_CountSmallerThan8(benchmark::State& state) {
  auto get_array = [](int count) {
    std::vector<int> small_array;
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberSmallerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_StdVector_CountLargerThan8(benchmark::State& state) {
  auto get_array = [](int count) {
    std::vector<int> small_array;
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberLargerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_StdVector_CountSmallerThan8_Reserve(benchmark::State& state) {
  auto get_array = [](int count) {
    std::vector<int> small_array;
    small_array.reserve(count);
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberSmallerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

static void BM_StdVector_CountLargerThan8_Reserve(benchmark::State& state) {
  auto get_array = [](int count) {
    std::vector<int> small_array;
    small_array.reserve(count);
    for (int i = 0; i < count; i++) {
      small_array.push_back(i);
    }
    return small_array;
  };

  int total = 0;
  for (auto _ : state) {
    for (auto count : kNumberLargerThan8) {
      auto array = get_array(count);
      for (auto i : array) {
        total += i;
      }
    }
  }
}

BENCHMARK(BM_InlineVector_CountSmallerThan8);
BENCHMARK(BM_InlineVector_CountLargerThan8);
BENCHMARK(BM_InlineVector_CountLargerThan8_Reserve);
BENCHMARK(BM_StdVector_CountSmallerThan8);
BENCHMARK(BM_StdVector_CountLargerThan8);
BENCHMARK(BM_StdVector_CountSmallerThan8_Reserve);
BENCHMARK(BM_StdVector_CountLargerThan8_Reserve);

}  // namespace base
}  // namespace lynx
