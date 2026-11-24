// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/keyboard_event.h"

namespace lynx {
namespace event {

KeyboardEvent::KeyboardEvent(const std::string& event_name,
                             const std::string& key_code)
    : Event(event_name, Event::EventType::kKeyboardEvent, Event::Capture::kYes,
            Event::Bubbles::kYes, Event::Cancelable::kNo,
            Event::ComposedMode::kComposed) {
  key_code_ = key_code;
}

KeyboardEvent::KeyboardEvent(const std::string& event_name,
                             const lepus::Value& event_param)
    : Event(event_name, EventType::kKeyboardEvent, Capture::kYes, Bubbles::kYes,
            Cancelable::kYes, ComposedMode::kScoped, event_param) {
  BASE_STATIC_STRING_DECL(kKey, "key");
  const auto& key = detail_.GetProperty(kKey);
  if (key.IsString()) {
    key_code_ = key.StdString();
  }
}

KeyboardEvent::~KeyboardEvent() = default;

}  // namespace event
}  // namespace lynx
