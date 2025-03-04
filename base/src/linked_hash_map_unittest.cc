// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/linked_hash_map.h"

#include <set>
#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

TEST(LinkedHashMap, insert) {
  LinkedHashMap<std::string, std::string> map;
  map.insert_or_assign(std::string("key4"), std::string("value4"));
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2222"));
  EXPECT_EQ(map.find("key2")->second, "value2222");
  const std::string value2 = "value2";
  EXPECT_FALSE(map.insert_or_assign(std::string("key2"), value2).second);
  EXPECT_EQ(map.find("key2")->second, value2);
  map[std::string("key1")] = std::string("value1111");
  EXPECT_EQ(map.find("key1")->second, "value1111");
  map[std::string("key1")] = std::string("value1");
  EXPECT_EQ(map.find("key1")->second, "value1");
  int idx = 4;
  for (auto it = map.begin(); it != map.end(); it++) {
    EXPECT_EQ(it->first, std::string("key") + std::to_string(idx));
    EXPECT_EQ(it->second, std::string("value") + std::to_string(idx));
    idx--;
  }
  EXPECT_TRUE(map.size() == 4);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, insert_if_absent) {
  LinkedHashMap<std::string, std::string> map;
  const std::string value2222 = "value2222";
  map.insert_if_absent(std::string("key4"), std::string("value4"));
  map.insert_if_absent(std::string("key3"), std::string("value3"));
  map.insert_if_absent(std::string("key2"), value2222);
  EXPECT_TRUE(map.size() == 3);
  const std::string key2 = "key2";
  std::string value2 = "value2";
  EXPECT_EQ(map.find(key2)->second, value2222);
  EXPECT_FALSE(map.insert_if_absent(key2, value2).second);
  EXPECT_EQ(map.find(key2)->second, value2222);
  EXPECT_FALSE(map.insert_if_absent(key2, std::move(value2)).second);
  EXPECT_EQ(map.find(key2)->second, value2222);
  EXPECT_EQ(value2, "value2");  // value2 not moved
  EXPECT_TRUE(map.insert_if_absent("key5", std::move(value2)).second);
  EXPECT_TRUE(value2.empty());  // value2 is moved
  EXPECT_TRUE(map.size() == 4);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, operator_bracket) {
  LinkedHashMap<std::string, std::string> map;
  map["abc"] = "123";
  std::string abc = "abc";
  std::string xyz = "xyz";
  EXPECT_EQ(map[abc], "123");
  map[std::move(abc)] = "321";
  map[std::move(xyz)] = "456";
  EXPECT_FALSE(abc.empty());
  EXPECT_TRUE(xyz.empty());
  EXPECT_EQ(map["abc"], "321");
  EXPECT_EQ(map["xyz"], "456");
}

TEST(LinkedHashMap, emplace) {
  LinkedHashMap<std::string, std::string> map;

  std::string key1 = "key1";
  map.emplace_or_assign(key1, "value1_abc_123", 6);

  std::string key2 = "key2";
  map[key2] = "value2";
  map.emplace_or_assign(key2, 5, 'v');

  EXPECT_EQ(map[key1], "value1");
  EXPECT_EQ(map[key2], "vvvvv");

  map.emplace_or_assign(std::move(key1), std::move(key2));

  EXPECT_TRUE(key2.empty());
  EXPECT_EQ(map["key1"], "key2");
}

