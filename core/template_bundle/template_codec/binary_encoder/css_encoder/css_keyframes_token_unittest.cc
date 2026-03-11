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

// Test parsing comma-separated percentage list: "0%,50%,100%"
TEST(EncodeCSSKeyframesToken, CommaSeparatedPercentages) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "slideAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "0%,50%,100%"
          },
          "style" : [{
            "name" : "transform",
            "value" : "translateX(0px)\\"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  // Should be flattened into 3 separate keyframe entries with the same style
  EXPECT_EQ(token.styles_.size(), 3);
  EXPECT_NE(token.styles_.find("0%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("50%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("100%"), token.styles_.end());

  // All keys should share the same style map content
  EXPECT_EQ(token.styles_["0%"]->size(), 1);
  EXPECT_EQ(token.styles_["50%"]->size(), 1);
  EXPECT_EQ(token.styles_["100%"]->size(), 1);
}

// Test parsing comma-separated "from, to" keywords
TEST(EncodeCSSKeyframesToken, CommaSeparatedFromTo) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "fadeAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "from,to"
          },
          "style" : [{
            "name" : "opacity",
            "value" : "1"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 2);
  EXPECT_NE(token.styles_.find("from"), token.styles_.end());
  EXPECT_NE(token.styles_.find("to"), token.styles_.end());
}

// Test parsing with whitespace around commas: "0%, 25%, 50% , 75% , 100%"
TEST(EncodeCSSKeyframesToken, CommaSeparatedWithWhitespace) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "bounceAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "0%, 25%, 50% , 75% , 100%"
          },
          "style" : [{
            "name" : "top",
            "value" : "0px"
          }, {
            "name" : "left",
            "value" : "100px"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 5);
  EXPECT_NE(token.styles_.find("0%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("25%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("50%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("75%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("100%"), token.styles_.end());

  // Verify styles are correctly parsed
  for (const auto& pair : token.styles_) {
    EXPECT_EQ(pair.second->size(), 2);
  }
}

// Test parsing mixed comma-separated and single entries
TEST(EncodeCSSKeyframesToken, MixedCommaSeparatedAndSingle) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "complexAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "0%,100%"
          },
          "style" : [{
            "name" : "opacity",
            "value" : "0"
          }]
        }, {
          "keyText" : {
            "value" : "50%"
          },
          "style" : [{
            "name" : "opacity",
            "value" : "1"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 3);
  EXPECT_NE(token.styles_.find("0%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("50%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("100%"), token.styles_.end());

  // Verify different styles for different keys
  EXPECT_EQ(token.styles_["0%"]->size(), 1);
  EXPECT_EQ(token.styles_["50%"]->size(), 1);
  EXPECT_EQ(token.styles_["100%"]->size(), 1);
}

// Test parsing with trailing/leading whitespace
TEST(EncodeCSSKeyframesToken, LeadingTrailingWhitespace) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "trimAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "  0%  ,  50%  ,  100%  "
          },
          "style" : [{
            "name" : "width",
            "value" : "100%"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 3);
  EXPECT_NE(token.styles_.find("0%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("50%"), token.styles_.end());
  EXPECT_NE(token.styles_.find("100%"), token.styles_.end());
}

// Test parsing single keyframe (no commas)
TEST(EncodeCSSKeyframesToken, SingleKeyframeNoComma) {
  std::string json_input = R"({
        "type" : "KeyframesRule",
        "name" : {
          "value" : "singleAnimation"
        },
        "styles" : [{
          "keyText" : {
            "value" : "50%"
          },
          "style" : [{
            "name" : "width",
            "value" : "1.5px"
          }]
        }]
      })";

  auto json = base::strToJson(json_input.c_str());
  encoder::CSSKeyframesToken token(json, "file", tasm::CompileOptions());

  EXPECT_EQ(token.styles_.size(), 1);
  EXPECT_NE(token.styles_.find("50%"), token.styles_.end());
  EXPECT_EQ(token.styles_["50%"]->size(), 1);
}

}  // namespace test
}  // namespace encoder
}  // namespace lynx
