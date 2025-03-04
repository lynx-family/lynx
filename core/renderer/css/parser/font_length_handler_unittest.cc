// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/aspect_ratio_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(FontLengthHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDLineHeight;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value(5);
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetPattern() == CSSValuePattern::NUMBER);
  EXPECT_EQ(output[id].GetValue().Number(), 5);

  output.clear();
  impl = lepus::Value("10");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetPattern() == CSSValuePattern::NUMBER);
  EXPECT_EQ(output[id].GetValue().Number(), 10);

  output.clear();
  impl = lepus::Value("20px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetPattern() == CSSValuePattern::PX);
  EXPECT_EQ(output[id].GetValue().Number(), 20);

  output.clear();
  impl = lepus::Value("30rpx");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 30);

  output.clear();
  impl = lepus::Value("40px 123456");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("normal");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  float UNDEFINED = 10E20;
  EXPECT_EQ(output[id].GetValue().Number(), UNDEFINED);
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