TEST(LinkedHashMap, range_insert) {
  // insert x values in vector, range insert x-15 values from vector to map,
  // check values
  const int nb_values = 1000;
  std::vector<std::pair<int, int>> values;
  for (int i = 0; i < nb_values; i++) {
    values.push_back(std::make_pair(i, i + 1));
  }

  LinkedHashMap<int, int> map;
  map[-1] = 1;
  map[-2] = 2;
  map.insert(values.begin() + 10, values.end() - 5);

  EXPECT_EQ(map.size(), static_cast<size_t>(987));
  EXPECT_EQ(map.at(-1), 1);
  for (int i = 10, j = 2; i < nb_values - 5; i++, j++) {
    EXPECT_EQ(map.at(i), i + 1);
  }
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  map.erase(-2);
  map.erase(99);
  map.erase(199);
  EXPECT_EQ(map.size(), static_cast<size_t>(984));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, duplicated_in_initializer) {
  LinkedHashMap<std::string, int> map = {
      {"Key2", 2}, {"Key4", 4},   {"Key6", 6},   {"Key8", 8},
      {"Key9", 9}, {"Key10", 10}, {"Key11", 11}, {"Key2", 12}};
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(map["Key2"], 12);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, position_order) {
  LinkedHashMap<std::string, int> map = {
      {"Key2", 2}, {"Key4", 4},   {"Key6", 6},   {"Key8", 8},
      {"Key9", 9}, {"Key10", 10}, {"Key11", 11}, {"Key12", 12}};

  map.insert_or_assign(std::string("Key1"), 1);
  map.insert_or_assign(std::string("Key3"), 3);
  map.insert_or_assign(std::string("Key5"), 5);
  EXPECT_EQ(
      decltype(map)::Testing::count_of_nodes_on_pool(map),
      std::min<size_t>(
          map.size(),
          std::max<size_t>((size_t)8, decltype(map)::kInitialAllocationSize)));

  const std::vector<std::pair<std::string, int>> vector_values = {
      {"Key2", 2}, {"Key4", 4},   {"Key6", 6},   {"Key8", 8},
      {"Key9", 9}, {"Key10", 10}, {"Key11", 11}, {"Key12", 12},
      {"Key1", 1}, {"Key3", 3},   {"Key5", 5}};
  EXPECT_EQ(map.size(), vector_values.size());
  std::vector<std::pair<std::string, int>> temp_array;
  for (auto value : map) {
    temp_array.emplace_back(value);
  }
  EXPECT_EQ(temp_array.size(), vector_values.size());
  for (size_t i = 0; i < temp_array.size(); i++) {
    EXPECT_EQ(temp_array[i].first, vector_values[i].first);
    EXPECT_EQ(temp_array[i].second, vector_values[i].second);
  }
  EXPECT_EQ(map.front(), vector_values.front());
  EXPECT_EQ(map.back(), vector_values.back());
}

TEST(LinkedHashMap, copy_move_status) {
  LinkedHashMap<std::string, std::string, 12, 6> map;
  // Reserve enough
  map.reserve(10);
  for (int i = 0; i < 10; i++) {
    map.insert_or_assign(std::string("key") + std::to_string(i),
                         std::string("value") + std::to_string(i));
  }
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // Insert another node, allocate node not on pool, not perfect.
  map.insert_or_assign("key10", "value10");
  EXPECT_TRUE(map.size() == 11);
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map),
            map.size() - 1);
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, false));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // Find trigger map to be built.
  EXPECT_EQ(map.find("key9")->second,
            "value9");  // Linear find, not perfect path
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, true, false));
  EXPECT_EQ(map.find("key8")->second, "value8");  // Hash find
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // Copied map should always be perfect and no map.
  auto map_copy = map;
  EXPECT_TRUE(map_copy.size() == 11);
  EXPECT_TRUE(
      decltype(map_copy)::Testing::assume_status(map_copy, false, true));
  EXPECT_TRUE(decltype(map_copy)::Testing::check_consistency(map_copy));

  // Find trigger map to be built.
  EXPECT_EQ(map_copy.find("key4")->second,
            "value4");  // Linear find, perfect path
  EXPECT_TRUE(decltype(map_copy)::Testing::assume_status(map_copy, true, true));
  EXPECT_EQ(map_copy.find("key6")->second, "value6");  // Hash find
  EXPECT_TRUE(decltype(map_copy)::Testing::check_consistency(map_copy));

  // Make map_copy not perfect.
  EXPECT_TRUE(map_copy.erase("key8") == 1);
  EXPECT_TRUE(map_copy.size() == 10);
  EXPECT_TRUE(
      decltype(map_copy)::Testing::assume_status(map_copy, true, false));
  EXPECT_TRUE(decltype(map_copy)::Testing::check_consistency(map_copy));

  // Copy to another map.
  decltype(map_copy) map_copy_copy;
  for (int i = 0; i < 20; i++) {
    map_copy_copy.insert_or_assign(std::string("key") + std::to_string(i),
                                   std::string("value") + std::to_string(i));
  }
  EXPECT_TRUE(decltype(map_copy_copy)::Testing::assume_status(map_copy_copy,
                                                              true, false));
  map_copy_copy = map_copy;
  EXPECT_TRUE(decltype(map_copy_copy)::Testing::assume_status(map_copy_copy,
                                                              true, true));
  EXPECT_EQ(map_copy_copy.size(), map_copy.size());
  EXPECT_TRUE(
      decltype(map_copy_copy)::Testing::check_consistency(map_copy_copy));
  map_copy_copy.clear();
  EXPECT_TRUE(
      decltype(map_copy_copy)::Testing::check_consistency(map_copy_copy));

  // Move, keep status
  auto map_move = std::move(map_copy);
  EXPECT_TRUE(map_copy.empty());
  EXPECT_TRUE(
      decltype(map_copy)::Testing::assume_status(map_copy, false, true));
  EXPECT_TRUE(decltype(map_copy)::Testing::check_consistency(map_copy));
  EXPECT_TRUE(map_move.size() == 10);
  EXPECT_TRUE(
      decltype(map_move)::Testing::assume_status(map_move, true, false));
  EXPECT_TRUE(decltype(map_move)::Testing::check_consistency(map_move));

  decltype(map_copy) tiny_map;
  tiny_map["key0"] = "value0";
  EXPECT_TRUE(
      decltype(tiny_map)::Testing::assume_status(tiny_map, false, true));
  decltype(map_copy) map_move_move;
  for (int i = 0; i < 20; i++) {
    map_move_move.insert_or_assign(std::string("key") + std::to_string(i),
                                   std::string("value") + std::to_string(i));
  }
  EXPECT_TRUE(decltype(map_move_move)::Testing::assume_status(map_move_move,
                                                              true, false));
  map_move_move = std::move(tiny_map);
  EXPECT_TRUE(tiny_map.empty());
  EXPECT_TRUE(map_move_move.size() == 1);
  EXPECT_TRUE(decltype(map_move_move)::Testing::assume_status(map_move_move,
                                                              false, true));
}

