// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/css_fragment.h"

#include "core/renderer/css/shared_css_fragment.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

class CSSFragmentTest : public ::testing::Test {
 protected:
  CSSFragmentTest() {}
  ~CSSFragmentTest() {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CSSFragmentTest, TestGetFontFaceRule) {
  SharedCSSFragment fragment(0, {}, {}, {}, {});

  const auto& font_face_token_map = fragment.GetFontFaceRuleMap();
  EXPECT_TRUE(font_face_token_map.empty());

  const auto& font_face_rule = fragment.GetFontFaceRule("test");
  EXPECT_TRUE(font_face_rule.empty());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
