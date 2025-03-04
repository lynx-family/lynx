// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/four_sides_shorthand_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

TEST(FourSidesShorthandHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDMargin;
  StyleMap output;
  CSSParserConfigs configs;

  // input invalid.
  auto impl = lepus::Value(true);
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  // valid cases.
  output.clear();
  impl = lepus::Value("2px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDMarginTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginLeft) == output.end());
  EXPECT_EQ(output[kPropertyIDMarginLeft], output[kPropertyIDMarginRight]);
  EXPECT_EQ(output[kPropertyIDMarginLeft], output[kPropertyIDMarginTop]);
  EXPECT_EQ(output[kPropertyIDMarginLeft], output[kPropertyIDMarginRight]);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_EQ((int)output[kPropertyIDMarginTop].GetValue().Number(), 2);

  output.clear();
  impl = lepus::Value("2px 3px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDMarginTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginLeft) == output.end());
  EXPECT_EQ(output[kPropertyIDMarginTop], output[kPropertyIDMarginBottom]);
  EXPECT_EQ(output[kPropertyIDMarginRight], output[kPropertyIDMarginLeft]);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsPx());
  EXPECT_EQ((int)output[kPropertyIDMarginTop].GetValue().Number(), 2);
  EXPECT_EQ((int)output[kPropertyIDMarginRight].GetValue().Number(), 3);

  output.clear();
  impl = lepus::Value("2px 3px 4px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDMarginTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginLeft) == output.end());
  EXPECT_NE(output[kPropertyIDMarginTop], output[kPropertyIDMarginBottom]);
  EXPECT_EQ(output[kPropertyIDMarginRight], output[kPropertyIDMarginLeft]);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginBottom].IsPx());
  EXPECT_EQ((int)output[kPropertyIDMarginTop].GetValue().Number(), 2);
  EXPECT_EQ((int)output[kPropertyIDMarginRight].GetValue().Number(), 3);
  EXPECT_EQ((int)output[kPropertyIDMarginBottom].GetValue().Number(), 4);
  EXPECT_EQ((int)output[kPropertyIDMarginLeft].GetValue().Number(), 3);

  output.clear();
  impl = lepus::Value("2px 3px 4px 5px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDMarginTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginLeft) == output.end());
  EXPECT_NE(output[kPropertyIDMarginTop], output[kPropertyIDMarginBottom]);
  EXPECT_NE(output[kPropertyIDMarginRight], output[kPropertyIDMarginLeft]);
  EXPECT_NE(output[kPropertyIDMarginTop], output[kPropertyIDMarginRight]);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginBottom].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginLeft].IsPx());
  EXPECT_EQ((int)output[kPropertyIDMarginTop].GetValue().Number(), 2);
  EXPECT_EQ((int)output[kPropertyIDMarginRight].GetValue().Number(), 3);
  EXPECT_EQ((int)output[kPropertyIDMarginBottom].GetValue().Number(), 4);
  EXPECT_EQ((int)output[kPropertyIDMarginLeft].GetValue().Number(), 5);

  output.clear();
  impl = lepus::Value(" 2px  3px    4px     5px  ");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDMarginTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDMarginLeft) == output.end());
  EXPECT_NE(output[kPropertyIDMarginTop], output[kPropertyIDMarginBottom]);
  EXPECT_NE(output[kPropertyIDMarginRight], output[kPropertyIDMarginLeft]);
  EXPECT_NE(output[kPropertyIDMarginTop], output[kPropertyIDMarginRight]);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginBottom].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginLeft].IsPx());
  EXPECT_EQ((int)output[kPropertyIDMarginTop].GetValue().Number(), 2);
  EXPECT_EQ((int)output[kPropertyIDMarginRight].GetValue().Number(), 3);
  EXPECT_EQ((int)output[kPropertyIDMarginBottom].GetValue().Number(), 4);
  EXPECT_EQ((int)output[kPropertyIDMarginLeft].GetValue().Number(), 5);

  output.clear();
  impl = lepus::Value("2px 3em 4rem 5rpx");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPx());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsEm());
  EXPECT_TRUE(output[kPropertyIDMarginBottom].IsRem());
  EXPECT_TRUE(output[kPropertyIDMarginLeft].IsRpx());

  output.clear();
  impl = lepus::Value("2% 3");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[kPropertyIDMarginTop].IsPercent());
  EXPECT_TRUE(output[kPropertyIDMarginRight].IsNumber());

  // invalid cases.
  output.clear();
  impl = lepus::Value("2test");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output.find(kPropertyIDMarginTop) == output.end());

  output.clear();
  impl = lepus::Value("test");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output.find(kPropertyIDMarginTop) == output.end());

  // padding
  id = kPropertyIDPadding;
  output.clear();
  impl = lepus::Value("2px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDPaddingTop) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDPaddingRight) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDPaddingBottom) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDPaddingLeft) == output.end());
  EXPECT_EQ(output[kPropertyIDPaddingLeft], output[kPropertyIDPaddingRight]);
  EXPECT_EQ(output[kPropertyIDPaddingLeft], output[kPropertyIDPaddingTop]);
  EXPECT_EQ(output[kPropertyIDPaddingLeft], output[kPropertyIDPaddingRight]);
  EXPECT_TRUE(output[kPropertyIDPaddingTop].IsPx());
  EXPECT_EQ((int)output[kPropertyIDPaddingTop].GetValue().Number(), 2);

  // border-width
  id = kPropertyIDBorderWidth;
  output.clear();
  impl = lepus::Value("2px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDBorderTopWidth) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderRightWidth) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderBottomWidth) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderLeftWidth) == output.end());
  EXPECT_EQ(output[kPropertyIDBorderLeftWidth],
            output[kPropertyIDBorderRightWidth]);
  EXPECT_EQ(output[kPropertyIDBorderLeftWidth],
            output[kPropertyIDBorderTopWidth]);
  EXPECT_EQ(output[kPropertyIDBorderLeftWidth],
            output[kPropertyIDBorderRightWidth]);
  EXPECT_TRUE(output[kPropertyIDBorderTopWidth].IsPx());
  EXPECT_EQ((int)output[kPropertyIDBorderTopWidth].GetValue().Number(), 2);

  // border-style
  id = kPropertyIDBorderStyle;
  output.clear();
  impl = lepus::Value("solid dashed dotted double");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDBorderTopStyle) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderRightStyle) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderBottomStyle) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderLeftStyle) == output.end());
  EXPECT_TRUE(output[kPropertyIDBorderTopStyle].IsEnum());
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderTopStyle].GetValue().Number(),
      BorderStyleType::kSolid);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderRightStyle].GetValue().Number(),
      BorderStyleType::kDashed);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderBottomStyle].GetValue().Number(),
      BorderStyleType::kDotted);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderLeftStyle].GetValue().Number(),
      BorderStyleType::kDouble);

  output.clear();
  impl = lepus::Value("groove ridge inset outset");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderTopStyle].GetValue().Number(),
      BorderStyleType::kGroove);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderRightStyle].GetValue().Number(),
      BorderStyleType::kRidge);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderBottomStyle].GetValue().Number(),
      BorderStyleType::kInset);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderLeftStyle].GetValue().Number(),
      BorderStyleType::kOutset);

  output.clear();
  impl = lepus::Value("hidden none");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderTopStyle].GetValue().Number(),
      BorderStyleType::kHide);
  EXPECT_EQ(
      (BorderStyleType)output[kPropertyIDBorderRightStyle].GetValue().Number(),
      BorderStyleType::kNone);

  output.clear();
  impl = lepus::Value("notstyle");

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  // border-color
  id = kPropertyIDBorderColor;
  output.clear();
  impl = lepus::Value("red #00ff00 #00ff00ee rgb(0,0,255)");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(4));
  EXPECT_FALSE(output.find(kPropertyIDBorderTopColor) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderRightColor) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderBottomColor) == output.end());
  EXPECT_FALSE(output.find(kPropertyIDBorderLeftColor) == output.end());
  EXPECT_TRUE(output[kPropertyIDBorderTopColor].IsNumber());
  EXPECT_EQ(output[kPropertyIDBorderTopColor].GetValue().UInt32(), 4294901760);
  EXPECT_EQ(output[kPropertyIDBorderRightColor].GetValue().UInt32(),
            4278255360);
  EXPECT_EQ(output[kPropertyIDBorderBottomColor].GetValue().UInt32(),
            3993042688);
  EXPECT_EQ(output[kPropertyIDBorderLeftColor].GetValue().UInt32(), 4278190335);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