TEST(LinkedHashMap, foreach_status) {
  std::set<std::string> key_set;
  LinkedHashMap<std::string, std::string, 12, 6> map;
  map.reserve(20);
  for (int i = 0; i < 10; i++) {
    key_set.insert(std::string("key") + std::to_string(i));
    map.insert_or_assign(std::string("key") + std::to_string(i),
                         std::string("value") + std::to_string(i));
  }
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map), map.size());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));

  auto key_set2 = key_set;

  // Perfect map iterates base on array.
  map.foreach ([&](const std::string& key, const std::string& value) {
    EXPECT_TRUE(key_set.count(key) == 1);
    key_set.erase(key);
  });
  EXPECT_TRUE(key_set.empty());

  // Make map not perfect, iterate base on linked nodes.
  map.erase("key5");
  EXPECT_TRUE(map.size() == 9);
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, false));
  map.foreach ([&](const std::string& key, const std::string& value) {
    EXPECT_TRUE(key_set2.count(key) == 1);
    key_set2.erase(key);
  });
  EXPECT_TRUE(key_set2.size() == 1);
  EXPECT_TRUE(key_set2.count("key5") == 1);
}

TEST(LinkedHashMap, find) {
  LinkedHashMap<std::string, std::string, 6, 10> map;
  map.reserve(5);
  map.insert_or_assign(std::string("key4"), std::string("value4"));
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2"));
  map.insert_or_assign(std::string("key1"), std::string("value1"));
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // FindBuildMapThreshold == 10, will not build map, all linear find, perfect
  // path
  EXPECT_EQ(map.find("key1")->second, "value1");
  EXPECT_EQ(map.find("key2")->second, "value2");
  EXPECT_EQ(map.find("key5"), map.end());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // Insert to build map, linear find
  map.insert_or_assign(std::string("key5"), std::string("value5"));
  map.insert_or_assign(std::string("key6"), std::string("value6"));
  EXPECT_TRUE(decltype(map)::Testing::assume_status(
      map, false, false));  // Not not on pool, not perfect
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
  map.insert_or_assign(std::string("key3"), std::string("value33333"));
  map.insert_or_assign(std::string("key7"), std::string("value7"));
  EXPECT_TRUE(map.size() == 7);
  EXPECT_EQ(map.find("key5")->second, "value5");
  EXPECT_EQ(map.find("key7")->second, "value7");
  EXPECT_EQ(map.find("key0"), map.end());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, false));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  map.insert_or_assign(std::string("key8"), std::string("value8"));
  EXPECT_TRUE(map.size() == 8);
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, true, false));
  EXPECT_EQ(map.find("key1")->second, "value1");  // hash find
  EXPECT_EQ(map.find("key2")->second, "value2");  // hash find
  EXPECT_EQ(map.find("key0"), map.end());         // hash find
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, find_inner_map_created) {
  LinkedHashMap<std::string, std::string> map;
  for (int i = 0; i < 100; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(map.find(std::string("key") + std::to_string(i))->second,
              std::string("value") + std::to_string(i));
  }
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, contains) {
  LinkedHashMap<std::string, std::string> map;
  map.insert_or_assign(std::string("key4"), std::string("value4"));
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2"));
  map.insert_or_assign(std::string("key1"), std::string("value1"));
  EXPECT_TRUE(map.contains("key2"));
  EXPECT_FALSE(map.contains("key5"));
}

