// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/boost/unordered.h"

#include <algorithm>
#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

#define TEST_INSERT(T)                                          \
  TEST(BoostUnordered, T##_insert) {                            \
    boost::T<std::string, std::string> map{{"key0", "value0"}}; \
    EXPECT_TRUE(map.insert({"key1", "value1"}).second);         \
    map["key2"] = "value2";                                     \
    EXPECT_TRUE(map.emplace("key3", "value3").second);          \
    EXPECT_TRUE(map.try_emplace("key4", "value4").second);      \
                                                                \
    EXPECT_TRUE(map.size() == 5);                               \
    EXPECT_FALSE(map.insert({"key1", "0"}).second);             \
    EXPECT_FALSE(map.emplace("key2", "0").second);              \
    EXPECT_FALSE(map.try_emplace("key3", "0").second);          \
  }

TEST_INSERT(unordered_flat_map)
TEST_INSERT(unordered_node_map)

#define TEST_ERASE(T)                       \
  TEST(BoostUnordered, T##_erase) {         \
    boost::T<int, std::string> map;         \
    for (int i = 0; i < 100; i++) {         \
      map[i] = "value" + std::to_string(i); \
    }                                       \
    for (int i = 0; i < 100; i += 2) {      \
      map.erase(i);                         \
    }                                       \
    int total = 0;                          \
    for (const auto& [key, _] : map) {      \
      total += key;                         \
    }                                       \
    EXPECT_TRUE(total == 2500);             \
  }

TEST_ERASE(unordered_flat_map)
TEST_ERASE(unordered_node_map)

#define TEST_COPY(T)                                \
  TEST(BoostUnordered, T##_copy) {                  \
    boost::T<int, std::string> map0;                \
    for (int i = 0; i < 100; i++) {                 \
      map0[i] = std::to_string(i);                  \
    }                                               \
                                                    \
    auto map1 = map0;                               \
    EXPECT_TRUE(map1.size() == map0.size());        \
    int total = 0;                                  \
    for (const auto& [key, value] : map1) {         \
      total += key;                                 \
      EXPECT_TRUE(key == std::atoi(value.c_str())); \
    }                                               \
    EXPECT_TRUE(total == 4950);                     \
  }

TEST_COPY(unordered_flat_map)
TEST_COPY(unordered_node_map)

#define TEST_MOVE(T)                                \
  TEST(BoostUnordered, T##_move) {                  \
    boost::T<int, std::string> map0;                \
    for (int i = 0; i < 100; i++) {                 \
      map0[i] = std::to_string(i);                  \
    }                                               \
                                                    \
    auto map1 = std::move(map0);                    \
    EXPECT_TRUE(map1.size() == 100);                \
    EXPECT_TRUE(map0.empty());                      \
    int total = 0;                                  \
    for (const auto& [key, value] : map1) {         \
      total += key;                                 \
      EXPECT_TRUE(key == std::atoi(value.c_str())); \
    }                                               \
    EXPECT_TRUE(total == 4950);                     \
  }

TEST_MOVE(unordered_flat_map)
TEST_MOVE(unordered_node_map)

#define TEST_FIND(T)                           \
  TEST(BoostUnordered, T##_find) {             \
    boost::T<int, std::string> map;            \
    for (int i = 0; i < 100; i += 2) {         \
      map[i] = std::to_string(i);              \
    }                                          \
                                               \
    for (int i = 0; i < 100; i++) {            \
      if (i % 2 == 0) {                        \
        EXPECT_TRUE(map.find(i) != map.end()); \
      } else {                                 \
        EXPECT_TRUE(map.find(i) == map.end()); \
      }                                        \
    }                                          \
  }

TEST_FIND(unordered_flat_map)
TEST_FIND(unordered_node_map)

template <class SET>
static void TestSet() {
  auto to_s = [](SET& set) -> std::string {
    std::string result;
    for (auto& i : set) {
      result += std::to_string(i);
    }
    std::sort(result.begin(), result.end());
    return result;
  };

  SET set;
  set.insert(1);
  set.insert(5);
  set.insert(3);
  set.insert(7);
  set.insert(6);
  set.insert(2);
  set.insert(4);
  auto it = set.insert(8);
  EXPECT_EQ(*it.first, 8);
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(set.insert(3).second);
  EXPECT_EQ(to_s(set), "12345678");

  EXPECT_TRUE(set.size() == 8);

  set.erase(5);
  set.erase(1);
  EXPECT_TRUE(set.size() == 6);
  EXPECT_EQ(to_s(set), "234678");

  EXPECT_TRUE(set.contains(6));
  EXPECT_FALSE(set.contains(1));
  EXPECT_FALSE(set.contains(5));

  auto find3 = set.find(3);
  auto find1 = set.find(1);
  EXPECT_EQ(*find3, 3);
  EXPECT_TRUE(find1 == set.end());

  EXPECT_EQ(to_s(set), "234678");
  set.erase(find3);
  EXPECT_EQ(to_s(set), "24678");

  set.clear();
  EXPECT_TRUE(set.empty());

  // Check functionality after clear.
  set.insert(1);
  EXPECT_TRUE(set.size() == 1);
  EXPECT_TRUE(set.contains(1));
  EXPECT_TRUE(set.find(1) != set.end());
}

