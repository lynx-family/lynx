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

#include "base/include/boost/unordered.h"
#include "base/include/hybrid_map.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/include/vector.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

#define SEED 34862

template <class _Key, class _Tp, class _Hash = std::hash<_Key>,
          class _Pred = std::equal_to<_Key>,
          class _Alloc = std::allocator<std::pair<const _Key, _Tp>>>
using std_unordered_map = std::unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>;

template <class _Value, class _Hash = std::hash<_Value>,
          class _Pred = std::equal_to<_Value>,
          class _Alloc = std::allocator<_Value>>
using std_unordered_set = std::unordered_set<_Value, _Hash, _Pred, _Alloc>;

template <class _Key, class _Tp, class _Compare = std::less<_Key>,
          class _Allocator = std::allocator<std::pair<const _Key, _Tp>>>
using std_map = std::map<_Key, _Tp, _Compare, _Allocator>;

template <class _Key, class _Compare = std::less<_Key>,
          class _Allocator = std::allocator<_Key>>
using std_set = std::set<_Key, _Compare, _Allocator>;

template <class _Tp>
using std_shared_ptr = std::shared_ptr<_Tp>;

using std_string = std::string;

template <class Key, class T, class Hash = boost::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<const Key, T>>>
using boost_unordered_flat_map =
    boost::unordered_flat_map<Key, T, Hash, KeyEqual, Allocator>;

template <class Key, class Hash = boost::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>>
using boost_unordered_flat_set =
    boost::unordered_flat_set<Key, Hash, KeyEqual, Allocator>;

namespace boost {
template <>
struct hash<lynx::base::String> {
  std::size_t operator()(lynx::base::String const& val) const {
    return val.hash();
  }
};
}  // namespace boost

struct rand_base_string {
  rand_base_string(uint32_t length)
      : dist(0, sizeof(chrs) - 2), gen(SEED), length(length) {}
  lynx::base::String operator()() {
    uint32_t cnt = length;
    std::string s;
    s.reserve(cnt);
    while (cnt--) s += chrs[dist(gen)];
    return lynx::base::String(std::move(s));
  }

 private:
  inline static auto& chrs =
      "0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::uniform_int_distribution<std::string::size_type> dist;
  std::mt19937 gen;
  uint32_t length;
};

static rand_base_string rnd_gen_base_string(10);

using namespace lynx::base;   // NOLINT
using namespace lynx::lepus;  // NOLINT