TEST(LinkedHashMap, erase) {
  LinkedHashMap<std::string, std::string, 20, 2> map;
  map.reserve(3);
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2"));
  map.insert_or_assign(std::string("key1"), std::string("value1"));
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_EQ(map.erase(map.begin())->second, "value2");
  EXPECT_EQ(map.erase(map.begin())->second, "value1");
  EXPECT_TRUE(map.size() == 1);
  for (auto it = map.begin(); it != map.end(); it++) {
    EXPECT_EQ(it->first, "key1");
    EXPECT_EQ(it->second, "value1");
  }
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
  EXPECT_TRUE(map.erase("key2") == 0);
  EXPECT_TRUE(map.erase("key1") == 1);
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  map["key5"] = "value5";
  EXPECT_EQ(map.erase(map.begin()), map.end());
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, true));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // erase to empty, pool of map could be reused.
  for (size_t i = 0; i < 15; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  EXPECT_TRUE(decltype(map)::Testing::count_of_nodes_on_pool(map) == 3);
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, false, false));
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // find trigger build map
  EXPECT_EQ(map.erase(map.find("key0"))->first, "key1");
  EXPECT_FALSE(map.contains("key0"));
  EXPECT_EQ(map.find("key0"), map.end());
  EXPECT_TRUE(decltype(map)::Testing::assume_status(map, true, false));
  EXPECT_EQ(map.erase(map.find("key10"))->first, "key11");
  EXPECT_TRUE(map.erase("key11") == 1);
  EXPECT_TRUE(map.size() == 12);
}

TEST(LinkedHashMap, merge) {
  LinkedHashMap<int, int> map0;
  LinkedHashMap<int, int> map1;

  for (size_t i = 0; i < 5; i++) {
    map1[i] = i * 10;
  }
  map0.merge(map1);
  EXPECT_TRUE(map0.size() == 5);

  LinkedHashMap<int, int> map2;
  map2[2] = 20;
  map2[10] = 100;
  map2.merge(map1);
  EXPECT_TRUE(map2.size() == 6);
}

TEST(LinkedHashMap, clear) {
  LinkedHashMap<std::string, std::string> map;
  map.reserve(decltype(map)::kInitialAllocationSize + 1);
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2"));
  map.insert_or_assign(std::string("key1"), std::string("value1"));
  map.clear();
  EXPECT_EQ(map.size(), static_cast<size_t>(0));
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // after map.clear(), its pool is cleared and could be reused.
  for (size_t i = 0; i < decltype(map)::kInitialAllocationSize + 10; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map),
            decltype(map)::kInitialAllocationSize + 1);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, clear_pool) {
  LinkedHashMap<std::string, std::string> map;
  map.reserve(decltype(map)::kInitialAllocationSize + 1);
  map.insert_or_assign(std::string("key3"), std::string("value3"));
  map.insert_or_assign(std::string("key2"), std::string("value2"));
  map.insert_or_assign(std::string("key1"), std::string("value1"));
  map.clear(true);
  EXPECT_EQ(map.size(), static_cast<size_t>(0));
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));

  // after map.clear(true), its pool size is reset to kInitialAllocationSize
  for (size_t i = 0; i < decltype(map)::kInitialAllocationSize + 10; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map),
            decltype(map)::kInitialAllocationSize);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, reserve) {
  LinkedHashMap<std::string, std::string> map;
  map.reserve(2);
  map.reserve(decltype(map)::kInitialAllocationSize + 4);
  map.reserve(1);
  for (size_t i = 0; i < decltype(map)::kInitialAllocationSize + 10; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map),
            decltype(map)::kInitialAllocationSize + 4);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

