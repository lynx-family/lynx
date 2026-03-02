// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>
#include <unordered_map>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/default_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/fling_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/gesture_handler/handler/longpress_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/native_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/tap_gesture_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;

class BaseGestureHandlerTest : public ::testing::Test {};

TEST_F(BaseGestureHandlerTest,
       ConvertToGestureHandler_CreatesExpectedHandlers) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  GestureMap detectors;
  detectors.emplace(1, MakeDetector(1, GestureHandlerType::Pan));
  detectors.emplace(2, MakeDetector(2, GestureHandlerType::Tap));
  detectors.emplace(3, MakeDetector(3, GestureHandlerType::LongPress));
  detectors.emplace(4, MakeDetector(4, GestureHandlerType::Fling));
  detectors.emplace(5, MakeDetector(5, GestureHandlerType::Default));
  detectors.emplace(6, MakeDetector(6, GestureHandlerType::Native));
  detectors.emplace(7, MakeDetector(7, GestureHandlerType::Rotation));

  GestureHandlerMap handlers = BaseGestureHandler::ConvertToGestureHandler(
      1, page_view.get(), member.GetWeakPtr(), detectors);

  ASSERT_NE(handlers.find(1), handlers.end());
  ASSERT_NE(handlers.find(2), handlers.end());
  ASSERT_NE(handlers.find(3), handlers.end());
  ASSERT_NE(handlers.find(4), handlers.end());
  ASSERT_NE(handlers.find(5), handlers.end());
  ASSERT_NE(handlers.find(6), handlers.end());
  EXPECT_EQ(handlers.find(7), handlers.end());
}

TEST_F(BaseGestureHandlerTest, EnableFlags_DefaultAllFalse) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  auto detector = MakeDetector(1, GestureHandlerType::Pan, {});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  EXPECT_FALSE(handler.IsOnBeginEnable());
  EXPECT_FALSE(handler.IsOnUpdateEnable());
  EXPECT_FALSE(handler.IsOnStartEnable());
  EXPECT_FALSE(handler.IsOnEndEnable());
}

TEST_F(BaseGestureHandlerTest, EnableFlags_EnablesOnlySpecifiedCallbacks) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  auto detector =
      MakeDetector(1, GestureHandlerType::Pan,
                   {GestureConstants::ON_TOUCHES_DOWN,
                    GestureConstants::ON_BEGIN, GestureConstants::ON_END});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  EXPECT_TRUE(handler.IsOnBeginEnable());
  EXPECT_FALSE(handler.IsOnUpdateEnable());
  EXPECT_FALSE(handler.IsOnStartEnable());
  EXPECT_TRUE(handler.IsOnEndEnable());
}

TEST_F(BaseGestureHandlerTest, SendGestureEvent_ForwardsToEventDelegate) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(9, GestureHandlerType::Pan, {GestureConstants::ON_BEGIN});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());

  PointerEvent ev =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {1, 2}, 123);
  Value empty;

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(9), _, _, _, _, _, _))
      .Times(1);
  handler.SendGestureEvent(GestureConstants::ON_BEGIN, &ev, empty);
}

TEST_F(BaseGestureHandlerTest, OnTouchesDown_EmitsOnlyWhenEnabled) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  PointerEvent ev =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {1, 2}, 1);

  {
    auto detector = MakeDetector(1, GestureHandlerType::Pan,
                                 {GestureConstants::ON_TOUCHES_DOWN});
    PanGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());
    EXPECT_CALL(delegate,
                OnGestureHandlerEvent(StrEq(GestureConstants::ON_TOUCHES_DOWN),
                                      Eq(1), Eq(1), _, _, _, _, _, _))
        .Times(1);
    handler.OnTouchesDown(&ev);
  }

  {
    auto detector = MakeDetector(2, GestureHandlerType::Pan, {});
    PanGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());
    EXPECT_CALL(delegate, OnGestureHandlerEvent(_, _, _, _, _, _, _, _, _))
        .Times(0);
    handler.OnTouchesDown(&ev);
  }
}

TEST_F(BaseGestureHandlerTest, OnTouchesMove_EmitsOnlyWhenEnabled) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  PointerEvent ev =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {1, 2}, 1);

  auto detector = MakeDetector(1, GestureHandlerType::Pan,
                               {GestureConstants::ON_TOUCHES_MOVE});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_TOUCHES_MOVE),
                                    Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.OnTouchesMove(&ev);
}

TEST_F(BaseGestureHandlerTest, OnTouchesUp_EmitsOnlyWhenEnabled) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  PointerEvent ev =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {1, 2}, 1);

  auto detector = MakeDetector(1, GestureHandlerType::Pan,
                               {GestureConstants::ON_TOUCHES_UP});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_TOUCHES_UP),
                                    Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.OnTouchesUp(&ev);
}

TEST_F(BaseGestureHandlerTest, OnTouchesCancel_EmitsOnlyWhenEnabled) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  PointerEvent ev =
      MakePointerEvent(PointerEvent::EventType::kCancel, {1, 2}, 1);

  auto detector = MakeDetector(1, GestureHandlerType::Pan,
                               {GestureConstants::ON_TOUCHES_CANCEL});
  PanGestureHandler handler(1, page_view.get(), detector, member.GetWeakPtr());
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_TOUCHES_CANCEL),
                                    Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.OnTouchesCancel(&ev);
}
}  // namespace testing

}  // namespace clay
