// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_INPUT_EVENT_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_INPUT_EVENT_H_

#include <cstdint>

namespace lynx {
namespace tasm {

class PlatformInputEvent {
 public:
  static constexpr int kPointerEvent = 0;
  static constexpr int kKeyboardEvent = 1;

  PlatformInputEvent(int int_event_data[], float float_event_data[]);

  int EventType() const { return event_type_; }
  int ActionType() const { return action_type_; }
  int EventSource() const { return event_source_; }
  int64_t TimeStamp() const { return time_stamp_; }

 private:
  int event_type_{0};
  int action_type_{0};
  int event_source_{0};
  int64_t time_stamp_{0};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_INPUT_EVENT_H_
