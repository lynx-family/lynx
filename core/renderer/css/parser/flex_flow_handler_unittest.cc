// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/flex_flow_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace starlight;
namespace tasm {
namespace test {

TEST(FlexFlowHandler, OneDirection) {
  auto id = CSSPropertyID::kPropertyIDFlexFlow;
  StyleMap output;
  CSSParserConfigs configs;
  lepus::Value impl;

  output.clear();
  impl = lepus::Value("row");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kRow));

  output.clear();
  impl = lepus::Value("row-reverse");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kRowReverse));

  output.clear();
  impl = lepus::Value("column");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kColumn));

  output.clear();
  impl = lepus::Value("column-reverse");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kColumnReverse));
}

TEST(FlexFlowHandler, OneWrap) {
  auto id = CSSPropertyID::kPropertyIDFlexFlow;
  StyleMap output;
  CSSParserConfigs configs;
  lepus::Value impl;

  output.clear();
  impl = lepus::Value("nowrap");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kNowrap));

  output.clear();
  impl = lepus::Value("wrap");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kWrap));

  output.clear();
  impl = lepus::Value("wrap-reverse");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(1));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kWrapReverse));
}

TEST(FlexFlowHandler, TwoValues) {
  auto id = CSSPropertyID::kPropertyIDFlexFlow;
  StyleMap output;
  CSSParserConfigs configs;
  lepus::Value impl;

  output.clear();
  impl = lepus::Value("row nowrap");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kRow));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kNowrap));

  output.clear();
  impl = lepus::Value("column-reverse wrap-reverse");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kColumnReverse));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kWrapReverse));

  // wrap direction
  output.clear();
  impl = lepus::Value("wrap-reverse column-reverse ");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDFlexDirection].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexDirection].AsNumber(),
            static_cast<double>(FlexDirectionType::kColumnReverse));
  EXPECT_TRUE(output[kPropertyIDFlexWrap].IsEnum());
  EXPECT_EQ(output[kPropertyIDFlexWrap].AsNumber(),
            static_cast<double>(FlexWrapType::kWrapReverse));
}

TEST(FlexFlowHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDFlexFlow;
  StyleMap output;
  CSSParserConfigs configs;
  lepus::Value impl;
  bool ret;

  output.clear();
  impl = lepus::Value("invalid");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("row row");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("wrap wrap");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("column row");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
