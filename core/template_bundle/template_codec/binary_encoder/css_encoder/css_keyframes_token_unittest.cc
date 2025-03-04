// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"

#include "core/base/json/json_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace encoder {
namespace test {

TEST(EncodeCSSKeyframesToken, CSSKeyframesToken0) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "xxx"
        },
        "styles" : [{
          "keyText" : {
            "value" : "to"
          },
          "style" : [{
            "name" : "top",
            "value" : "200px"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());

  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 1);
  EXPECT_EQ(token.styles_["to"]->size(), 1);
}

TEST(EncodeCSSKeyframesToken, GetCSSKeyframesTokenName0) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "xxx"
        }
      })";

  auto json = base::strToJson(json_input.c_str());
  EXPECT_EQ(encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(json), "xxx");
}

TEST(EncodeCSSKeyframesToken, GetCSSKeyframesTokenName1) {
  std::string json_input = R"({
        "type" : "KeyframesRuleXX",
        "name" : {
          "value" : "xxx"
        }
      })";

  auto json = base::strToJson(json_input.c_str());
  EXPECT_EQ(encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(json), "");
}

TEST(EncodeCSSKeyframesToken, IsCSSKeyframesToken0) {
  std::string json_input = R"({
        "type" : "KeyframesRule"
      })";

  auto json = base::strToJson(json_input.c_str());
  EXPECT_TRUE(encoder::CSSKeyframesToken::IsCSSKeyframesToken(json));
}

TEST(EncodeCSSKeyframesToken, IsCSSKeyframesToken1) {
  std::string json_input = R"({
        "type" : "xxxx"
      })";

  auto json = base::strToJson(json_input.c_str());
  EXPECT_FALSE(encoder::CSSKeyframesToken::IsCSSKeyframesToken(json));
}

}  // namespace test
}  // namespace encoder
}  // namespace lynx
