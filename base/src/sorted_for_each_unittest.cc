// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/sorted_for_each.h"

#include <unordered_map>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::base::testing {
namespace {

template <class Key, class Value>
class FooMap : public std::unordered_map<Key, Value> {
 public:
  FooMap() : std::unordered_map<Key, Value>() {}
  FooMap(std::initializer_list<std::pair<const Key, Value>> value)
      : std::unordered_map<Key, Value>(std::move(value)) {}

 private:
  // making FooMap move only
  std::unique_ptr<int> ptr_ = std::make_unique<int>(1);
};
}  // namespace

TEST(SortedForEachTest, SortedTest) {
  std::vector<int> before{6, 3, 1, 7, 1}, actual{}, expected{};

  auto f = [&actual](const auto& i) { actual.push_back(i); };
  base::sorted_for_each(before.begin(), before.end(), f);

  expected = {1, 1, 3, 6, 7};
  ASSERT_EQ(expected, actual);
}

TEST(SortedForEachTest, MoveOnlyTest) {
  // should be able to deal with move only containers
  FooMap<int, std::unique_ptr<int>> map{};
  map.insert({2, std::make_unique<int>(2)});
  map.insert({3, std::make_unique<int>(3)});
  map.insert({1, std::make_unique<int>(1)});

  std::vector<int> actual{}, expected{};
  base::sorted_for_each(
      map.begin(), map.end(),
      [&actual](const auto& it) { actual.push_back(it.first); },
      // custom compare
      [](const auto& a, const auto& b) { return a.first < b.first; });

  expected = {1, 2, 3};

  ASSERT_EQ(expected, actual);

  actual = {}, expected = {3, 2, 1};
  base::sorted_for_each(
      map.cbegin(), map.cend(),
      [&actual](const auto& it) { actual.push_back(it.first); },
      // custom compare
      [](const auto& a, const auto& b) { return a.first >= b.first; });

  ASSERT_EQ(expected, actual);
}

TEST(SortedForEachTest, ConstIterTest) {
  FooMap<int, int> map{{1, 1}, {3, 3}, {2, 2}, {8, 8}, {6, 6}};

  std::vector<int> actual{}, expected{};
  base::sorted_for_each(map.cbegin(), map.cend(), [&actual](const auto& it) {
    actual.push_back(it.first);
  });

  expected = {1, 2, 3, 6, 8};

  ASSERT_EQ(actual, expected);
}

}  // namespace lynx::base::testing
