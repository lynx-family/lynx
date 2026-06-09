// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/text_decoration_handler.h"

#include "base/include/value/array.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
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
  auto arr1 = output[id].GetArray();
  EXPECT_EQ((starlight::TextDecorationType)arr1->get(0).Number(),
            starlight::TextDecorationType::kNone);

  output.clear();
  impl = lepus::Value("underline line-through");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto arr2 = output[id].GetArray();
  EXPECT_EQ((starlight::TextDecorationType)arr2->get(0).Number(),
            starlight::TextDecorationType::kUnderLine);
  EXPECT_EQ((starlight::TextDecorationType)arr2->get(1).Number(),
            starlight::TextDecorationType::kLineThrough);

  output.clear();
  impl = lepus::Value("yellow dashed underline");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto arr3 = output[id].GetArray();
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(0).Number(),
            starlight::TextDecorationType::kColor);
  EXPECT_EQ(arr3->get(1).Number(), 0xffffff00);  // yellow rgb(255,255,0)
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(2).Number(),
            starlight::TextDecorationType::kDashed);
  EXPECT_EQ((starlight::TextDecorationType)arr3->get(3).Number(),
            starlight::TextDecorationType::kUnderLine);

  output.clear();
  impl = lepus::Value("underline 2px dashed yellow");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(output[id].IsArray());
  auto arr4 = output[id].GetArray();
  ASSERT_EQ(arr4->size(), 7);
  EXPECT_EQ((starlight::TextDecorationType)arr4->get(0).Number(),
            starlight::TextDecorationType::kUnderLine);
  EXPECT_EQ((starlight::TextDecorationType)arr4->get(1).Number(),
            starlight::TextDecorationType::kThickness);
  EXPECT_EQ(arr4->get(2).Number(), 2);
  EXPECT_EQ((CSSValuePattern)arr4->get(3).Number(), CSSValuePattern::PX);
  EXPECT_EQ((starlight::TextDecorationType)arr4->get(4).Number(),
            starlight::TextDecorationType::kDashed);
  EXPECT_EQ((starlight::TextDecorationType)arr4->get(5).Number(),
            starlight::TextDecorationType::kColor);
  EXPECT_EQ(arr4->get(6).Number(), 0xffffff00);  // yellow rgb(255,255,0)

  output.clear();
  impl = lepus::Value("dashed red underline 4px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(output[id].IsArray());
  auto arr6 = output[id].GetArray();
  ASSERT_EQ(arr6->size(), 7);
  EXPECT_EQ((starlight::TextDecorationType)arr6->get(0).Number(),
            starlight::TextDecorationType::kDashed);
  EXPECT_EQ((starlight::TextDecorationType)arr6->get(1).Number(),
            starlight::TextDecorationType::kColor);
  EXPECT_EQ(arr6->get(2).Number(), 0xffff0000);  // red rgb(255,0,0)
  EXPECT_EQ((starlight::TextDecorationType)arr6->get(3).Number(),
            starlight::TextDecorationType::kUnderLine);
  EXPECT_EQ((starlight::TextDecorationType)arr6->get(4).Number(),
            starlight::TextDecorationType::kThickness);
  EXPECT_EQ(arr6->get(5).Number(), 4);
  EXPECT_EQ((CSSValuePattern)arr6->get(6).Number(), CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("none 2px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(output[id].IsArray());
  auto arr5 = output[id].GetArray();
  ASSERT_EQ(arr5->size(), 1);
  EXPECT_EQ((starlight::TextDecorationType)arr5->get(0).Number(),
            starlight::TextDecorationType::kNone);

  output.clear();
  impl = lepus::Value("underline 1px 2px");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
