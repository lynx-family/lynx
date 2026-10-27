// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>

#include "clay/ui/component/keyframes_data.h"
#include "clay/ui/component/page_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(PageViewTest, EmptyKeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);
  KeyframesData keyframes_data{0, nullptr};
  page_view->SetKeyframesData(&keyframes_data);
  EXPECT_EQ(page_view->GetKeyframesMap("name"), nullptr);
}

TEST(PageViewTest, KeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  ClayAnimationPropertyValue prop_values_at_0[] = {
      {ClayAnimationPropertyType::kBackgroundColor, clay::Value{0xFFFF0000u}},
      {ClayAnimationPropertyType::kOpacity, clay::Value{0.0f}}};

  ClayAnimationPropertyValue prop_values_at_50[] = {
      {ClayAnimationPropertyType::kBackgroundColor, clay::Value{0xFFFF0000u}},
      {ClayAnimationPropertyType::kOpacity, clay::Value{0.5f}}};

  ClayAnimationPropertyValue prop_values_at_100[] = {
      {ClayAnimationPropertyType::kBackgroundColor, clay::Value{0xFFFF0000u}},
      {ClayAnimationPropertyType::kOpacity, clay::Value{1.0f}}};

  ClayKeyframe keyframes[] = {
      {0.0f, sizeof(prop_values_at_0) / sizeof(prop_values_at_0[0]),
       prop_values_at_0},
      {0.5f, sizeof(prop_values_at_50) / sizeof(prop_values_at_50[0]),
       prop_values_at_50},
      {1.0f, sizeof(prop_values_at_100) / sizeof(prop_values_at_100[0]),
       prop_values_at_100}};

  ClayKeyframesRule keyframe_rules[] = {
      {"anim_1", sizeof(keyframes) / sizeof(keyframes[0]), keyframes},
      {"anim_2", sizeof(keyframes) / sizeof(keyframes[0]), keyframes},
      {"anim_3", sizeof(keyframes) / sizeof(keyframes[0]), keyframes}};

  KeyframesData keyframes_data{
      sizeof(keyframe_rules) / sizeof(keyframe_rules[0]), keyframe_rules};
  page_view->SetKeyframesData(&keyframes_data);

  auto check_keyframes_map = [&page_view](const char* anim_name) {
    const KeyframesMap* ret = page_view->GetKeyframesMap(anim_name);
    EXPECT_TRUE(ret);
    auto it = ret->find(ClayAnimationPropertyType::kBackgroundColor);
    EXPECT_TRUE(it != ret->end());
    it = ret->find(ClayAnimationPropertyType::kOpacity);
    EXPECT_TRUE(it != ret->end());
  };

  check_keyframes_map("anim_1");
  check_keyframes_map("anim_2");
  check_keyframes_map("anim_3");

  // Reset KeyframesData with only one keyframe rule anim_1
  KeyframesData keyframes_data_2{1, keyframe_rules};
  page_view->SetKeyframesData(&keyframes_data_2);
  check_keyframes_map("anim_1");
}

}  // namespace clay
