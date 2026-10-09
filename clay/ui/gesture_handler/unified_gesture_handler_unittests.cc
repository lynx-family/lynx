// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture_handler/gesture_handler_dispatcher.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/window/viewport_metrics.h"
#include "core/gesture/gesture_types.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay::testing {
namespace {

GestureMap UnifiedDetectors(uint32_t id) {
  lynx::tasm::gesture::GestureDefinition definition;
  definition.gesture_id = id;
  definition.gesture_type = lynx::tasm::GestureType::TAP;
  definition.callbacks = {
      lynx::tasm::gesture::GestureCallbackType::kTouchesDown,
      lynx::tasm::gesture::GestureCallbackType::kTouchesCancel,
      lynx::tasm::gesture::GestureCallbackType::kBegin,
      lynx::tasm::gesture::GestureCallbackType::kStart,
      lynx::tasm::gesture::GestureCallbackType::kEnd};
  GestureMap result;
  result.emplace(id, std::move(definition));
  return result;
}

TEST(UnifiedGestureHandlerTest, PanAndNativeReleasePreserveCallbackOrder) {
  for (auto type :
       {lynx::tasm::GestureType::PAN, lynx::tasm::GestureType::NATIVE}) {
    auto runner = TestTaskRunner::Create();
    auto page = MakeTestPageView(0, runner);
    MockEventDelegate delegate;
    page->SetEventDelegate(&delegate);
    auto view = std::make_unique<View>(10, page.get());
    auto detector =
        MakeDetector(1, type,
                     {lynx::tasm::gesture::GestureCallbackType::kBegin,
                      lynx::tasm::gesture::GestureCallbackType::kStart,
                      lynx::tasm::gesture::GestureCallbackType::kUpdate,
                      lynx::tasm::gesture::GestureCallbackType::kEnd});
    view->SetGestureDetectorMap({{1, detector}});
    std::vector<std::string> events;
    EXPECT_CALL(delegate,
                OnUnifiedGestureHandlerEvent(::testing::_, 10, 1, ::testing::_))
        .WillRepeatedly([&](const std::string& name, int, uint32_t, Value) {
          events.push_back(name);
        });
    HitTestResult hits{view->GetHitTestTargetWeakPtr()};
    auto* dispatcher = page->GetGestureHandlerDispatcher();
    dispatcher->HandlePointerDown(
        MakePointerEvent(PointerEvent::EventType::kDownEvent, {}, 1000000),
        hits);
    dispatcher->HandlePointerMove(
        MakePointerEvent(PointerEvent::EventType::kMoveEvent, {20, 0}, 1010000),
        hits);
    dispatcher->HandlePointerMove(
        MakePointerEvent(PointerEvent::EventType::kMoveEvent, {30, 0}, 1020000),
        hits);
    dispatcher->HandlePointerUp(
        MakePointerEvent(PointerEvent::EventType::kUpEvent, {30, 0}, 1030000),
        hits);
    EXPECT_EQ(events,
              (std::vector<std::string>{"onBegin", "onStart", "onUpdate",
                                        "onUpdate", "onEnd"}));
    view->Destroy();
  }
}

TEST(UnifiedGestureHandlerTest,
     DefaultAndExplicitTapDistancesUseLogicalPixels) {
  for (bool explicit_distance : {false, true}) {
    auto runner = TestTaskRunner::Create();
    auto page = MakeTestPageView(0, runner);
    ViewportMetrics metrics;
    metrics.device_pixel_ratio = 2;
    page->SetViewportMetrics(metrics);
    MockEventDelegate delegate;
    page->SetEventDelegate(&delegate);
    auto view = std::make_unique<View>(1, page.get());
    auto detector =
        MakeDetector(1, lynx::tasm::GestureType::TAP,
                     {lynx::tasm::gesture::GestureCallbackType::kStart,
                      lynx::tasm::gesture::GestureCallbackType::kEnd});
    if (explicit_distance) {
      detector.config.max_distance = 10;
    }
    view->SetGestureDetectorMap({{1, detector}});
    EXPECT_CALL(delegate,
                OnUnifiedGestureHandlerEvent("onStart", 1, 1, ::testing::_))
        .Times(1);
    EXPECT_CALL(delegate,
                OnUnifiedGestureHandlerEvent("onEnd", 1, 1, ::testing::_))
        .Times(2);
    HitTestResult hits{view->GetHitTestTargetWeakPtr()};
    auto* dispatcher = page->GetGestureHandlerDispatcher();
    for (float distance : {10.f, 10.25f}) {
      const float point = page->ConvertFrom<kPixelTypeLogical>(distance);
      dispatcher->HandlePointerDown(
          MakePointerEvent(PointerEvent::EventType::kDownEvent, {}, 1000000),
          hits);
      dispatcher->HandlePointerMove(
          MakePointerEvent(PointerEvent::EventType::kMoveEvent, {point, point},
                           1010000),
          hits);
      dispatcher->HandlePointerUp(
          MakePointerEvent(PointerEvent::EventType::kUpEvent, {}, 1020000),
          hits);
    }
    view->Destroy();
  }
}

class OffsetOverlayView : public OverlayView {
 public:
  OffsetOverlayView(uint32_t id, PageView* page_view, Point touch_offset)
      : OverlayView(id, page_view), touch_offset_(touch_offset) {}

