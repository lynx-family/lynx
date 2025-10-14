// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/event/gesture_event.h"

#include <sstream>

namespace clay {
namespace {
const char* c_point_event_type_string[] = {
    "Unkown", "TouchDown", "TouchUp",      "TouchMove",     "Hover",
    "Signal", "Cancel",    "PanZoomStart", "PanZoomUpdate", "PanZoomEnd"};
}  // namespace

std::string PointerEvent::ToString() const {
  if (type < EventType::kUnkownEvent || type > EventType::kPanZoomEndEvent) {
    return "InvalidPointerEvent";
  }
  std::stringstream ss;
  ss << "PointerEvent{" << c_point_event_type_string[static_cast<int>(type)]
     << " pointer_id=" << pointer_id << " Position=" << position.x() << ","
     << position.y() << " Delta=" << delta.width() << "," << delta.height()
     << " ScrollDeltaX=" << scroll_delta_x << " ScrollDeltaY=" << scroll_delta_y
     << "}";
  return ss.str();
}
}  // namespace clay
