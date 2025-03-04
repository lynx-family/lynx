// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/border_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

TEST(BorderHandler, Border) {
  auto id = CSSPropertyID::kPropertyIDBorder;
  auto style_id = CSSPropertyID::kPropertyIDBorderTopStyle;
  auto color_id = CSSPropertyID::kPropertyIDBorderTopColor;
  auto width_id = CSSPropertyID::kPropertyIDBorderTopWidth;
  StyleMap output;
  CSSParserConfigs configs;

  // input invalid.
  auto impl = lepus::Value(111);
  bool ret;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("10px double red");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(12));
  EXPECT_EQ(output[width_id].GetValue().Number(), 10);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0xffff0000);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);

  output.clear();
  impl = lepus::Value("10px double");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(8));
  EXPECT_EQ(output[width_id].GetValue().Number(), 10);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);

  output.clear();
  impl = lepus::Value("double");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  configs.enable_new_border_handler = true;
  // The new handler will fill the default value
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(12));
  EXPECT_EQ(output[width_id].GetValue().Number(), 0);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::NUMBER);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0xff000000);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);

  output.clear();
  impl = lepus::Value("double double");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
}

TEST(BorderHandler, BorderLine) {
  StyleMap output;
  CSSParserConfigs configs;
  CSSPropertyID lines[] = {
      CSSPropertyID::kPropertyIDBorderRight,
      CSSPropertyID::kPropertyIDBorderLeft,
      CSSPropertyID::kPropertyIDBorderTop,
      CSSPropertyID::kPropertyIDBorderBottom,
      CSSPropertyID::kPropertyIDOutline,
  };
  CSSPropertyID width_id;
  CSSPropertyID style_id;
  CSSPropertyID color_id;
  for (const auto& id : lines) {
    switch (id) {
      case kPropertyIDBorderTop:
        width_id = kPropertyIDBorderTopWidth;
        style_id = kPropertyIDBorderTopStyle;
        color_id = kPropertyIDBorderTopColor;
        break;
      case kPropertyIDBorderRight:
        width_id = kPropertyIDBorderRightWidth;
        style_id = kPropertyIDBorderRightStyle;
        color_id = kPropertyIDBorderRightColor;
        break;
      case kPropertyIDBorderBottom:
        width_id = kPropertyIDBorderBottomWidth;
        style_id = kPropertyIDBorderBottomStyle;
        color_id = kPropertyIDBorderBottomColor;
        break;
      case kPropertyIDBorderLeft:
        width_id = kPropertyIDBorderLeftWidth;
        style_id = kPropertyIDBorderLeftStyle;
        color_id = kPropertyIDBorderLeftColor;
        break;
      case kPropertyIDOutline:
        width_id = kPropertyIDOutlineWidth;
        style_id = kPropertyIDOutlineStyle;
        color_id = kPropertyIDOutlineColor;
        break;
      default:
        break;
    }
    output.clear();
    auto impl = lepus::Value("10px ridge black");
    EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_TRUE(output.find(id) == output.end());
    EXPECT_EQ(output.size(), static_cast<size_t>(3));
    EXPECT_EQ(output[width_id].GetValue().Number(), 10);
    EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
    EXPECT_EQ(output[style_id].GetValue().Number(),
              static_cast<double>(BorderStyleType::kRidge));
    EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
    EXPECT_EQ(output[color_id].GetValue().Number(), 0xff000000);
    EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);
  }
}

TEST(BorderHandler, Outline) {
  auto id = CSSPropertyID::kPropertyIDOutline;
  auto style_id = CSSPropertyID::kPropertyIDOutlineStyle;
  auto color_id = CSSPropertyID::kPropertyIDOutlineColor;
  auto width_id = CSSPropertyID::kPropertyIDOutlineWidth;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_length_unit_check = false;

  // input invalid.
  auto impl = lepus::Value(111);
  bool ret;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("10px double red");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(3));
  EXPECT_EQ(output[width_id].GetValue().Number(), 10);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0xffff0000);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);

  output.clear();
  impl = lepus::Value("10px double");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_EQ(output[width_id].GetValue().Number(), 10);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);

  output.clear();
  impl = lepus::Value("thick double red");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(3));

  EXPECT_EQ(output[width_id].GetValue().Number(), 5);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDouble));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0xffff0000);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);

  output.clear();
  impl = lepus::Value("medium dashed #0000ff");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(3));

  EXPECT_EQ(output[width_id].GetValue().Number(), 3);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kDashed));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0xff0000ff);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);

  output.clear();
  impl = lepus::Value("thin outset hsla(89,43%,51%,0.3)");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output.size(), static_cast<size_t>(3));

  EXPECT_EQ(output[width_id].GetValue().Number(), 1);
  EXPECT_EQ(output[width_id].GetPattern(), CSSValuePattern::PX);
  EXPECT_EQ(output[style_id].GetValue().Number(),
            static_cast<double>(BorderStyleType::kOutset));
  EXPECT_EQ(output[style_id].GetPattern(), CSSValuePattern::ENUM);
  EXPECT_EQ(output[color_id].GetValue().Number(), 0x4c84b84c);
  EXPECT_EQ(output[color_id].GetPattern(), CSSValuePattern::NUMBER);
}

static bool ToBorderStyleType(const std::string& str,
                              starlight::BorderStyleType& result) {
  if (str == "solid") {
    result = BorderStyleType::kSolid;
  } else if (str == "dashed") {
    result = BorderStyleType::kDashed;
  } else if (str == "dotted") {
    result = BorderStyleType::kDotted;
  } else if (str == "double") {
    result = BorderStyleType::kDouble;
  } else if (str == "groove") {
    result = BorderStyleType::kGroove;
  } else if (str == "ridge") {
    result = BorderStyleType::kRidge;
  } else if (str == "inset") {
    result = BorderStyleType::kInset;
  } else if (str == "outset") {
    result = BorderStyleType::kOutset;
  } else if (str == "hidden") {
    result = BorderStyleType::kHide;
  } else if (str == "none") {
    result = BorderStyleType::kNone;
  } else {
    return false;
  }
  return true;
}

TEST(BorderStyleHandler, BorderStyle) {
  StyleMap output;
  CSSParserConfigs configs;
  auto id = CSSPropertyID::kPropertyIDBorderTopStyle;
  std::string styles[] = {"none",   "hidden", "dotted", "dashed", "solid",
                          "double", "groove", "ridge",  "inset",  "outset"};

  for (const auto& style : styles) {
    output.clear();
    auto impl = lepus::Value(style);
    EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
    EXPECT_TRUE(output.find(id) != output.end());
    BorderStyleType type;
    ToBorderStyleType(style, type);
    EXPECT_EQ(output[id].GetValue().Int32(), static_cast<int>(type));
    EXPECT_EQ(output[id].GetPattern(), CSSValuePattern::ENUM);
  }
  output.clear();
  auto impl = lepus::Value("invalid");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
}

TEST(BorderWidthHandler, BorderWidth) {
  StyleMap output;
  CSSParserConfigs configs;
  auto id = CSSPropertyID::kPropertyIDBorderTopWidth;
  output.clear();
  auto impl = lepus::Value("thin");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output[id].GetValue().Number(), 1);
  EXPECT_EQ(output[id].GetPattern(), CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("10px");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_EQ(output[id].GetValue().Number(), 10);
  EXPECT_EQ(output[id].GetPattern(), CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("invalid");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.find(id) == output.end());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
