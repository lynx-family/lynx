// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/transform_origin_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(TransformOriginHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDTransformOrigin;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("10px");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 10);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(arr->get(2).Number(), 50);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  impl = lepus::Value("10px 10%");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 10);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PX);
  EXPECT_EQ(arr->get(2).Number(), 10);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  impl = lepus::Value("left top");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 0);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).Number(), 0);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  impl = lepus::Value("bottom right");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 100);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).Number(), 100);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  impl = lepus::Value("right bottom");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 100);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).Number(), 100);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  impl = lepus::Value("center  center ");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 50);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).Number(), 50);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);
}

TEST(TransformOriginHandler, Compatibility) {
  auto id = CSSPropertyID::kPropertyIDTransformOrigin;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("center, center");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(), 50);
  EXPECT_EQ(arr->get(1).Number(), (int)CSSValuePattern::PERCENT);
  EXPECT_EQ(arr->get(2).Number(), 50);
  EXPECT_EQ(arr->get(3).Number(), (int)CSSValuePattern::PERCENT);

  output.clear();
  configs.enable_new_transform_handler = true;
  impl = lepus::Value("center, center");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_FALSE(output[id].IsArray());
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
