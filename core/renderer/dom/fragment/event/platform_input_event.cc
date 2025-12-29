// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_input_event.h"

#include <chrono>
#include <utility>

namespace lynx {
namespace tasm {

PlatformInputEvent::PlatformInputEvent(int int_event_data[],
                                       float float_event_data[]) {
  event_type_ = int_event_data[0];
  action_type_ = int_event_data[1];
  event_source_ = int_event_data[2];
  time_stamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
}

}  // namespace tasm
}  // namespace lynx