template <class MAP>
static void TestMap1() {
  auto to_s = [](MAP& map) -> std::string {
    std::string result;
    for (auto& i : map) {
      result += i.second;
    }
    std::sort(result.begin(), result.end());
    return result;
  };

  MAP map;
  EXPECT_TRUE(map.empty());

  map.insert({1, "1"});
  map.insert({5, "5"});
  map.insert({3, "3"});
  map.insert({7, "7"});
  map.insert({6, "6"});
  map.insert({2, "2"});
  map.insert({4, "4"});
  auto it = map.insert({8, "8"});
  EXPECT_EQ(it.first->first, 8);
  EXPECT_EQ(it.first->second, "8");
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(map.insert({3, "3"}).second);
  EXPECT_EQ(to_s(map), "12345678");

  EXPECT_TRUE(map.size() == 8);

  typename MAP::value_type pair{0, "0"};
  map.insert(pair);
  EXPECT_EQ(to_s(map), "012345678");

  map.erase(5);
  map.erase(1);
  map.erase(1024);
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(to_s(map), "0234678");

  EXPECT_TRUE(map.contains(0));
  EXPECT_TRUE(map.contains(6));
  EXPECT_FALSE(map.contains(1));
  EXPECT_FALSE(map.contains(5));

  auto find3 = map.find(3);
  auto find1 = map.find(1);
  EXPECT_TRUE(find1 == map.end());
  EXPECT_EQ(find3->first, 3);
  EXPECT_EQ(find3->second, "3");
  find3->second = "333";
  EXPECT_EQ(to_s(map), "023334678");

  map.erase(find3);
  EXPECT_EQ(to_s(map), "024678");

  EXPECT_EQ(map[1], "");
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(to_s(map), "024678");

  map[1] = "1";
  map[5] = "5";
  map[8] = "888";
  EXPECT_TRUE(map.size() == 8);
  EXPECT_EQ(to_s(map), "0124567888");

  map.clear();
  EXPECT_TRUE(map.empty());

  // Check functionality after clear.
  map.insert({1, "1"});
  EXPECT_TRUE(map.size() == 1);
  EXPECT_TRUE(map.contains(1));
  EXPECT_TRUE(map.find(1) != map.end());
}

template <class MAP>
static void TestMap2() {
  auto to_s = [](MAP& map) -> std::string {
    std::string result;
    for (auto& i : map) {
      result += std::to_string(i.second);
    }
    std::sort(result.begin(), result.end());
    return result;
  };

  MAP map;
  EXPECT_TRUE(map.empty());

  map.insert({"1", 1});
  map.insert({"5", 5});
  map.insert({"3", 3});
  map.insert({"7", 7});
  map.insert({"6", 6});
  map.insert({"2", 2});
  map.insert({"4", 4});
  auto it = map.insert({"8", 8});
  EXPECT_EQ(it.first->first, "8");
  EXPECT_EQ(it.first->second, 8);
  EXPECT_TRUE(it.second);
  EXPECT_FALSE(map.insert({"3", 3}).second);
  EXPECT_EQ(to_s(map), "12345678");

  EXPECT_TRUE(map.size() == 8);

  typename MAP::value_type pair{"0", 0};
  map.insert(pair);
  EXPECT_EQ(to_s(map), "012345678");

  map.erase("5");
  map.erase("1");
  map.erase("abc");
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(to_s(map), "0234678");

  EXPECT_TRUE(map.contains("0"));
  EXPECT_TRUE(map.contains("6"));
  EXPECT_FALSE(map.contains("1"));
  EXPECT_FALSE(map.contains("5"));

  auto find3 = map.find("3");
  auto find1 = map.find("1");
  EXPECT_TRUE(find1 == map.end());
  EXPECT_EQ(find3->first, "3");
  EXPECT_EQ(find3->second, 3);
  find3->second = 333;
  EXPECT_EQ(to_s(map), "023334678");

  map.erase(find3);
  EXPECT_EQ(to_s(map), "024678");

  EXPECT_EQ(map["1"], 0);
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(to_s(map), "0024678");

  map["1"] = 1;
  map["5"] = 5;
  map["8"] = 888;
  EXPECT_TRUE(map.size() == 8);
  EXPECT_EQ(to_s(map), "0124567888");

  std::string key = "a";
  map[std::move(key)] = 999;
  EXPECT_EQ(to_s(map), "0124567888999");
  EXPECT_TRUE(key.empty());

  map.clear();
  EXPECT_TRUE(map.empty());

  // Check functionality after clear.
  map.insert({"1", 1});
  EXPECT_TRUE(map.size() == 1);
  EXPECT_TRUE(map.contains("1"));
  EXPECT_TRUE(map.find("1") != map.end());
}

TEST(BoostUnordered, Set) {
  TestSet<boost::unordered_flat_set<int>>();
  TestSet<boost::unordered_node_set<int>>();
}

TEST(BoostUnordered, Map) {
  TestMap1<boost::unordered_flat_map<int, std::string>>();
  TestMap1<boost::unordered_node_map<int, std::string>>();
  TestMap2<boost::unordered_flat_map<std::string, int>>();
  TestMap2<boost::unordered_node_map<std::string, int>>();
}

}  // namespace test
}  // namespace base
}  // namespace lynx
