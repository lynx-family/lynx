// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"
#include "clay/ui/window/viewport_metrics.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;

class PanGestureHandlerTest : public ::testing::Test {};

TEST_F(PanGestureHandlerTest, HandleConfigMap_MinDistanceDipToPx) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  ViewportMetrics metrics;
  metrics.device_pixel_ratio = 2.0;
  page_view->SetViewportMetrics(metrics);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MIN_DISTANCE, Value(10.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START}, {},
                   std::move(config));
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move1 =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {15, 0}, 2);
  PointerEvent move2 =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {25, 0}, 3);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move1, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move2, 0, 0, false, nullptr);
  EXPECT_TRUE(handler.IsActive());
}

TEST_F(PanGestureHandlerTest, OnHandle_Down_SetsBegin_EmitsOnBeginOnce) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan, {GestureConstants::ON_BEGIN});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {10, 10}, 1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
}

TEST_F(PanGestureHandlerTest, OnHandle_Move_BelowThreshold_DoesNotActivate) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  Value::Map config_map;
  config_map.emplace(GestureConstants::MIN_DISTANCE, Value(100.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START}, {},
                   std::move(config));

  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());
  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_FALSE(handler.IsActive());
}

TEST_F(PanGestureHandlerTest, OnHandle_Move_WhenActive_EmitsOnUpdate) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MIN_DISTANCE, Value(0.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START,
                    GestureConstants::ON_UPDATE},
                   {}, std::move(config));
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {1, 1}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_UPDATE), Eq(1),
                                    Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_TRUE(handler.IsActive());
}

TEST_F(PanGestureHandlerTest, OnHandle_Up_FailsAndEmitsOnEnd) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_END});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&up, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(PanGestureHandlerTest, Activation_WorksForNegativeMovementToo) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MIN_DISTANCE, Value(10.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START}, {},
                   std::move(config));
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {-100, 0}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_TRUE(handler.IsActive());
}
}  // namespace testing

}  // namespace clay
