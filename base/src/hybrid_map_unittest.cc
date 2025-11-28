// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/hybrid_map.h"

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>

#include "base/include/boost/unordered.h"
#include "base/include/vector.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

static std::string ConcatOrderedMap(
    const std::map<std::string, std::string> map) {
  std::string result;
  for (const auto& p : map) {
    result += p.first;
    result += p.second;
  }
  return result;
}

using MapPolicyStdMap = MapPolicy<std::map>;
using MapPolicyStdUnorderedMap = MapPolicy<std::unordered_map>;
using MapPolicyLinearFlatMap = MapPolicy<LinearFlatMap>;
using MapPolicyOrderedFlatMap = MapPolicy<OrderedFlatMap>;
using MapPolicyBoostFlatMap = MapPolicy<boost::unordered_flat_map>;

template <size_t N, typename... Args>
using MapPolicyInlineLinearFlatMap =
    InlineFlatMapPolicy<InlineLinearFlatMap, N, Args...>;

template <size_t N>
using MapPolicyInlineOrderedFlatMap =
    InlineFlatMapPolicy<InlineOrderedFlatMap, N>;

template <class MAP>
static void Test_MapInsertOrAssign() {
  MAP map{{"3", "c"}, {"2", "b"}, {"1", "a"}};
  EXPECT_TRUE(map.size() == 3);
  EXPECT_EQ(map["1"], "a");
  EXPECT_EQ(map["2"], "b");
  EXPECT_EQ(map["3"], "c");
  EXPECT_EQ(map["4"], "");

  auto r = map.insert_or_assign("4", "d");
  EXPECT_FALSE(r.second);
  EXPECT_EQ(map["4"], "d");

  std::string s5 = "5";
  std::string se = "e";
  auto r2 = map.insert_or_assign(s5, std::move(se));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(map["5"], "e");
  EXPECT_EQ(s5, "5");
  EXPECT_TRUE(se.empty());

  std::string s6 = "6";
  std::string sf = "f";
  auto r3 = map.insert_or_assign(std::move(s6), std::move(sf));
  EXPECT_TRUE(r3.second);
  EXPECT_EQ(map["6"], "f");
  EXPECT_TRUE(s6.empty());
  EXPECT_TRUE(sf.empty());

  std::string s7 = "7";
  std::string sg = "g";
  auto r4 = map.insert_or_assign(std::move(s7), sg);
  EXPECT_TRUE(r4.second);
  EXPECT_EQ(map["7"], "g");
  EXPECT_TRUE(s7.empty());
  EXPECT_EQ(sg, "g");

  EXPECT_EQ(map.size(), 7u);
}

template <class MAP>
static void Test_MapInsertOrAssign2() {
  MAP m;
  {
    auto [it, inserted] = m.insert_or_assign("fruit", "apple");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(*it, "apple");
    EXPECT_EQ(m.size(), 1u);
  }

  {
    auto [it, inserted] = m.insert_or_assign("fruit", "banana");
    EXPECT_FALSE(inserted);
    EXPECT_EQ(*it, "banana");
    EXPECT_EQ(m.size(), 1u);
  }

  m.insert_or_assign("empty", "");
  EXPECT_EQ(m["empty"], "");
}

