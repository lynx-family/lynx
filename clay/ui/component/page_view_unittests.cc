// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/testing/ui_test.h"
#include "clay/ui/window/pointer_data_packet.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

class TestPageView : public PageView {
 public:
  using PageView::ActivateTouchPseudoStatus;
  using PageView::DeactivateTouchPseudoStatus;
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

  void OnActiveChanged(int view_id, bool active) override {
    active_changes_.emplace_back(view_id, active);
  }

  int button_ = -1;
  int buttons_ = -1;
  int calls_ = 0;
  std::vector<std::pair<int, bool>> active_changes_;
};

class TapReturnPageView : public PageView {
 public:
  TapReturnPageView(fml::RefPtr<fml::TaskRunner> runner, const bool* enabled)
      : PageView(0, nullptr, std::move(runner)), enabled_(enabled) {}

 protected:
  bool ReadTapReturnSetting() const override { return *enabled_; }

 private:
  const bool* enabled_;
};

class TapReturnTest : public UITest {
 protected:
  std::unique_ptr<PageView> CreatePageView() override {
    return std::make_unique<TapReturnPageView>(ui_task_runner(), &enabled_);
  }

  void UISetUp() override {
    page_->SetTapSlop(8);
    tap_target_ = AddTarget(1, {0, 0}, {100, 100});
    tap_target_->AddEventCallback("tap");
    AddTarget(2, {100, 0}, {100, 100});
    find_view_by_id_callback_ = [this](int id) -> BaseView* {
      const auto it = targets_.find(id);
      return it == targets_.end() ? nullptr : it->second;
    };
    touch_event_callback_ = [this](const std::string& name, int id) {
      if (name == "tap") {
        tap_targets_.push_back(id);
      } else if (name == "longpress") {
        long_press_targets_.push_back(id);
      }
    };
  }

  View* AddTarget(int id, FloatPoint position, FloatSize size,
                  BaseView* parent = nullptr) {
    auto* target = new View(id, page_.get());
    target->SetBound(position.x(), position.y(), size.width(), size.height());
    (parent ? parent : page_.get())->AddChild(target);
    targets_[id] = target;
    return target;
  }

  void DeleteTarget(BaseView*& target) {
    target->Parent()->RemoveChild(target);
    targets_.erase(target->id());
    delete target;
    target = nullptr;
  }

  void ResetTargets(int target_id) {
    page_->ResetPageView();
    targets_.clear();
    tap_target_ = AddTarget(target_id, {0, 0}, {100, 100});
    tap_target_->AddEventCallback("tap");
  }

  ScrollView* ResetWithVerticalScrollTarget(float content_height) {
    page_->ResetPageView();
    targets_.clear();
    auto* scroll = new ScrollView(3, ScrollDirection::kVertical, page_.get());
    scroll->SetBound(0, 0, 100, 100);
    page_->AddChild(scroll);
    targets_[3] = scroll;
    auto* content = new View(4, page_.get());
    content->SetBound(0, 0, 100, content_height);
    content->AddEventCallback("tap");
    scroll->AddChild(content, 0);
    targets_[4] = content;
    tap_target_ = content;
    scroll->OnLayoutUpdated();
    return scroll;
  }

  void DispatchOutAndBack(
      int pointer_id, FloatPoint up = {12, 10},
      PointerEvent::DeviceType device = PointerEvent::kTouch) {
    const FloatPoint down_position{10, 10};
    const FloatPoint outside_position{150, 10};
    auto down = CreatePointer(pointer_id, PointerEvent::EventType::kDownEvent,
                              down_position);
    auto outside =
        CreatePointer(pointer_id, PointerEvent::EventType::kMoveEvent,
                      outside_position, outside_position - down_position);
    auto back = CreatePointer(pointer_id, PointerEvent::EventType::kMoveEvent,
                              up, up - outside_position);
    auto end = CreatePointer(pointer_id, PointerEvent::EventType::kUpEvent, up);
    down.device = outside.device = back.device = end.device = device;
    end.buttons = 0;
    page_->DispatchPointerEvent({down});
    page_->DispatchPointerEvent({outside});
    page_->DispatchPointerEvent({back});
    page_->DispatchPointerEvent({end});
  }

  bool enabled_ = true;
  BaseView* tap_target_ = nullptr;
  std::unordered_map<int, BaseView*> targets_;
  std::vector<int> tap_targets_;
  std::vector<int> long_press_targets_;
};

