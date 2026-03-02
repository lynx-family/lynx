// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <cmath>
#include <limits>
#include <memory>

#include "clay/public/value.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/gesture_handler_dispatcher.h"
#include "clay/ui/gesture_handler/handler/default_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::StrEq;

class DefaultGestureHandlerTest : public ::testing::Test {};

TEST_F(DefaultGestureHandlerTest, HandleConfigMap_TapSlopApplied) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  auto* dispatcher = page_view->GetGestureHandlerDispatcher();
  ASSERT_NE(dispatcher, nullptr);

  TestGestureArenaMember member(1);
  member.SetCanConsume(true);

  Value::Map config_map;
  config_map.emplace("tapSlop", Value(1));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(1, GestureHandlerType::Default, {}, {}, std::move(config));
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  EXPECT_EQ(dispatcher->GetGestureRecognizedTargetSet().count(1), 0u);

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {3, 0}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();

  handler.HandleMotionEvent(&down, 0, 0, false, bundle);
  handler.HandleMotionEvent(&move, 0, 0, false, bundle);

  EXPECT_EQ(dispatcher->GetGestureRecognizedTargetSet().count(1), 1u);
}

TEST_F(DefaultGestureHandlerTest, OnHandle_Down_SetsBegin_EmitsOnBeginOnce) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector = MakeDetector(1, GestureHandlerType::Default,
                               {GestureConstants::ON_BEGIN});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {10, 10}, 1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
}

TEST_F(DefaultGestureHandlerTest, OnHandle_Move_SetsDirectionWhenUndetermined) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);
  member.SetCanConsume(true);

  auto detector = MakeDetector(1, GestureHandlerType::Default, {});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 1}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();

  handler.HandleMotionEvent(&down, 0, 0, false, bundle);
  handler.HandleMotionEvent(&move, 0, 0, false, bundle);

  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_HORIZONTAL);
}

TEST_F(DefaultGestureHandlerTest, OnHandle_Move_LocksDeltaToDirectionAxis) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);
  member.SetCanConsume(true);
  member.SetScrollContainerDirection(GestureConstants::DIRECTION_UNDETERMINED);

  auto detector = MakeDetector(1, GestureHandlerType::Default, {});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 5}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();

  handler.HandleMotionEvent(&down, 0, 0, false, bundle);
  handler.HandleMotionEvent(&move, 0, 0, false, bundle);

  auto calls = member.TakeScrollCalls();
  ASSERT_FALSE(calls.empty());
  EXPECT_EQ(bundle->GestureDirection(), GestureConstants::DIRECTION_HORIZONTAL);
  EXPECT_EQ(calls.back().second, 0);
}

TEST_F(DefaultGestureHandlerTest,
       OnHandle_SimultaneousConsumption_UsesBundleDeltasAndReturns) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);

  auto detector = MakeDetector(1, GestureHandlerType::Default, {});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {1, 1}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();
  bundle->SetIsNeedConsumedSimultaneousGesture(true);
  bundle->SetSimultaneousDeltaX(7);
  bundle->SetSimultaneousDeltaY(-9);

  handler.HandleMotionEvent(&move, 0, 0, true, bundle);
  auto calls = member.TakeScrollCalls();
  ASSERT_EQ(calls.size(), 1u);
  EXPECT_EQ(calls[0].first, 7);
  EXPECT_EQ(calls[0].second, -9);
}

TEST_F(DefaultGestureHandlerTest, OnHandle_ShouldFail_WhenMemberCannotConsume) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  member.SetCanConsume(false);

  auto detector =
      MakeDetector(1, GestureHandlerType::Default,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_END});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, bundle);
  handler.HandleMotionEvent(&move, 0, 0, false, bundle);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(DefaultGestureHandlerTest,
       OnHandle_ShouldFail_WhenDirectionMismatchWithScrollContainer) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  member.SetCanConsume(true);
  member.SetScrollContainerDirection(GestureConstants::DIRECTION_VERTICAL);

  auto detector =
      MakeDetector(1, GestureHandlerType::Default,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_END});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);
  auto bundle = std::make_shared<GestureExtraBundle>();

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(1), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, bundle);
  handler.HandleMotionEvent(&move, 0, 0, false, bundle);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(DefaultGestureHandlerTest, OnHandle_NullPointer_UsesFlingDeltas) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);
  member.SetCanConsume(true);

  auto detector = MakeDetector(1, GestureHandlerType::Default, {});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  auto bundle = std::make_shared<GestureExtraBundle>();
  handler.HandleMotionEvent(nullptr, 10, 0, false, bundle);
  EXPECT_TRUE(handler.IsActive());
}

TEST_F(DefaultGestureHandlerTest,
       AdditionalParams_ContainsScrollDeltaAndBorders) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  member.SetScroll(3, 4);
  member.SetBorder(true, true);
  member.SetBorder(false, false);
  member.SetCanConsume(true);

  auto detector = MakeDetector(1, GestureHandlerType::Default,
                               {GestureConstants::ON_UPDATE});
  DefaultGestureHandler handler(1, page_view.get(), detector,
                                member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {10, 0}, 2);

  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_UPDATE), Eq(1),
                                    Eq(1), _, _, _, _, _, _))
      .WillOnce(Invoke([&](const std::string&, int, uint32_t, float, float,
                           float, float, int64_t, Value& params) {
        ASSERT_TRUE(params.IsMap());
        const auto& m = params.GetMap();
        EXPECT_NE(m.find("scrollX"), m.end());
        EXPECT_NE(m.find("scrollY"), m.end());
        EXPECT_NE(m.find("deltaX"), m.end());
        EXPECT_NE(m.find("deltaY"), m.end());
        EXPECT_NE(m.find("isAtStart"), m.end());
        EXPECT_NE(m.find("isAtEnd"), m.end());
      }));

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
}
}  // namespace testing

}  // namespace clay
