// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_pointer_event.h"

namespace lynx {
namespace tasm {

PlatformPointerEvent::PlatformPointerEvent(int int_event_data[],
                                           float float_event_data[])
    : PlatformInputEvent(int_event_data, float_event_data) {
  pointer_count_ = int_event_data[3];
  for (int i = 0; i < pointer_count_; ++i) {
    int offset = i * 3;
    pointer_id_.push_back(static_cast<int>(float_event_data[offset]));
    pointer_x_.push_back(float_event_data[offset + 1]);
    pointer_y_.push_back(float_event_data[offset + 2]);
  }
}

}  // namespace tasm
}  // namespace lynx
