// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/auto_font_size_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(AutoFontSizeHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDXAutoFontSize;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr0 = output[id].GetValue().Array();
  EXPECT_FALSE(arr0->get(0).Bool());

  output.clear();
  impl = lepus::Value("false");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr1 = output[id].GetValue().Array();
  EXPECT_FALSE(arr1->get(0).Bool());

  output.clear();
  impl = lepus::Value("true 8px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr2 = output[id].GetValue().Array();
  EXPECT_TRUE(arr2->get(0).Bool());
  EXPECT_EQ(arr2->get(1).Number(), 8.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(2).Number()),
            CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("true 8px 20.8px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr3 = output[id].GetValue().Array();
  EXPECT_TRUE(arr3->get(0).Bool());
  EXPECT_EQ(arr3->get(1).Number(), 8.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(2).Number()),
            CSSValuePattern::PX);
  EXPECT_EQ(arr3->get(3).Number(), 20.8);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(4).Number()),
            CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("true 8px 20.8px 3px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr4 = output[id].GetValue().Array();
  EXPECT_TRUE(arr4->get(0).Bool());
  EXPECT_EQ(arr4->get(5).Number(), 3.f);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr4->get(6).Number()),
            CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("true 8px 20.8px 3px 88");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("true px 20.8px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("true px 20.8px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value(10);
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