template <class MAP>
static void Test_MapEmplace() {
  MAP map;
  auto r = map.emplace(std::piecewise_construct,
                       std::tuple<const char*, size_t>("123", 2),
                       std::tuple<const char*, size_t>("abc", 2));
  EXPECT_TRUE(r.second);
  EXPECT_EQ(*r.first, "ab");
  auto r2 = map.emplace(std::piecewise_construct,
                        std::tuple<const char*, size_t>("112", 2),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_TRUE(r2.second);
  EXPECT_EQ(*r2.first, "xy");

  EXPECT_EQ(map.size(), 2u);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");

  auto r3 = map.emplace(std::piecewise_construct, std::forward_as_tuple("12"),
                        std::tuple<const char*, size_t>("xyz", 2));
  EXPECT_FALSE(r3.second);
  EXPECT_EQ(*r3.first, "ab");

  EXPECT_EQ(map.size(), 2u);

  auto r4 = map.try_emplace("11", "ab");
  EXPECT_FALSE(r4.second);
  EXPECT_EQ(*r4.first, "xy");

  std::string s11 = "11";
  std::string sXYZ = "xyz";
  auto r5 = map.try_emplace(std::move(s11), std::move(sXYZ));
  EXPECT_FALSE(r5.second);
  EXPECT_EQ(*r5.first, "xy");
  EXPECT_EQ(s11, "11");
  EXPECT_EQ(sXYZ, "xyz");

  std::string s13 = "13";
  auto r6 = map.try_emplace(std::move(s13), std::move(sXYZ));
  EXPECT_TRUE(r6.second);
  EXPECT_EQ(*r6.first, "xyz");
  EXPECT_TRUE(s13.empty());
  EXPECT_TRUE(sXYZ.empty());

  EXPECT_EQ(map.size(), 3u);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");

  std::string s14 = "14";
  std::string sUVW = "uvw";
  auto r7 = map.try_emplace(s14, sUVW);
  EXPECT_TRUE(r7.second);
  EXPECT_EQ(*r7.first, "uvw");
  EXPECT_EQ(s14, "14");
  EXPECT_EQ(sUVW, "uvw");

  EXPECT_EQ(map.size(), 4u);
  EXPECT_EQ(map["12"], "ab");
  EXPECT_EQ(map["11"], "xy");
  EXPECT_EQ(map["13"], "xyz");
  EXPECT_EQ(map["14"], "uvw");
}

template <class MAP>
static void Test_MapElementAccess() {
  MAP m{{"apple", "red"}, {"banana", "yellow"}};

  EXPECT_EQ(m["apple"], "red");

  m["apple"] = "green";
  EXPECT_EQ(m["apple"], "green");
  EXPECT_EQ(m.at("apple"), "green");

  EXPECT_EQ(m["grape"], "");
  EXPECT_EQ(m.at("grape"), "");
  EXPECT_EQ(m.size(), 3u);
}

template <class MAP>
static void Test_MapInsertUpdate() {
  MAP m;

  auto ret1 = m.insert("fruit", "apple");
  EXPECT_TRUE(ret1.second);
  auto ret2 = m.insert("fruit", "banana");
  EXPECT_FALSE(ret2.second);
  EXPECT_EQ(*ret2.first, "apple");

  auto emp_ret = m.emplace("color", "blue");
  EXPECT_TRUE(emp_ret.second);
  EXPECT_EQ(*emp_ret.first, "blue");

  m["color"] = "red";
  EXPECT_EQ(m["color"], "red");
}

template <class MAP>
static void Test_MapEraseOperations() {
  MAP m{{"A", "1"}, {"B", "2"}, {"C", "3"}};
  EXPECT_EQ(m.size(), 3u);
  EXPECT_EQ(m["A"], "1");
  EXPECT_EQ(m["B"], "2");
  EXPECT_EQ(m["C"], "3");

  size_t cnt = m.erase("B");
  EXPECT_EQ(cnt, 1u);
  EXPECT_EQ(m.size(), 2u);
  EXPECT_FALSE(m.contains("B"));

  EXPECT_EQ(m.erase("X"), 0u);
}

template <class MAP>
static void Test_MapEdgeCases() {
  MAP m;

  m[""] = "empty_key";
  m.emplace("empty_value", "");
  EXPECT_EQ(m[""], "empty_key");
  EXPECT_EQ(m["empty_value"], "");

  std::string big_key(1000, 'K');
  std::string big_value(10000, 'V');
  m[big_key] = big_value;
  EXPECT_EQ(m[big_key].size(), 10000u);
}

template <class MAP>
static void Test_MapEmplacePiecewise() {
  MAP m;

  auto emp_it =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple(5, 'X'));
  ASSERT_TRUE(emp_it.second);
  EXPECT_EQ(*emp_it.first, "XXXXX");

  m.emplace(std::piecewise_construct, std::forward_as_tuple(3, 'K'),
            std::forward_as_tuple(3, 'k'));
  EXPECT_EQ(m["KKK"], "kkk");

  auto emp_fail =
      m.emplace(std::piecewise_construct, std::forward_as_tuple("piece_key"),
                std::forward_as_tuple("new_value"));
  EXPECT_FALSE(emp_fail.second);
  EXPECT_EQ(m["piece_key"], "XXXXX");
}

