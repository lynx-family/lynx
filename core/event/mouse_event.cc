// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/mouse_event.h"

namespace lynx {
namespace event {

MouseEvent::MouseEvent(const std::string& event_name,
                       const lepus::Value& event_param)
    : Event(event_name, EventType::kMouseEvent, Capture::kYes,
            event_name == "mouseenter" || event_name == "mouseleave"
                ? Bubbles::kNo
                : Bubbles::kYes,
            Cancelable::kYes, ComposedMode::kScoped, event_param) {}

}  // namespace event
}  // namespace lynx
