// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/text_stroke_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
TEST(TextStrokeHandler0, Handle) {
  auto id = CSSPropertyID::kPropertyIDTextStroke;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value(" 1px");
  id = CSSPropertyID::kPropertyIDTextStroke;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());

  output.clear();
  impl = lepus::Value(" yellow");
  id = CSSPropertyID::kPropertyIDTextStroke;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());
}

TEST(TextStrokeHandler1, Handle) {
  auto id = CSSPropertyID::kPropertyIDTextStroke;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("1px yellow");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output[id].GetValue().Number(),
            0xffffff00);  // yellow rgb(255,255,0)
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 1);
  EXPECT_TRUE(output[id].IsPx());

  output.clear();
  impl = lepus::Value("            2px       yellow      ");
  id = CSSPropertyID::kPropertyIDTextStroke;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output[id].GetValue().Number(),
            0xffffff00);  // yellow rgb(255,255,0)
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 2.0);
  EXPECT_TRUE(output[id].IsPx());

  output.clear();
  impl = lepus::Value("         yellow         1px       ");
  id = CSSPropertyID::kPropertyIDTextStroke;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_EQ(output[id].GetValue().Number(),
            0xffffff00);  // yellow rgb(255,255,0)
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].GetValue().IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 1.0);
  EXPECT_TRUE(output[id].IsPx());

  output.clear();
  impl = lepus::Value("NoNe");
  id = CSSPropertyID::kPropertyIDTextStroke;
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  id = CSSPropertyID::kPropertyIDTextStrokeColor;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());
  id = CSSPropertyID::kPropertyIDTextStrokeWidth;
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsEmpty());
}

}  // namespace tasm
}  // namespace lynx
