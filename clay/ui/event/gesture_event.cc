// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/gesture_event.h"

#include <iterator>
#include <sstream>

namespace clay {
namespace {
const char* c_point_event_type_string[] = {
    "Unkown", "TouchDown", "TouchUp",      "TouchMove",     "Hover",
    "Signal", "Cancel",    "PanZoomStart", "PanZoomUpdate", "PanZoomEnd",
#if defined(OS_WIN) || defined(OS_MAC)
    "Add",    "Remove",
#endif
};

}  // namespace

std::string PointerEvent::ToString() const {
  const auto type_index = static_cast<size_t>(type);
  if (type_index >= std::size(c_point_event_type_string)) {
    return "InvalidPointerEvent";
  }
  std::stringstream ss;
  ss << "PointerEvent{" << c_point_event_type_string[type_index]
     << " pointer_id=" << pointer_id << " Position=" << position.x() << ","
     << position.y() << " Delta=" << delta.width() << "," << delta.height()
     << " PanDelta=" << pan_delta.width() << "," << pan_delta.height()
     << " ScrollDeltaX=" << scroll_delta_x << " ScrollDeltaY=" << scroll_delta_y
     << "}";
  return ss.str();
}
}  // namespace clay
