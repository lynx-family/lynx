// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_GESTURE_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_GESTURE_EVENT_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

class GestureEvent : public CustomEvent {
 public:
  GestureEvent(int id, uint32_t gesture_id, std::string name,
               lepus::Value param_value)
      : CustomEvent(id, std::move(name), "detail", std::move(param_value)),
        gesture_id_(gesture_id) {}

  uint32_t GestureId() const { return gesture_id_; }

 private:
  uint32_t gesture_id_;
  std::string param_name_;
  lepus::Value param_value_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_GESTURE_EVENT_H_
