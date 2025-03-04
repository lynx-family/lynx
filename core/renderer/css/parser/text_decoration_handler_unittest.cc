// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/text_decoration_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(TextDecorationHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDTextDecoration;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("none");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  auto arr1 = output[id].GetValue().Array();
  EXPECT_EQ((starlight::TextDecorationType)arr1->get(0).Number(),
            starlight::TextDecorationType::kNone);

  output.clear();
  impl = lepus::Value("underline line-through");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto arr2 = output[id].GetValue().Array();
  EXPECT_EQ((starlight::TextDecorationType)arr2->get(0).Number(),
            starlight::TextDecorationType::kUnderLine);
  EXPECT_EQ((starlight::TextDecorationType)arr2->get(1).Number(),
            starlight::TextDecorationType::kLineThrough);

  output.clear();
  impl = lepus::Value("yellow dashed underline");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto arr3 = output[id].GetValue().Array();
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(0).Number(),
            starlight::TextDecorationType::kColor);
  EXPECT_EQ(arr3->get(1).Number(), 0xffffff00);  // yellow rgb(255,255,0)
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(2).Number(),
            starlight::TextDecorationType::kDashed);
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(3).Number(),
            starlight::TextDecorationType::kUnderLine);
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
