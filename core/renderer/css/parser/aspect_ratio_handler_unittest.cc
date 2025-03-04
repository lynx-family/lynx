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
TEST(AspectRatioHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDAspectRatio;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("10/100");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_FLOAT_EQ(output[id].GetValue().Number(), 0.1);

  output.clear();
  impl = lepus::Value("10");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 10);

  output.clear();
  impl = lepus::Value(0.25);
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.25);

  output.clear();
  impl = lepus::Value(-0.75);
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), -0.75);
}
}  // namespace test

}  // namespace tasm
}  // namespace lynx
