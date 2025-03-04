// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/timing_function_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(TimingFunctionHandler, Keywords) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDAnimationTimingFunction;
  std::vector<std::pair<std::string, starlight::TimingFunctionType>> cases = {
      {"linear", starlight::TimingFunctionType::kLinear},
      {"ease-in", starlight::TimingFunctionType::kEaseIn},
      {"ease-out", starlight::TimingFunctionType::kEaseOut},
      {"ease-in-ease-out", starlight::TimingFunctionType::kEaseInEaseOut},
      {"ease", starlight::TimingFunctionType::kEaseInEaseOut},
      {"ease-in-out", starlight::TimingFunctionType::kEaseInEaseOut}};
  CSSParserConfigs configs;
  StyleMap output;
  for (const auto& s : cases) {
    output.clear();
    EXPECT_TRUE(
        UnitHandler::Process(id, lepus::Value(s.first), output, configs));
    EXPECT_FALSE(output.empty());

    EXPECT_TRUE(output[id].IsArray());
    const auto& arr = output[id].GetValue().Array();
    EXPECT_TRUE(arr->get(0).IsNumber());
    EXPECT_EQ(arr->get(0).Number(), static_cast<int>(s.second));
  }
}

TEST(TimingFunctionHandler, Square) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::string s = "square-bezier(1, 0.5)";
  EXPECT_TRUE(UnitHandler::Process(id, lepus::Value(s), output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].GetValue().IsArray());
  EXPECT_TRUE(output[id].GetValue().Array()->get(0).IsArray());
  const auto& arr = output[id].GetValue().Array()->get(0).Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kSquareBezier));
  EXPECT_EQ(arr->get(1).Number(), 1);
  EXPECT_EQ(arr->get(2).Number(), 0.5);
}

TEST(TimingFunctionHandler, Cubic) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::string s = "cubic-bezier(1, 0.5, 0.5, 1)";
  EXPECT_TRUE(UnitHandler::Process(id, lepus::Value(s), output, configs));
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].GetValue().IsArray());
  EXPECT_TRUE(output[id].GetValue().Array()->get(0).IsArray());
  const auto& arr = output[id].GetValue().Array()->get(0).Array();

  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kCubicBezier));
  EXPECT_EQ(arr->get(1).Number(), 1);
  EXPECT_EQ(arr->get(2).Number(), 0.5);
  EXPECT_EQ(arr->get(3).Number(), 0.5);
  EXPECT_EQ(arr->get(4).Number(), 1);
}

TEST(TimingFunctionHandler, StepsKeywords) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::vector<std::pair<std::string, starlight::StepsType>> cases = {
      {"step-start", starlight::StepsType::kStart},
      {"step-end", starlight::StepsType::kEnd}};

  for (const auto& s : cases) {
    output.clear();
    SCOPED_TRACE(s.first);
    EXPECT_TRUE(
        UnitHandler::Process(id, lepus::Value(s.first), output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output[id].GetValue().IsArray());
    const auto& arr = output[id].GetValue().Array();
    EXPECT_TRUE(arr->get(0).IsArray());
    const auto& item = arr->get(0).Array();
    EXPECT_EQ(item->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kSteps));
    EXPECT_EQ(item->get(1).Number(), 1);
    EXPECT_EQ(item->get(2).Number(), static_cast<int>(s.second));
  }
}

TEST(TimingFunctionHandler, StepsFunction) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::vector<std::pair<std::string, starlight::StepsType>> cases = {
      {"steps(1, jump-start)", starlight::StepsType::kStart},
      {"steps(1, jump-end)", starlight::StepsType::kEnd},
      {"steps(1, jump-none)", starlight::StepsType::kJumpNone},
      {"steps(1, jump-both)", starlight::StepsType::kJumpBoth},
      {"steps(1, jump-start)", starlight::StepsType::kStart},
      {"steps(1, jump-end)", starlight::StepsType::kEnd},
  };

  for (const auto& s : cases) {
    output.clear();
    SCOPED_TRACE(s.first);
    EXPECT_TRUE(
        UnitHandler::Process(id, lepus::Value(s.first), output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output[id].GetValue().IsArray());
    EXPECT_TRUE(output[id].GetValue().Array()->get(0).IsArray());
    const auto& arr = output[id].GetValue().Array()->get(0).Array();

    EXPECT_EQ(arr->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kSteps));
    EXPECT_EQ(arr->get(1).Number(), 1);
    EXPECT_EQ(arr->get(2).Number(), static_cast<int>(s.second));
  }
}

TEST(TimingFunctionHandler, Multi) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::vector<std::pair<std::string,
                        std::pair<starlight::StepsType, starlight::StepsType>>>
      cases = {{"steps(1, jump-start), step-end",
                {starlight::StepsType::kStart, starlight::StepsType::kEnd}},
               {"step-start, steps(1, jump-end)",
                {starlight::StepsType::kStart, starlight::StepsType::kEnd}}};

  for (const auto& s : cases) {
    output.clear();
    EXPECT_TRUE(
        UnitHandler::Process(id, lepus::Value(s.first), output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output[id].IsArray());
    EXPECT_TRUE(output[id].GetValue().Array()->get(0).IsArray());
    {
      const auto& arr = output[id].GetValue().Array()->get(0).Array();

      EXPECT_EQ(arr->get(0).Number(),
                static_cast<int>(starlight::TimingFunctionType::kSteps));
      EXPECT_EQ(arr->get(1).Number(), 1);
      EXPECT_EQ(arr->get(2).Number(), static_cast<int>(s.second.first));
    }
    {
      const auto& arr = output[id].GetValue().Array()->get(1).Array();

      EXPECT_EQ(arr->get(0).Number(),
                static_cast<int>(starlight::TimingFunctionType::kSteps));
      EXPECT_EQ(arr->get(1).Number(), 1);
      EXPECT_EQ(arr->get(2).Number(), static_cast<int>(s.second.second));
    }
  }
}

TEST(TimingFunctionHandler, Invalid) {
  CSSPropertyID id = CSSPropertyID::kPropertyIDTransitionTimingFunction;
  tasm::CSSValue out;
  CSSParserConfigs configs;
  StyleMap output;
  std::vector<std::string> cases = {
      "",         "hello",     "ease, ",         "cubic-bezier(1, 0.5)",
      "steps(1)", "steps(1,)", "steps(1, hello)"};

  for (const auto& s : cases) {
    output.clear();
    EXPECT_FALSE(UnitHandler::Process(id, lepus::Value(s), output, configs));
    EXPECT_TRUE(output.empty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
