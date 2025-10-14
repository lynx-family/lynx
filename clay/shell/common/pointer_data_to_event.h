// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_POINTER_DATA_TO_EVENT_H_
#define CLAY_SHELL_COMMON_POINTER_DATA_TO_EVENT_H_

#include <vector>

#include "clay/ui/event/gesture_event.h"
#include "clay/ui/window/pointer_data_packet.h"

namespace clay {

std::vector<clay::PointerEvent> GetEventsFromPointerDataPacket(
    const clay::PointerDataPacket* packet);

}  // namespace clay
#endif  // CLAY_SHELL_COMMON_POINTER_DATA_TO_EVENT_H_
