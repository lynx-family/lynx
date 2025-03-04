// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/string/string_number_convert.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(StringNumberConvertTest, StringToDoubleTest) {
  double num1 = 0;
  EXPECT_TRUE(StringToDouble("123.456", num1));
  EXPECT_EQ(123.456, num1);

  EXPECT_FALSE(StringToDouble("12c.456", num1));
  EXPECT_EQ(123.456, num1);
}

TEST(StringNumberConvertTest, StringToIntTest) {
  int64_t num1 = 0;
  EXPECT_FALSE(StringToInt("123.456", num1, 10));

  EXPECT_TRUE(StringToInt("123", num1, 16));
  EXPECT_EQ(291, num1);
  EXPECT_TRUE(StringToInt("123", num1, 10));
  EXPECT_EQ(123, num1);
  EXPECT_TRUE(StringToInt("123", num1, 8));
  EXPECT_EQ(83, num1);
  EXPECT_FALSE(StringToInt("123", num1, 2));
  EXPECT_TRUE(StringToInt("10", num1, 2));
  EXPECT_EQ(2, num1);

  EXPECT_FALSE(StringToInt("123abc", num1, 10));
}

}  // namespace base
}  // namespace lynx
