// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/gesture_handler/handler/tap_gesture_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;

class TapGestureHandlerTest : public ::testing::Test {};

TEST_F(TapGestureHandlerTest,
       HandleConfigMap_MaxDurationAndMaxDistanceApplied) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DURATION,
                     Value(static_cast<int64_t>(50)));
  config_map.emplace(GestureConstants::MAX_DISTANCE, Value(5.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_END}, {},
                   std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(TapGestureHandlerTest,
       OnHandle_Down_SetsBegin_EmitsOnBegin_StartsTimer) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DURATION,
                     Value(static_cast<int64_t>(30)));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap, {GestureConstants::ON_BEGIN}, {},
                   std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
}

TEST_F(TapGestureHandlerTest, OnHandle_Move_ExceedDistance_Fails) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DISTANCE, Value(1.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap, {}, {}, std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(TapGestureHandlerTest, OnHandle_Up_WhenNotFailed_ActivatesStartEnd) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DISTANCE, Value(1000.0f));
  config_map.emplace(GestureConstants::MAX_DURATION,
                     Value(static_cast<int64_t>(1000)));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START,
                    GestureConstants::ON_END},
                   {}, std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&up, 0, 0, false, nullptr);
  EXPECT_TRUE(handler.IsActive());
}

TEST_F(TapGestureHandlerTest, TimerExpiry_FailsGesture) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DURATION,
                     Value(static_cast<int64_t>(30)));
  config_map.emplace(GestureConstants::MAX_DISTANCE, Value(1000.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap, {GestureConstants::ON_END}, {},
                   std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  runner->AdvanceBy(fml::TimeDelta::FromMilliseconds(30));
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(TapGestureHandlerTest, DistanceFail_WorksForNegativeMovementToo) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  Value::Map config_map;
  config_map.emplace(GestureConstants::MAX_DISTANCE, Value(1.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Tap, {}, {}, std::move(config));
  TapGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {-10, 0}, 2);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}
}  // namespace testing

}  // namespace clay
