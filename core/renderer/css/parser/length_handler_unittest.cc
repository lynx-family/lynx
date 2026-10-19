// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/length_handler.h"

#include <string>
#include <tuple>

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(LengthHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDWidth;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value(true);
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("10px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 10);
  EXPECT_TRUE(output[id].IsPx());

  impl = lepus::Value("1em");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 1);
  EXPECT_TRUE(output[id].IsEm());

  impl = lepus::Value("2.1rem");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 2.1);
  EXPECT_TRUE(output[id].IsRem());

  impl = lepus::Value("0.7rpx");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 0.7);
  EXPECT_TRUE(output[id].IsRpx());

  impl = lepus::Value("0.7vw");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 0.7);
  EXPECT_TRUE(output[id].IsVw());

  impl = lepus::Value("0.7vh");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 0.7);
  EXPECT_TRUE(output[id].IsVh());

  impl = lepus::Value("10%");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 10);
  EXPECT_TRUE(output[id].IsPercent());

  impl = lepus::Value("calc(2px + 3rpx)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsString());
  EXPECT_EQ(output[id].GetValue().StringView(), "calc(2px + 3rpx)");
  EXPECT_TRUE(output[id].IsCalc());

  output.clear();
  impl = lepus::Value("abcd");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.find(id), output.end());
}

TEST(LengthHandler, Process) {
  auto impl = lepus::Value(true);
  CSSValue css_value;
  CSSParserConfigs configs;
  EXPECT_FALSE(LengthHandler::Process(impl, css_value, configs));

  impl = lepus::Value("10px");
  EXPECT_TRUE(LengthHandler::Process(impl, css_value, configs));
  EXPECT_TRUE(css_value.IsPx());
  EXPECT_EQ(css_value.GetNumber(), 10);
}

TEST(LengthHandler, NumericLengthUsesPxPattern) {
  auto impl = lepus::Value(10);
  CSSValue css_value;
  CSSParserConfigs configs;

  EXPECT_TRUE(LengthHandler::Process(impl, css_value, configs));
  EXPECT_TRUE(css_value.IsPx());
  EXPECT_EQ(css_value.GetNumber(), 10);
}

TEST(LengthHandler, BareNumbersMatchExplicitPx) {
  CSSParserConfigs configs;
  const CSSPropertyID properties[] = {kPropertyIDWidth,
                                      kPropertyIDHeight,
                                      kPropertyIDMinWidth,
                                      kPropertyIDMaxHeight,
                                      kPropertyIDTop,
                                      kPropertyIDLeft,
                                      kPropertyIDMarginLeft,
                                      kPropertyIDPaddingTop,
                                      kPropertyIDBorderTopWidth,
                                      kPropertyIDFontSize,
                                      kPropertyIDFlexBasis,
                                      kPropertyIDLetterSpacing,
                                      kPropertyIDLineSpacing,
                                      kPropertyIDListMainAxisGap,
                                      kPropertyIDListCrossAxisGap};
  for (bool check_units : {false, true}) {
    configs.enable_length_unit_check = check_units;
    for (auto property : properties) {
      for (double number : {0.0, 12.5, -2.5}) {
        SCOPED_TRACE(static_cast<int>(property));
        SCOPED_TRACE(number);
        auto raw = std::to_string(number);
        auto text = UnitHandler::Process(property, lepus::Value(raw), configs);
        auto typed =
            UnitHandler::Process(property, lepus::Value(number), configs);
        auto px =
            UnitHandler::Process(property, lepus::Value(raw + "px"), configs);
        ASSERT_EQ(text.size(), 1u);
        ASSERT_EQ(typed.size(), 1u);
        ASSERT_EQ(px.size(), 1u);
        EXPECT_TRUE(text[property].IsPx());
        EXPECT_EQ(text[property], px[property]);
        EXPECT_EQ(typed[property].GetNumber(), text[property].GetNumber());
        EXPECT_EQ(typed[property].GetPattern(), text[property].GetPattern());
      }
    }
  }
}

