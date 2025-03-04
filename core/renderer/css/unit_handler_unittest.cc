// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/unit_handler.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

using tasm::UnitHandler;

namespace css {
namespace testing {

TEST(UnitHandler, Process2) {
  tasm::StyleMap output;
  tasm::CSSParserConfigs configs;
  lepus::Value v("10px");
  EXPECT_FALSE(UnitHandler::Process(tasm::kPropertyStart, v, output, configs));
  EXPECT_FALSE(UnitHandler::Process(tasm::kPropertyEnd, v, output, configs));
  EXPECT_FALSE(
      UnitHandler::Process((tasm::CSSPropertyID)-100, v, output, configs));
  EXPECT_FALSE(
      UnitHandler::Process((tasm::CSSPropertyID)99999999, v, output, configs));
  EXPECT_TRUE(UnitHandler::Process(tasm::kPropertyIDWidth, v, output, configs));
}

TEST(UnitHandler, Process3) {
  lepus::Value v("10px");
  tasm::CSSParserConfigs configs;
  EXPECT_TRUE(UnitHandler::Process(tasm::kPropertyStart, v, configs).empty());
  EXPECT_TRUE(UnitHandler::Process(tasm::kPropertyEnd, v, configs).empty());
  EXPECT_TRUE(
      UnitHandler::Process((tasm::CSSPropertyID)-100, v, configs).empty());
  EXPECT_TRUE(
      UnitHandler::Process((tasm::CSSPropertyID)99999999, v, configs).empty());
  EXPECT_FALSE(
      UnitHandler::Process(tasm::kPropertyIDWidth, v, configs).empty());
}

}  // namespace testing

}  // namespace css
}  // namespace lynx
