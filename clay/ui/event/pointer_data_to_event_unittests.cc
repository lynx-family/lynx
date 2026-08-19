// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "build/build_config.h"
#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/window/pointer_data.h"
#include "clay/ui/window/pointer_data_packet.h"
#include "clay/ui/window/pointer_data_packet_converter.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

#if defined(OS_WIN) || defined(OS_MAC)
TEST(PointerDataToEventTest, PreservesGestureIdentityForPenLifecycle) {
  auto packet = std::make_unique<PointerDataPacket>(4);
  PointerData data{};
  data.kind = PointerData::DeviceKind::kStylus;
  data.device = 7;
  data.pointer_identifier = 42;
  data.physical_x = 12;
  data.physical_y = 34;

  data.change = PointerData::Change::kAdd;
  data.synthesized = 1;
  packet->SetPointerData(0, data);
  data.change = PointerData::Change::kHover;
  data.synthesized = 0;
  packet->SetPointerData(1, data);
  data.change = PointerData::Change::kCancel;
  packet->SetPointerData(2, data);
  data.change = PointerData::Change::kRemove;
  packet->SetPointerData(3, data);

  auto events = GetEventsFromPointerDataPacket(packet.get());

  ASSERT_EQ(events.size(), 4u);
  EXPECT_EQ(events[0].type, PointerEvent::EventType::kAddEvent);
  EXPECT_EQ(events[1].type, PointerEvent::EventType::kHoverEvent);
  EXPECT_EQ(events[2].type, PointerEvent::EventType::kCancel);
  EXPECT_EQ(events[3].type, PointerEvent::EventType::kRemoveEvent);
  EXPECT_EQ(events[0].device, PointerEvent::DeviceType::kStylus);
  EXPECT_TRUE(events[0].synthesized);
  EXPECT_FALSE(events[1].synthesized);
  EXPECT_EQ(events[3].position, FloatPoint(12, 34));
  EXPECT_EQ(events[3].device_id, 7);
  EXPECT_EQ(events[3].pointer_id, 42);
}

TEST(PointerDataToEventTest, PreservesGestureIdentityForHoverDevices) {
  const PointerData::DeviceKind kinds[] = {
      PointerData::DeviceKind::kMouse,
      PointerData::DeviceKind::kInvertedStylus,
  };
  for (const auto kind : kinds) {
    SCOPED_TRACE(static_cast<int64_t>(kind));
    auto packet = std::make_unique<PointerDataPacket>(2);
    PointerData data{};
    data.kind = kind;
    data.pointer_identifier = 42;
    data.synthesized = 1;

    data.change = PointerData::Change::kAdd;
    packet->SetPointerData(0, data);
    data.change = PointerData::Change::kRemove;
    packet->SetPointerData(1, data);

    auto events = GetEventsFromPointerDataPacket(packet.get());

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].type, PointerEvent::EventType::kAddEvent);
    EXPECT_EQ(events[1].type, PointerEvent::EventType::kRemoveEvent);
    EXPECT_EQ(events[0].pointer_id, 42);
    EXPECT_TRUE(events[0].synthesized);
    EXPECT_EQ(events[0].device,
              kind == PointerData::DeviceKind::kMouse
                  ? PointerEvent::DeviceType::kMouse
                  : PointerEvent::DeviceType::kInvertedStylus);
  }
}

TEST(PointerDataToEventTest, AssignsDistinctGestureIdsAcrossPenContacts) {
  auto packet = std::make_unique<PointerDataPacket>(6);
  PointerData data{};
  data.kind = PointerData::DeviceKind::kStylus;
  data.device = 7;

  data.change = PointerData::Change::kAdd;
  packet->SetPointerData(0, data);
  data.change = PointerData::Change::kDown;
  data.buttons = PointerEvent::MouseButton::kPrimary;
  packet->SetPointerData(1, data);
  data.change = PointerData::Change::kUp;
  data.buttons = 0;
  packet->SetPointerData(2, data);
  data.change = PointerData::Change::kDown;
  data.buttons = PointerEvent::MouseButton::kPrimary;
  packet->SetPointerData(3, data);
  data.change = PointerData::Change::kUp;
  data.buttons = 0;
  packet->SetPointerData(4, data);
  data.change = PointerData::Change::kRemove;
  packet->SetPointerData(5, data);

  PointerDataPacketConverter converter;
  auto converted_packet = converter.Convert(std::move(packet));
  auto events = GetEventsFromPointerDataPacket(converted_packet.get());

  ASSERT_EQ(events.size(), 6u);
  for (const auto& event : events) {
    EXPECT_EQ(event.device_id, 7);
  }
  EXPECT_EQ(events[0].pointer_id, 0);
  EXPECT_EQ(events[1].pointer_id, 1);
  EXPECT_EQ(events[2].pointer_id, 1);
  EXPECT_EQ(events[3].pointer_id, 2);
  EXPECT_EQ(events[4].pointer_id, 2);
  EXPECT_EQ(events[5].pointer_id, 0);
  EXPECT_EQ(events[0].type, PointerEvent::EventType::kAddEvent);
  EXPECT_EQ(events[1].type, PointerEvent::EventType::kDownEvent);
  EXPECT_EQ(events[2].type, PointerEvent::EventType::kUpEvent);
  EXPECT_EQ(events[3].type, PointerEvent::EventType::kDownEvent);
  EXPECT_EQ(events[4].type, PointerEvent::EventType::kUpEvent);
  EXPECT_EQ(events[5].type, PointerEvent::EventType::kRemoveEvent);
}
#endif

TEST(PointerDataToEventTest, PreservesLegacyPointerConversion) {
  const PointerData::DeviceKind kinds[] = {
    PointerData::DeviceKind::kTouch,
    PointerData::DeviceKind::kTrackpad,
#if !defined(OS_WIN) && !defined(OS_MAC)
    PointerData::DeviceKind::kStylus,
    PointerData::DeviceKind::kInvertedStylus,
#endif
  };
  for (const auto kind : kinds) {
    SCOPED_TRACE(static_cast<int64_t>(kind));
    PointerDataPacket packet(2);
    PointerData data{};
    data.kind = kind;
    data.pointer_identifier = 42;
    data.physical_x = 12;
    data.physical_y = 34;
    data.synthesized = 1;
    data.change = PointerData::Change::kAdd;
    packet.SetPointerData(0, data);
    data.change = PointerData::Change::kRemove;
    packet.SetPointerData(1, data);

    auto events = GetEventsFromPointerDataPacket(&packet);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, PointerEvent::EventType::kCancel);
    EXPECT_EQ(events[0].pointer_id, 42);
    EXPECT_EQ(events[0].position, FloatPoint(0, 0));
    EXPECT_FALSE(events[0].synthesized);
    EXPECT_EQ(events[0].device, kind == PointerData::DeviceKind::kTrackpad
                                    ? PointerEvent::DeviceType::kTrackpad
                                    : PointerEvent::DeviceType::kTouch);
  }
}

}  // namespace testing
}  // namespace clay