template <class MAP>
static void Test_MapForeach() {
  std::map<std::string, std::string> visited;
  MAP map;
  map["B"] = "2";
  map["A"] = "1";
  map["C"] = "3";
  const auto& const_map = map;
  const_map.for_each([&](const std::string& key, const std::string& value) {
    visited[key] = value;
  });
  EXPECT_TRUE(ConcatOrderedMap(visited) == "A1B2C3");

  visited.clear();
  auto map2 = map;
  map2.for_each([&](const std::string& key, std::string& value) {
    visited[key] = value;
    if (key == "B") {
      value = "22";
    }
  });
  EXPECT_TRUE(ConcatOrderedMap(visited) == "A1B2C3");
  EXPECT_TRUE(map2["B"] == "22");
}

template <class MAP>
static void Test_MapIterator() {
  std::map<std::string, std::string> visited;
  MAP map;
  map["B"] = "2";
  map["A"] = "1";
  map["C"] = "3";
  EXPECT_TRUE(map.find_iterator("B")->first == "B");
  EXPECT_TRUE(map.find_iterator("B")->second == "2");
  EXPECT_EQ(map.find_iterator("D"), map.end());

  const auto& const_map = map;
  for (const auto& p : const_map) {
    visited[p.first] = p.second;
  }
  EXPECT_TRUE(ConcatOrderedMap(visited) == "A1B2C3");

  visited.clear();
  auto map2 = map;
  for (auto it = map2.begin(); it != map2.end(); it++) {
    visited[it->first] = it->second;
    if (it->first == "B") {
      it->second = "22";
    }
  }
  EXPECT_TRUE(ConcatOrderedMap(visited) == "A1B2C3");
  EXPECT_TRUE(map2["B"] == "22");

  visited.clear();
  for (auto it = map.cbegin(); it != map.cend(); it++) {
    visited[it->first] = it->second;
  }
  EXPECT_TRUE(ConcatOrderedMap(visited) == "A1B2C3");
}

template <class MAP>
static void Test_MapEraseIterator() {
  MAP map;
  for (int i = 1; i <= 10; ++i) {
    map["key_" + std::to_string(i)] = std::to_string(i);
  }
  for (auto it = map.begin(); it != map.end();) {
    if (std::stoi(it->second) % 2 == 0) {
      it = map.erase_iterator(it);
    } else {
      ++it;
    }
  }

  EXPECT_TRUE(map.size() == 5);
  for (const auto& pair : map) {
    EXPECT_TRUE(std::stoi(pair.second) % 2 != 0);
  }

  const auto& const_map = map;
  auto it = const_map.find_iterator("key_5");
  EXPECT_TRUE(it != const_map.end());
  EXPECT_TRUE(it->second == "5");
  map.erase_iterator(it);
  EXPECT_TRUE(map.size() == 4);
  it = const_map.find_iterator("key_5");
  EXPECT_TRUE(it == const_map.end());

  int i = 0;
  while (!map.empty()) {
    map.erase_iterator(map.begin());
    i++;
  }
  EXPECT_EQ(i, 4);
}

template <class MAP>
static void assert_map_content(MAP& map, const base::Vector<std::string> keys,
                               const base::Vector<std::string> values) {
  EXPECT_EQ(keys.size(), values.size());
  EXPECT_EQ(map.size(), keys.size());
  if (keys.size() > 0) {
    EXPECT_TRUE(!map.empty());
  } else {
    EXPECT_TRUE(map.empty());
  }
  for (size_t i = 0; i < keys.size(); i++) {
    auto it = map.find(keys[i]);
    EXPECT_TRUE(it != nullptr);
    EXPECT_TRUE(*it == values[i]);
    EXPECT_TRUE(map.at(keys[i]) == values[i]);
    EXPECT_TRUE(map.count(keys[i]) == 1);
    EXPECT_TRUE(map.contains(keys[i]));
  }
};

