// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CUSTOM_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CUSTOM_EVENT_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/lynx_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

class CustomEvent : public LynxEvent {
 public:
  CustomEvent(int id, std::string name, std::string param_name,
              lepus::Value param_value)
      : LynxEvent(id, std::move(name), LynxEventType::kCustom),
        param_name_(std::move(param_name)),
        param_value_(std::move(param_value)) {}

  const std::string& ParamName() const { return param_name_; }

  const lepus::Value& ParamValue() const { return param_value_; }

 private:
  std::string param_name_;
  lepus::Value param_value_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_EVENT_CUSTOM_EVENT_H_
