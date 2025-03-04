// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/bool_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(BoolHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDImplicitAnimation;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value(10);
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("true");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsBoolean());
  EXPECT_TRUE(output[id].GetValue().Bool());

  output.clear();
  impl = lepus::Value("True");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsBoolean());
  EXPECT_TRUE(output[id].GetValue().Bool());

  output.clear();
  impl = lepus::Value("false");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsBoolean());
  EXPECT_FALSE(output[id].GetValue().Bool());

  output.clear();
  impl = lepus::Value("False");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsBoolean());
  EXPECT_FALSE(output[id].GetValue().Bool());
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
