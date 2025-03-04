// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "pow10.h"

#include "gtest/gtest.h"

namespace rapidjson {
namespace internal {
  TEST (POW10TEST, BoundaryValue) {
    EXPECT_EQ(Pow10(0), 1e+0);
    EXPECT_EQ(Pow10(308), 1e+308);
  }

  TEST (POW10TEST, NormalValue) {
    EXPECT_EQ(Pow10(1), 1e+1);
    EXPECT_EQ(Pow10(100), 1e+100);
    EXPECT_EQ(Pow10(154), 1e+154);
    EXPECT_EQ(Pow10(200), 1e+200);
    EXPECT_EQ(Pow10(307), 1e+307);
  }

  TEST (POW10TEST, ImplicitlyTypedValue) {
    EXPECT_EQ(Pow10(2.0), 1e+2);
  }

} // namespace internal
} // namespace rapidjson