  bool ShouldChangeOffset() const override { return true; }
  Point GetTouchOffset() const override { return touch_offset_; }

 private:
  Point touch_offset_;
};

TEST(UnifiedGestureHandlerTest, CancelKeepsCoordinatesAndEndsBeginOnce) {
  auto runner = TestTaskRunner::Create();
  auto page = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page->SetEventDelegate(&delegate);
  auto* dispatcher = page->GetGestureHandlerDispatcher();
  auto view = std::make_unique<View>(1, page.get());
  view->SetBound(10.5f, 20.25f, 100.f, 100.f);
  EXPECT_FLOAT_EQ(view->Left(), 10.5f);
  const auto local_point = view->GetPointBySelf({11.75f, 22.75f});
  EXPECT_FLOAT_EQ(local_point.x(), 1.25f);
  EXPECT_FLOAT_EQ(local_point.y(), 2.5f);
  auto detectors = UnifiedDetectors(9);
  detectors.merge(UnifiedDetectors(3));
  view->SetGestureDetectorMap(detectors);
  Value::Map consume;
  consume.emplace("consume", Value(false));
  view->ConsumeGesture(9, Value(std::move(consume)));
  EXPECT_TRUE(view->ShouldConsumeGesture());

  std::vector<std::string> events;
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent(::testing::_, 1, 3, ::testing::_))
      .WillRepeatedly([&](const std::string& name, int, uint32_t,
                          Value params) {
        events.push_back(name);
        const auto& map = params.GetMap();
        EXPECT_EQ(map.at("type").GetString(),
                  name == "onTouchesDown" || name == "onBegin" ? "touchstart"
                                                               : "touchcancel");
        EXPECT_DOUBLE_EQ(map.at("x").GetDouble(), 1.25);
        EXPECT_DOUBLE_EQ(map.at("y").GetDouble(), 2.5);
        EXPECT_DOUBLE_EQ(map.at("pageX").GetDouble(), 11.75);
        EXPECT_GT(map.at("timestamp").GetLong(), 1000000000000LL);
      });

  HitTestResult hits{view->GetHitTestTargetWeakPtr()};
  auto down = MakePointerEvent(PointerEvent::EventType::kDownEvent,
                               {11.75f, 22.75f}, 1000000);
  dispatcher->HandlePointerDown(down, hits);
  auto cancel = MakePointerEvent(PointerEvent::EventType::kCancel,
                                 {11.75f, 22.75f}, 1001000);
  dispatcher->HandlePointerCancel(cancel, hits);
  dispatcher->HandlePointerCancel(cancel, hits);
  runner->AdvanceBy(fml::TimeDelta::FromSeconds(1));

