// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/animation_property_handler.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST(AnimationPropertyHandler, Handler) {
  auto id = CSSPropertyID::kPropertyIDTransitionProperty;

  std::vector<std::pair<std::string, starlight::AnimationPropertyType>> cases =
      {{"hello", starlight::AnimationPropertyType::kNone},
       {"width", starlight::AnimationPropertyType::kWidth},
       {"all", starlight::AnimationPropertyType::kAll}};
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

TEST(AnimationPropertyHandler, Multi) {
  auto id = CSSPropertyID::kPropertyIDTransitionProperty;
  auto impl = lepus::Value(
      "none, opacity, scaleX, scaleY, scaleXY, width, height, "
      "background-color, color,visibility, left, top, right,bottom,transform, "
      "all, max-width, max-height, min-width, min-height");
  StyleMap output;
  CSSParserConfigs configs;
  auto ret = UnitHandler::Process(id, impl, output, configs);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(output.empty());
  EXPECT_TRUE(output[id].IsArray());
  EXPECT_TRUE(output[id].GetValue().Array());
  const auto& arr = output[id].GetValue().Array();
  EXPECT_EQ(arr->get(0).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kNone));
  EXPECT_EQ(arr->get(1).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kOpacity));
  EXPECT_EQ(arr->get(2).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kScaleX));
  EXPECT_EQ(arr->get(3).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kScaleY));
  EXPECT_EQ(arr->get(4).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kScaleXY));
  EXPECT_EQ(arr->get(5).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kWidth));
  EXPECT_EQ(arr->get(6).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kHeight));
  EXPECT_EQ(
      arr->get(7).Number(),
      static_cast<int>(starlight::AnimationPropertyType::kBackgroundColor));
  EXPECT_EQ(arr->get(8).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kColor));
  EXPECT_EQ(arr->get(9).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kVisibility));
  EXPECT_EQ(arr->get(10).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kLeft));
  EXPECT_EQ(arr->get(11).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kTop));
  EXPECT_EQ(arr->get(12).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kRight));
  EXPECT_EQ(arr->get(13).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kBottom));
  EXPECT_EQ(arr->get(14).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kTransform));
  EXPECT_EQ(arr->get(15).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kAll));
  EXPECT_EQ(arr->get(16).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kMaxWidth));
  EXPECT_EQ(arr->get(17).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kMaxHeight));
  EXPECT_EQ(arr->get(18).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kMinWidth));
  EXPECT_EQ(arr->get(19).Number(),
            static_cast<int>(starlight::AnimationPropertyType::kMinHeight));
}

TEST(AnimationPropertyHandler, Invalid) {
  auto id = CSSPropertyID::kPropertyIDTransitionProperty;
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
