// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/filter_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(FilterHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDFilter;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("grays(1");

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("grayscale(80%)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(3));
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::FilterType::kGrayscale));
  EXPECT_EQ(arr->get(1).Number(), 80);
  EXPECT_EQ(arr->get(2).Number(), static_cast<int>(CSSValuePattern::PERCENT));

  impl = lepus::Value("blur(20px)");
  output.clear();
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 3);
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::FilterType::kBlur));
  EXPECT_EQ(arr->get(1).Number(), 20);
  EXPECT_EQ(arr->get(2).Number(), static_cast<int>(CSSValuePattern::PX));
}
}  // namespace test
}  // namespace tasm
}  // namespace lynx