class TapReturnDefaultSettingTest : public TapReturnTest {
 protected:
  std::unique_ptr<PageView> CreatePageView() override {
    return UITest::CreatePageView();
  }
};
}  // namespace

TEST_F_UI(TapReturnDefaultSettingTest, AllowsReturnTap) {
  DispatchOutAndBack(1);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, ReturningNearDownFiresTap) {
  DispatchOutAndBack(1);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, WithinSlopMoveAcrossSiblingKeepsLegacyUpTarget) {
  DispatchOutAndBack(1);
  ASSERT_EQ(tap_targets_, std::vector<int>({1}));
  tap_targets_.clear();

  targets_.at(2)->AddEventCallback("tap");
  auto down = CreatePointer(2, PointerEvent::EventType::kDownEvent, {98, 10});
  auto move =
      CreatePointer(2, PointerEvent::EventType::kMoveEvent, {102, 10}, {4, 0});
  auto up = CreatePointer(2, PointerEvent::EventType::kUpEvent, {102, 10});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({move});
  page_->DispatchPointerEvent({up});
  EXPECT_EQ(tap_targets_, std::vector<int>({2}));
}

TEST_F_UI(TapReturnTest, WithinSlopUpAcrossSiblingKeepsLegacyUpTarget) {
  targets_.at(2)->AddEventCallback("tap");
  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {98, 10});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {102, 10});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({up});
  EXPECT_EQ(tap_targets_, std::vector<int>({2}));
}

