// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/number_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(NumberHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDOpacity;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value(true);

  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("test");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("0.85");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.85);

  output.clear();
  impl = lepus::Value(0.99);
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 0.99);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