TEST(LinkedHashMap, set_pool_capacity) {
  LinkedHashMap<std::string, std::string> map;
  map.reserve(2);
  map.set_pool_capacity(decltype(map)::kInitialAllocationSize + 4);
  map.set_pool_capacity(1);
  for (size_t i = 0; i < decltype(map)::kInitialAllocationSize + 10; i++) {
    map[std::string("key") + std::to_string(i)] =
        std::string("value") + std::to_string(i);
  }
  EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map), 1u);
  EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));
}

#define TEST_COPY_MOVE_POOL_SIZE(n)                                          \
  TEST(LinkedHashMap, copy_move_pool_size_##n) {                             \
    LinkedHashMap<std::string, std::string> map(n);                          \
    map.insert_or_assign(std::string("key5"), std::string("value5"));        \
    map.insert_or_assign(std::string("key4"), std::string("value4"));        \
    map.insert_or_assign(std::string("key3"), std::string("value3"));        \
    map.insert_or_assign(std::string("key2"), std::string("value2"));        \
    map.insert_or_assign(std::string("key1"), std::string("value1"));        \
    EXPECT_EQ(decltype(map)::Testing::count_of_nodes_on_pool(map),           \
              std::min<size_t>(map.size(), n));                              \
    EXPECT_TRUE(decltype(map)::Testing::check_consistency(map));             \
                                                                             \
    auto Check = [](const LinkedHashMap<std::string, std::string>& target) { \
      int idx = 5;                                                           \
      for (auto it = target.begin(); it != target.end(); it++) {             \
        EXPECT_EQ(it->first, std::string("key") + std::to_string(idx));      \
        EXPECT_EQ(it->second, std::string("value") + std::to_string(idx));   \
        idx--;                                                               \
      }                                                                      \
    };                                                                       \
                                                                             \
    /* copy, all nodes should be on pool */                                  \
    LinkedHashMap<std::string, std::string> map2;                            \
    map2 = map;                                                              \
    Check(map2);                                                             \
    EXPECT_EQ(decltype(map2)::Testing::count_of_nodes_on_pool(map2),         \
              map2.size());                                                  \
    EXPECT_TRUE(decltype(map2)::Testing::check_consistency(map2));           \
                                                                             \
    /* move, pool is also moved */                                           \
    LinkedHashMap<std::string, std::string> map3;                            \
    map3 = std::move(map2);                                                  \
    Check(map3);                                                             \
    EXPECT_EQ(decltype(map3)::Testing::count_of_nodes_on_pool(map3),         \
              map3.size());                                                  \
    EXPECT_TRUE(decltype(map3)::Testing::check_consistency(map3));           \
    EXPECT_TRUE(map2.empty());                                               \
                                                                             \
    LinkedHashMap<std::string, std::string> map4(map);                       \
    Check(map4);                                                             \
    EXPECT_EQ(decltype(map4)::Testing::count_of_nodes_on_pool(map4),         \
              map4.size());                                                  \
    EXPECT_TRUE(decltype(map4)::Testing::check_consistency(map4));           \
                                                                             \
    LinkedHashMap<std::string, std::string> map5(std::move(map4));           \
    Check(map5);                                                             \
    EXPECT_EQ(decltype(map5)::Testing::count_of_nodes_on_pool(map5),         \
              map5.size());                                                  \
    EXPECT_TRUE(decltype(map5)::Testing::check_consistency(map5));           \
    EXPECT_TRUE(map4.empty());                                               \
                                                                             \
    /* after moved, map4's pool_size is reset to kInitialAllocationSize. */  \
    map4.insert_or_assign(std::string("key5"), std::string("value5"));       \
    map4.insert_or_assign(std::string("key4"), std::string("value4"));       \
    map4.insert_or_assign(std::string("key3"), std::string("value3"));       \
    map4.insert_or_assign(std::string("key2"), std::string("value2"));       \
    map4.insert_or_assign(std::string("key1"), std::string("value1"));       \
    Check(map4);                                                             \
    EXPECT_EQ(decltype(map4)::Testing::count_of_nodes_on_pool(map4),         \
              std::min<size_t>(decltype(map4)::kInitialAllocationSize,       \
                               map4.size()));                                \
    EXPECT_TRUE(decltype(map4)::Testing::check_consistency(map4));           \
  }  // namespace test

TEST_COPY_MOVE_POOL_SIZE(0u)
TEST_COPY_MOVE_POOL_SIZE(2u)
TEST_COPY_MOVE_POOL_SIZE(4u)
TEST_COPY_MOVE_POOL_SIZE(6u)

}  // namespace test
}  // namespace base
}  // namespace lynx
