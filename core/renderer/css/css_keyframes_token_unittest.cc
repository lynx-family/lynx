// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/css/css_keyframes_token.h"

#include <cmath>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

bool NearlyEqual(float a, float b, float epsilon = 1e-5) {
  return std::fabs(a - b) < epsilon;
}

TEST(CSSKeyframesToken, ParseKeyStr0) {
  std::string input = "from";

  EXPECT_EQ(CSSKeyframesToken::ParseKeyStr(input), 0);
}

TEST(CSSKeyframesToken, ParseKeyStr1) {
  std::string input = "to";

  EXPECT_EQ(CSSKeyframesToken::ParseKeyStr(input), 1);
}

TEST(CSSKeyframesToken, ParseKeyStr2) {
  std::string input = "99";

  EXPECT_TRUE(NearlyEqual(CSSKeyframesToken::ParseKeyStr(input), 0.99));
}

TEST(CSSKeyframesToken, ParseKeyStr3) {
  std::string input = "-1";

  EXPECT_EQ(CSSKeyframesToken::ParseKeyStr(input), 0);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
