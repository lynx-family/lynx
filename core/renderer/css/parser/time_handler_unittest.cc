// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/time_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
TEST(TimeHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDAnimationDuration;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_new_time_handler = true;
  auto impl = lepus::Value(true);
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  output.clear();
  impl = lepus::Value("2s");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 2000);

  output.clear();
  impl = lepus::Value("2ms");
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 2);

  output.clear();
  impl = lepus::Value("2000ms, 1s, 10s");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsArray());
  auto arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(3));
  EXPECT_EQ(arr->get(0).Number(), 2000);
  EXPECT_EQ(arr->get(1).Number(), 1000);
  EXPECT_EQ(arr->get(2).Number(), 10000);

  output.clear();
  impl = lepus::Value("2000ms,1s,   010ms ");
  UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) != output.end());
  EXPECT_TRUE(output[id].IsArray());
  arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->size(), static_cast<size_t>(3));
  EXPECT_EQ(arr->get(0).Number(), 2000);
  EXPECT_EQ(arr->get(1).Number(), 1000);
  EXPECT_EQ(arr->get(2).Number(), 10);
}

TEST(TimeHandler, Invalid) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDAnimationDuration;
  CSSParserConfigs configs;
  configs.enable_new_time_handler = true;

  StyleMap output;
  auto impl = lepus::Value("200");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  impl = lepus::Value("0");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  impl = lepus::Value("abc");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));

  impl = lepus::Value("7 ms");
  EXPECT_FALSE(UnitHandler::Process(id, impl, output, configs));
}

TEST(TimeHandler, Compatibility) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDAnimationDuration;
  CSSParserConfigs configs;
  auto impl = lepus::Value("200");
  StyleMap output;
  EXPECT_TRUE(UnitHandler::Process(id, impl, output, configs));
  EXPECT_TRUE(output[id].IsNumber());
  EXPECT_EQ(output[id].GetValue().Number(), 200);
}

TEST(TimeHandler, Negative) {
  CSSParserConfigs configs;
  configs.enable_new_time_handler = true;
  auto impl = lepus::Value("-2ms");
  StyleMap output;
  EXPECT_FALSE(UnitHandler::Process(kPropertyIDAnimationDuration, impl, output,
                                    configs));
  EXPECT_TRUE(
      UnitHandler::Process(kPropertyIDAnimationDelay, impl, output, configs));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), -2);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
