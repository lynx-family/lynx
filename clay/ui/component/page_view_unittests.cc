// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
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
      page_->DispatchPointerEvent({event});
      event.type = PointerEvent::EventType::kUpEvent;
      event.buttons = 0;
      page_->DispatchPointerEvent({event});
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

}  // namespace
#endif

}  // namespace clay
