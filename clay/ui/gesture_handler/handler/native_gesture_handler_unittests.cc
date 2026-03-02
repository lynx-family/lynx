// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <memory>

#include "clay/public/value.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/gesture_handler/handler/native_gesture_handler.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

using ::testing::_;
using ::testing::Eq;
using ::testing::StrEq;

class NativeGestureHandlerTest : public ::testing::Test {};

TEST_F(NativeGestureHandlerTest, BehavesLikePanHandlerOnSameSequence) {
  auto runner = TestTaskRunner::Create();
  auto page_view = MakeTestPageView(0, runner);
  MockEventDelegate delegate;
  page_view->SetEventDelegate(&delegate);

  TestGestureArenaMember member(1);
  Value::Map config_map;
  config_map.emplace(GestureConstants::MIN_DISTANCE, Value(10.0f));
  Value config(std::move(config_map));
  auto detector =
      MakeDetector(2, GestureHandlerType::Native,
                   {GestureConstants::ON_BEGIN, GestureConstants::ON_START}, {},
                   std::move(config));

  NativeGestureHandler handler(1, page_view.get(), detector,
                               member.GetWeakPtr());

  PointerEvent down =
      MakePointerEvent(PointerEvent::EventType::kDownEvent, {0, 0}, 1);
  PointerEvent move =
      MakePointerEvent(PointerEvent::EventType::kMoveEvent, {-100, 0}, 2);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_BEGIN),
                                              Eq(1), Eq(2), _, _, _, _, _, _))
      .Times(1);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(StrEq(GestureConstants::ON_START),
                                              Eq(1), Eq(2), _, _, _, _, _, _))
      .Times(1);

  handler.HandleMotionEvent(&down, 0, 0, false, nullptr);
  handler.HandleMotionEvent(&move, 0, 0, false, nullptr);
  EXPECT_TRUE(handler.IsActive());
}
}  // namespace testing

}  // namespace clay
