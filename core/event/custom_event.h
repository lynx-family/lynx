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
              const std::string& param_name, int64_t time_stamp = 0,
              Capture capture = Capture::kNo, Bubbles bubbles = Bubbles::kNo,
              Cancelable cancelable = Cancelable::kYes,
              ComposedMode composed_mode = ComposedMode::kScoped,
              PhaseType phase_type = PhaseType::kNone,
              bool enable_legacy_frontend_event_param = false);

  void HandleEventBaseDetail(bool is_core_event = false) override;
  void HandleEventCustomDetail() override;

 private:
  bool ShouldUseLegacyFrontendEventParam() const;
  void ApplyLegacyFrontendEventParam();

  lepus::Value event_param_{lepus::Dictionary::Create()};
  std::string param_name_{""};
  bool enable_legacy_frontend_event_param_{false};
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_CUSTOM_EVENT_H_