TEST_F_UI(TapReturnTest, RestoredTapDoesNotCrossSibling) {
  targets_.at(2)->AddEventCallback("tap");
  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {98, 10});
  auto outside =
      CreatePointer(1, PointerEvent::EventType::kMoveEvent, {150, 10}, {52, 0});
  auto back = CreatePointer(1, PointerEvent::EventType::kMoveEvent, {102, 10},
                            {-48, 0});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {102, 10});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({outside});
  page_->DispatchPointerEvent({back});
  page_->DispatchPointerEvent({up});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, UpFarFromDownDoesNotTap) {
  DispatchOutAndBack(1, {80, 10});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, DisabledSettingKeepsLegacyMovementThreshold) {
  enabled_ = false;
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, SettingIsSampledAtDown) {
  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({down});
  enabled_ = false;
  auto outside = CreatePointer(1, PointerEvent::EventType::kMoveEvent,
                               {150, 10}, {140, 0});
  auto back = CreatePointer(1, PointerEvent::EventType::kMoveEvent, {12, 10},
                            {-138, 0});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {12, 10});
  up.buttons = 0;
  page_->DispatchPointerEvent({outside});
  page_->DispatchPointerEvent({back});
  page_->DispatchPointerEvent({up});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));

  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, CustomTapSlopControlsUpValidation) {
  page_->SetTapSlop(4);
  DispatchOutAndBack(1, {15, 10});
  EXPECT_TRUE(tap_targets_.empty());

  page_->SetTapSlop(20);
  DispatchOutAndBack(2, {25, 10});
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, MouseAndStylusKeepLegacyMovementThreshold) {
  DispatchOutAndBack(1, {12, 10}, PointerEvent::kMouse);
  DispatchOutAndBack(2, {12, 10}, PointerEvent::kStylus);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, ExtraTouchesCancelUntilAllTouchesEnd) {
  auto first = CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({first});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  enabled_ = false;
  auto second = CreatePointer(2, PointerEvent::EventType::kDownEvent, {20, 10});
  auto third = CreatePointer(3, PointerEvent::EventType::kDownEvent, {30, 10});
  page_->DispatchPointerEvent({second});
  page_->DispatchPointerEvent({third});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  first.type = PointerEvent::EventType::kUpEvent;
  first.position = {12, 10};
  first.buttons = 0;
  page_->DispatchPointerEvent({first});

  auto fourth = CreatePointer(4, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({fourth});
  page_->DispatchPointerEvent({CreatePointer(
      4, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  page_->DispatchPointerEvent({CreatePointer(
      4, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  fourth.type = PointerEvent::EventType::kUpEvent;
  fourth.position = {12, 10};
  fourth.buttons = 0;
  page_->DispatchPointerEvent({fourth});

  second.type = third.type = PointerEvent::EventType::kUpEvent;
  second.buttons = third.buttons = 0;
  page_->DispatchPointerEvent({second});
  page_->DispatchPointerEvent({third});
  EXPECT_TRUE(tap_targets_.empty());

  enabled_ = true;
  DispatchOutAndBack(5);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, RecycleClearsUnfinishedSecondaryTouches) {
  page_->DispatchPointerEvent(
      {CreatePointer(0, PointerEvent::EventType::kDownEvent, {10, 10})});
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {20, 10})});
  auto cancel = CreatePointer(0, PointerEvent::EventType::kCancel, {10, 10});
  cancel.buttons = 0;
  page_->DispatchPointerEvent({cancel});

  page_->ResetPageView(true);
  targets_.clear();
  tap_target_ = AddTarget(3, {0, 0}, {100, 100});
  tap_target_->AddEventCallback("tap");
  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({3}));
}

TEST_F_UI(TapReturnTest, OwnerlessFirstTouchPreventsSecondTouchTakeover) {
  auto first = CreatePointer(1, PointerEvent::EventType::kDownEvent, {150, 10});
  page_->DispatchPointerEvent({first});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {300, 10}, {150, 0})});

  auto second = CreatePointer(2, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({second});
  page_->DispatchPointerEvent({CreatePointer(
      2, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  page_->DispatchPointerEvent({CreatePointer(
      2, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  second.type = PointerEvent::EventType::kUpEvent;
  second.position = {12, 10};
  second.buttons = 0;
  page_->DispatchPointerEvent({second});
  first.type = PointerEvent::EventType::kUpEvent;
  first.position = {300, 10};
  first.buttons = 0;
  page_->DispatchPointerEvent({first});
  EXPECT_TRUE(tap_targets_.empty());

  DispatchOutAndBack(3);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, ReplacingTargetWithSameIdDoesNotMigrateTap) {
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  DeleteTarget(tap_target_);
  tap_target_ = AddTarget(1, {0, 0}, {100, 100});
  tap_target_->AddEventCallback("tap");
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {12, 10});
  up.buttons = 0;
  page_->DispatchPointerEvent({up});
  EXPECT_TRUE(tap_targets_.empty());

  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, ResetDuringTouchStartDoesNotMigrateTap) {
  touch_event_callback_ = [this](const std::string& name, int id) {
    if (name == "touchstart" && id == 1) {
      ResetTargets(2);
    } else if (name == "tap") {
      tap_targets_.push_back(id);
    }
  };
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());

  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({2}));
}

TEST_F_UI(TapReturnTest, ResetDuringTouchEndDoesNotMigrateTap) {
  touch_event_callback_ = [this](const std::string& name, int id) {
    if (name == "touchend" && id == 1) {
      ResetTargets(2);
    } else if (name == "tap") {
      tap_targets_.push_back(id);
    }
  };
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());

  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({2}));
}

TEST_F_UI(TapReturnTest, ResetWhileTouchIsDownPreventsSecondTouchTakeover) {
  auto first = CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({first});
  ResetTargets(2);

  DispatchOutAndBack(2);
  EXPECT_TRUE(tap_targets_.empty());

  first.type = PointerEvent::EventType::kUpEvent;
  first.buttons = 0;
  page_->DispatchPointerEvent({first});
  DispatchOutAndBack(3);
  EXPECT_EQ(tap_targets_, std::vector<int>({2}));
}

TEST_F_UI(TapReturnTest, ResetDuringTapCallbackKeepsNextSequenceValid) {
  touch_event_callback_ = [this](const std::string& name, int id) {
    if (name != "tap") {
      return;
    }
    tap_targets_.push_back(id);
    if (id == 1) {
      ResetTargets(2);
    }
  };
  DispatchOutAndBack(1);
  DispatchOutAndBack(2);
  EXPECT_EQ(tap_targets_, std::vector<int>({1, 2}));
}

TEST_F_UI(TapReturnTest, BatchedUpAndReusedDownKeepIndependentSequences) {
  auto first_down =
      CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({first_down});
  auto first_up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {10, 10});
  first_up.buttons = 0;
  auto second_down =
      CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10});
  page_->DispatchPointerEvent({first_up, second_down});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  auto second_up =
      CreatePointer(1, PointerEvent::EventType::kUpEvent, {12, 10});
  second_up.buttons = 0;
  page_->DispatchPointerEvent({second_up});
  EXPECT_EQ(tap_targets_, std::vector<int>({1, 1}));
}

