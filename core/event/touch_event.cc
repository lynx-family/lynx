// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/touch_event.h"

namespace lynx {
namespace event {

static int64_t g_unique_touch_event_id_ = 0;
static int64_t GetNextUniqueTouchEventID() {
  return g_unique_touch_event_id_++;
}

TouchEvent::TouchEvent(const std::string& event_name, float x, float y,
                       float page_x, float page_y)
    : Event(event_name, Event::EventType::kTouchEvent, Event::Bubbles::kYes,
            Event::Cancelable::kNo, Event::ComposedMode::kComposed),
      x_(x),
      y_(y),
      page_x_(page_x),
      page_y_(page_y),
      identifier_(GetNextUniqueTouchEventID()) {}

}  // namespace event
}  // namespace lynx
