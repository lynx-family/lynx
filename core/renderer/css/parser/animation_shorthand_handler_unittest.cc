// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/animation_shorthand_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(AnimationShorthandHandler, Animation) {
  auto id = CSSPropertyID::kPropertyIDAnimation;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value("rotate 10s ease 1s 10 forwards");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(), "rotate");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 10000);
  auto timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 1000);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(), 10);
  EXPECT_EQ(output[kPropertyIDAnimationFillMode].GetValue().Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));

  output.clear();
  impl = lepus::Value("10");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(), "none");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 0);
  EXPECT_TRUE(output[kPropertyIDAnimationTimingFunction].GetValue().IsArray());
  timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kLinear));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 0);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(), 10);

  output.clear();
  impl = lepus::Value("10s 10 test");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(), "test");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 10000);
  EXPECT_TRUE(output[kPropertyIDAnimationTimingFunction].GetValue().IsArray());
  timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kLinear));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 0);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(), 10);

  output.clear();
  impl = lepus::Value("10s ease 1s forwards 10 item1-ani-frames");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());
  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(),
            "item1-ani-frames");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 10000);
  EXPECT_TRUE(output[kPropertyIDAnimationTimingFunction].GetValue().IsArray());
  timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 1000);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(), 10);
  EXPECT_EQ(output[kPropertyIDAnimationFillMode].GetValue().Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));

  output.clear();
  impl = lepus::Value("10s ease 1ms forwards infinite test paused reverse");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());

  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(), "test");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 10000);
  EXPECT_TRUE(output[kPropertyIDAnimationTimingFunction].GetValue().IsArray());
  timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 1);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(),
            10E8);
  EXPECT_EQ(output[kPropertyIDAnimationFillMode].GetValue().Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));
  EXPECT_EQ(output[kPropertyIDAnimationPlayState].GetValue().Number(),
            static_cast<int>(starlight::AnimationPlayStateType::kPaused));
  EXPECT_EQ(output[kPropertyIDAnimationDirection].GetValue().Number(),
            static_cast<int>(starlight::AnimationDirectionType::kReverse));
}

TEST(AnimationShorthandHandler, Default) {
  auto id = CSSPropertyID::kPropertyIDAnimation;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value("test");
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output.find(id) == output.end());

  EXPECT_EQ(output[kPropertyIDAnimationName].GetValue().StringView(), "test");
  EXPECT_EQ(output[kPropertyIDAnimationDuration].GetValue().Number(), 0);
  EXPECT_TRUE(output[kPropertyIDAnimationTimingFunction].GetValue().IsArray());
  auto timing_function =
      output[kPropertyIDAnimationTimingFunction].GetValue().Array();
  EXPECT_EQ(timing_function->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kLinear));
  EXPECT_EQ(output[kPropertyIDAnimationDelay].GetValue().Number(), 0);
  EXPECT_EQ(output[kPropertyIDAnimationIterationCount].GetValue().Number(), 1);
  EXPECT_EQ(output[kPropertyIDAnimationFillMode].GetValue().Number(),
            static_cast<int>(starlight::AnimationFillModeType::kNone));
  EXPECT_EQ(output[kPropertyIDAnimationPlayState].GetValue().Number(),
            static_cast<int>(starlight::AnimationPlayStateType::kRunning));
  EXPECT_EQ(output[kPropertyIDAnimationDirection].GetValue().Number(),
            static_cast<int>(starlight::AnimationDirectionType::kNormal));
}

TEST(AnimationShorthandHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDAnimation;
  StyleMap output;
  CSSParserConfigs configs;
  std::vector<std::string> cases = {"test test", "12s 12s 10ms", "ease ease",
                                    "test, "};

  for (const auto& s : cases) {
    output.clear();
    EXPECT_FALSE(UnitHandler::Process(id, lepus::Value(s), output, configs));
    EXPECT_TRUE(output.empty());
  }
}

TEST(AnimationShorthandHandler, Legacy) {
  auto id = CSSPropertyID::kPropertyIDEnterTransitionName;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value(true);
  bool ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());

  impl = lepus::Value("test 10s ease 1s infinite forwards");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationName) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationDuration) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationTimingFunction) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationFillMode) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationIterationCount) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationDelay) == output.end());
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationName))
                .StringView(),
            "test");
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationDuration))
                .Number(),
            10000);
  EXPECT_EQ(
      (starlight::TimingFunctionType)output[id]
          .GetValue()
          .Table()
          ->GetValue(std::to_string(tasm::kPropertyIDAnimationTimingFunction))
          .Array()
          ->get(0)
          .Number(),
      starlight::TimingFunctionType::kEaseInEaseOut);
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationDelay))
                .Number(),
            1000);
  EXPECT_EQ(
      output[id]
          .GetValue()
          .Table()
          ->GetValue(std::to_string(tasm::kPropertyIDAnimationIterationCount))
          .Number(),
      10E8);
  EXPECT_EQ((starlight::AnimationFillModeType)output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationFillMode))
                .Number(),
            starlight::AnimationFillModeType::kForwards);

  output.clear();
  impl = lepus::Value("10s ease 1s forwards infinite test");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationName) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationDuration) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationTimingFunction) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationFillMode) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationIterationCount) == output.end());
  EXPECT_TRUE(output.find(kPropertyIDAnimationDelay) == output.end());
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationName))
                .StringView(),
            "test");
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationDuration))
                .Number(),
            10000);
  EXPECT_EQ(
      output[id]
          .GetValue()
          .Table()
          ->GetValue(std::to_string(tasm::kPropertyIDAnimationTimingFunction))
          .Array()
          ->get(0)
          .Number(),
      static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationDelay))
                .Number(),
            1000);
  EXPECT_EQ(
      output[id]
          .GetValue()
          .Table()
          ->GetValue(std::to_string(tasm::kPropertyIDAnimationIterationCount))
          .Number(),
      10E8);
  EXPECT_EQ(output[id]
                .GetValue()
                .Table()
                ->GetValue(std::to_string(tasm::kPropertyIDAnimationFillMode))
                .Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));

  output.clear();
  impl = lepus::Value("10s ease 1s forwards infinite test");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_FALSE(output.find(id) == output.end());
  EXPECT_TRUE(output[id].IsMap());
  auto table = output[id].GetValue().Table();
  auto holder =
      table->GetValue(base::String(std::to_string(kPropertyIDAnimationName)));
  EXPECT_EQ(holder.StringView(), "test");
  holder = table->GetValue(
      base::String(std::to_string(kPropertyIDAnimationDuration)));
  EXPECT_EQ(holder.Number(), 10000);
  holder =
      table->GetValue(base::String(std::to_string(kPropertyIDAnimationDelay)));
  EXPECT_EQ(holder.Number(), 1000);
  holder = table->GetValue(
      base::String(std::to_string(kPropertyIDAnimationTimingFunction)));
  EXPECT_TRUE(holder.IsArray());
  EXPECT_EQ(holder.Array()->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  holder = table->GetValue(
      base::String(std::to_string(kPropertyIDAnimationFillMode)));
  EXPECT_EQ(holder.Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));
  holder = table->GetValue(
      base::String(std::to_string(kPropertyIDAnimationIterationCount)));
  EXPECT_EQ(holder.Number(), 10E8);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
