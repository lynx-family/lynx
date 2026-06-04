// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/offset_distance_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

namespace {

void ExpectOffsetDistance(const lepus::Value& input, double expected) {
  StyleMap output;
  CSSParserConfigs configs;
  ASSERT_TRUE(
      UnitHandler::Process(kPropertyIDOffsetDistance, input, output, configs));
  ASSERT_FALSE(output.empty());
  ASSERT_NE(output.find(kPropertyIDOffsetDistance), output.end());
  EXPECT_TRUE(output[kPropertyIDOffsetDistance].IsNumber());
  EXPECT_DOUBLE_EQ(output[kPropertyIDOffsetDistance].GetNumber(), expected);
}
}  // namespace

TEST(OffsetDistanceHandler, Number) {
  ExpectOffsetDistance(lepus::Value(0), 0);
  ExpectOffsetDistance(lepus::Value(1), 1);
  ExpectOffsetDistance(lepus::Value("0"), 0);
  ExpectOffsetDistance(lepus::Value("1"), 1);
}

TEST(OffsetDistanceHandler, Percentage) {
  ExpectOffsetDistance(lepus::Value("0%"), 0);
  ExpectOffsetDistance(lepus::Value("50%"), 0.5);
  ExpectOffsetDistance(lepus::Value("100%"), 1);
}

TEST(OffsetDistanceHandler, Invalid) {
  StyleMap output;
  CSSParserConfigs configs;

  EXPECT_FALSE(UnitHandler::Process(kPropertyIDOffsetDistance,
                                    lepus::Value("100% foo"), output, configs));
  EXPECT_TRUE(output.empty());

  EXPECT_FALSE(UnitHandler::Process(kPropertyIDOffsetDistance,
                                    lepus::Value("auto"), output, configs));
  EXPECT_TRUE(output.empty());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
