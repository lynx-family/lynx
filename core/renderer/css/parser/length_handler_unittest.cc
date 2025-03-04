// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/length_handler.h"

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
  EXPECT_EQ(output[id].GetValue().Number(), 10);
  EXPECT_TRUE(output[id].IsPx());

  impl = lepus::Value("1em");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 1);
  EXPECT_TRUE(output[id].IsEm());

  impl = lepus::Value("2.1rem");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 2.1);
  EXPECT_TRUE(output[id].IsRem());

  impl = lepus::Value("0.7rpx");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.7);
  EXPECT_TRUE(output[id].IsRpx());

  impl = lepus::Value("0.7vw");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.7);
  EXPECT_TRUE(output[id].IsVw());

  impl = lepus::Value("0.7vh");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.7);
  EXPECT_TRUE(output[id].IsVh());

  impl = lepus::Value("10%");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 10);
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
  EXPECT_EQ(css_value.GetValue().Number(), 10);
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