TEST_F_UI(TapReturnTest, CancelDoesNotTapAndClearsSequence) {
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {150, 10}, {140, 0})});
  page_->DispatchPointerEvent({CreatePointer(
      1, PointerEvent::EventType::kMoveEvent, {12, 10}, {-138, 0})});
  auto cancel = CreatePointer(1, PointerEvent::EventType::kCancel, {12, 10});
  cancel.buttons = 0;
  page_->DispatchPointerEvent({cancel});
  EXPECT_TRUE(tap_targets_.empty());

  DispatchOutAndBack(1);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, MainDragRecognizerKeepsLegacyMovementPolicy) {
  auto drag = std::make_unique<DragGestureRecognizer>(page_->gesture_manager());
  drag->SetTouchSlop(8);
  tap_target_->AddGestureRecognizer(std::move(drag));
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, EventThroughOverlayWithoutCompetitionAllowsReturnTap) {
  auto* overlay = new OverlayView(3, page_.get());
  overlay->SetBound(0, 0, 1000, 1000);
  page_->AddChild(overlay);
  auto* child = AddTarget(4, {0, 0}, {100, 100}, overlay);
  child->SetEventThrough(true);
  Layout();

  DispatchOutAndBack(1);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
}

TEST_F_UI(TapReturnTest, AnonymousOverlayDragKeepsLegacyMovementPolicy) {
  auto* overlay = new OverlayView(3, page_.get());
  overlay->SetBound(0, 0, 1000, 1000);
  page_->AddChild(overlay);
  auto* child = AddTarget(-1, {0, 0}, {100, 100}, overlay);
  int drag_start_count = 0;
  auto drag = std::make_unique<DragGestureRecognizer>(page_->gesture_manager());
  drag->SetDragStartCallback(
      [&drag_start_count](const FloatPoint&) { ++drag_start_count; });
  child->AddGestureRecognizer(std::move(drag));
  Layout();

  DispatchOutAndBack(1);
  EXPECT_EQ(drag_start_count, 1);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, ScrollDragDoesNotTapAfterReturningToDown) {
  auto* scroll = ResetWithVerticalScrollTarget(500);

  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {50, 50});
  auto drag =
      CreatePointer(1, PointerEvent::EventType::kMoveEvent, {50, 10}, {0, -40});
  auto back =
      CreatePointer(1, PointerEvent::EventType::kMoveEvent, {50, 50}, {0, 40});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {50, 50});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({drag});
  EXPECT_GT(scroll->GetScrollOffset().y(), 0);
  page_->DispatchPointerEvent({back});
  page_->DispatchPointerEvent({up});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, ScrollAncestorWithoutConsumedScrollAllowsReturnTap) {
  auto* scroll = ResetWithVerticalScrollTarget(100);
  DispatchOutAndBack(1);
  EXPECT_EQ(scroll->GetScrollOffset().y(), 0);
  EXPECT_EQ(tap_targets_, std::vector<int>({4}));

  tap_targets_.clear();
  scroll = ResetWithVerticalScrollTarget(500);
  DispatchOutAndBack(2);
  EXPECT_EQ(scroll->GetScrollOffset().y(), 0);
  EXPECT_EQ(tap_targets_, std::vector<int>({4}));
}

TEST_F_UI(TapReturnTest, ActualScrollWithCustomSlopCancelsReturnTap) {
  auto* scroll = ResetWithVerticalScrollTarget(500);
  scroll->SetTouchSlop(-1);
  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {50, 50});
  auto scroll_move =
      CreatePointer(1, PointerEvent::EventType::kMoveEvent, {50, 30}, {0, -20});
  auto far_perpendicular = CreatePointer(1, PointerEvent::EventType::kMoveEvent,
                                         {200, 30}, {150, 0});
  auto back = CreatePointer(1, PointerEvent::EventType::kMoveEvent, {52, 50},
                            {-148, 20});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {52, 50});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({scroll_move});
  EXPECT_GT(scroll->GetScrollOffset().y(), 0);
  page_->DispatchPointerEvent({far_perpendicular});
  page_->DispatchPointerEvent({back});
  page_->DispatchPointerEvent({up});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, ActualScrollBelowTapSlopCancelsReturnTap) {
  auto* scroll = ResetWithVerticalScrollTarget(500);
  scroll->SetTouchSlop(-1);
  auto down = CreatePointer(1, PointerEvent::EventType::kDownEvent, {50, 50});
  auto scroll_move =
      CreatePointer(1, PointerEvent::EventType::kMoveEvent, {50, 46}, {0, -4});
  auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {52, 50});
  up.buttons = 0;
  page_->DispatchPointerEvent({down});
  page_->DispatchPointerEvent({scroll_move});
  EXPECT_GT(scroll->GetScrollOffset().y(), 0);
  page_->DispatchPointerEvent({up});
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, ConsumeSlideKeepsLegacyMovementPolicy) {
  Value angles{Value{Value(-180), Value(180)}};
  tap_target_->SetConsumeSlideEventDirection(angles.GetArray());
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, MainLongPressKeepsLegacyMovementPolicy) {
  tap_target_->AddGestureRecognizer(
      std::make_unique<LongPressGestureRecognizer>(page_->gesture_manager(),
                                                   1));
  DispatchOutAndBack(1);
  EXPECT_TRUE(tap_targets_.empty());
}

