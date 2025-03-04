// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/animation_fill_mode_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(AnimationFillModeHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDAnimationFillMode;

  std::vector<std::pair<std::string, starlight::AnimationFillModeType>> cases =
      {
          {"none", starlight::AnimationFillModeType::kNone},
          {"forwards", starlight::AnimationFillModeType::kForwards},
          {"backwards", starlight::AnimationFillModeType::kBackwards},
          {"both", starlight::AnimationFillModeType::kBoth},
      };
  StyleMap output;
  CSSParserConfigs configs;

  for (const auto& s : cases) {
    EXPECT_TRUE(
        UnitHandler::Process(id, lepus::Value(s.first), output, configs));
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output[id].IsEnum());
    EXPECT_EQ(output[id].GetValue().Number(), static_cast<int>(s.second));
  }
}

TEST(AnimationFillModeHandler, Multi) {
  auto id = CSSPropertyID::kPropertyIDAnimationFillMode;
  auto impl = lepus::Value("forwards, backwards");
  StyleMap output;
  CSSParserConfigs configs;
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  EXPECT_TRUE(output[id].GetValue().IsArray());
  const auto& arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::AnimationFillModeType::kForwards));
  EXPECT_EQ(arr->get(1).Number(),
            static_cast<int>(starlight::AnimationFillModeType::kBackwards));
}

TEST(AnimationFillModeHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDAnimationFillMode;
  std::vector<std::string> cases = {"invalid,", "2", "2s"};
  StyleMap output;
  CSSParserConfigs configs;
  for (const auto& s : cases) {
    EXPECT_FALSE(UnitHandler::Process(id, lepus::Value(s), output, configs));
    EXPECT_TRUE(output.empty());
  }
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
