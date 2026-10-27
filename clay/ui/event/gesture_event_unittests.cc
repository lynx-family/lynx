// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/gesture_event.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace testing {

TEST(GestureEventTest, BasicTest) {
  PointerEvent event(PointerEvent::EventType::kUnkownEvent);
  EXPECT_EQ(event.type, PointerEvent::EventType::kUnkownEvent);

  event.type = PointerEvent::EventType::kMoveEvent;
  EXPECT_NE(event.type, PointerEvent::EventType::kUpEvent);
  EXPECT_EQ(event.type, PointerEvent::EventType::kMoveEvent);

  FloatPoint p1(1, 1);
  FloatPoint p2(2, 3);
  event.position = p1 + p2;
  EXPECT_EQ(event.position, FloatPoint(3, 4));

  FloatSize d1(0.1, 0.2);
  d1 += FloatSize(0.01, 0.02);
  event.delta = d1;
  EXPECT_EQ(event.delta, FloatSize(0.11, 0.22));
}

}  // namespace testing

}  // namespace clay
