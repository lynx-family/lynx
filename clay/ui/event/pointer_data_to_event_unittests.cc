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
TEST(PointerDataToEventTest, PreservesTouchAndTrackpadLifecycle) {
  const PointerData::DeviceKind kinds[] = {
      PointerData::DeviceKind::kTouch,
      PointerData::DeviceKind::kTrackpad,
  };
  for (const auto kind : kinds) {
    SCOPED_TRACE(static_cast<int64_t>(kind));
    auto packet = std::make_unique<PointerDataPacket>(2);
    PointerData data{};
    data.kind = kind;
    data.pointer_identifier = 42;
    data.physical_x = 12;
    data.physical_y = 34;
    data.synthesized = 1;

    data.change = PointerData::Change::kAdd;
    packet->SetPointerData(0, data);
    data.change = PointerData::Change::kRemove;
    packet->SetPointerData(1, data);

    auto events = GetEventsFromPointerDataPacket(packet.get());

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, PointerEvent::EventType::kCancel);
    EXPECT_EQ(events[0].pointer_id, 42);
    EXPECT_EQ(events[0].position, FloatPoint(0, 0));
    EXPECT_FALSE(events[0].synthesized);
    EXPECT_EQ(events[0].device, kind == PointerData::DeviceKind::kTouch
                                    ? PointerEvent::DeviceType::kTouch
                                    : PointerEvent::DeviceType::kTrackpad);
  }
}

#elif defined(OS_ANDROID)
TEST(PointerDataToEventTest, PreservesAndroidPointerConversion) {
  auto packet = std::make_unique<PointerDataPacket>(2);
  PointerData data{};
  data.kind = PointerData::DeviceKind::kStylus;
  data.pointer_identifier = 42;
  data.physical_x = 12;
  data.physical_y = 34;
  data.synthesized = 1;

  data.change = PointerData::Change::kAdd;
  packet->SetPointerData(0, data);
  data.change = PointerData::Change::kRemove;
  packet->SetPointerData(1, data);

  auto events = GetEventsFromPointerDataPacket(packet.get());

  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].type, PointerEvent::EventType::kCancel);
  EXPECT_EQ(events[0].device, PointerEvent::DeviceType::kTouch);
  EXPECT_EQ(events[0].pointer_id, 42);
  EXPECT_EQ(events[0].position, FloatPoint(0, 0));
  EXPECT_FALSE(events[0].synthesized);
}
#endif

}  // namespace testing
}  // namespace clay
