// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/transition_shorthand_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(TransitionShorthandHandler, ToLonghand) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("width 2s ease-in 1ms");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  std::vector<std::pair<std::string, CSSPropertyID>> longhands = {
      {"width", CSSPropertyID::kPropertyIDTransitionProperty},
      {"2s", CSSPropertyID::kPropertyIDTransitionDuration},
      {"1ms", CSSPropertyID::kPropertyIDTransitionDelay},
      {"ease-in", CSSPropertyID::kPropertyIDTransitionTimingFunction},
  };

  for (const auto& longhand : longhands) {
    StyleMap longhand_output;
    EXPECT_TRUE(UnitHandler::Process(longhand.second,
                                     lepus::Value(longhand.first),
                                     longhand_output, configs));
    EXPECT_EQ(output[longhand.second], longhand_output[longhand.second]);
  }
}

TEST(TransitionShorthandHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("width 2s ease-in 1ms");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kWidth));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 2000);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 1);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kEaseIn));
  }

  output.clear();
  impl = lepus::Value("width 2s ease");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kWidth));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 2000);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 0);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kEaseInEaseOut));
  }

  output.clear();
  impl = lepus::Value("width 2s");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kWidth));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 2000);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 0);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kLinear));
  }

  output.clear();
  impl = lepus::Value("width ease-out");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kWidth));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 0);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 0);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kEaseOut));
  }

  impl = lepus::Value("width");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kWidth));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 0);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 0);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kLinear));
  }

  impl = lepus::Value("hello");
  ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kNone));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 0);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 0);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kLinear));
  }
}

TEST(TransitionShorthandHandler, Negative) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("none -2s ease-in 1ms");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kNone));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 1);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), -2000);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kEaseIn));
  }
}

TEST(TransitionShorthandHandler, None) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;

  output.clear();
  auto impl = lepus::Value("none 2s ease-in 1ms");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  {
    auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
    EXPECT_TRUE(property.GetValue().IsNumber());
    EXPECT_EQ(property.GetValue().Number(),
              static_cast<int>(starlight::AnimationPropertyType::kNone));
    auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
    EXPECT_TRUE(duration.GetValue().IsNumber());
    EXPECT_EQ(duration.GetValue().Number(), 2000);
    auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
    EXPECT_TRUE(delay.GetValue().IsNumber());
    EXPECT_EQ(delay.GetValue().Number(), 1);
    auto timing_function =
        output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
    EXPECT_TRUE(timing_function.GetValue().IsArray());
    EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
              static_cast<int>(starlight::TimingFunctionType::kEaseIn));
  }
}

TEST(TransitionShorthandHandler, Multi) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;

  auto impl = lepus::Value("width 2s ease-in 1ms, height 10s");
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  // shorthand -> longhand
  EXPECT_TRUE(output.find(id) == output.end());
  auto property = output[CSSPropertyID::kPropertyIDTransitionProperty];
  EXPECT_TRUE(property.GetValue().IsArray());
  EXPECT_EQ(property.GetValue().Array()->get(0).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kWidth));
  EXPECT_EQ(property.GetValue().Array()->get(1).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kHeight));
  auto duration = output[CSSPropertyID::kPropertyIDTransitionDuration];
  EXPECT_TRUE(duration.GetValue().IsArray());
  EXPECT_EQ(duration.GetValue().Array()->get(0).Number(), 2000);
  EXPECT_EQ(duration.GetValue().Array()->get(1).Number(), 10000);

  auto delay = output[CSSPropertyID::kPropertyIDTransitionDelay];
  EXPECT_TRUE(delay.GetValue().IsArray());
  EXPECT_EQ(delay.GetValue().Array()->get(0).Number(), 1);
  EXPECT_EQ(delay.GetValue().Array()->get(1).Number(), 0);

  auto timing_function =
      output[CSSPropertyID::kPropertyIDTransitionTimingFunction];
  EXPECT_TRUE(timing_function.GetValue().IsArray());

  EXPECT_EQ(timing_function.GetValue().Array()->get(0).Number(),
            static_cast<int>(starlight::TimingFunctionType::kEaseIn));
  EXPECT_EQ(timing_function.GetValue().Array()->get(1).Number(),
            static_cast<int>(starlight::TimingFunctionType::kLinear));
}

TEST(TransitionShorthandHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDTransition;
  StyleMap output;
  CSSParserConfigs configs;
  configs.enable_css_strict_mode = true;

  std::vector<std::string> cases = {"width 2s ease-in 1ms, ",
                                    "width 2s ease-in 1ms 1ms", "none 1s, none",
                                    "none, hello", "hello, world"};
  for (const auto& s : cases) {
    EXPECT_FALSE(UnitHandler::Process(id, lepus::Value(s), output, configs));
    EXPECT_TRUE(output.empty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
