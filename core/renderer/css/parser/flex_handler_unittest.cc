// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/flex_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

TEST(FlexHandler, One) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_new_flex_handler = true;

  output.clear();
  auto impl = lepus::Value("2");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);

  output.clear();
  impl = lepus::Value("20px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPx());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 20);
}

TEST(FlexHandler, Two) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  // flex-grow | flex-basis
  auto impl = lepus::Value("3 100px");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPx());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 100);

  output.clear();
  // flex-grow | flex-shrink
  impl = lepus::Value("2 3");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);
}

TEST(FlexHandler, Three) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  // flex-grow | flex-shrink | flex-basis
  auto impl = lepus::Value("2 3 10%");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPercent());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 10);

  output.clear();
  // flex-basis | flex-grow | flex-shrink
  impl = lepus::Value("10% 2 3");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPercent());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 10);

  output.clear();
  // flex-grow | flex-shrink | flex-basis
  impl = lepus::Value("2 3 0");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);

  output.clear();
  // flex-grow | flex-shrink | flex-basis(compat)
  impl = lepus::Value("2 3 5");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 5);

  output.clear();
  // flex-grow | flex-shrink | flex-basis
  impl = lepus::Value("1 0 100px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPx());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 100);
}

TEST(FlexHandler, EnableLengthUnitCheck) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_length_unit_check = true;

  output.clear();
  // flex-grow | flex-shrink | flex-basis
  auto impl = lepus::Value("2 3 10%");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPercent());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 10);

  // invalid
  impl = lepus::Value("10 2 3");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

TEST(FlexHandler, Legacy) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_new_flex_handler = false;

  auto impl = lepus::Value("20px");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsPx());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 20);

  impl = lepus::Value("none");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(),
            static_cast<int>(starlight::LengthValueType::kAuto));
}

TEST(FlexHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("hello");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_EQ(output.size(), 0);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