TEST_F_UI(TapReturnTest, BoundLongPressAllowsQuickReturnTap) {
  tap_target_->AddEventCallback("longpress");
  DispatchOutAndBack(1);
  EXPECT_EQ(tap_targets_, std::vector<int>({1}));
  EXPECT_TRUE(long_press_targets_.empty());
}

TEST_F_UI(TapReturnTest, AcceptedBoundLongPressSuppressesTap) {
  page_->SetLongPressDuration(1);
  tap_target_->AddEventCallback("longpress");
  page_->DispatchPointerEvent(
      {CreatePointer(1, PointerEvent::EventType::kDownEvent, {10, 10})});
  AsyncStart();
  ui_task_runner()->PostDelayedTask(
      [this]() {
        auto up = CreatePointer(1, PointerEvent::EventType::kUpEvent, {10, 10});
        up.buttons = 0;
        page_->DispatchPointerEvent({up});
        EXPECT_EQ(long_press_targets_, std::vector<int>({1}));
        EXPECT_TRUE(tap_targets_.empty());
        AsyncEnd();
      },
      fml::TimeDelta::FromMilliseconds(10));
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

TEST(PageViewTest, TouchActivePseudoStatusPropagatesToAncestors) {
  TestPageView page_view(0, nullptr, nullptr);
  RecordingEventDelegate event_delegate;
  page_view.SetEventDelegate(&event_delegate);

  auto* parent = new View(1, &page_view);
  auto* child = new View(2, &page_view);
  page_view.AddChild(parent);
  parent->AddChild(child);

  page_view.ActivateTouchPseudoStatus(7, child);
  page_view.DeactivateTouchPseudoStatus(7);

  const std::vector<std::pair<int, bool>> expected = {
      {2, true}, {1, true}, {0, true}, {2, false}, {1, false}, {0, false}};
  EXPECT_EQ(event_delegate.active_changes_, expected);
}

TEST(PageViewTest, PointerDownActivatesPseudoStatusForSupportedDevices) {
  for (auto device : {PointerEvent::kTouch, PointerEvent::kStylus,
                      PointerEvent::kInvertedStylus, PointerEvent::kMouse}) {
    SCOPED_TRACE(device);
    TestPageView page(0, nullptr, nullptr);
    RecordingEventDelegate delegate;
    page.SetEventDelegate(&delegate);
    page.SetBound(0, 0, 100, 100);
    PointerEvent event(PointerEvent::EventType::kDownEvent);
    event.device = device;
    event.position = {50, 50};
    std::vector<PointerEvent> events{event};
    page.gesture_manager()->HandlePointerEvents(&page, events);
#if defined(OS_WIN) || defined(OS_MAC)
    const bool should_activate = device != PointerEvent::kMouse;
#else
    const bool should_activate = device == PointerEvent::kTouch;
#endif
    const std::vector<std::pair<int, bool>> expected(should_activate ? 1 : 0,
                                                     {0, true});
    EXPECT_EQ(delegate.active_changes_, expected);
  }
}

TEST(PageViewTest, TouchActivePseudoStatusHonorsPropagationAttribute) {
  TestPageView page_view(0, nullptr, nullptr);
  RecordingEventDelegate event_delegate;
  page_view.SetEventDelegate(&event_delegate);

  auto* parent = new View(1, &page_view);
  auto* child = new View(2, &page_view);
  page_view.AddChild(parent);
  parent->AddChild(child);
  child->SetAttribute("enable-touch-pseudo-propagation", Value(false));

  page_view.ActivateTouchPseudoStatus(7, child);
  page_view.DeactivateTouchPseudoStatus(7);

  const std::vector<std::pair<int, bool>> expected = {{2, true}, {2, false}};
  EXPECT_EQ(event_delegate.active_changes_, expected);
}

TEST(PageViewTest, TouchActivePseudoStatusTracksInitialPointer) {
  TestPageView page_view(0, nullptr, nullptr);
  RecordingEventDelegate event_delegate;
  page_view.SetEventDelegate(&event_delegate);

  auto* child = new View(1, &page_view);
  page_view.AddChild(child);

  page_view.ActivateTouchPseudoStatus(7, child);
  page_view.ActivateTouchPseudoStatus(8, child);
  page_view.DeactivateTouchPseudoStatus(8);

  const std::vector<std::pair<int, bool>> active_expected = {{1, true},
                                                             {0, true}};
  EXPECT_EQ(event_delegate.active_changes_, active_expected);

  page_view.DeactivateTouchPseudoStatus(7);
  const std::vector<std::pair<int, bool>> inactive_expected = {
      {1, true}, {0, true}, {1, false}, {0, false}};
  EXPECT_EQ(event_delegate.active_changes_, inactive_expected);
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

#if defined(OS_WIN) || defined(OS_MAC)
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::InSequence;

class DesktopEventDelegate : public testing::MockEventDelegate {
 public:
  MOCK_METHOD(void, OnMouseEvent,
              (const std::string&, int, int, int, float, float, float, float,
               float),
              (override));
  MOCK_METHOD(void, OnWheelEvent,
              (const std::string&, int, float, float, float, float, float,
               float),
              (override));
  MOCK_METHOD(void, OnTouchEvent,
              (const std::string&, int, float, float, float, float),
              (override));
};

class DesktopEventTest : public UITest {
 protected:
  void UISetUp() override {
    page_->SetEventDelegate(&events_);
    EXPECT_CALL(
        events_,
        OnMouseEvent(::testing::AnyOf("mouseenter", "mouseleave", "mouseover"),
                     _, _, _, _, _, _, _, _))
        .Times(::testing::AnyNumber());
    page_->SetBound(0, 0, 400, 200);
    for (int id : {1, 2}) {
      auto* view = new View(id, page_.get());
      page_->AddChild(view);
      view->SetBound((id - 1) * 200, 0, 200, 200);
    }
    find_view_by_id_callback_ = [this](int id) -> BaseView* {
      for (auto* view : page_->GetChildren()) {
        if (view->id() == id) return view;
      }
      return nullptr;
    };
  }

  ::testing::NiceMock<DesktopEventDelegate> events_;
};

TEST_F_UI(DesktopEventTest, WheelPreservesPhaseAxisAndTarget) {
  {
    InSequence sequence;
    EXPECT_CALL(events_, OnWheelEvent("wheel", 1, _, _, _, _, 0, 40));
    EXPECT_CALL(events_, OnWheelEvent("wheel", 1, _, _, _, _, 0, 20));
    EXPECT_CALL(events_, OnWheelEvent("wheel", 2, _, _, _, _, 40, 0));
  }
  PointerEvent wheel(PointerEvent::EventType::kSignalEvent);
  wheel.device = PointerEvent::kMouse;
  wheel.signal_kind = PointerEvent::SignalKind::kScroll;
  wheel.position = {50, 50};
  wheel.scroll_delta_x = 10;
  wheel.scroll_delta_y = 40;
  page_->DispatchPointerEvent({wheel});
  wheel.position = {250, 50};
  wheel.scroll_delta_x = 50;
  wheel.scroll_delta_y = 20;
  page_->DispatchPointerEvent({wheel});
  page_->gesture_manager()->EndMouseWheelTransactionByForce();
  wheel.scroll_delta_x = 40;
  wheel.scroll_delta_y = 10;
  page_->DispatchPointerEvent({wheel});
  page_->gesture_manager()->EndMouseWheelTransactionByForce();
}

TEST_F_UI(DesktopEventTest, BatchedWheelPreservesPhaseAndAxis) {
  InSequence sequence;
  EXPECT_CALL(events_, OnWheelEvent("wheel", 1, _, _, _, _, 0, 40));
  EXPECT_CALL(events_, OnWheelEvent("wheel", 1, _, _, _, _, 0, 20));
  PointerEvent wheel(PointerEvent::EventType::kSignalEvent);
  wheel.device = PointerEvent::kMouse;
  wheel.signal_kind = PointerEvent::SignalKind::kScroll;
  wheel.position = {50, 50};
  wheel.scroll_delta_x = 10;
  wheel.scroll_delta_y = 40;
  auto update = wheel;
  update.scroll_delta_y = 20;
  page_->DispatchPointerEvent({wheel, update});
  page_->gesture_manager()->EndMouseWheelTransactionByForce();
}

TEST_F_UI(DesktopEventTest, TrackpadPanZoomPreservesLegacyEvents) {
  EXPECT_CALL(events_, OnTouchEvent(_, _, _, _, _, _)).Times(0);
  InSequence sequence;
  EXPECT_CALL(events_, OnWheelEvent("wheel", 1, _, _, _, _, 12, -30));
  EXPECT_CALL(events_, OnMouseEvent("zoom", 1, _, _, 1.5f, _, _, _, _));
  EXPECT_CALL(events_, OnWheelEvent("wheel", 2, _, _, _, _, -4, 8));
  PointerEvent event(PointerEvent::EventType::kPanZoomStartEvent);
  event.device = PointerEvent::kTrackpad;
  event.position = {50, 50};
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kPanZoomUpdateEvent;
  event.position = {250, 50};
  event.pan_delta = {12, -30};
  page_->DispatchPointerEvent({event});
  event.scale = 1.5f;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kPanZoomEndEvent;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kPanZoomStartEvent;
  event.scale = 1;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kPanZoomUpdateEvent;
  event.pan_delta = {-4, 8};
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kPanZoomEndEvent;
  page_->DispatchPointerEvent({event});
}

TEST_F_UI(DesktopEventTest, MouseButtonsPreserveCompatibilityInBothModes) {
  uint64_t timestamp = 400000;
  for (bool aligned : {false, true}) {
    page_->SetAlignMouseEventWithW3C(aligned);
    for (const auto& [buttons, web_button] : std::array<std::pair<int, int>, 6>{
             {{PointerEvent::kPrimary, 0},
              {PointerEvent::kSecondary, 2},
              {PointerEvent::kMiddle, 1},
              {PointerEvent::kBack, 3},
              {PointerEvent::kForward, 4},
              {PointerEvent::kPrimary | PointerEvent::kSecondary, 3}}}) {
      SCOPED_TRACE(aligned);
      SCOPED_TRACE(buttons);
      const int button = aligned ? web_button : buttons;
      InSequence sequence;
      EXPECT_CALL(events_, OnMouseEvent("mousedown", 1, button, buttons, 1, 50,
                                        50, 50, 50));
      EXPECT_CALL(events_,
                  OnMouseEvent("mouseup", 1, button, 0, 1, 50, 50, 50, 50));
      EXPECT_CALL(events_,
                  OnMouseEvent("mouseclick", 1, button, 0, 1, 50, 50, 50, 50))
          .Times(buttons == PointerEvent::kPrimary ? 1 : 0);
      EXPECT_CALL(events_, OnTouchEvent("tap", 1, 50, 50, 50, 50))
          .Times(buttons == PointerEvent::kPrimary ? 1 : 0);
      PointerEvent event(PointerEvent::EventType::kDownEvent);
      event.device = PointerEvent::kMouse;
      event.position = {50, 50};
      event.buttons = buttons;
      event.timestamp = timestamp;
      page_->DispatchPointerEvent({event});
      event.type = PointerEvent::EventType::kUpEvent;
      event.buttons = 0;
      event.timestamp++;
      page_->DispatchPointerEvent({event});
      timestamp += 400000;
    }
  }
}

TEST_F_UI(DesktopEventTest, MouseLongPressPreservesTouchCompatibility) {
  find_view_by_id_callback_(1)->AddEventCallback("mouselongpress");
  InSequence sequence;
  EXPECT_CALL(events_, OnMouseEvent("mousedown", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouselongpress", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("longpress", 1, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouseup", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("tap", _, _, _, _, _)).Times(0);
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::kMouse;
  event.position = {50, 50};
  event.buttons = PointerEvent::kPrimary;
  page_->DispatchPointerEvent({event});
  AsyncStart();
  ui_task_runner()->PostDelayedTask(
      [this, event]() mutable {
        event.type = PointerEvent::EventType::kUpEvent;
        event.buttons = 0;
        page_->DispatchPointerEvent({event});
        AsyncEnd();
      },
      fml::TimeDelta::FromMilliseconds(600));
}

TEST_F_UI(DesktopEventTest, TouchCaptureSurvivesCrossingAndCancellation) {
  std::vector<std::pair<std::string, int>> touches;
  EXPECT_CALL(events_, OnTouchEvent(_, _, _, _, _, _))
      .WillRepeatedly([&](const std::string& name, int id, float, float, float,
                          float) { touches.emplace_back(name, id); });
  for (auto device :
       {PointerData::DeviceKind::kTouch, PointerData::DeviceKind::kStylus,
        PointerData::DeviceKind::kInvertedStylus}) {
    SCOPED_TRACE(static_cast<int>(device));
    touches.clear();
    PointerDataPacket packet(1);
    PointerData data{};
    data.kind = device;
    data.change = PointerData::Change::kDown;
    data.pointer_identifier = 17;
    data.device = 7;
    data.physical_x = 50;
    data.physical_y = 50;
    data.buttons = PointerEvent::kPrimary;
    packet.SetPointerData(0, data);
    auto event = GetEventsFromPointerDataPacket(&packet).front();
    page_->DispatchPointerEvent({event});
    event.type = PointerEvent::EventType::kMoveEvent;
    event.position = {250, 50};
    event.delta = {200, 0};
    page_->DispatchPointerEvent({event});
    event.type = PointerEvent::EventType::kCancel;
    event.buttons = 0;
    page_->DispatchPointerEvent({event});
    EXPECT_THAT(touches, ElementsAre(std::make_pair("touchstart", 1),
                                     std::make_pair("touchmove", 1),
                                     std::make_pair("touchcancel", 1)));
    touches.clear();
    event.type = PointerEvent::EventType::kDownEvent;
    event.buttons = PointerEvent::kPrimary;
    event.delta = {};
    page_->DispatchPointerEvent({event});
    event.type = PointerEvent::EventType::kUpEvent;
    event.buttons = 0;
    page_->DispatchPointerEvent({event});
    EXPECT_THAT(touches, ElementsAre(std::make_pair("touchstart", 2),
                                     std::make_pair("touchend", 2),
                                     std::make_pair("tap", 2)));
  }
}

TEST_F_UI(DesktopEventTest, BatchedTouchesKeepIndependentTargets) {
  InSequence sequence;
  EXPECT_CALL(events_, OnTouchEvent("touchstart", 1, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("touchstart", 2, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("touchmove", 1, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("touchmove", 2, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("touchcancel", 1, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("touchend", 2, _, _, _, _));
  EXPECT_CALL(events_, OnTouchEvent("tap", _, _, _, _, _)).Times(0);
  auto first = CreatePointer(17, PointerEvent::EventType::kDownEvent, {50, 50});
  auto second =
      CreatePointer(18, PointerEvent::EventType::kDownEvent, {250, 50});
  page_->DispatchPointerEvent({first, second});
  first.type = PointerEvent::EventType::kMoveEvent;
  first.position = {250, 50};
  first.delta = {200, 0};
  second.type = PointerEvent::EventType::kMoveEvent;
  second.position = {50, 50};
  second.delta = {-200, 0};
  page_->DispatchPointerEvent({first, second});
  first.type = PointerEvent::EventType::kCancel;
  second.type = PointerEvent::EventType::kUpEvent;
  first.buttons = second.buttons = 0;
  page_->DispatchPointerEvent({first, second});
}

TEST_F_UI(DesktopEventTest, DispatchesMouseDoubleClickOnSecondTap) {
  auto view = std::make_unique<View>(1, page_.get());
  view->SetBound(0, 0, 100, 100);
  page_->AddChild(view.get());

  InSequence sequence;
  EXPECT_CALL(events_, OnMouseEvent("mousedown", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouseup", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouseclick", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mousedown", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouseup", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mouseclick", 1, _, _, _, _, _, _, _));
  EXPECT_CALL(events_, OnMouseEvent("mousedblclick", 1, _, _, _, _, _, _, _));

  for (int pointer_id : {1, 2}) {
    const uint64_t timestamp = 100000 + pointer_id * 100000;
    auto down = CreatePointer(pointer_id, PointerEvent::EventType::kDownEvent,
                              {10, 10}, {}, timestamp);
    down.device = PointerEvent::DeviceType::kMouse;
    page_->DispatchPointerEvent({down});
    auto up = CreatePointer(pointer_id, PointerEvent::EventType::kUpEvent,
                            {10, 10}, {}, timestamp + 1000);
    up.device = PointerEvent::DeviceType::kMouse;
    up.buttons = 0;
    page_->DispatchPointerEvent({up});
  }
}

}  // namespace
#endif

}  // namespace clay
