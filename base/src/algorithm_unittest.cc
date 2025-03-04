// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/algorithm.h"

#include <algorithm>
#include <random>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

static bool CompareLess(int a, int b) { return a < b; }
static bool CompareGreater(int a, int b) { return a > b; }

TEST(Algorithm, Sort) {
  std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  std::vector<int> expected = v;

  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(v.begin(), v.end(), g);

  EXPECT_NE(expected, v);

  InsertionSort(v.data(), v.size(), CompareLess);

  EXPECT_EQ(expected, v);
}

TEST(Algorithm, SortCompareGreater) {
  std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  std::vector<int> expected = v;
  std::reverse(expected.begin(), expected.end());

  InsertionSort(v.data(), v.size(), CompareGreater);

  EXPECT_EQ(expected, v);
}

}  // namespace base
}  // namespace lynx
