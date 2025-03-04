// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/auto_font_size_preset_sizes_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(XAutoFontSizePresetSizesHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDXAutoFontSizePresetSizes;
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
  EXPECT_TRUE(arr0->size() == 0);

  output.clear();
  impl = lepus::Value("10px");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr1 = output[id].GetValue().Array();
  EXPECT_TRUE(arr1->size() == 2);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr1->get(1).Number()),
            CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("10rpx 8px 2em");
  UnitHandler::Process(id, impl, output, configs);
  auto arr2 = output[id].GetValue().Array();
  EXPECT_TRUE(arr2->size() == 6);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(1).Number()),
            CSSValuePattern::RPX);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(3).Number()),
            CSSValuePattern::PX);
  EXPECT_EQ(static_cast<CSSValuePattern>(arr2->get(5).Number()),
            CSSValuePattern::EM);
  EXPECT_EQ(arr2->get(2).Number(), 8.f);

  output.clear();
  impl = lepus::Value("true");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("10px dd 30px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
