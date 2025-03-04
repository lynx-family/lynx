// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_KEYBOARD_EVENT_H_
#define CORE_EVENT_KEYBOARD_EVENT_H_

#include <string>

#include "core/event/event.h"

namespace lynx {
namespace event {

class KeyboardEvent : public Event {
 public:
  KeyboardEvent(const std::string& event_name, const std::string& key_code);
  ~KeyboardEvent();

  const std::string& key_code() const { return key_code_; }

 private:
  std::string key_code_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_KEYBOARD_EVENT_H_