  EXPECT_EQ(events, (std::vector<std::string>{"onTouchesDown", "onBegin",
                                              "onTouchesCancel", "onEnd"}));
  view->Destroy();
}

TEST(UnifiedGestureHandlerTest, OverlayOffsetPreservesMemberLocalCoordinates) {
  auto runner = TestTaskRunner::Create();
  auto page = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page->SetEventDelegate(&delegate);
  auto* dispatcher = page->GetGestureHandlerDispatcher();
  auto overlay =
      std::make_unique<OffsetOverlayView>(1, page.get(), Point{100, 200});
  auto view = std::make_unique<View>(2, page.get());
  view->SetBound(10.5f, 20.25f, 100.f, 100.f);
  overlay->AddChild(view.get());
  view->SetGestureDetectorMap(UnifiedDetectors(3));

  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent(::testing::_, 2, 3, ::testing::_))
      .WillRepeatedly([&](const std::string&, int, uint32_t, Value params) {
        const auto& map = params.GetMap();
        EXPECT_DOUBLE_EQ(map.at("x").GetDouble(), 1.25);
        EXPECT_DOUBLE_EQ(map.at("y").GetDouble(), 2.5);
        EXPECT_DOUBLE_EQ(map.at("pageX").GetDouble(), 111.75);
        EXPECT_DOUBLE_EQ(map.at("pageY").GetDouble(), 222.75);
      });

  HitTestResult hits{view->GetHitTestTargetWeakPtr()};
  auto down = MakePointerEvent(PointerEvent::EventType::kDownEvent,
                               {111.75f, 222.75f}, 1000000);
  dispatcher->HandlePointerDown(down, hits);
  auto cancel = MakePointerEvent(PointerEvent::EventType::kCancel,
                                 {111.75f, 222.75f}, 1001000);
  dispatcher->HandlePointerCancel(cancel, hits);

  overlay->RemoveChild(view.get());
  view->Destroy();
  overlay->Destroy();
}

TEST(UnifiedGestureHandlerTest, FlingRunsFromPointerVelocityUntilCompletion) {
  auto runner = TestTaskRunner::Create();
  auto page = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page->SetEventDelegate(&delegate);
  auto* dispatcher = page->GetGestureHandlerDispatcher();
  auto view = std::make_unique<View>(1, page.get());

  auto pan = MakeDetector(1, lynx::tasm::GestureType::PAN);
  pan.relations.simultaneous = {2};
  auto fling = MakeDetector(2, lynx::tasm::GestureType::FLING,
                            {lynx::tasm::gesture::GestureCallbackType::kUpdate,
                             lynx::tasm::gesture::GestureCallbackType::kEnd});
  fling.relations.simultaneous = {1};
  view->SetGestureDetectorMap({{1, pan}, {2, fling}});

  std::vector<double> fling_deltas;
  int end_count = 0;
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent("onUpdate", 1, 2, ::testing::_))
      .WillRepeatedly([&](const std::string&, int, uint32_t, Value params) {
        fling_deltas.push_back(params.GetMap().at("deltaX").GetDouble());
      });
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent("onEnd", 1, 2, ::testing::_))
      .WillOnce([&](const std::string&, int, uint32_t, Value) { ++end_count; });

  HitTestResult hits{view->GetHitTestTargetWeakPtr()};
  dispatcher->HandlePointerDown(
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1000000),
      hits);
  dispatcher->HandlePointerMove(
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 1010000),
      hits);
  dispatcher->HandlePointerMove(
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {30, 0}, 1020000),
      hits);
  dispatcher->HandlePointerMove(
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {60, 0}, 1030000),
      hits);
  dispatcher->HandlePointerUp(
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {80, 0}, 1040000),
      hits);

  auto* animation_handler = page->GetAnimationHandler();
  animation_handler->DoAnimationFrame(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  animation_handler->DoAnimationFrame(16);
  EXPECT_TRUE(std::any_of(fling_deltas.begin(), fling_deltas.end(),
                          [](double delta) { return delta < 0; }));

  animation_handler->DoAnimationFrame(1800);
  EXPECT_EQ(end_count, 1);
  const size_t update_count_after_end = fling_deltas.size();
  animation_handler->DoAnimationFrame(1816);
  EXPECT_EQ(fling_deltas.size(), update_count_after_end);
  EXPECT_EQ(end_count, 1);

  view->Destroy();
}

