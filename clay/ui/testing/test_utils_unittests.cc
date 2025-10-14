// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/testing/test_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(TestUtilsTest, ValueEquals) {
  Value a(1);
  Value b(1);
  Value c(2);
  Value d(1.0);
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_NE(a, d);

  Value arr1{Value(1), Value(2)};
  Value arr2{Value(1), Value(2)};
  Value arr3{Value(2), Value(1)};
  EXPECT_EQ(arr1, arr2);
  EXPECT_NE(arr1, arr3);
}

TEST(TestUtilsTest, ValuePrint) {
  Value a(1);
  Value b(1.0);
  Value c(std::string("abc"));
  Value d(false);
  Value e{Value(1), Value(2)};
  Value f{{"a", Value(true)}};

  EXPECT_EQ(::testing::PrintToString(a), "(int) 1");
  EXPECT_EQ(::testing::PrintToString(b), "(double) 1");
  EXPECT_EQ(::testing::PrintToString(c), "\"abc\"");
  EXPECT_EQ(::testing::PrintToString(d), "false");
  EXPECT_EQ(::testing::PrintToString(e), "[(int) 1, (int) 2]");
  EXPECT_EQ(::testing::PrintToString(f), "{a: true}");
}

}  // namespace clay
