// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/window/pointer_data_packet.h"
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

TEST(PointerDataToEventTest, PreservesWheelAndTrackpadData) {
  PointerDataPacket packet(4);
  PointerData data{};
  data.kind = PointerData::DeviceKind::kMouse;
  data.signal_kind = PointerData::SignalKind::kScroll;
  data.physical_x = 12;
  data.physical_y = 34;
  data.scroll_delta_x = 10;
  data.scroll_delta_y = -40;
  data.is_precise_scroll = 1;
  packet.SetPointerData(0, data);
  data = {};
  data.kind = PointerData::DeviceKind::kTrackpad;
  data.change = PointerData::Change::kPanZoomStart;
  packet.SetPointerData(1, data);
  data.change = PointerData::Change::kPanZoomUpdate;
  data.pan_x = 8;
  data.pan_y = 16;
  data.pan_delta_x = 2;
  data.pan_delta_y = -4;
  data.scale = 1.5;
  data.rotation = 0.25;
  packet.SetPointerData(2, data);
  data.change = PointerData::Change::kPanZoomEnd;
  packet.SetPointerData(3, data);

  const auto events = GetEventsFromPointerDataPacket(&packet);
  ASSERT_EQ(events.size(), 4u);
  EXPECT_EQ(events[0].device, PointerEvent::kMouse);
  EXPECT_EQ(events[0].type, PointerEvent::EventType::kSignalEvent);
  EXPECT_EQ(events[0].signal_kind, PointerEvent::SignalKind::kScroll);
  EXPECT_EQ(events[0].position, FloatPoint(12, 34));
  EXPECT_EQ(events[0].scroll_delta_x, 10);
  EXPECT_EQ(events[0].scroll_delta_y, -40);
  EXPECT_TRUE(events[0].is_precise_scroll);
  EXPECT_EQ(events[1].type, PointerEvent::EventType::kPanZoomStartEvent);
  EXPECT_EQ(events[2].type, PointerEvent::EventType::kPanZoomUpdateEvent);
  EXPECT_EQ(events[3].type, PointerEvent::EventType::kPanZoomEndEvent);
  EXPECT_EQ(events[2].device, PointerEvent::kTrackpad);
  EXPECT_EQ(events[2].pan, FloatPoint(8, 16));
  EXPECT_EQ(events[2].pan_delta, FloatSize(2, -4));
  EXPECT_EQ(events[2].scale, 1.5);
  EXPECT_EQ(events[2].rotation, 0.25);
}

TEST(PointerDataToEventTest, PreservesTouchContactDataAndCancellation) {
  PointerDataPacket packet(3);
  PointerData data{};
  data.kind = PointerData::DeviceKind::kTouch;
  data.device = 7;
  data.pointer_identifier = 42;
  data.time_stamp = 123456;
  data.physical_x = 12;
  data.physical_y = 34;
  data.pressure = 0.75;
  data.radius_major = 4;
  data.radius_minor = 2;
  data.change = PointerData::Change::kDown;
  data.buttons = PointerEvent::kPrimary;
  packet.SetPointerData(0, data);
  data.change = PointerData::Change::kMove;
  data.physical_delta_x = 2;
  data.physical_delta_y = -4;
  packet.SetPointerData(1, data);
  data.change = PointerData::Change::kCancel;
  data.buttons = 0;
  packet.SetPointerData(2, data);

  const auto events = GetEventsFromPointerDataPacket(&packet);
  ASSERT_EQ(events.size(), 3u);
  EXPECT_EQ(events[0].type, PointerEvent::EventType::kDownEvent);
  EXPECT_EQ(events[1].type, PointerEvent::EventType::kMoveEvent);
  EXPECT_EQ(events[2].type, PointerEvent::EventType::kCancel);
  EXPECT_TRUE(events[0].down);
  EXPECT_TRUE(events[1].down);
  EXPECT_FALSE(events[2].down);
  EXPECT_EQ(events[1].delta, FloatSize(2, -4));
  EXPECT_EQ(events[0].buttons, PointerEvent::kPrimary);
  EXPECT_EQ(events[2].buttons, 0);
  for (const auto& event : events) {
    EXPECT_EQ(event.device, PointerEvent::kTouch);
    EXPECT_EQ(event.device_id, 7);
    EXPECT_EQ(event.pointer_id, 42);
    EXPECT_EQ(event.timestamp, 123456u);
    EXPECT_EQ(event.position, FloatPoint(12, 34));
    EXPECT_EQ(event.pressure, 0.75);
    EXPECT_EQ(event.radius_major, 4);
    EXPECT_EQ(event.radius_minor, 2);
  }
}

}  // namespace testing

}  // namespace clay
