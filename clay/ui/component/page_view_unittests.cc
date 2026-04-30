// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(PageViewTest, EmptyKeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);
  Value keyframes_data;
  page_view->SetKeyframesData(keyframes_data);
  EXPECT_EQ(page_view->GetKeyframesMap("name"), nullptr);
}

TEST(PageViewTest, KeyframesData) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  Value keyframes_data = Value{
      {"anim_1",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
      {"anim_2",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
      {"anim_3",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
  };

  page_view->SetKeyframesData(keyframes_data);

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

  Value keyframes_data_2 = Value{
      {"anim_1",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{0.0}}}},
           {"0.5", Value{{"background-color", Value{0xFFFF0000u}},
                         {"opacity", Value{0.5}}}},
           {"1", Value{{"background-color", Value{0xFFFF0000u}},
                       {"opacity", Value{1.0}}}},
       }},
  };
  page_view->SetKeyframesData(keyframes_data_2);

  check_keyframes_map("anim_1");
}

TEST(PageViewTest, IgnoreFocusInheritance) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  auto* parent = new View(1, page_view.get());
  auto* child = new View(2, page_view.get());
  auto* grandchild = new View(3, page_view.get());
  auto* sibling = new View(4, page_view.get());

  page_view->AddChild(parent);
  parent->AddChild(child);
  child->AddChild(grandchild);
  page_view->AddChild(sibling);

  parent->SetAttribute("ignore-focus", Value(true));
  EXPECT_TRUE(parent->ShouldIgnoreFocus());
  EXPECT_TRUE(child->ShouldIgnoreFocus());
  EXPECT_TRUE(grandchild->ShouldIgnoreFocus());

  child->SetAttribute("ignore-focus", Value(false));
  EXPECT_FALSE(child->ShouldIgnoreFocus());
  EXPECT_FALSE(grandchild->ShouldIgnoreFocus());
  EXPECT_FALSE(sibling->ShouldIgnoreFocus());
}

TEST(PageViewTest, AncestorTouchTargetDoesNotPreserveFocusedChild) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  auto* parent = new View(1, page_view.get());
  auto* child = new View(2, page_view.get());

  page_view->AddChild(parent);
  parent->AddChild(child);

  child->SetFocusable(true);
  child->RequestFocus();

  EXPECT_FALSE(page_view->ShouldPreserveFocusForTouchTarget(parent));
  EXPECT_TRUE(page_view->ShouldPreserveFocusForTouchTarget(child));
}

}  // namespace clay
