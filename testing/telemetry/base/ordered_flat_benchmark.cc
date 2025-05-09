// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>

#include "base/include/vector.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

using namespace std;          // NOLINT
using namespace lynx::base;   // NOLINT
using namespace lynx::lepus;  // NOLINT

namespace lynx {
namespace base {
namespace {
template <class T>
T Cast(size_t v);

template <>
int Cast(size_t v) {
  return (int)v;
}

template <>
string Cast(size_t v) {
  return "string_" + to_string(v);
}

template <>
shared_ptr<string> Cast(size_t v) {
  return make_shared<string>(to_string(v));
}

template <>
String Cast(size_t v) {
  return String(std::string("string_") + to_string(v));
}

template <>
Value Cast(size_t v) {
  if (v % 3 == 1) {
    return Value(std::to_string(v));
  } else if (v % 3 == 2) {
    return Value(lepus::Dictionary::Create());
  } else {
    return Value((int32_t)v);
  }
}
}  // namespace

#define STRINGIFY(s) #s
#define FOREACH_STRINGIFY_1(a) #a
#define FOREACH_STRINGIFY_2(a, ...) #a ", " FOREACH_STRINGIFY_1(__VA_ARGS__)
#define FOREACH_STRINGIFY_3(a, ...) #a ", " FOREACH_STRINGIFY_2(__VA_ARGS__)
#define FOREACH_STRINGIFY_4(a, ...) #a ", " FOREACH_STRINGIFY_3(__VA_ARGS__)
#define FOREACH_STRINGIFY_N(_4, _3, _2, _1, N, ...) FOREACH_STRINGIFY##N
#define FOREACH_STRINGIFY(...)                     \
  FOREACH_STRINGIFY_N(__VA_ARGS__, _4, _3, _2, _1) \
  (__VA_ARGS__)

#define TEST_FUNC_MAP_INSERT(TYPE, DATA_COUNT, MAP, ...)                   \
  static void BM_##MAP##_insert_##DATA_COUNT##_##TYPE(                     \
      benchmark::State& state) {                                           \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using MapType = MAP<__VA_ARGS__>;                                      \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    vector<pair<MapType::key_type, MapType::mapped_type>> data;            \
                                                                           \
    /* Generate data. Keys should not be ordered. */                       \
    int data_index = 0;                                                    \
    data.resize(kDataCount);                                               \
    for (size_t i = 0; i < kDataCount / 2; i++) {                          \
      data[data_index].first = Cast<MapType::key_type>(i);                 \
      data[data_index++].second = Cast<MapType::mapped_type>(i);           \
    }                                                                      \
    for (size_t i = kDataCount - 1; i >= kDataCount / 2; i--) {            \
      data[data_index].first = Cast<MapType::key_type>(i);                 \
      data[data_index++].second = Cast<MapType::mapped_type>(i);           \
    }                                                                      \
                                                                           \
    size_t total = 0;                                                      \
    for (auto _ : state) {                                                 \
      for (int i = 0; i < 10; i++) {                                       \
        MapType map;                                                       \
        for (auto it = data.begin(); it != data.end(); it++) {             \
          map[it->first] = it->second;                                     \
        }                                                                  \
        total += map.size();                                               \
      }                                                                    \
    }                                                                      \
  }

#define TEST_FUNC_MAP_FIND(TYPE, DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)        \
  static void BM_##MAP##_find_##DATA_COUNT##_##TYPE(benchmark::State& state) { \
    constexpr size_t kDataCount = DATA_COUNT;                                  \
    constexpr size_t kNotFoundCount = NOT_FOUND_COUNT;                         \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">");     \
    using MapType = MAP<__VA_ARGS__>;                                          \
    vector<pair<MapType::key_type, MapType::mapped_type>> data;                \
    vector<pair<MapType::key_type, MapType::mapped_type>> find_data;           \
                                                                               \
    /* Generate data.  */                                                      \
    data.resize(kDataCount);                                                   \
    find_data.resize(kDataCount + kNotFoundCount);                             \
    for (size_t i = 0; i < kDataCount; i++) {                                  \
      data[i].first = Cast<MapType::key_type>(i);                              \
      data[i].second = Cast<MapType::mapped_type>(i);                          \
      find_data[i].first = Cast<MapType::key_type>(i);                         \
      find_data[i].second = Cast<MapType::mapped_type>(i);                     \
    }                                                                          \
                                                                               \
    for (size_t i = 0; i < kNotFoundCount; i++) {                              \
      find_data[kDataCount + i].first =                                        \
          Cast<MapType::key_type>(kDataCount + i);                             \
    }                                                                          \
                                                                               \
    MapType map;                                                               \
    auto rng = std::default_random_engine{};                                   \
    std::shuffle(std::begin(data), std::end(data), rng);                       \
    for (auto it = data.begin(); it != data.end(); it++) {                     \
      map[it->first] = it->second;                                             \
    }                                                                          \
                                                                               \
    size_t total = 0;                                                          \
    for (auto _ : state) {                                                     \
      for (auto it = find_data.begin(); it != find_data.end(); it++) {         \
        if (map.find(it->first) != map.end()) {                                \
          total++;                                                             \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

#define TEST_MAP_INSERT_II(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ii, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_ii)

#define TEST_MAP_INSERT_SS(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ss, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_ss)

#define TEST_MAP_INSERT_IS(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(is, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_is)

#define TEST_MAP_INSERT_SV(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(sv, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_sv)

TEST_MAP_INSERT_II(8, map, int, int);
TEST_MAP_INSERT_II(8, unordered_map, int, int);
TEST_MAP_INSERT_II(8, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(8, InlineOrderedFlatMap, int, int, 8);

TEST_MAP_INSERT_II(2048, map, int, int);
TEST_MAP_INSERT_II(2048, unordered_map, int, int);
TEST_MAP_INSERT_II(2048, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(2048, InlineOrderedFlatMap, int, int, 2048);

TEST_MAP_INSERT_SS(8, map, string, string);
TEST_MAP_INSERT_SS(8, unordered_map, string, string);
TEST_MAP_INSERT_SS(8, OrderedFlatMap, string, string);
TEST_MAP_INSERT_SS(8, InlineOrderedFlatMap, string, string, 8);

TEST_MAP_INSERT_SS(32, map, string, string);
TEST_MAP_INSERT_SS(32, unordered_map, string, string);
TEST_MAP_INSERT_SS(32, OrderedFlatMap, string, string);
TEST_MAP_INSERT_SS(32, InlineOrderedFlatMap, string, string, 32);

TEST_MAP_INSERT_IS(8, map, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(8, unordered_map, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(8, OrderedFlatMap, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(8, InlineOrderedFlatMap, int, shared_ptr<string>, 8);

TEST_MAP_INSERT_IS(96, map, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(96, unordered_map, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(96, OrderedFlatMap, int, shared_ptr<string>);
TEST_MAP_INSERT_IS(96, InlineOrderedFlatMap, int, shared_ptr<string>, 96);

TEST_MAP_INSERT_SV(8, map, String, Value);
TEST_MAP_INSERT_SV(8, unordered_map, String, Value);
TEST_MAP_INSERT_SV(8, OrderedFlatMap, String, Value);
TEST_MAP_INSERT_SV(8, InlineOrderedFlatMap, String, Value, 8);

TEST_MAP_INSERT_SV(12, map, String, Value);
TEST_MAP_INSERT_SV(12, unordered_map, String, Value);
TEST_MAP_INSERT_SV(12, OrderedFlatMap, String, Value);
TEST_MAP_INSERT_SV(12, InlineOrderedFlatMap, String, Value, 32);

#define TEST_MAP_FIND_I(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(i, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_i)

#define TEST_MAP_FIND_S(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(s, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_s)

TEST_MAP_FIND_I(6, 2, map, int, int);
TEST_MAP_FIND_I(6, 2, unordered_map, int, int);
TEST_MAP_FIND_I(6, 2, OrderedFlatMap, int, int);

TEST_MAP_FIND_I(256, 32, map, int, int);
TEST_MAP_FIND_I(256, 32, unordered_map, int, int);
TEST_MAP_FIND_I(256, 32, OrderedFlatMap, int, int);

TEST_MAP_FIND_S(6, 2, map, string, string);
TEST_MAP_FIND_S(6, 2, unordered_map, string, string);
TEST_MAP_FIND_S(6, 2, OrderedFlatMap, string, string);

TEST_MAP_FIND_S(256, 32, map, string, string);
TEST_MAP_FIND_S(256, 32, unordered_map, string, string);
TEST_MAP_FIND_S(256, 32, OrderedFlatMap, string, string);

}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
