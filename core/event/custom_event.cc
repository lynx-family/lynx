// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/custom_event.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace event {

CustomEvent::CustomEvent(const std::string& event_name,
                         const lepus::Value& event_param,
                         const std::string& param_name, float time_stamp)
    : Event(event_name, time_stamp, Event::EventType::kCustomEvent,
            Event::Bubbles::kNo, Event::Cancelable::kYes,
            Event::ComposedMode::kComposed, Event::PhaseType::kNone),
      event_param_(event_param),
      param_name_(param_name) {
  event_type_ = Event::EventType::kCustomEvent;
}

void CustomEvent::HandleEventCustomDetail() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CUSTOM_EVENT_CUSTOM_DETAIL, "name", type_);
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");

  auto dict = detail_.Table();
  int64_t time_stamp = 0;
  if (event_param_.IsTable() && event_param_.Table()->Contains(kTimestamp)) {
    time_stamp = event_param_.Table()->GetValue(kTimestamp).Number();
    // TODO(hexionghui): In order to prevent the e2e test from failing, the
    // timestamp will be deleted here, and the e2e test will be modified later
    // to avoid this.
    event_param_.Table()->Erase(kTimestamp);
  } else {
    time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  }
  time_stamp_ = time_stamp;
  dict->SetValue(kTimestamp, time_stamp);
  dict->SetValue(param_name_, event_param_);
  if (param_name_ == "params") {
    BASE_STATIC_STRING_DECL(kDetail, "detail");
    dict->SetValue(kDetail, event_param_);
  }
}

}  // namespace event
}  // namespace lynx