namespace lynx {
namespace base {

using SKeyPolicy = KeyPolicy<String>;
template <class K, class T, class KeyPolicy>
using LinearFlatMap2 = LinearFlatMap<K, T, KeyPolicy>;

// Policy of <base::String, lepus::Value> maps the same as lepus::Dictionary
using SVHybridMapSmallMapPolicy = typename Dictionary::SmallMapPolicy;
using SVHybridMapBigMapPolicy = typename Dictionary::BigMapPolicy;
using SVHybridMapTransferPolicy = typename Dictionary::PlainBytesTransferPolicy;
using SVHybridMapIteratorPolicy = typename Dictionary::IteratorPolicy;
static constexpr auto kSVHybridMapSmallMaxMaxSize =
    Dictionary::kSmallMapMaximumSize;

template <typename Key, typename T, size_t MaxSmallMapSize,
          typename SmallMapPolicy, typename BigMapPolicy,
          typename TransferPolicy = DefaultTransferPolicy,
          typename IteratorPolicy =
              DefaultIteratorPolicy<Key, T, SmallMapPolicy, BigMapPolicy>>
using HybridMap2 = HybridMap<Key, T, MaxSmallMapSize, SmallMapPolicy,
                             BigMapPolicy, TransferPolicy, IteratorPolicy>;

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
std_string Cast(size_t v) {
  return rnd_gen_base_string().str();

  // if (v % 2 == 0) {
  //   return "string_" + std::to_string(v);
  // } else {
  //   return std::to_string(v) + "_string";
  // }
}

template <>
std_shared_ptr<std_string> Cast(size_t v) {
  return std::make_shared<std_string>(std::to_string(v));
}

template <>
String Cast(size_t v) {
  return rnd_gen_base_string();

  // if (v % 2 == 0) {
  //   return String(std_string("string_") + std::to_string(v));
  // } else {
  //   return String(std::to_string(v) + std_string("_string"));
  // }
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
#define FOREACH_STRINGIFY_5(a, ...) #a ", " FOREACH_STRINGIFY_4(__VA_ARGS__)
#define FOREACH_STRINGIFY_6(a, ...) #a ", " FOREACH_STRINGIFY_5(__VA_ARGS__)
#define FOREACH_STRINGIFY_7(a, ...) #a ", " FOREACH_STRINGIFY_6(__VA_ARGS__)
#define FOREACH_STRINGIFY_8(a, ...) #a ", " FOREACH_STRINGIFY_7(__VA_ARGS__)
#define FOREACH_STRINGIFY_N(_8, _7, _6, _5, _4, _3, _2, _1, N, ...) \
  FOREACH_STRINGIFY##N
#define FOREACH_STRINGIFY(...)                                     \
  FOREACH_STRINGIFY_N(__VA_ARGS__, _8, _7, _6, _5, _4, _3, _2, _1) \
  (__VA_ARGS__)

#define TEST_FUNC_SET_INSERT(TYPE, DATA_COUNT, SET, ...)                   \
  static void BM_##SET##_insert_##DATA_COUNT##_##TYPE(                     \
      benchmark::State& state) {                                           \
    state.SetLabel(STRINGIFY(SET) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using SetType = SET<__VA_ARGS__>;                                      \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    constexpr size_t kDestructionBatchCount = 50;                          \
    std::vector<SetType::key_type> data;                                   \
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
    volatile size_t total = 0;                                             \
    for (auto _ : state) {                                                 \
      SetType* sets[kDestructionBatchCount];                               \
      for (size_t i = 0; i < kDestructionBatchCount; i++) {                \
        sets[i] = new SetType();                                           \
        for (const auto& key : data) {                                     \
          sets[i]->insert(key);                                            \
        }                                                                  \
        total += sets[i]->size();                                          \
      }                                                                    \
      state.PauseTiming();                                                 \
      for (size_t i = 0; i < kDestructionBatchCount; i++) {                \
        delete sets[i]; /* destruction time not measured */                \
      }                                                                    \
      state.ResumeTiming();                                                \
    }                                                                      \
  }

template <bool RESERVE, typename T>
void CallReserve(T& obj, size_t size) {
  if constexpr (RESERVE) {
    obj.reserve(size);
  }
}

#define TEST_FUNC_MAP_INSERT(TYPE, DATA_COUNT, RESERVE, MAP, ...)          \
  static void BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_##TYPE(         \
      benchmark::State& state) {                                           \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">" + \
                   (RESERVE ? std::string(" reserve") : std::string()));   \
    using MapType = MAP<__VA_ARGS__>;                                      \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    constexpr size_t kDestructionBatchCount = 50;                          \
    std::vector<std::pair<MapType::key_type, MapType::mapped_type>> data;  \
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
    volatile size_t total = 0;                                             \
    for (auto _ : state) {                                                 \
      MapType* maps[kDestructionBatchCount];                               \
      for (size_t i = 0; i < kDestructionBatchCount; i++) {                \
        maps[i] = new MapType();                                           \
        auto& m = *maps[i];                                                \
        CallReserve<RESERVE>(m, kDataCount);                               \
        for (auto it = data.begin(); it != data.end(); it++) {             \
          m[it->first] = it->second;                                       \
        }                                                                  \
        total += m.size();                                                 \
      }                                                                    \
      state.PauseTiming();                                                 \
      for (size_t i = 0; i < kDestructionBatchCount; i++) {                \
        delete maps[i]; /* destruction time not measured */                \
      }                                                                    \
      state.ResumeTiming();                                                \
    }                                                                      \
  }

#if 1
template <typename MAP, typename K>
inline bool map_contains(const MAP& map, const K& key) {
  if constexpr (has_contains_method_v<MAP>) {
    return map.contains(key);
  } else {
    return map.find(key) != map.end();
  }
}
#else
template <typename MAP, typename K>
inline bool map_contains(const MAP& map, const K& key) {
  return map.find(key) != map.end();
}
#endif

#define TEST_FUNC_MAP_FIND(TYPE, DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)        \
  static void BM_##MAP##_find_##DATA_COUNT##_##TYPE(benchmark::State& state) { \
    constexpr size_t kDataCount = DATA_COUNT;                                  \
    constexpr size_t kNotFoundCount = NOT_FOUND_COUNT;                         \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">");     \
    using MapType = MAP<__VA_ARGS__>;                                          \
    std::vector<std::pair<MapType::key_type, MapType::mapped_type>> data;      \
    std::vector<std::pair<MapType::key_type, MapType::mapped_type>> find_data; \
                                                                               \
    /* Generate data. */                                                       \
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
    for (auto it = data.begin(); it != data.end(); it++) {                     \
      map[it->first] = it->second;                                             \
    }                                                                          \
                                                                               \
    auto rng = std::default_random_engine{};                                   \
    std::shuffle(std::begin(find_data), std::end(find_data), rng);             \
                                                                               \
    volatile size_t total = 0;                                                 \
    for (auto _ : state) {                                                     \
      for (auto it = find_data.begin(); it != find_data.end(); it++) {         \
        if (map_contains(map, it->first)) {                                    \
          total++;                                                             \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

#define TEST_FUNC_MAP_ITERATE(TYPE, DATA_COUNT, MAP, ...)                  \
  static void BM_##MAP##_iterate_##DATA_COUNT##_##TYPE(                    \
      benchmark::State& state) {                                           \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using MapType = MAP<__VA_ARGS__>;                                      \
    std::vector<std::pair<MapType::key_type, MapType::mapped_type>> data;  \
                                                                           \
    /* Generate data. */                                                   \
    data.resize(kDataCount);                                               \
    for (size_t i = 0; i < kDataCount; i++) {                              \
      data[i].first = Cast<MapType::key_type>(i);                          \
      data[i].second = Cast<MapType::mapped_type>(i);                      \
    }                                                                      \
                                                                           \
    MapType map;                                                           \
    for (auto it = data.begin(); it != data.end(); it++) {                 \
      map[it->first] = it->second;                                         \
    }                                                                      \
                                                                           \
    volatile size_t total = 0;                                             \
    for (auto _ : state) {                                                 \
      for (const auto& pair : map) {                                       \
        /* Accessing the first byte of the key and value. */               \
        total += *(unsigned char*)(&pair.first);                           \
        total += *(unsigned char*)(&pair.second);                          \
      }                                                                    \
    }                                                                      \
  }

#define TEST_FUNC_MAP_ITERATE_FOREACH(TYPE, DATA_COUNT, MAP, ...)          \
  static void BM_##MAP##_iterate_foreach_##DATA_COUNT##_##TYPE(            \
      benchmark::State& state) {                                           \
    constexpr size_t kDataCount = DATA_COUNT;                              \
    state.SetLabel(STRINGIFY(MAP) "<" FOREACH_STRINGIFY(__VA_ARGS__) ">"); \
    using MapType = MAP<__VA_ARGS__>;                                      \
    std::vector<std::pair<MapType::key_type, MapType::mapped_type>> data;  \
                                                                           \
    /* Generate data. */                                                   \
    data.resize(kDataCount);                                               \
    for (size_t i = 0; i < kDataCount; i++) {                              \
      data[i].first = Cast<MapType::key_type>(i);                          \
      data[i].second = Cast<MapType::mapped_type>(i);                      \
    }                                                                      \
                                                                           \
    MapType map;                                                           \
    for (auto it = data.begin(); it != data.end(); it++) {                 \
      map[it->first] = it->second;                                         \
    }                                                                      \
                                                                           \
    volatile size_t total = 0;                                             \
    for (auto _ : state) {                                                 \
      map.for_each([&](const auto& key, auto& value) {                     \
        /* Accessing the first byte of the key and value. */               \
        total += *(unsigned char*)(&key);                                  \
        total += *(unsigned char*)(&value);                                \
      });                                                                  \
    }                                                                      \
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

#define TEST_MAP_INSERT_II(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(II, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_II)

#define TEST_MAP_INSERT_IR(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(IR, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_IR)

#define TEST_MAP_INSERT_ss(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ss, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_ss)

#define TEST_MAP_INSERT_ISP(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(ISP, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_ISP)

#define TEST_MAP_INSERT_sSP(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(sSP, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_sSP)

#define TEST_MAP_INSERT_SSP(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SSP, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_SSP)

#define TEST_MAP_INSERT_SV(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SV, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_SV)

#define TEST_MAP_INSERT_IV(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(IV, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_IV)

#define TEST_MAP_INSERT_SS(DATA_COUNT, RESERVE, MAP, ...)         \
  TEST_FUNC_MAP_INSERT(SS, DATA_COUNT, RESERVE, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_insert_##DATA_COUNT##_##RESERVE##_SS)

#if 0
/**
 * <int>
 * data count
 *   0~48: LinearFlatSet best
 *   48~1024: OrderedFlatSet best
 *   > 1024: std::unordered_set best
 */
TEST_SET_INSERT_I(8, std_set, int);
TEST_SET_INSERT_I(8, std_unordered_set, int);
TEST_SET_INSERT_I(8, boost_unordered_flat_set, int);
TEST_SET_INSERT_I(8, OrderedFlatSet, int);
TEST_SET_INSERT_I(8, LinearFlatSet, int);

TEST_SET_INSERT_I(48, std_set, int);
TEST_SET_INSERT_I(48, std_unordered_set, int);
TEST_SET_INSERT_I(48, boost_unordered_flat_set, int);
TEST_SET_INSERT_I(48, OrderedFlatSet, int);
TEST_SET_INSERT_I(48, LinearFlatSet, int);

TEST_SET_INSERT_I(1024, std_set, int);
TEST_SET_INSERT_I(1024, std_unordered_set, int);
TEST_SET_INSERT_I(1024, boost_unordered_flat_set, int);
TEST_SET_INSERT_I(1024, OrderedFlatSet, int);
TEST_SET_INSERT_I(1024, LinearFlatSet, int);

/**
 * <std::string>
 * data count
 *   0~26: LinearFlatSet best
 *   == 15: OrderedFlatSet == unordered_set
 *   > 26: std::unordered_set best
 */
TEST_SET_INSERT_s(8, std_set, std_string);
TEST_SET_INSERT_s(8, std_unordered_set, std_string);
TEST_SET_INSERT_s(8, OrderedFlatSet, std_string);
TEST_SET_INSERT_s(8, LinearFlatSet, std_string);

TEST_SET_INSERT_s(15, std_set, std_string);
TEST_SET_INSERT_s(15, std_unordered_set, std_string);
TEST_SET_INSERT_s(15, OrderedFlatSet, std_string);
TEST_SET_INSERT_s(15, LinearFlatSet, std_string);

TEST_SET_INSERT_s(26, std_set, std_string);
TEST_SET_INSERT_s(26, std_unordered_set, std_string);
TEST_SET_INSERT_s(26, OrderedFlatSet, std_string);
TEST_SET_INSERT_s(26, LinearFlatSet, std_string);

/**
 * <base::string>
 * data count
 *   0~48: LinearFlatSet best
 *   == 15: OrderedFlatSet == unordered_set
 *   > 48: std::unordered_set best
 */
TEST_SET_INSERT_S(8, std_set, String);
TEST_SET_INSERT_S(8, std_unordered_set, String);
TEST_SET_INSERT_S(8, OrderedFlatSet, String);
TEST_SET_INSERT_S(8, LinearFlatSet, String);

TEST_SET_INSERT_S(15, std_set, String);
TEST_SET_INSERT_S(15, std_unordered_set, String);
TEST_SET_INSERT_S(15, OrderedFlatSet, String);
TEST_SET_INSERT_S(15, LinearFlatSet, String);

TEST_SET_INSERT_S(48, std_set, String);
TEST_SET_INSERT_S(48, std_unordered_set, String);
TEST_SET_INSERT_S(48, OrderedFlatSet, String);
TEST_SET_INSERT_S(48, LinearFlatSet, String);

/**
 * <int, int>
 * data count
 *   0~52: LinearFlatMap best
 *   52~720: OrderedFlatMap best
 *   > 720: std::unordered_map best
 */
TEST_MAP_INSERT_II(16, false, std_map, int, int);
TEST_MAP_INSERT_II(16, false, std_unordered_map, int, int);
TEST_MAP_INSERT_II(16, false, boost_unordered_flat_map, int, int);
TEST_MAP_INSERT_II(16, false, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(16, false, LinearFlatMap, int, int);

TEST_MAP_INSERT_II(32, false, std_map, int, int);
TEST_MAP_INSERT_II(32, false, std_unordered_map, int, int);
TEST_MAP_INSERT_II(32, false, boost_unordered_flat_map, int, int);
TEST_MAP_INSERT_II(32, false, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(32, false, LinearFlatMap, int, int);

TEST_MAP_INSERT_II(64, false, std_map, int, int);
TEST_MAP_INSERT_II(64, false, std_unordered_map, int, int);
TEST_MAP_INSERT_II(64, false, boost_unordered_flat_map, int, int);
TEST_MAP_INSERT_II(64, false, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(64, false, LinearFlatMap, int, int);

TEST_MAP_INSERT_II(128, false, std_map, int, int);
TEST_MAP_INSERT_II(128, false, std_unordered_map, int, int);
TEST_MAP_INSERT_II(128, false, boost_unordered_flat_map, int, int);
TEST_MAP_INSERT_II(128, false, OrderedFlatMap, int, int);
TEST_MAP_INSERT_II(128, false, LinearFlatMap, int, int);

/**
 * <int, Range>
 * data count
 *   0~64: LinearFlatMap best
 *   64~400: OrderedFlatMap best
 *   > 400: std::unordered_map best
 */
TEST_MAP_INSERT_IR(8, false, std_map, int, Range);
TEST_MAP_INSERT_IR(8, false, std_unordered_map, int, Range);
TEST_MAP_INSERT_IR(8, false, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(8, false, LinearFlatMap, int, Range);

TEST_MAP_INSERT_IR(64, false, std_map, int, Range);
TEST_MAP_INSERT_IR(64, false, std_unordered_map, int, Range);
TEST_MAP_INSERT_IR(64, false, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(64, false, LinearFlatMap, int, Range);

TEST_MAP_INSERT_IR(400, false, std_map, int, Range);
TEST_MAP_INSERT_IR(400, false, std_unordered_map, int, Range);
TEST_MAP_INSERT_IR(400, false, OrderedFlatMap, int, Range);
TEST_MAP_INSERT_IR(400, false, LinearFlatMap, int, Range);

/**
 * <std::string, std::string>
 * data count
 *   0~24: LinearFlatMap best
 *   > 24: std::unordered_map best
 */
TEST_MAP_INSERT_ss(8, false, std_map, std_string, std_string);
TEST_MAP_INSERT_ss(8, false, std_unordered_map, std_string, std_string);
TEST_MAP_INSERT_ss(8, false, OrderedFlatMap, std_string, std_string);
TEST_MAP_INSERT_ss(8, false, LinearFlatMap, std_string, std_string);

TEST_MAP_INSERT_ss(24, false, std_map, std_string, std_string);
TEST_MAP_INSERT_ss(24, false, std_unordered_map, std_string, std_string);
TEST_MAP_INSERT_ss(24, false, OrderedFlatMap, std_string, std_string);
TEST_MAP_INSERT_ss(24, false, LinearFlatMap, std_string, std_string);

/**
 * <int, std::shared_ptr<string>>
 * data count
 *   0~128: LinearFlatMap best
 *   =40: std::unordered_map == OrderedFlatMap
 *   > 128: std::unordered_map best
 */
TEST_MAP_INSERT_ISP(8, false, std_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, false, std_unordered_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, false, OrderedFlatMap, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(8, false, LinearFlatMap, int, std_shared_ptr<string>);

TEST_MAP_INSERT_ISP(48, false, std_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, false, std_unordered_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, false, OrderedFlatMap, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(48, false, LinearFlatMap, int, std_shared_ptr<string>);

TEST_MAP_INSERT_ISP(128, false, std_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, false, std_unordered_map, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, false, OrderedFlatMap, int, std_shared_ptr<string>);
TEST_MAP_INSERT_ISP(128, false, LinearFlatMap, int, std_shared_ptr<string>);

/**
 * <std::string, std::shared_ptr<string>>
 * data count
 *   0~32: LinearFlatMap best
 *   > 32: std::unordered_map best
 */
TEST_MAP_INSERT_sSP(8, false, std_map, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, false, std_unordered_map, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, false, OrderedFlatMap, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(8, false, LinearFlatMap, std_string, std_shared_ptr<string>);

TEST_MAP_INSERT_sSP(32, false, std_map, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, false, std_unordered_map, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, false, OrderedFlatMap, std_string, std_shared_ptr<string>);
TEST_MAP_INSERT_sSP(32, false, LinearFlatMap, std_string, std_shared_ptr<string>);

/**
 * <base::String, std::shared_ptr<string>>
 * data count
 *   0~48: LinearFlatMap best
 *   > 48: std::unordered_map best
 */
TEST_MAP_INSERT_SSP(8, false, std_map, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, false, std_unordered_map, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, false, OrderedFlatMap, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(8, false, LinearFlatMap, String, std_shared_ptr<string>);

TEST_MAP_INSERT_SSP(48, false, std_map, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, false, std_unordered_map, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, false, OrderedFlatMap, String, std_shared_ptr<string>);
TEST_MAP_INSERT_SSP(48, false, LinearFlatMap, String, std_shared_ptr<string>);
#endif

#if 0
TEST_MAP_INSERT_SV(4, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(4, false, LinearFlatMap, String, Value);

TEST_MAP_INSERT_SV(8, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(8, false, LinearFlatMap, String, Value);

TEST_MAP_INSERT_SV(16, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(16, false, LinearFlatMap, String, Value);

TEST_MAP_INSERT_SV(32, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(32, false, LinearFlatMap, String, Value);
#elif 0
/**
 * <base::String, base::Value>
 * data count
 *   0~32: LinearFlatMap best
 *   > 32: std::unordered_map best
 */
// TEST_MAP_INSERT_SV(2, false, std_map, String, Value);
TEST_MAP_INSERT_SV(2, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(2, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(2, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(2, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(2, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(2, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(4, false, std_map, String, Value);
TEST_MAP_INSERT_SV(4, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(4, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(4, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(4, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(4, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(4, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(8, false, std_map, String, Value);
TEST_MAP_INSERT_SV(8, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(8, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(8, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(8, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(8, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(8, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(12, false, std_map, String, Value);
TEST_MAP_INSERT_SV(12, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(12, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(12, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(12, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(12, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(12, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(16, false, std_map, String, Value);
TEST_MAP_INSERT_SV(16, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(16, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(16, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(16, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(16, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(16, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(24, false, std_map, String, Value);
TEST_MAP_INSERT_SV(24, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(24, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(24, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(24, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(24, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(24, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(32, false, std_map, String, Value);
TEST_MAP_INSERT_SV(32, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(32, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(32, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(32, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(32, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(32, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(48, false, std_map, String, Value);
TEST_MAP_INSERT_SV(48, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(48, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(48, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(48, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(48, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(48, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(72, false, std_map, String, Value);
TEST_MAP_INSERT_SV(72, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(72, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(72, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(72, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(72, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(72, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);

// TEST_MAP_INSERT_SV(96, false, std_map, String, Value);
TEST_MAP_INSERT_SV(96, false, std_unordered_map, String, Value);
TEST_MAP_INSERT_SV(96, true, std_unordered_map, String, Value);
// TEST_MAP_INSERT_SV(96, false, OrderedFlatMap, String, Value);
// TEST_MAP_INSERT_SV(96, false, LinearFlatMap, String, Value);
TEST_MAP_INSERT_SV(96, false, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
TEST_MAP_INSERT_SV(96, true, HybridMap, String, Value,
                   kSVHybridMapSmallMaxMaxSize, SVHybridMapSmallMapPolicy,
                   SVHybridMapBigMapPolicy, SVHybridMapTransferPolicy);
#endif

#if 0
/**
 * <int, base::Value>
 * data count
 *   0~128: LinearFlatMap best
 *   > 128: std::unordered_map best
 */
TEST_MAP_INSERT_IV(8, false, std_map, int, Value);
TEST_MAP_INSERT_IV(8, false, std_unordered_map, int, Value);
TEST_MAP_INSERT_IV(8, false, OrderedFlatMap, int, Value);
TEST_MAP_INSERT_IV(8, false, LinearFlatMap, int, Value);

TEST_MAP_INSERT_IV(128, false, std_map, int, Value);
TEST_MAP_INSERT_IV(128, false, std_unordered_map, int, Value);
TEST_MAP_INSERT_IV(128, false, OrderedFlatMap, int, Value);
TEST_MAP_INSERT_IV(128, false, LinearFlatMap, int, Value);

/**
 * <base::String, base::String>
 * data count
 *   0~40: LinearFlatMap best
 *   > 40: std::unordered_map best
 */
TEST_MAP_INSERT_SS(8, false, std_map, String, String);
TEST_MAP_INSERT_SS(8, false, std_unordered_map, String, String);
TEST_MAP_INSERT_SS(8, false, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(8, false, LinearFlatMap, String, String);

TEST_MAP_INSERT_SS(24, false, std_map, String, String);
TEST_MAP_INSERT_SS(24, false, std_unordered_map, String, String);
TEST_MAP_INSERT_SS(24, false, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(24, false, LinearFlatMap, String, String);

TEST_MAP_INSERT_SS(40, false, std_map, String, String);
TEST_MAP_INSERT_SS(40, false, std_unordered_map, String, String);
TEST_MAP_INSERT_SS(40, false, OrderedFlatMap, String, String);
TEST_MAP_INSERT_SS(40, false, LinearFlatMap, String, String);

#define TEST_MAP_FIND_I(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(I, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_I)

#define TEST_MAP_FIND_s(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(s, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_s)
#endif

#define TEST_MAP_FIND_S(DATA_COUNT, NOT_FOUND_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_FIND(S, DATA_COUNT, NOT_FOUND_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_find_##DATA_COUNT##_S)
#if 0
TEST_MAP_FIND_I(16, 2, std_map, int, int);
TEST_MAP_FIND_I(16, 2, std_unordered_map, int, int);
TEST_MAP_FIND_I(16, 2, boost_unordered_flat_map, int, int);
TEST_MAP_FIND_I(16, 2, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(16, 2, LinearFlatMap, int, int);

TEST_MAP_FIND_I(100, 16, std_map, int, int);
TEST_MAP_FIND_I(100, 16, std_unordered_map, int, int);
TEST_MAP_FIND_I(100, 16, boost_unordered_flat_map, int, int);
TEST_MAP_FIND_I(100, 16, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(100, 16, LinearFlatMap, int, int);

TEST_MAP_FIND_I(2, 1, std_map, int, int);
TEST_MAP_FIND_I(2, 1, std_unordered_map, int, int);  // std::unordered_map is faster
TEST_MAP_FIND_I(2, 1, boost_unordered_flat_map, int, int);
TEST_MAP_FIND_I(2, 1, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(2, 1, LinearFlatMap, int, int);

TEST_MAP_FIND_I(12, 2, std_map, int, int);
TEST_MAP_FIND_I(12, 2, std_unordered_map, int, int);
TEST_MAP_FIND_I(12, 2, boost_unordered_flat_map, int, int);
TEST_MAP_FIND_I(12, 2, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(12, 2, LinearFlatMap, int,
                int);  // 12 equivalent to OrderedFlatMap

TEST_MAP_FIND_I(128, 16, std_map, int, int);
TEST_MAP_FIND_I(128, 16, std_unordered_map, int, int);
TEST_MAP_FIND_I(128, 16, boost_unordered_flat_map, int, int);
TEST_MAP_FIND_I(128, 16, OrderedFlatMap, int, int);
TEST_MAP_FIND_I(128, 16, LinearFlatMap, int, int);

TEST_MAP_FIND_s(3, 1, std_map, std_string, std_string);
TEST_MAP_FIND_s(3, 1, std_unordered_map, std_string, std_string);
TEST_MAP_FIND_s(3, 1, OrderedFlatMap, std_string, std_string);
TEST_MAP_FIND_s(
    3, 1, LinearFlatMap, std_string,
    string);  // equivalent to std::std_unordered_map, faster than OrderedFlatMap

// If most keys share the same prefix substring, LinearFlatMap scores would be
// worse
TEST_MAP_FIND_s(30, 4, std_map, std_string, std_string);
TEST_MAP_FIND_s(30, 4, std_unordered_map, std_string, std_string);
TEST_MAP_FIND_s(30, 4, OrderedFlatMap, std_string, std_string);
TEST_MAP_FIND_s(
    30, 4, LinearFlatMap, std_string,
    string);  // 30 equivalent to ordered, less than 30 LinearFlatMap is faster

TEST_MAP_FIND_S(2, 1, std_map, String, String);
TEST_MAP_FIND_S(2, 1, std_unordered_map, String, String);
TEST_MAP_FIND_S(2, 1, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(
    2, 1, LinearFlatMap, String,
    String);  // equivalent to std::std_unordered_map, faster than OrderedFlatMap

TEST_MAP_FIND_S(6, 1, std_map, String, String);
TEST_MAP_FIND_S(6, 1, std_unordered_map, String, String);
TEST_MAP_FIND_S(6, 1, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(6, 1, LinearFlatMap, String,
                String);  // a little slower than std::std_unordered_map, faster
                          // than OrderedFlatMap

// If most keys share the same prefix substring, LinearFlatMap scores would be
// worse
TEST_MAP_FIND_S(80, 4, std_map, String, String);
TEST_MAP_FIND_S(80, 4, std_unordered_map, String, String);
TEST_MAP_FIND_S(80, 4, OrderedFlatMap, String, String);
TEST_MAP_FIND_S(
    80, 4, LinearFlatMap, String,
    String);  // 80 equivalent to ordered, less than 80 LinearFlatMap is faster
#elif 0
TEST_MAP_FIND_S(2, 0, std_unordered_map, String, String);
TEST_MAP_FIND_S(2, 0, LinearFlatMap, String, String);
TEST_MAP_FIND_S(2, 0, LinearFlatMap2, String, String, SKeyPolicy);

TEST_MAP_FIND_S(4, 1, std_unordered_map, String, String);
TEST_MAP_FIND_S(4, 1, LinearFlatMap, String, String);
TEST_MAP_FIND_S(4, 1, LinearFlatMap2, String, String, SKeyPolicy);

TEST_MAP_FIND_S(8, 1, std_unordered_map, String, String);
TEST_MAP_FIND_S(8, 1, LinearFlatMap, String, String);
TEST_MAP_FIND_S(8, 1, LinearFlatMap2, String, String, SKeyPolicy);

TEST_MAP_FIND_S(16, 2, std_unordered_map, String, String);
TEST_MAP_FIND_S(16, 2, LinearFlatMap, String, String);
TEST_MAP_FIND_S(16, 2, LinearFlatMap2, String, String, SKeyPolicy);
#endif

#if 1
TEST_MAP_FIND_S(2, 1, std_unordered_map, String, Value);
TEST_MAP_FIND_S(2, 1, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(2, 1, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(4, 1, std_unordered_map, String, Value);
TEST_MAP_FIND_S(4, 1, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(4, 1, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(6, 1, std_unordered_map, String, Value);
TEST_MAP_FIND_S(6, 1, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(6, 1, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(8, 1, std_unordered_map, String, Value);
TEST_MAP_FIND_S(8, 1, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(8, 1, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(10, 2, std_unordered_map, String, Value);
TEST_MAP_FIND_S(10, 2, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(10, 2, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(12, 2, std_unordered_map, String, Value);
TEST_MAP_FIND_S(12, 2, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(12, 2, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(16, 3, std_unordered_map, String, Value);
TEST_MAP_FIND_S(16, 3, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(16, 3, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(32, 4, std_unordered_map, String, Value);
TEST_MAP_FIND_S(32, 4, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(32, 4, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(64, 8, std_unordered_map, String, Value);
TEST_MAP_FIND_S(64, 8, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(64, 8, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

TEST_MAP_FIND_S(96, 12, std_unordered_map, String, Value);
TEST_MAP_FIND_S(96, 12, boost_unordered_flat_map, String, Value);
TEST_MAP_FIND_S(96, 12, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy);

#define TEST_MAP_ITERATE_SV(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_ITERATE(SV, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_iterate_##DATA_COUNT##_SV)

#define TEST_MAP_ITERATE_FOREACH_SV(DATA_COUNT, MAP, ...)         \
  TEST_FUNC_MAP_ITERATE_FOREACH(SV, DATA_COUNT, MAP, __VA_ARGS__) \
  BENCHMARK(BM_##MAP##_iterate_foreach_##DATA_COUNT##_SV)

TEST_MAP_ITERATE_SV(8, std_unordered_map, String, Value);
TEST_MAP_ITERATE_SV(8, boost_unordered_flat_map, String, Value);
TEST_MAP_ITERATE_SV(8, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                    SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                    SVHybridMapTransferPolicy);
TEST_MAP_ITERATE_SV(8, HybridMap2, String, Value, kSVHybridMapSmallMaxMaxSize,
                    SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                    SVHybridMapTransferPolicy, SVHybridMapIteratorPolicy);
TEST_MAP_ITERATE_FOREACH_SV(8, HybridMap2, String, Value,
                            kSVHybridMapSmallMaxMaxSize,
                            SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                            SVHybridMapTransferPolicy,
                            SVHybridMapIteratorPolicy);

TEST_MAP_ITERATE_SV(16, std_unordered_map, String, Value);
TEST_MAP_ITERATE_SV(16, boost_unordered_flat_map, String, Value);
TEST_MAP_ITERATE_SV(16, HybridMap, String, Value, kSVHybridMapSmallMaxMaxSize,
                    SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                    SVHybridMapTransferPolicy);
TEST_MAP_ITERATE_SV(16, HybridMap2, String, Value, kSVHybridMapSmallMaxMaxSize,
                    SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                    SVHybridMapTransferPolicy, SVHybridMapIteratorPolicy);
TEST_MAP_ITERATE_FOREACH_SV(16, HybridMap2, String, Value,
                            kSVHybridMapSmallMaxMaxSize,
                            SVHybridMapSmallMapPolicy, SVHybridMapBigMapPolicy,
                            SVHybridMapTransferPolicy,
                            SVHybridMapIteratorPolicy);
#endif

}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
