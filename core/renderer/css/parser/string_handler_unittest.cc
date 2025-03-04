// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/string_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(StringHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDFontFamily;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value(true);

  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("test");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsString());
  EXPECT_EQ(output[id].GetValue().StringView(), "test");

  output.clear();
  impl = lepus::Value("+(*^%$.");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsString());
  EXPECT_EQ(output[id].GetValue().StringView(), "+(*^%$.");

  output.clear();
  impl = lepus::Value();

  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value(20);

  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output.empty());
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
