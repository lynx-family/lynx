// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <random>

#include "base/include/linked_hash_map.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

namespace lynx {
namespace base {

// These tests are created to tune LinkedHashMap for proper template
// arguments for linear threshold. Without modifying LinkedHashMap,
// there is no need to run these test cases to prevent misreport
// on the benchmark platform.

#define TEST_FUNC_LINKED_HASH_MAP_ADJUSTMENT_FIND(LINEAR_THRESHOLD,                                    \
                                                  DATA_COUNT, NOT_FOUND_COUNT)                         \
  static void                                                                                          \
      BM_LinkedHashMap_LinearFindThreshold_find_##LINEAR_THRESHOLD##_##DATA_COUNT##_##NOT_FOUND_COUNT( \
          benchmark::State& state) {                                                                   \
    constexpr size_t kDataCount = DATA_COUNT;                                                          \
    constexpr size_t kNotFoundCount = NOT_FOUND_COUNT;                                                 \
    constexpr size_t kLinearFindThreshold = LINEAR_THRESHOLD;                                          \
    state.SetLabel(kLinearFindThreshold < kDataCount ? "hash_find"                                     \
                                                     : "linear_find");                                 \
    using Map = LinkedHashMap<std::string, int, kLinearFindThreshold,                                  \
                              kLinearFindThreshold>;                                                   \
    std::vector<std::pair<std::string, int>> data;                                                     \
    std::vector<std::pair<std::string, int>> find_data;                                                \
                                                                                                       \
    /* Generate data.  */                                                                              \
    data.resize(kDataCount);                                                                           \
    find_data.resize(kDataCount + kNotFoundCount);                                                     \
    for (size_t i = 0; i < kDataCount; i++) {                                                          \
      data[i].first = std::string("key_of_map") + std::to_string(i);                                   \
      data[i].second = i;                                                                              \
      find_data[i].first = std::string("key_of_map") + std::to_string(i);                              \
      find_data[i].second = i;                                                                         \
    }                                                                                                  \
                                                                                                       \
    for (size_t i = 0; i < kNotFoundCount; i++) {                                                      \
      find_data[kDataCount + i].first =                                                                \
          std::string("key_of_map") + std::to_string(kDataCount + i + 1);                              \
    }                                                                                                  \
                                                                                                       \
    Map map;                                                                                           \
    map.reserve(data.size());                                                                          \
    auto rng = std::default_random_engine{};                                                           \
    std::shuffle(std::begin(data), std::end(data), rng);                                               \
    for (auto it = data.begin(); it != data.end(); it++) {                                             \
      map.insert_or_assign(it->first, it->second);                                                     \
    }                                                                                                  \
                                                                                                       \
    size_t total = 0;                                                                                  \
    for (auto _ : state) {                                                                             \
      for (auto it = find_data.begin(); it != find_data.end(); it++) {                                 \
        if (map.contains(it->first)) {                                                                 \
          total++;                                                                                     \
        }                                                                                              \
      }                                                                                                \
    }                                                                                                  \
  }

#define TEST_FUNC_LINKED_HASH_MAP_ADJUSTMENT_INSERT(                                                     \
    LINEAR_THRESHOLD, DATA_COUNT, COLLISION_COUNT)                                                       \
  static void                                                                                            \
      BM_LinkedHashMap_LinearFindThreshold_insert_##LINEAR_THRESHOLD##_##DATA_COUNT##_##COLLISION_COUNT( \
          benchmark::State& state) {                                                                     \
    state.SetLabel("<std::string, int>");                                                                \
    constexpr size_t kDataCount = DATA_COUNT;                                                            \
    constexpr size_t kKeyCollisionCount = COLLISION_COUNT;                                               \
    constexpr size_t kLinearFindThreshold = LINEAR_THRESHOLD;                                            \
    using Map = LinkedHashMap<std::string, int, kLinearFindThreshold,                                    \
                              kLinearFindThreshold>;                                                     \
    std::vector<std::pair<std::string, int>> data;                                                       \
                                                                                                         \
    /* Generate data.  */                                                                                \
    data.resize(kDataCount + kKeyCollisionCount);                                                        \
    for (size_t i = 0; i < kDataCount; i++) {                                                            \
      data[i].first = std::string("key_of_map") + std::to_string(i);                                     \
      data[i].second = i;                                                                                \
    }                                                                                                    \
    for (size_t i = 0; i < kKeyCollisionCount; i++) {                                                    \
      data[kDataCount + i].first =                                                                       \
          std::string("key_of_map") + std::to_string(i);                                                 \
      data[kDataCount + i].second = i;                                                                   \
    }                                                                                                    \
                                                                                                         \
    size_t total = 0;                                                                                    \
    for (auto _ : state) {                                                                               \
      for (int i = 0; i < 10; i++) {                                                                     \
        Map map;                                                                                         \
        map.reserve(data.size());                                                                        \
        if (i % 2 == 0) {                                                                                \
          for (auto it = data.rbegin(); it != data.rend(); it++) {                                       \
            map.insert_or_assign(it->first, it->second);                                                 \
          }                                                                                              \
        } else {                                                                                         \
          for (auto it = data.begin(); it != data.end(); it++) {                                         \
            map.insert_or_assign(it->first, it->second);                                                 \
          }                                                                                              \
        }                                                                                                \
        total += map.size();                                                                             \
      }                                                                                                  \
    }                                                                                                    \
  }

#define TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(LINEAR_THRESHOLD, DATA_COUNT, \
                                               COLLISION_COUNT)              \
  TEST_FUNC_LINKED_HASH_MAP_ADJUSTMENT_INSERT(LINEAR_THRESHOLD, DATA_COUNT,  \
                                              COLLISION_COUNT)               \
  BENCHMARK(                                                                 \
      BM_LinkedHashMap_LinearFindThreshold_insert_##LINEAR_THRESHOLD##_##DATA_COUNT##_##COLLISION_COUNT)

TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(6, 12, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(8, 12, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(10, 12, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(12, 12, 0);

TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(0, 20, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(4, 20, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(8, 20, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(12, 20, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(16, 20, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(20, 20, 0);

TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(0, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(10, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(20, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(30, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(40, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(50, 60, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_INSERT(60, 60, 2);

#define TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(LINEAR_THRESHOLD, DATA_COUNT, \
                                             NOT_FOUND_COUNT)              \
  TEST_FUNC_LINKED_HASH_MAP_ADJUSTMENT_FIND(LINEAR_THRESHOLD, DATA_COUNT,  \
                                            NOT_FOUND_COUNT)               \
  BENCHMARK(                                                               \
      BM_LinkedHashMap_LinearFindThreshold_find_##LINEAR_THRESHOLD##_##DATA_COUNT##_##NOT_FOUND_COUNT)

TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(0, 2, 0);
TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(16, 2, 0);

TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(0, 4, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(16, 4, 2);

TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(0, 8, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(16, 8, 2);

TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(0, 16, 2);
TEST_LINKED_HASH_MAP_ADJUSTMENT_FIND(24, 16, 2);

}  // namespace base
}  // namespace lynx
