// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

class TestPageView : public PageView {
 public:
  using PageView::PageView;
  using PageView::ReportTopViewEvent;
};

class RecordingEventDelegate : public testing::MockEventDelegate {
 public:
  void OnMouseEvent(const std::string&, int, int button, int buttons, float,
                    float, float, float, float) override {
    button_ = button;
    buttons_ = buttons;
    calls_++;
  }

  int button_ = -1;
  int buttons_ = -1;
  int calls_ = 0;
};

}  // namespace

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

TEST(PageViewTest, RemoveKeyframe) {
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, nullptr);

  Value keyframes_data = Value{
      {"anim_1",
       Value{
           {"0", Value{{"background-color", Value{0xFFFF0000u}}}},
           {"1", Value{{"background-color", Value{0xFF0000FFu}}}},
       }},
      {"anim_2",
       Value{
           {"0", Value{{"opacity", Value{0.0}}}},
           {"1", Value{{"opacity", Value{1.0}}}},
       }},
  };

  page_view->SetKeyframesData(keyframes_data);
  EXPECT_NE(page_view->GetKeyframesMap("anim_1"), nullptr);
  EXPECT_NE(page_view->GetKeyframesMap("anim_2"), nullptr);

  page_view->RemoveKeyframe("anim_1");
  EXPECT_EQ(page_view->GetKeyframesMap("anim_1"), nullptr);
  EXPECT_NE(page_view->GetKeyframesMap("anim_2"), nullptr);

  page_view->RemoveKeyframe("");
  page_view->RemoveKeyframe("missing_anim");
  EXPECT_NE(page_view->GetKeyframesMap("anim_2"), nullptr);
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

TEST(PageViewTest, AlignsMouseButtonWithW3C) {
  TestPageView page_view(0, nullptr, nullptr);
  RecordingEventDelegate event_delegate;
  page_view.SetEventDelegate(&event_delegate);
  page_view.SetBound(0, 0, 100, 100);

  struct TestCase {
    int buttons;
    int aligned_button;
  };
  constexpr std::array<TestCase, 5> kTestCases = {{
      {PointerEvent::kPrimary, 0},
      {PointerEvent::kSecondary, 2},
      {PointerEvent::kMiddle, 1},
      {PointerEvent::kBack, 3},
      {PointerEvent::kForward, 4},
  }};

  for (bool aligned : {false, true}) {
    page_view.SetAlignMouseEventWithW3C(aligned);
    for (const auto& test_case : kTestCases) {
      event_delegate.calls_ = 0;
      PointerEvent event(PointerEvent::EventType::kDownEvent);
      event.device = PointerEvent::DeviceType::kMouse;
      event.position = {1, 1};
      event.buttons = test_case.buttons;
      page_view.ReportTopViewEvent(event, kClayEventTypeMouseDown);
      const int expected_button =
          aligned ? test_case.aligned_button : test_case.buttons;
      EXPECT_EQ(event_delegate.button_, expected_button);
      EXPECT_EQ(event_delegate.buttons_, test_case.buttons);
      event.buttons = 0;
      page_view.ReportTopViewEvent(event, kClayEventTypeMouseUp);
      EXPECT_EQ(event_delegate.button_, expected_button);
      EXPECT_EQ(event_delegate.buttons_, 0);
      EXPECT_EQ(event_delegate.calls_, 2);
    }
  }
}

}  // namespace clay