TEST(LengthHandler, ShorthandAndNestedLengthsMatchPx) {
  const std::tuple<CSSPropertyID, const char*, const char*> cases[] = {
      {kPropertyIDMargin, "1 2.5 0 -4", "1px 2.5px 0px -4px"},
      {kPropertyIDPadding, "1 2", "1px 2px"},
      {kPropertyIDBorderWidth, "1 2 3 4", "1px 2px 3px 4px"},
      {kPropertyIDBorder, "2 solid red", "2px solid red"},
      {kPropertyIDOutline, "2 solid red", "2px solid red"},
      {kPropertyIDBorderRadius, "1 2 / 3 4", "1px 2px / 3px 4px"},
      {kPropertyIDBoxShadow, "1 2 3 4 red", "1px 2px 3px 4px red"},
      {kPropertyIDTextShadow, "1 2 3 red", "1px 2px 3px red"},
      {kPropertyIDBackgroundSize, "1 2", "1px 2px"},
      {kPropertyIDBackgroundPosition, "1 2", "1px 2px"},
      {kPropertyIDBackground, "red 1 2 / 3 4", "red 1px 2px / 3px 4px"},
      {kPropertyIDTransform, "translate(1, 2) scale(2)",
       "translate(1px, 2px) scale(2)"},
      {kPropertyIDTransformOrigin, "1 2", "1px 2px"},
      {kPropertyIDFilter, "blur(2)", "blur(2px)"},
      {kPropertyIDTextStroke, "2 red", "2px red"},
      {kPropertyIDGap, "1 2", "1px 2px"},
      {kPropertyIDClipPath, "inset(1 2 3 4)", "inset(1px 2px 3px 4px)"},
      {kPropertyIDClipPath, "circle(2 at 3 4)", "circle(2px at 3px 4px)"},
      {kPropertyIDClipPath, "super-ellipse(100 100 2 3 at 100)",
       "super-ellipse(100px 100px 2 3 at 100px)"},
      {kPropertyIDGridTemplateColumns, "repeat(2, 3 4)", "repeat(2, 3px 4px)"},
      {kPropertyIDBackgroundSize, "1\t2", "1px 2px"}};
  for (bool check_units : {false, true}) {
    CSSParserConfigs configs;
    configs.enable_length_unit_check = check_units;
    for (const auto& [property, raw, px] : cases) {
      SCOPED_TRACE(raw);
      auto actual = UnitHandler::Process(property, lepus::Value(raw), configs);
      auto expected = UnitHandler::Process(property, lepus::Value(px), configs);
      ASSERT_FALSE(expected.empty());
      ASSERT_EQ(actual.size(), expected.size());
      for (const auto& entry : expected) {
        ASSERT_TRUE(actual.find(entry.first) != actual.end());
        EXPECT_EQ(actual[entry.first], entry.second);
      }
    }
  }
}

TEST(LengthHandler, TextDecorationPatternLengths) {
  StyleMap output;
  CSSParserConfigs configs;

  EXPECT_TRUE(
      UnitHandler::Process(CSSPropertyID::kPropertyIDXTextDecorationWidth,
                           lepus::Value("8px"), output, configs));
  ASSERT_TRUE(output.find(CSSPropertyID::kPropertyIDXTextDecorationWidth) !=
              output.end());
  EXPECT_TRUE(output[CSSPropertyID::kPropertyIDXTextDecorationWidth].IsPx());
  EXPECT_EQ(output[CSSPropertyID::kPropertyIDXTextDecorationWidth].GetNumber(),
            8);

  EXPECT_TRUE(UnitHandler::Process(CSSPropertyID::kPropertyIDXTextDecorationGap,
                                   lepus::Value("4px"), output, configs));
  ASSERT_TRUE(output.find(CSSPropertyID::kPropertyIDXTextDecorationGap) !=
              output.end());
  EXPECT_TRUE(output[CSSPropertyID::kPropertyIDXTextDecorationGap].IsPx());
  EXPECT_EQ(output[CSSPropertyID::kPropertyIDXTextDecorationGap].GetNumber(),
            4);

  output.clear();
  EXPECT_FALSE(
      UnitHandler::Process(CSSPropertyID::kPropertyIDXTextDecorationWidth,
                           lepus::Value("8px 0.5"), output, configs));
  EXPECT_TRUE(output.empty());
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
