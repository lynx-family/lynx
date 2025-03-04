// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/shadow_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(ShadowHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDBoxShadow;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value();
  bool ret;

  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("1px 2px 3px 4px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(1));
  EXPECT_TRUE(arr->get(0).IsTable());
  auto table = arr->get(0).Table();
  EXPECT_TRUE(table->GetValue("enable").Bool());
  EXPECT_EQ(table->GetValue("color").Number(), 4294901760);
  auto item_arr = table->GetValue("h_offset").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 1);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  item_arr = table->GetValue("v_offset").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 2);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  item_arr = table->GetValue("blur").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 3);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  item_arr = table->GetValue("spread").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 4);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);

  output.clear();
  impl = lepus::Value("1px 2px 3px inset rgb(255, 0, 0)");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(1));
  EXPECT_TRUE(arr->get(0).IsTable());
  table = arr->get(0).Table();
  EXPECT_TRUE(table->GetValue("enable").Bool());
  EXPECT_EQ(table->GetValue("color").Number(), 4294901760);
  item_arr = table->GetValue("h_offset").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 1);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  item_arr = table->GetValue("v_offset").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 2);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  item_arr = table->GetValue("blur").Array();
  EXPECT_EQ(item_arr->get(0).Number(), 3);
  EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  EXPECT_EQ(table->GetValue("option").Number(),
            (int)starlight::ShadowOption::kInset);

  output.clear();
  impl =
      lepus::Value("1px 2px 3px inset rgb(255, 0, 0), 1px 2px 3px inset red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(2));
  for (size_t i = 0; i < arr->size(); i++) {
    EXPECT_TRUE(arr->get(i).IsTable());
    table = arr->get(i).Table();
    EXPECT_TRUE(table->GetValue("enable").Bool());
    EXPECT_EQ(table->GetValue("color").Number(), 0xffff0000);
    item_arr = table->GetValue("h_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 1);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("v_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 2);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("blur").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 3);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    EXPECT_EQ(table->GetValue("option").Number(),
              (int)starlight::ShadowOption::kInset);
  }

  output.clear();
  impl = lepus::Value("inset 1px 2px 3px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(1));
  for (size_t i = 0; i < arr->size(); i++) {
    EXPECT_TRUE(arr->get(i).IsTable());
    table = arr->get(i).Table();
    EXPECT_TRUE(table->GetValue("enable").Bool());
    EXPECT_EQ(table->GetValue("color").Number(), 0xffff0000);
    item_arr = table->GetValue("h_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 1);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("v_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 2);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("blur").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 3);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    EXPECT_EQ(table->GetValue("option").Number(),
              (int)starlight::ShadowOption::kInset);
  }
}

TEST(ShadowHandler, None) {
  auto id = CSSPropertyID::kPropertyIDBoxShadow;
  StyleMap output;
  CSSParserConfigs configs;
  auto impl = lepus::Value("none");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 0);

  impl = lepus::Value("NONE");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 0);

  impl = lepus::Value("none ");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), 0);

  impl = lepus::Value("none,");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
}

TEST(ShadowHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDBoxShadow;
  StyleMap output;
  CSSParserConfigs configs;

  bool ret;
  output.clear();
  auto impl = lepus::Value("1px 2px 3px inset red,");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("inset 1px 2px 3px inset red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("1px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST(ShadowHandler, TextShadow) {
  auto id = CSSPropertyID::kPropertyIDTextShadow;
  StyleMap output;
  CSSParserConfigs configs;

  bool ret;
  output.clear();
  auto impl = lepus::Value("1px 2px 3px inset red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("inset 1px 2px 3px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  // spread for text-shadow
  impl = lepus::Value("1px 2px 3px 4px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);

  output.clear();
  impl = lepus::Value("1px 2px 3px red");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(1));
  for (size_t i = 0; i < arr->size(); i++) {
    EXPECT_TRUE(arr->get(i).IsTable());
    auto table = arr->get(i).Table();
    EXPECT_TRUE(table->GetValue("enable").Bool());
    EXPECT_EQ(table->GetValue("color").Number(), 0xFFFF0000);
    auto item_arr = table->GetValue("h_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 1);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("v_offset").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 2);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
    item_arr = table->GetValue("blur").Array();
    EXPECT_EQ(item_arr->get(0).Number(), 3);
    EXPECT_EQ((CSSValuePattern)item_arr->get(1).Number(), CSSValuePattern::PX);
  }
}

}  // namespace test

}  // namespace tasm
}  // namespace lynx
