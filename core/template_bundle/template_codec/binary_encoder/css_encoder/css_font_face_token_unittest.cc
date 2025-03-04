// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_font_face_token.h"

#include "core/base/json/json_util.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {

class CSSFontFaceRuleForEncode : public ::testing::Test {
 protected:
  CSSFontFaceRuleForEncode() {}
  ~CSSFontFaceRuleForEncode() {}

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(CSSFontFaceRuleForEncode, CSSFontFaceTokenConstruct) {
  lepus::Value obj = lepus::Value::CreateObject();
  obj.SetProperty("key1", lepus::Value("value1"));
  obj.SetProperty("key2", lepus::Value("value2"));
  obj.SetProperty("key3", lepus::Value("value3"));
  obj.SetProperty("key4", lepus::Value("       "));
  obj.SetProperty("font-family", lepus::Value("font-family-value"));

  CSSFontFaceToken token(obj);

  EXPECT_EQ(token.attrs_.size(), 5);
  EXPECT_EQ(token.font_family_, "font-family-value");
}

TEST_F(CSSFontFaceRuleForEncode, CSSFontFaceTokenConstructWithJson0) {
  std::string json_input = R"({
    "type": "FontFaceRule",
    "style" : {
        "font-family" : {
            "value": "font-family-value"
        },
        "key1" : {
            "value": "value1"
        },
        "key2" : {
            "value": "value2"
        },
        "key3" : {
            "value": "value3"
        }
    }
  })";

  auto json = base::strToJson(json_input.c_str());
  CSSFontFaceToken token(json, "json");

  EXPECT_EQ(token.attrs_.size(), 4);
  EXPECT_EQ(token.font_family_, "font-family-value");
  EXPECT_EQ(token.attrs_["key1"], "value1");
  EXPECT_EQ(token.attrs_["key2"], "value2");
  EXPECT_EQ(token.attrs_["key3"], "value3");
}

TEST_F(CSSFontFaceRuleForEncode, CSSFontFaceTokenConstructWithJson1) {
  std::string json_input = R"({
    "type": "FontFaceRule",
    "style" : [
        {
            "name": "font-family",
            "value": "font-family-value"
        },
        {
            "name": "key1",
            "value": "value1"
        },
        {
            "name": "key2",
            "value": "value2"
        },
        {
            "name": "key3",
            "value": "value3"
        }
    ]
  })";

  auto json = base::strToJson(json_input.c_str());
  CSSFontFaceToken token(json, "json");

  EXPECT_EQ(token.attrs_.size(), 4);
  EXPECT_EQ(token.font_family_, "font-family-value");
  EXPECT_EQ(token.attrs_["key1"], "value1");
  EXPECT_EQ(token.attrs_["key2"], "value2");
  EXPECT_EQ(token.attrs_["key3"], "value3");
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
