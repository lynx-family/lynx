// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_WHEEL_EVENT_H_
#define CORE_EVENT_WHEEL_EVENT_H_

#include <string>

#include "core/event/event.h"

namespace lynx {
namespace event {

class WheelEvent : public Event {
 public:
  WheelEvent(const std::string& event_name, const lepus::Value& event_param);
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_WHEEL_EVENT_H_
