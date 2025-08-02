// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_CUSTOM_EVENT_H_
#define CORE_EVENT_CUSTOM_EVENT_H_

#include <string>

#include "core/event/event.h"

namespace lynx {
namespace event {

class CustomEvent : public Event {
 public:
  CustomEvent(const std::string& event_name, const lepus::Value& event_param,
              const std::string& param_name, float time_stamp = 0);

  void HandleEventCustomDetail() override;

 private:
  lepus::Value event_param_{lepus::Dictionary::Create()};
  std::string param_name_{""};
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_CUSTOM_EVENT_H_
