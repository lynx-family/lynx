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
  EXPECT_EQ(output[id].GetNumber(), 0.85);

  output.clear();
  impl = lepus::Value(0.99);
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetNumber(), 0.99);
}

TEST(NumberHandler, NonNegative) {
  auto id = CSSPropertyID::kPropertyIDFlexShrink;
  StyleMap output;
  CSSParserConfigs configs;

  output.emplace_or_assign(id, 1, CSSValuePattern::NUMBER);
  EXPECT_FALSE(UnitHandler::Process(id, lepus::Value("-1"), output, configs));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[id].GetNumber(), 1);

  EXPECT_FALSE(UnitHandler::Process(id, lepus::Value(-2), output, configs));
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[id].GetNumber(), 1);

  EXPECT_TRUE(UnitHandler::Process(id, lepus::Value("0"), output, configs));
  EXPECT_EQ(output[id].GetNumber(), 0);

  output.emplace_or_assign(id, 1, CSSValuePattern::NUMBER);
  EXPECT_TRUE(UnitHandler::Process(id, lepus::Value("-0"), output, configs));
  EXPECT_EQ(output[id].GetNumber(), 0);

  EXPECT_TRUE(UnitHandler::Process(id, lepus::Value(2), output, configs));
  EXPECT_EQ(output[id].GetNumber(), 2);

  auto generic_id = CSSPropertyID::kPropertyIDOrder;
  EXPECT_TRUE(
      UnitHandler::Process(generic_id, lepus::Value(-1), output, configs));
  EXPECT_EQ(output[generic_id].GetNumber(), -1);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
