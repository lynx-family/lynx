// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_input_event.h"

#include <chrono>

namespace lynx {
namespace tasm {

PlatformInputEvent::PlatformInputEvent(int int_event_data[],
                                       float float_event_data[],
                                       int64_t time_stamp) {
  event_type_ = int_event_data[0];
  action_type_ = int_event_data[1];
  event_source_ = int_event_data[2];
  // Gesture timing must use sample time, even when dispatch is delayed.
  time_stamp_ = time_stamp != 0
                    ? time_stamp
                    : std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now().time_since_epoch())
                          .count();
}

}  // namespace tasm
}  // namespace lynx