template <class MAP>
static void Test_MapMisc_4_As_SmallMap_MaxSize() {
  {
    MAP map_small{{"A", "1"}, {"B", "2"}, {"C", "3"}, {"D", "4"}};
    EXPECT_TRUE(map_small.using_small_map());
    MAP map_small2{{"A", "1"}, {"B", "2"}};
    EXPECT_TRUE(map_small2.using_small_map());
    MAP map_big{{"A", "1"}, {"B", "2"}, {"C", "3"}, {"D", "4"}, {"E", "5"}};
    EXPECT_FALSE(map_big.using_small_map());
    MAP map_big2{{"A", "1"}, {"B", "2"}, {"C", "3"},
                 {"D", "4"}, {"E", "5"}, {"F", "6"}};
    EXPECT_FALSE(map_big2.using_small_map());

    MAP map(map_small);
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"A", "B", "C", "D"}, {"1", "2", "3", "4"});
    map = map_small2;
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"A", "B"}, {"1", "2"});
    map = map_big;
    EXPECT_FALSE(map.using_small_map());
    assert_map_content(map, {"A", "B", "C", "D", "E"},
                       {"1", "2", "3", "4", "5"});
    map = map_big2;
    EXPECT_FALSE(map.using_small_map());
    assert_map_content(map, {"A", "B", "C", "D", "E", "F"},
                       {"1", "2", "3", "4", "5", "6"});
    map = map_small;
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"A", "B", "C", "D"}, {"1", "2", "3", "4"});

    MAP map2(map_big);
    EXPECT_FALSE(map2.using_small_map());
    assert_map_content(map2, {"A", "B", "C", "D", "E"},
                       {"1", "2", "3", "4", "5"});

    MAP map3(std::move(map));
    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(map3.using_small_map());
    assert_map_content(map3, {"A", "B", "C", "D"}, {"1", "2", "3", "4"});

    MAP map_big_copy = map_big;
    EXPECT_FALSE(map_big_copy.using_small_map());

    MAP map4(std::move(map_big_copy));
    EXPECT_TRUE(map_big_copy.empty());
    EXPECT_FALSE(map4.using_small_map());
    assert_map_content(map4, {"A", "B", "C", "D", "E"},
                       {"1", "2", "3", "4", "5"});

    MAP map_small_copy = map_small;
    map4 = std::move(map_small_copy);
    EXPECT_TRUE(map_small_copy.empty());
    EXPECT_TRUE(map4.using_small_map());
    assert_map_content(map4, {"A", "B", "C", "D"}, {"1", "2", "3", "4"});

    map4 = std::move(map_small2);
    EXPECT_TRUE(map_small2.empty());
    EXPECT_TRUE(map4.using_small_map());
    assert_map_content(map4, {"A", "B"}, {"1", "2"});

    map4 = std::move(map_big2);
    EXPECT_TRUE(map_big2.empty());
    EXPECT_FALSE(map4.using_small_map());
    assert_map_content(map4, {"A", "B", "C", "D", "E", "F"},
                       {"1", "2", "3", "4", "5", "6"});

    map4 = std::move(map_big);
    EXPECT_TRUE(map_big.empty());
    EXPECT_FALSE(map4.using_small_map());
    assert_map_content(map4, {"A", "B", "C", "D", "E"},
                       {"1", "2", "3", "4", "5"});
  }

  {
    MAP map;
    map["a"] = "1";
    map["b"] = "2";
    map["c"] = "3";
    map["d"] = "4";
    EXPECT_TRUE(map.using_small_map());
    map["d"] = "5";
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d"}, {"1", "2", "3", "5"});
    map.reserve(10);
    EXPECT_FALSE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d"}, {"1", "2", "3", "5"});
  }

  {
    MAP map;
    map["a"] = "1";
    map["b"] = "2";
    map["c"] = "3";
    map["d"] = "4";
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d"}, {"1", "2", "3", "4"});
    std::string c_str = "c";
    map[c_str] = "33";
    map["d"] = "44";
    EXPECT_TRUE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d"}, {"1", "2", "33", "44"});
    map["e"] = "5";
    EXPECT_FALSE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d", "e"},
                       {"1", "2", "33", "44", "5"});
    std::string f_str = "f";
    map[f_str] = "6";
    EXPECT_FALSE(map.using_small_map());
    assert_map_content(map, {"a", "b", "c", "d", "e", "f"},
                       {"1", "2", "33", "44", "5", "6"});
  }

  MAP map;
  EXPECT_TRUE(map.using_small_map());
  EXPECT_TRUE(map.empty());
  EXPECT_TRUE(map.size() == 0);

  auto ret = map.insert("apple", "red");
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(map["apple"], "red");
  EXPECT_EQ(*ret.first, "red");
  assert_map_content(map, {"apple"}, {"red"});

  ret = map.emplace(std::piecewise_construct, std::forward_as_tuple(3, 'K'),
                    std::forward_as_tuple(3, 'k'));
  ASSERT_TRUE(ret.second);
  EXPECT_EQ(map["KKK"], "kkk");
  EXPECT_EQ(*ret.first, "kkk");
  assert_map_content(map, {"apple", "KKK"}, {"red", "kkk"});

  ret = map.try_emplace("KKK", "ab");
  EXPECT_FALSE(ret.second);
  EXPECT_EQ(*ret.first, "kkk");
  EXPECT_TRUE(map.using_small_map());
  assert_map_content(map, {"apple", "KKK"}, {"red", "kkk"});

  map["banana"] = "black";
  EXPECT_EQ(map["banana"], "black");
  assert_map_content(map, {"apple", "KKK", "banana"}, {"red", "kkk", "black"});
  std::string banana = "banana";
  ret = map.insert_or_assign(banana, "pink");
  EXPECT_FALSE(ret.second);
  EXPECT_EQ(map["banana"], "pink");
  assert_map_content(map, {"apple", "KKK", "banana"}, {"red", "kkk", "pink"});
  ret = map.insert_or_assign(std::move(banana), "yellow");
  EXPECT_FALSE(ret.second);
  EXPECT_EQ(map["banana"], "yellow");
  assert_map_content(map, {"apple", "KKK", "banana"}, {"red", "kkk", "yellow"});
  EXPECT_TRUE(map.using_small_map());

  std::string sJJJ = "JJJ";
  ret = map.insert_or_assign(std::move(sJJJ), "jjj");
  EXPECT_TRUE(ret.second);
  EXPECT_TRUE(sJJJ.empty());
  EXPECT_EQ(map["JJJ"], "jjj");
  EXPECT_TRUE(map.using_small_map());
  assert_map_content(map, {"apple", "KKK", "banana", "JJJ"},
                     {"red", "kkk", "yellow", "jjj"});

  EXPECT_EQ(map.at("apple"), "red");
  EXPECT_TRUE(map.count("banana") == 1);
  EXPECT_TRUE(map.count("orange") == 0);
  EXPECT_TRUE(map.find("banana") != nullptr);
  EXPECT_TRUE(map.find("orange") == nullptr);
  EXPECT_TRUE(map.contains("banana"));
  EXPECT_FALSE(map.contains("orange"));

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    map_copy["AAA"] = "aaa";
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
    map_copy["AAA"] = "aaaa";
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaaa"});
    auto ret = map_copy.insert_or_assign("AAA", "aaaaa");
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(*ret.first, "aaaaa");
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaaaa"});

    ret = map_copy.emplace(std::piecewise_construct,
                           std::forward_as_tuple(3, 'K'),
                           std::forward_as_tuple(5, 'k'));
    ASSERT_FALSE(ret.second);
    EXPECT_EQ(*ret.first, "kkk");
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaaaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    auto ret = map_copy.insert("AAA", "aaa");
    EXPECT_TRUE(ret.second);
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    EXPECT_EQ(*ret.first, "aaa");
    EXPECT_EQ(ret.first, map_copy.find("AAA"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    auto ret = map_copy.insert("apple", "green");
    EXPECT_FALSE(ret.second);
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    const std::string key = "apple";
    std::string value = "green";
    auto ret = map_copy.insert(key, value);
    EXPECT_FALSE(ret.second);
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    const std::string key = "apple";
    std::string value = "green";
    auto ret = map_copy.insert(key, std::move(value));
    EXPECT_FALSE(ret.second);
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    std::string key = "apple";
    std::string value = "green";
    auto ret = map_copy.insert(std::move(key), value);
    EXPECT_FALSE(ret.second);
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    std::string key = "apple";
    std::string value = "green";
    auto ret = map_copy.insert(std::move(key), std::move(value));
    EXPECT_FALSE(ret.second);
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    std::pair<std::string, std::string> data = {"AAA", "aaa"};
    auto ret = map_copy.insert(data.first, data.second);
    EXPECT_TRUE(ret.second);
    EXPECT_FALSE(data.first.empty());
    EXPECT_FALSE(data.second.empty());
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    EXPECT_EQ(*ret.first, "aaa");
    EXPECT_EQ(ret.first, map_copy.find("AAA"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    std::pair<std::string, std::string> data = {"AAA", "aaa"};
    auto ret = map_copy.insert(std::move(data.first), std::move(data.second));
    EXPECT_TRUE(ret.second);
    EXPECT_TRUE(data.first.empty());
    EXPECT_TRUE(data.second.empty());
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    EXPECT_EQ(*ret.first, "aaa");
    EXPECT_EQ(ret.first, map_copy.find("AAA"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    const std::pair<const std::string, std::string> data = {"AAA", "aaa"};
    auto ret = map_copy.insert(data.first, data.second);
    EXPECT_TRUE(ret.second);
    EXPECT_FALSE(data.first.empty());
    EXPECT_FALSE(data.second.empty());
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    EXPECT_EQ(*ret.first, "aaa");
    EXPECT_EQ(ret.first, map_copy.find("AAA"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    std::pair<const std::string, std::string> data = {"AAA", "aaa"};
    auto ret = map_copy.insert(std::move(data.first), std::move(data.second));
    EXPECT_TRUE(ret.second);
    EXPECT_FALSE(data.first.empty());
    EXPECT_TRUE(data.second.empty());
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    EXPECT_EQ(*ret.first, "aaa");
    EXPECT_EQ(ret.first, map_copy.find("AAA"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
    const std::string key = "apple";
    auto ret = map_copy.insert_or_assign(key, "green");
    EXPECT_FALSE(ret.second);                 // assignment took place
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    EXPECT_EQ(*ret.first, "green");
    EXPECT_EQ(ret.first, map_copy.find("apple"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"green", "kkk", "yellow", "jjj"});
    auto ret2 = map_copy.insert_or_assign("AAA", "aaa");
    EXPECT_TRUE(ret2.second);  // insertion took place
    EXPECT_EQ(*ret2.first, "aaa");
    EXPECT_EQ(ret2.first, map_copy.find("AAA"));
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"green", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
    auto ret = map_copy.insert_or_assign("apple", "green");
    EXPECT_FALSE(ret.second);                 // assignment took place
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    EXPECT_EQ(*ret.first, "green");
    EXPECT_EQ(ret.first, map_copy.find("apple"));
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"green", "kkk", "yellow", "jjj"});
    auto ret2 = map_copy.insert_or_assign("AAA", "aaa");
    EXPECT_TRUE(ret2.second);
    EXPECT_EQ(*ret2.first, "aaa");
    EXPECT_EQ(ret2.first, map_copy.find("AAA"));
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"green", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    const std::string key = "apple";
    auto ret = map_copy.emplace(key, "green", 4);
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(*ret.first, "red");
    EXPECT_EQ(ret.first, map_copy.find(key));
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
    const std::string key2 = "AAA";
    auto ret2 = map_copy.emplace(key2, "aaaaaa", 3);
    EXPECT_TRUE(ret2.second);
    EXPECT_EQ(*ret2.first, "aaa");
    EXPECT_EQ(ret2.first, map_copy.find("AAA"));
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    const std::string key = "apple";
    auto ret = map_copy.emplace(key, "green", 4);
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(*ret.first, "red");
    EXPECT_EQ(ret.first, map_copy.find(key));
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
    std::string key2 = "AAA";
    auto ret2 = map_copy.emplace(std::move(key2), "aaaaaa", 3);
    EXPECT_TRUE(key2.empty());
    EXPECT_TRUE(ret2.second);
    EXPECT_EQ(*ret2.first, "aaa");
    EXPECT_EQ(ret2.first, map_copy.find("AAA"));
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }

  {
    auto map_copy = map;
    EXPECT_TRUE(map_copy.using_small_map());
    auto ret = map_copy.emplace(std::piecewise_construct,
                                std::forward_as_tuple(3, 'K'),
                                std::forward_as_tuple(3, 'i'));
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(*ret.first, "kkk");
    EXPECT_EQ(ret.first, map_copy.find("KKK"));
    EXPECT_TRUE(map_copy.using_small_map());  // not transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ"},
                       {"red", "kkk", "yellow", "jjj"});
    auto ret2 = map_copy.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(3, 'A'),
                                 std::forward_as_tuple(3, 'a'));
    EXPECT_TRUE(ret2.second);
    EXPECT_EQ(*ret2.first, "aaa");
    EXPECT_EQ(ret2.first, map_copy.find("AAA"));
    EXPECT_FALSE(map_copy.using_small_map());  // transferred
    assert_map_content(map_copy, {"apple", "KKK", "banana", "JJJ", "AAA"},
                       {"red", "kkk", "yellow", "jjj", "aaa"});
  }
}

#define RUN_TESTS_FOR_MAP()        \
  Test_MapInsertOrAssign<MAP>();   \
  Test_MapInsertOrAssign2<MAP>();  \
  Test_MapEmplace<MAP>();          \
  Test_MapElementAccess<MAP>();    \
  Test_MapInsertUpdate<MAP>();     \
  Test_MapEraseOperations<MAP>();  \
  Test_MapEdgeCases<MAP>();        \
  Test_MapEmplacePiecewise<MAP>(); \
  Test_MapForeach<MAP>();          \
  Test_MapIterator<MAP>();         \
  Test_MapEraseIterator<MAP>()

#define RUN_TESTS_OF_MAX_SMALL_SIZE(N, INLINE_N)                               \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N, MapPolicyStdMap,        \
                          MapPolicyStdUnorderedMap>;                           \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N,                         \
                          MapPolicyStdUnorderedMap, MapPolicyStdMap>;          \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N, MapPolicyLinearFlatMap, \
                          MapPolicyStdUnorderedMap>;                           \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N, MapPolicyLinearFlatMap, \
                          MapPolicyOrderedFlatMap>;                            \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N, MapPolicyLinearFlatMap, \
                          MapPolicyBoostFlatMap>;                              \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N,                         \
                          MapPolicyInlineLinearFlatMap<INLINE_N>,              \
                          MapPolicyBoostFlatMap>;                              \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    using MAP = HybridMap<std::string, std::string, N,                         \
                          MapPolicyInlineOrderedFlatMap<INLINE_N>,             \
                          MapPolicyBoostFlatMap>;                              \
    RUN_TESTS_FOR_MAP();                                                       \
    if (N == 4) {                                                              \
      Test_MapMisc_4_As_SmallMap_MaxSize<MAP>();                               \
    }                                                                          \
  }

TEST(HybridMap, All) {
  RUN_TESTS_OF_MAX_SMALL_SIZE(2, 1);
  RUN_TESTS_OF_MAX_SMALL_SIZE(2, 2);
  RUN_TESTS_OF_MAX_SMALL_SIZE(3, 2);
  RUN_TESTS_OF_MAX_SMALL_SIZE(4, 2);
  RUN_TESTS_OF_MAX_SMALL_SIZE(4, 4);
  RUN_TESTS_OF_MAX_SMALL_SIZE(4, 6);
  RUN_TESTS_OF_MAX_SMALL_SIZE(5, 4);
  RUN_TESTS_OF_MAX_SMALL_SIZE(6, 4);
  RUN_TESTS_OF_MAX_SMALL_SIZE(7, 4);
  RUN_TESTS_OF_MAX_SMALL_SIZE(8, 8);
  RUN_TESTS_OF_MAX_SMALL_SIZE(16, 8);
}

TEST(HybridMap, ReserveWithInline) {
  using MAP = HybridMap<std::string, std::string, 4,
                        MapPolicyInlineLinearFlatMap<2>, MapPolicyBoostFlatMap>;
  {
    MAP map;
    map["a"] = "1";
    map["b"] = "2";
    EXPECT_TRUE(map.using_small_map());
    EXPECT_TRUE(map.small_map().is_static_buffer());
    map["c"] = "3";
    EXPECT_TRUE(map.using_small_map());
    EXPECT_FALSE(map.small_map().is_static_buffer());
    map["d"] = "4";
    EXPECT_TRUE(map.using_small_map());
    EXPECT_FALSE(map.small_map().is_static_buffer());
    map["e"] = "5";
    EXPECT_FALSE(map.using_small_map());
  }

  {
    MAP map;
    EXPECT_TRUE(map.using_small_map());
    EXPECT_TRUE(map.small_map().is_static_buffer());
    map.reserve(2);
    EXPECT_TRUE(map.using_small_map());
    EXPECT_TRUE(map.small_map().is_static_buffer());
    map.reserve(3);
    EXPECT_TRUE(map.using_small_map());
    EXPECT_FALSE(map.small_map().is_static_buffer());
    map.reserve(4);
    EXPECT_TRUE(map.using_small_map());
    EXPECT_FALSE(map.small_map().is_static_buffer());
    map.reserve(5);
    EXPECT_FALSE(map.using_small_map());
  }
}

}  // namespace test
}  // namespace base
}  // namespace lynx
