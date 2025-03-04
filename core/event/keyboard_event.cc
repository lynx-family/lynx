// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/keyboard_event.h"

namespace lynx {
namespace event {

KeyboardEvent::KeyboardEvent(const std::string& event_name,
                             const std::string& key_code)
    : Event(event_name, Event::EventType::kKeyboardEvent, Event::Bubbles::kYes,
            Event::Cancelable::kNo, Event::ComposedMode::kComposed) {
  key_code_ = key_code;
}

KeyboardEvent::~KeyboardEvent() = default;

}  // namespace event
}  // namespace lynx
