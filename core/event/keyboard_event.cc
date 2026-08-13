// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/keyboard_event.h"

#include <chrono>

namespace lynx {
namespace event {
namespace {

int64_t NormalizeTimestamp(int64_t time_stamp) {
  if (time_stamp != 0) {
    return time_stamp;
  }
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace

KeyboardEvent::KeyboardEvent(const std::string& event_name,
                             const std::string& key_code)
    : Event(event_name, Event::EventType::kKeyboardEvent, Event::Capture::kYes,
            Event::Bubbles::kYes, Event::Cancelable::kNo,
            Event::ComposedMode::kComposed) {
  key_code_ = key_code;
  detail_.Table()->SetValue("key", key_code_);
}

KeyboardEvent::KeyboardEvent(const std::string& event_name,
                             const lepus::Value& event_param,
                             int64_t time_stamp)
    : Event(event_name, NormalizeTimestamp(time_stamp),
            Event::EventType::kKeyboardEvent, Event::Capture::kYes,
            Event::Bubbles::kYes, Event::Cancelable::kNo,
            Event::ComposedMode::kScoped, Event::PhaseType::kNone) {
  MergeEventDetail(event_param);
  if (event_param.IsTable()) {
    key_code_ = event_param.Table()->GetValue("key").StdString();
  }
}

KeyboardEvent::~KeyboardEvent() = default;

}  // namespace event
}  // namespace lynx
