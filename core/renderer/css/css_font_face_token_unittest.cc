// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/css_font_face_token.h"

#include "core/base/json/json_util.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {

class CSSFontFaceTokenTest : public ::testing::Test {
 protected:
  CSSFontFaceTokenTest() {}
  ~CSSFontFaceTokenTest() {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CSSFontFaceTokenTest, TestMakeCSSFontFaceToken) {
  lepus::Value obj = lepus::Value::CreateObject();
  obj.SetProperty("key1", lepus::Value("value1"));
  obj.SetProperty("key2", lepus::Value("value2"));
  obj.SetProperty("key3", lepus::Value("value3"));
  obj.SetProperty("key4", lepus::Value("       "));
  obj.SetProperty("font-family", lepus::Value("font-family-value"));

  CSSFontFaceRule* rule = MakeCSSFontFaceToken(obj);
  EXPECT_EQ(rule->second.size(), 5);
  EXPECT_EQ(rule->first, "font-family-value");

  EXPECT_EQ(rule->second["key1"], "value1");
  EXPECT_EQ(rule->second["key2"], "value2");
  EXPECT_EQ(rule->second["key3"], "value3");
  EXPECT_EQ(rule->second["key4"], "");
  EXPECT_EQ(rule->second["font-family"], "font-family-value");
  delete rule;
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
