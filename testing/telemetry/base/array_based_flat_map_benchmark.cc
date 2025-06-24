// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/value/lepus_value.h"
#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

using namespace std;          // NOLINT
using namespace lynx::base;   // NOLINT
using namespace lynx::lepus;  // NOLINT

namespace lynx {
namespace base {

// Test template codec ranges.
struct Range {
  uint32_t start;
  uint32_t end;
};

namespace {
template <class T>
T Cast(size_t v);

template <>
int Cast(size_t v) {
  return (int)v;
}

template <>
Range Cast(size_t v) {
  return {static_cast<uint32_t>(v), static_cast<uint32_t>(v)};
}

template <>
string Cast(size_t v) {
  if (v % 2 == 0) {
    return "string_" + to_string(v);
  } else {
    return to_string(v) + "_string";
  }
}

template <>
shared_ptr<string> Cast(size_t v) {
  return make_shared<string>(to_string(v));
}

template <>
String Cast(size_t v) {
  if (v % 2 == 0) {
    return String(std::string("string_") + to_string(v));
  } else {
    return String(to_string(v) + std::string("_string"));
  }
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

#define TEST_FUNC_SET_INSERT(TYPE, DATA_COUNT, SET, ...)                   \
  static void BM_##SET##_insert_##DATA_COUNT##_##TYPE(                     \
      benchmark::State& state) {                                           \
    state.SetLabel(STRINGIFY(SET) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using SetType = SET<__VA_ARGS__>;                                      \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    constexpr size_t kDestructionBatchCount = 50;                          \
    vector<SetType::key_type> data;                                        \
                                                                           \
    /* Generate data. Keys should not be ordered. */                       \
    int data_index = 0;                                                    \
    data.resize(kDataCount);                                               \
    for (size_t i = 0; i < kDataCount / 2; i++) {                          \
      data[data_index++] = Cast<SetType::key_type>(i);                     \
    }                                                                      \
    for (size_t i = kDataCount - 1; i >= kDataCount / 2; i--) {            \
      data[data_index++] = Cast<SetType::key_type>(i);                     \
    }                                                                      \
                                                                           \
    size_t total = 0;                                                      \
    for (auto _ : state) {                                                 \
      SetType* sets[kDestructionBatchCount];                               \
      for (int i = 0; i < kDestructionBatchCount; i++) {                   \
        sets[i] = new SetType();                                           \
        for (const auto& key : data) {                                     \
          sets[i]->insert(key);                                            \
        }                                                                  \
        total += sets[i]->size();                                          \
      }                                                                    \
      state.PauseTiming();                                                 \
      for (int i = 0; i < kDestructionBatchCount; i++) {                   \
        delete sets[i]; /* destruction time not measured */                \
      }                                                                    \
      state.ResumeTiming();                                                \
    }                                                                      \
  }

#define TEST_FUNC_MAP_INSERT(TYPE, DATA_COUNT, MAP, ...)                   \
  static void BM_##MAP##_insert_##DATA_COUNT##_##TYPE(                     \
      benchmark::State& state) {                                           \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using MapType = MAP<__VA_ARGS__>;                                      \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    constexpr size_t kDestructionBatchCount = 50;                          \
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
      MapType* maps[kDestructionBatchCount];                               \
      for (int i = 0; i < kDestructionBatchCount; i++) {                   \
        maps[i] = new MapType();                                           \
        for (auto it = data.begin(); it != data.end(); it++) {             \
          (*maps[i])[it->first] = it->second;                              \
        }                                                                  \
        total += maps[i]->size();                                          \
      }                                                                    \
      state.PauseTiming();                                                 \
      for (int i = 0; i < kDestructionBatchCount; i++) {                   \
        delete maps[i]; /* destruction time not measured */                \
      }                                                                    \
      state.ResumeTiming();                                                \
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

#define TEST_SET_INSERT_I(DATA_COUNT, SET, ...)         \
  TEST_FUNC_SET_INSERT(I, DATA_COUNT, SET, __VA_ARGS__) \
  BENCHMARK(BM_##SET##_insert_##DATA_COUNT##_I)

#define TEST_SET_INSERT_s(DATA_COUNT, SET, ...)         \
  TEST_FUNC_SET_INSERT(s, DATA_COUNT, SET, __VA_ARGS__) \
  BENCHMARK(BM_##SET##_insert_##DATA_COUNT##_s)

#define TEST_SET_INSERT_S(DATA_COUNT, SET, ...)         \
  TEST_FUNC_SET_INSERT(S, DATA_COUNT, SET, __VA_ARGS__) \
  BENCHMARK(BM_##SET##_insert_##DATA_COUNT##_S)

#define TEST_MAP_INSERT_II(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(II, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_II)

#define TEST_MAP_INSERT_IR(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(IR, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_IR)

#define TEST_MAP_INSERT_ss(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ss, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_ss)

#define TEST_MAP_INSERT_ISP(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ISP, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_ISP)

#define TEST_MAP_INSERT_sSP(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(sSP, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_sSP)

#define TEST_MAP_INSERT_SSP(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SSP, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_SSP)

#define TEST_MAP_INSERT_SV(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SV, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_SV)

#define TEST_MAP_INSERT_IV(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(IV, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_IV)

#define TEST_MAP_INSERT_SS(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SS, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_SS)

/**
 * <int>
 * data count
 *   0~48: LinearFlatSet best
 *   48~1024: OrderedFlatSet best
 *   > 1024: std::unordered_set best
 */
TEST_SET_INSERT_I(8, set, int);
TEST_SET_INSERT_I(8, unordered_set, int);
TEST_SET_INSERT_I(8, OrderedFlatSet, int);
TEST_SET_INSERT_I(8, LinearFlatSet, int);

TEST_SET_INSERT_I(48, set, int);
TEST_SET_INSERT_I(48, unordered_set, int);
TEST_SET_INSERT_I(48, OrderedFlatSet, int);
TEST_SET_INSERT_I(48, LinearFlatSet, int);

TEST_SET_INSERT_I(1024, set, int);
TEST_SET_INSERT_I(1024, unordered_set, int);
TEST_SET_INSERT_I(1024, OrderedFlatSet, int);
TEST_SET_INSERT_I(1024, LinearFlatSet, int);

/**
 * <std::string>
 * data count
 *   0~26: LinearFlatSet best
 *   == 15: OrderedFlatSet == unordered_set
 *   > 26: std::unordered_set best
 */
TEST_SET_INSERT_s(8, set, string);
TEST_SET_INSERT_s(8, unordered_set, string);
TEST_SET_INSERT_s(8, OrderedFlatSet, string);
TEST_SET_INSERT_s(8, LinearFlatSet, string);

TEST_SET_INSERT_s(15, set, string);
TEST_SET_INSERT_s(15, unordered_set, string);
TEST_SET_INSERT_s(15, OrderedFlatSet, string);
TEST_SET_INSERT_s(15, LinearFlatSet, string);

TEST_SET_INSERT_s(26, set, string);
TEST_SET_INSERT_s(26, unordered_set, string);
TEST_SET_INSERT_s(26, OrderedFlatSet, string);
TEST_SET_INSERT_s(26, LinearFlatSet, string);

/**
 * <base::string>
 * data count
 *   0~48: LinearFlatSet best
 *   == 15: OrderedFlatSet == unordered_set
 *   > 48: std::unordered_set best
 */
TEST_SET_INSERT_S(8, set, String);
TEST_SET_INSERT_S(8, unordered_set, String);
TEST_SET_INSERT_S(8, OrderedFlatSet, String);
TEST_SET_INSERT_S(8, LinearFlatSet, String);

TEST_SET_INSERT_S(15, set, String);
TEST_SET_INSERT_S(15, unordered_set, String);
TEST_SET_INSERT_S(15, OrderedFlatSet, String);
TEST_SET_INSERT_S(15, LinearFlatSet, String);

TEST_SET_INSERT_S(48, set, String);
TEST_SET_INSERT_S(48, unordered_set, String);
TEST_SET_INSERT_S(48, OrderedFlatSet, String);
TEST_SET_INSERT_S(48, LinearFlatSet, String);

/**
 * <int, int>
 * data count
 *   0~52: LinearFlatMap best
 *   52~720: OrderedFlatMap best
 *   > 720: std::unordered_map best
 */
TEST_MAP_INSERT_II(45, map, int, int);
TEST_MAP_INSERT_II(45, unordered_map, int, int);
TEST_MAP_INSERT_II(45, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(45, LinearFlatMap, int, int);

TEST_MAP_INSERT_II(52, map, int, int);
TEST_MAP_INSERT_II(52, unordered_map, int, int);
TEST_MAP_INSERT_II(52, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(52, LinearFlatMap, int, int);

TEST_MAP_INSERT_II(720, map, int, int);
TEST_MAP_INSERT_II(720, unordered_map, int, int);
TEST_MAP_INSERT_II(720, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(720, LinearFlatMap, int, int);

/**
 * <int, Range>
 * data count
 *   0~64: LinearFlatMap best
 *   64~400: OrderedFlatMap best
 *   > 400: std::unordered_map best
 */
TEST_MAP_INSERT_IR(8, map, int, Range);
TEST_MAP_INSERT_IR(8, unordered_map, int, Range);
TEST_MAP_INSERT_IR(8, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(8, LinearFlatMap, int, Range);

TEST_MAP_INSERT_IR(64, map, int, Range);
TEST_MAP_INSERT_IR(64, unordered_map, int, Range);
TEST_MAP_INSERT_IR(64, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(64, LinearFlatMap, int, Range);

TEST_MAP_INSERT_IR(400, map, int, Range);
TEST_MAP_INSERT_IR(400, unordered_map, int, Range);
TEST_MAP_INSERT_IR(400, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(400, LinearFlatMap, int, Range);

/**
 * <std::string, std::string>
 * data count
 *   0~24: LinearFlatMap best
 *   > 24: std::unordered_map best
 */
TEST_MAP_INSERT_ss(8, map, string, string);
TEST_MAP_INSERT_ss(8, unordered_map, string, string);
TEST_MAP_INSERT_ss(8, OrderedFlatMap, string, string);
TEST_MAP_INSERT_ss(8, LinearFlatMap, string, string);

TEST_MAP_INSERT_ss(24, map, string, string);
TEST_MAP_INSERT_ss(24, unordered_map, string, string);
TEST_MAP_INSERT_ss(24, OrderedFlatMap, string, string);
TEST_MAP_INSERT_ss(24, LinearFlatMap, string, string);

/**
 * <int, std::shared_ptr<string>>
 * data count
 *   0~128: LinearFlatMap best
 *   =40: std::unordered_map == OrderedFlatMap
 *   > 128: std::unordered_map best
 */
TEST_MAP_INSERT_ISP(8, map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, unordered_map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, OrderedFlatMap, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, LinearFlatMap, int, shared_ptr<string>);

TEST_MAP_INSERT_ISP(48, map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, unordered_map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, OrderedFlatMap, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, LinearFlatMap, int, shared_ptr<string>);

TEST_MAP_INSERT_ISP(128, map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, unordered_map, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, OrderedFlatMap, int, shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, LinearFlatMap, int, shared_ptr<string>);

/**
 * <std::string, std::shared_ptr<string>>
 * data count
 *   0~32: LinearFlatMap best
 *   > 32: std::unordered_map best
 */
TEST_MAP_INSERT_sSP(8, map, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, unordered_map, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, OrderedFlatMap, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, LinearFlatMap, string, shared_ptr<string>);

TEST_MAP_INSERT_sSP(32, map, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, unordered_map, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, OrderedFlatMap, string, shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, LinearFlatMap, string, shared_ptr<string>);

/**
 * <base::String, std::shared_ptr<string>>
 * data count
 *   0~48: LinearFlatMap best
 *   > 48: std::unordered_map best
 */
TEST_MAP_INSERT_SSP(8, map, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, unordered_map, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, OrderedFlatMap, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, LinearFlatMap, String, shared_ptr<string>);

TEST_MAP_INSERT_SSP(48, map, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, unordered_map, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, OrderedFlatMap, String, shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, LinearFlatMap, String, shared_ptr<string>);

/**
 * <base::String, base::Value>
 * data count
 *   0~32: LinearFlatMap best
 *   > 32: std::unordered_map best
 */
TEST_MAP_INSERT_SV(8, map, String, Value);
TEST_MAP_INSERT_SV(8, unordered_map, String, Value);
TEST_MAP_INSERT_SV(8, OrderedFlatMap, String, Value);
TEST_MAP_INSERT_SV(8, LinearFlatMap, String, Value);

TEST_MAP_INSERT_SV(32, map, String, Value);
TEST_MAP_INSERT_SV(32, unordered_map, String, Value);
TEST_MAP_INSERT_SV(32, OrderedFlatMap, String, Value);
TEST_MAP_INSERT_SV(32, LinearFlatMap, String, Value);

/**
 * <int, base::Value>
 * data count
 *   0~128: LinearFlatMap best
 *   > 128: std::unordered_map best
 */
TEST_MAP_INSERT_IV(8, map, int, Value);
TEST_MAP_INSERT_IV(8, unordered_map, int, Value);
TEST_MAP_INSERT_IV(8, OrderedFlatMap, int, Value);
TEST_MAP_INSERT_IV(8, LinearFlatMap, int, Value);

TEST_MAP_INSERT_IV(128, map, int, Value);
TEST_MAP_INSERT_IV(128, unordered_map, int, Value);
TEST_MAP_INSERT_IV(128, OrderedFlatMap, int, Value);
TEST_MAP_INSERT_IV(128, LinearFlatMap, int, Value);

/**
 * <base::String, base::String>
 * data count
 *   0~40: LinearFlatMap best
 *   > 40: std::unordered_map best
 */
TEST_MAP_INSERT_SS(8, map, String, String);
TEST_MAP_INSERT_SS(8, unordered_map, String, String);
TEST_MAP_INSERT_SS(8, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(8, LinearFlatMap, String, String);

TEST_MAP_INSERT_SS(24, map, String, String);
TEST_MAP_INSERT_SS(24, unordered_map, String, String);
TEST_MAP_INSERT_SS(24, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(24, LinearFlatMap, String, String);

TEST_MAP_INSERT_SS(40, map, String, String);
TEST_MAP_INSERT_SS(40, unordered_map, String, String);
TEST_MAP_INSERT_SS(40, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(40, LinearFlatMap, String, String);

#define TEST_MAP_FIND_I(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(I, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_I)

#define TEST_MAP_FIND_s(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(s, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_s)

#define TEST_MAP_FIND_S(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(S, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_S)

TEST_MAP_FIND_I(2, 1, map, int, int);
TEST_MAP_FIND_I(2, 1, unordered_map, int, int);  // std::unordered_map is faster
TEST_MAP_FIND_I(2, 1, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(2, 1, LinearFlatMap, int, int);

TEST_MAP_FIND_I(12, 2, map, int, int);
TEST_MAP_FIND_I(12, 2, unordered_map, int, int);
TEST_MAP_FIND_I(12, 2, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(12, 2, LinearFlatMap, int,
                int);  // 12 equivalent to OrderedFlatMap

TEST_MAP_FIND_I(128, 16, map, int, int);
TEST_MAP_FIND_I(128, 16, unordered_map, int, int);
TEST_MAP_FIND_I(128, 16, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(128, 16, LinearFlatMap, int, int);

TEST_MAP_FIND_s(3, 1, map, string, string);
TEST_MAP_FIND_s(3, 1, unordered_map, string, string);
TEST_MAP_FIND_s(3, 1, OrderedFlatMap, string, string);
TEST_MAP_FIND_s(
    3, 1, LinearFlatMap, string,
    string);  // equivalent to std::unordered_map, faster than OrderedFlatMap

// If most keys share the same prefix substring, LinearFlatMap scores would be
// worse
TEST_MAP_FIND_s(30, 4, map, string, string);
TEST_MAP_FIND_s(30, 4, unordered_map, string, string);
TEST_MAP_FIND_s(30, 4, OrderedFlatMap, string, string);
TEST_MAP_FIND_s(
    30, 4, LinearFlatMap, string,
    string);  // 30 equivalent to ordered, less than 30 LinearFlatMap is faster

TEST_MAP_FIND_S(2, 1, map, String, String);
TEST_MAP_FIND_S(2, 1, unordered_map, String, String);
TEST_MAP_FIND_S(2, 1, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(
    2, 1, LinearFlatMap, String,
    String);  // equivalent to std::unordered_map, faster than OrderedFlatMap

TEST_MAP_FIND_S(6, 1, map, String, String);
TEST_MAP_FIND_S(6, 1, unordered_map, String, String);
TEST_MAP_FIND_S(6, 1, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(6, 1, LinearFlatMap, String,
                String);  // a little slower than std::unordered_map, faster
                          // than OrderedFlatMap

// If most keys share the same prefix substring, LinearFlatMap scores would be
// worse
TEST_MAP_FIND_S(80, 4, map, String, String);
TEST_MAP_FIND_S(80, 4, unordered_map, String, String);
TEST_MAP_FIND_S(80, 4, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(
    80, 4, LinearFlatMap, String,
    String);  // 80 equivalent to ordered, less than 80 LinearFlatMap is faster
}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
