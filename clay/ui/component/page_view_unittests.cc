// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

class ClickTrackerPageView : public PageView {
 public:
  ClickTrackerPageView(fml::RefPtr<fml::TaskRunner> runner,
                       const float* boundary_tolerance)
      : PageView(0, nullptr, std::move(runner)),
        boundary_tolerance_(boundary_tolerance) {}

  void AcceptGesture(int pointer_id, GestureRecognizerType type) {
    CancelClickTrackingForGesture(pointer_id, type);
  }

 protected:
  bool ReadClickTrackerSetting() const override { return true; }
  float ReadClickTrackerBoundaryTolerance() const override {
    return std::max(0.f, *boundary_tolerance_);
  }

 private:
  const float* boundary_tolerance_;
};

class ClickTrackerTest : public UITest {
 protected:
  std::unique_ptr<PageView> CreatePageView() override {
    return std::make_unique<ClickTrackerPageView>(ui_task_runner(),
                                                  &boundary_tolerance_);
  }

  void UISetUp() override {
    page_->SetTapSlop(8);
    page_->AddEventCallback("tap");
    owner_ = AddTarget(1, {0, 0}, {100, 100});
    owner_->AddEventCallback("tap");
    touch_event_callback_ = [this](const std::string& name, int id) {
      if (name == "tap") {
        tap_targets_.push_back(id);
      }
    };
  }

  View* AddTarget(int id, FloatPoint position, FloatSize size) {
    auto* target = new View(id, page_.get());
    target->SetBound(position.x(), position.y(), size.width(), size.height());
    page_->AddChild(target);
    targets_[id] = target;
    return target;
  }

  void DispatchMovement(
      int id, FloatPoint from, FloatPoint to,
      PointerEvent::DeviceType device = PointerEvent::kTouch) {
    auto down = CreatePointer(id, PointerEvent::EventType::kDownEvent, from);
    auto move = CreatePointer(id, PointerEvent::EventType::kMoveEvent, to);
    auto up = CreatePointer(id, PointerEvent::EventType::kUpEvent, to);
    down.device = move.device = up.device = device;
    page_->DispatchPointerEvent({down});
    page_->DispatchPointerEvent({move});
    page_->DispatchPointerEvent({up});
  }

  View* owner_ = nullptr;
  float boundary_tolerance_ = 2.f;
  std::unordered_map<int, BaseView*> targets_;
  std::vector<int> tap_targets_;
};

}  // namespace

TEST_F_UI(ClickTrackerTest, InsideUpAfterMovementFiresTapOnOwner) {
  DispatchMovement(1, {10, 10}, {30, 10});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(ClickTrackerTest, OutsideUpSuppressesTap) {
  AddTarget(2, {100, 0}, {100, 100})->AddEventCallback("tap");
  DispatchMovement(1, {10, 10}, {110, 10});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, UpWithinBoundaryToleranceFiresTapOnOwner) {
  DispatchMovement(1, {10, 10}, {101.5f, 50});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(ClickTrackerTest, UpBeyondBoundaryToleranceSuppressesTap) {
  DispatchMovement(1, {10, 10}, {102.5f, 50});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, ReplacementInsideOwnerDoesNotUseTolerance) {
  auto* replacement = AddTarget(2, {0, 0}, {100, 100});
  replacement->AddEventCallback("tap");
  DispatchMovement(1, {10, 10}, {30, 10});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, BoundaryToleranceIsSnapshotAtDown) {
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  boundary_tolerance_ = 0.f;
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kMoveEvent, {101.5f, 50})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kUpEvent, {101.5f, 50})});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(ClickTrackerTest, NegativeBoundaryToleranceIsClampedToZero) {
  boundary_tolerance_ = -2.f;
  DispatchMovement(1, {10, 10}, {100.5f, 50});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, AcceptedTapKeepsClickTracking) {
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  static_cast<ClickTrackerPageView*>(page_.get())
      ->AcceptGesture(1, GestureRecognizerType::kTap);
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kMoveEvent, {30, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kUpEvent, {30, 10})});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(ClickTrackerTest, CompetingAcceptedGestureCancelsClickTracking) {
  const std::array competing_types = {GestureRecognizerType::kDoubleTap,
                                      GestureRecognizerType::kLongPress,
                                      GestureRecognizerType::kDragGesture,
                                      GestureRecognizerType::kHorizontalDrag,
                                      GestureRecognizerType::kVerticalDrag};
  int pointer_id = 1;
  for (const auto type : competing_types) {
    page_->DispatchPointerEvent({CreatePointer(
        pointer_id, PointerEvent::EventType::kDownEvent, {10, 10})});
    static_cast<ClickTrackerPageView*>(page_.get())
        ->AcceptGesture(pointer_id, type);
    page_->DispatchPointerEvent({CreatePointer(
        pointer_id, PointerEvent::EventType::kMoveEvent, {30, 10})});
    page_->DispatchPointerEvent({CreatePointer(
        pointer_id, PointerEvent::EventType::kUpEvent, {30, 10})});
    ++pointer_id;
  }
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, OutsideThenBackInsideFiresTap) {
  AddTarget(2, {100, 0}, {100, 100})->AddEventCallback("tap");
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kMoveEvent, {110, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kMoveEvent, {30, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kUpEvent, {30, 10})});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(ClickTrackerTest, DeletedOwnerDoesNotReceiveTap) {
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kMoveEvent, {30, 10})});
  page_->RemoveChild(owner_);
  delete owner_;
  owner_ = nullptr;
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kUpEvent, {30, 10})});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(ClickTrackerTest, MouseKeepsOriginalMovementThreshold) {
  DispatchMovement(1, {10, 10}, {30, 10}, PointerEvent::kMouse);
  EXPECT_TRUE(tap_targets_.empty());
}

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

}  // namespace clay
