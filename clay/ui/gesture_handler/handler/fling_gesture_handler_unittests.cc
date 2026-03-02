// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <limits>
#include <memory>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"
#include "clay/ui/gesture_handler/handler/fling_gesture_handler.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::StrEq;

class FlingGestureHandlerTest : public ::testing::Test {};

TEST_F(FlingGestureHandlerTest, OnHandle_DownOrMove_Ignores) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  TestGestureArenaMember member(1);
  auto detector = MakeDetector(1, GestureHandlerType::Fling, {});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(),
            GestureConstants::LYNX_STATE_UNDETERMINED);

  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {1, 1}, 2);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(),
            GestureConstants::LYNX_STATE_UNDETERMINED);
}

TEST_F(FlingGestureHandlerTest, OnHandle_Up_BeginsAndEmitsOnBegin) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(7, GestureHandlerType::Fling, {GestureConstants::ON_BEGIN});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 3);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(7), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(&up, 0, 0, false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_BEGIN);
}

TEST_F(FlingGestureHandlerTest,
       OnHandle_NullPointer_FirstDelta_BeginActivateStart) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(7, GestureHandlerType::Fling,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(7), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(7), _, _, _, _, _, _))
      .Times(1);

  auto bundle = std::make_shared<GestureExtraBundle>();
  handler.HandleMotionEvent(nullptr, 10, 0, false, bundle);
  EXPECT_TRUE(handler.IsActive());
}

TEST_F(FlingGestureHandlerTest,
       OnHandle_NullPointer_SubsequentDelta_EmitsOnUpdate) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(7, GestureHandlerType::Fling, {GestureConstants::ON_UPDATE});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  handler.Activate();
  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_UPDATE), Eq(1),
                                    Eq(7), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(nullptr, 10, 0, false, nullptr);
}

TEST_F(FlingGestureHandlerTest, OnHandle_SentinelMinDelta_FailsAndEmitsOnEnd) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  auto detector =
      MakeDetector(7, GestureHandlerType::Fling,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_END});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  PointerEvent up =
      MakePointerEvent(PointerEvent::EventType::kUpEvent, {0, 0}, 1);
  handler.HandleMotionEvent(&up, 0, 0, false, nullptr);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_END),
                                              Eq(1), Eq(7), _, _, _, _, _, _))
      .Times(1);
  handler.HandleMotionEvent(nullptr, std::numeric_limits<float>::min(),
                            std::numeric_limits<float>::min(), false, nullptr);
  EXPECT_EQ(handler.GetGestureStatus(), GestureConstants::LYNX_STATE_FAIL);
}

TEST_F(FlingGestureHandlerTest,
       AdditionalParams_ContainsScrollDeltaAndBorders) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  member.SetScroll(3, 4);
  member.SetBorder(true, true);
  member.SetBorder(false, false);

  auto detector =
      MakeDetector(7, GestureHandlerType::Fling, {GestureConstants::ON_UPDATE});
  FlingGestureHandler handler(1, page_view.get(), detector,
                              member.GetWeakPtr());

  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(StrEq(GestureConstants::ON_UPDATE), Eq(1),
                                    Eq(7), _, _, _, _, _, _))
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

  handler.Activate();
  handler.HandleMotionEvent(nullptr, 10, -5, false, nullptr);
}
}  // namespace testing

}  // namespace clay