TEST(UnifiedGestureHandlerTest, ClearingConfigRemovesMemberAndRestoresScroll) {
  auto runner = TestTaskRunner::Create();
  auto page = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page->SetEventDelegate(&delegate);
  auto* dispatcher = page->GetGestureHandlerDispatcher();
  auto view = std::make_unique<ScrollView>(1, page.get());
  view->SetGestureDetectorMap(UnifiedDetectors(3));
  EXPECT_GT(view->GestureArenaMemberId(), 0);
  EXPECT_FALSE(view->IsScrollEnabled());

  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent("onTouchesDown", 1, 3, ::testing::_))
      .Times(1);
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent("onBegin", 1, 3, ::testing::_))
      .Times(1);
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent("onEnd", 1, 3, ::testing::_))
      .Times(1);
  HitTestResult hits{view->GetHitTestTargetWeakPtr()};
  auto down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {1, 1}, 1000000);
  dispatcher->HandlePointerDown(down, hits);
  view->SetGestureDetectorMap({});
  EXPECT_EQ(view->GestureArenaMemberId(), 0);
  EXPECT_TRUE(view->IsScrollEnabled());
  view->SetScrollEnabled(false);
  view->SetGestureDetectorMap(UnifiedDetectors(3));
  view->SetGestureDetectorMap({});
  EXPECT_FALSE(view->IsScrollEnabled());
  runner->AdvanceBy(fml::TimeDelta::FromSeconds(1));
  auto up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {1, 1}, 2000000);
  dispatcher->HandlePointerUp(up, hits);
  dispatcher->HandlePointerDown(down, hits);
  view->Destroy();
}

TEST(UnifiedGestureHandlerTest, ResetPageViewFromCallbackKeepsHandlerAlive) {
  auto runner = TestTaskRunner::Create();
  auto page = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page->SetEventDelegate(&delegate);
  auto* dispatcher = page->GetGestureHandlerDispatcher();
  auto view = std::make_unique<View>(1, page.get());
  view->SetGestureDetectorMap(UnifiedDetectors(3));

  std::vector<std::string> events;
  EXPECT_CALL(delegate,
              OnUnifiedGestureHandlerEvent(::testing::_, 1, 3, ::testing::_))
      .WillRepeatedly([&](const std::string& name, int, uint32_t, Value) {
        events.push_back(name);
        if (name == "onTouchesDown") {
          page->ResetPageView();
        }
      });

  HitTestResult hits{view->GetHitTestTargetWeakPtr()};
  auto down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {1, 1}, 1000000);
  dispatcher->HandlePointerDown(down, hits);

  EXPECT_EQ(events, (std::vector<std::string>{"onTouchesDown"}));
  EXPECT_NE(page->GetGestureHandlerDispatcher(), dispatcher);
  auto replacement = std::make_unique<View>(2, page.get());
  replacement->SetGestureDetectorMap(UnifiedDetectors(4));
  EXPECT_TRUE(page->GetGestureHandlerDispatcher()->CanControlGesture(
      replacement->GestureArenaMemberId(), 4));
  replacement->Destroy();
  view->Destroy();
}

}  // namespace
}  // namespace clay::testing
