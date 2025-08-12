// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/style_object_encoder/style_object_parser.h"

#include "base/include/value/base_value.h"
#include "core/renderer/css/css_property_id.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx::tasm::test {
constexpr auto* kStyleObjectsJson = R"(
  {
    "simpleStyleObjects": [
    {
      "min-height": "100vh"
    },
    {
      "display": "flex"
    },
    {
      "flex-direction": "column"
    },
    {
      "align-items": "center"
    },
    {
      "justify-content": "center"
    },
    {
      "color": "red"
    },
    {
      "background-color": "#282c34"
    },
    {
      "background-color": "#e0e0e0"
    },
    {
      "color": "blue"
    },
    {
      "background-color": "yellow"
    },
    {
      "font-size": "20px"
    },
    {
      "font-size": "16px"
    },
    {
      "margin": "5px"
    },
    {
      "width": "160px"
    },
    {
      "height": "160px"
    },
    {
      "margin-bottom": "8px"
    },
    {
      "animation": "Logo--spin infinite 20s linear"
    },
    {
      "width": "120px"
    },
    {
      "height": "144px"
    },
    {
      "animation": "Logo--shake infinite 0.5s ease"
    },
    {
      "font-size": "36px"
    },
    {
      "font-weight": "700"
    },
    {
      "font-style": "italic"
    },
    {
      "font-size": "22px"
    },
    {
      "font-weight": "600"
    },
    {
      "color": "cyan"
    },
    {
      "color": "yellow"
    },
    {
      "color": "white"
    }
  ]
  }
  )";

TEST(StyleObjectParser, ParseSimpleStyleObject) {
  rapidjson::Document document;
  document.Parse(kStyleObjectsJson);
  if (document.HasMember("simpleStyleObjects")) {
    CompileOptions encoder_options;
    encoder_options.enable_simple_styling_ = true;
    auto style_object_parser =
        std::make_unique<StyleObjectParser>(encoder_options);
    style_object_parser->Parse(document["simpleStyleObjects"]);
    auto& style_objects = style_object_parser->StyleObjects();
    ASSERT_EQ(style_objects.size(), 28);
    auto& style_rule_min_height = style_objects.front();
    ASSERT_EQ(style_rule_min_height.Properties().size(), 1);
    ASSERT_TRUE(
        style_rule_min_height.Properties().contains(kPropertyIDMinHeight));
  }
}

TEST(StyleObjectParser, ParseFontFaceRule) {
  std::string json_input = R"(
  {
    "simpleStyleObjects": [
      {
      "type": "FontFaceRule",
      "style": [
        {
          "name": "font-family",
          "value": "test"
        },
        {
          "name": "src",
          "value": "link"
        }
      ]
    }
   ]
   })";

  rapidjson::Document document;
  document.Parse(json_input);
  CompileOptions encoder_options;
  encoder_options.enable_simple_styling_ = true;
  auto style_object_parser =
      std::make_unique<StyleObjectParser>(encoder_options);
  style_object_parser->Parse(document["simpleStyleObjects"]);
  auto& fontfaces = style_object_parser->StyleObjectsFontFaces();
  ASSERT_EQ(fontfaces.size(), 1);
  auto fontface = fontfaces.begin();
  EXPECT_EQ(fontface->first, "test");
  EXPECT_EQ(fontface->second[0]->GetAttrMap().at("font-family"), "test");
  EXPECT_EQ(fontface->second[0]->GetAttrMap().at("src"), "link");
}

}  // namespace lynx::tasm::test
