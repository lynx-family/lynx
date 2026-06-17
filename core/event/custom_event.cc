// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/custom_event.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace event {
namespace {

template <typename Callback>
void ForEachObjectProperty(const lepus::Value& value, Callback callback) {
  if (value.IsJSValue()) {
    value.IteratorJSValue(
        [&callback](const lepus::Value& key, const lepus::Value& value) {
          callback(key, value);
        });
    return;
  }

  if (!value.IsTable()) {
    return;
  }
  auto table = value.Table();
  table->for_each([&callback](const auto& key, const auto& value) {
    callback(lepus::Value(key), value);
  });
}

}  // namespace

CustomEvent::CustomEvent(const std::string& event_name,
                         const lepus::Value& event_param,
                         const std::string& param_name, int64_t time_stamp,
                         Capture capture, Bubbles bubbles,
                         Cancelable cancelable, ComposedMode composed_mode,
                         PhaseType phase_type,
                         bool enable_legacy_frontend_event_param)
    : Event(event_name, time_stamp, EventType::kCustomEvent, capture, bubbles,
            cancelable, composed_mode, phase_type),
      event_param_(event_param),
      param_name_(param_name),
      enable_legacy_frontend_event_param_(enable_legacy_frontend_event_param) {
  event_type_ = EventType::kCustomEvent;
}

void CustomEvent::HandleEventBaseDetail(bool is_core_event) {
  if (ShouldUseLegacyFrontendEventParam() && !event_param_.IsObject()) {
    detail_ = event_param_;
    return;
  }
  Event::HandleEventBaseDetail(is_core_event);
  ApplyLegacyFrontendEventParam();
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
  if (param_name_ == "params" && !from_frontend_) {
    BASE_STATIC_STRING_DECL(kDetail, "detail");
    dict->SetValue(kDetail, event_param_);
  }
}

bool CustomEvent::ShouldUseLegacyFrontendEventParam() const {
  return enable_legacy_frontend_event_param_ && from_frontend_;
}

void CustomEvent::ApplyLegacyFrontendEventParam() {
  if (!ShouldUseLegacyFrontendEventParam()) {
    return;
  }
  if (event_param_.IsObject()) {
    ForEachObjectProperty(event_param_, [this](const lepus::Value& key,
                                               const lepus::Value& value) {
      detail_.Table()->SetValue(key.String(), value);
    });
  } else {
    detail_ = event_param_;
  }
}

}  // namespace event
}  // namespace lynx
