// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "build/build_config.h"
#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/window/pointer_data.h"
#include "clay/ui/window/pointer_data_packet.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

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
