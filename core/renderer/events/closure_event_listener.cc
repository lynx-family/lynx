// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/events/closure_event_listener.h"

#include <utility>

#include "base/include/value/array.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace event {

ClosureEventListener::ClosureEventListener(
    ClosureListener&& closure, const EventListener::Options& options,
    ClosureType closure_type, const lepus::Value& lepus_object)
    : event::EventListener(event::EventListener::Type::kClosureEventListener,
                           options),
      closure_(std::move(closure)),
      closure_type_(closure_type),
      lepus_object_(lepus_object) {}

void ClosureEventListener::Invoke(event::Event* event) {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, CLOSURE_EVENT_LISTENER_INVOKE,
      [&event, listener = this](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("name", event ? event->type() : "");
        ctx.event()->add_debug_annotations(
            "type", std::to_string(static_cast<int>(listener->closure_type_)));
      });
  LOGI("ClosureEventListener::Invoke name: "
       << (event ? event->type() : "")
       << ", type: " << static_cast<int>(closure_type_));
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    closure_(
        pub::ValueUtils::ConvertValueToLepusValue(*message_event->message()));
  }
  if (event->event_type() == event::Event::EventType::kTouchEvent ||
      event->event_type() == event::Event::EventType::kCustomEvent) {
    if (!event->target() || !event->current_target()) {
      LOGE(
          "ClosureEventListener::Invoke error: the target or current_target is "
          "null.");
      return;
    }
    event->HandleEventBaseDetail(closure_type_ == ClosureType::kCore);
    auto args = lepus::CArray::Create();
    args->emplace_back(event->current_target()->GetEventControlInfo(
        event->type(), options_.IsGlobal()));
    args->emplace_back(event->detail());
    closure_(lepus::Value(std::move(args)));
  }
}

bool ClosureEventListener::Matches(EventListener* listener) {
  if (listener->type() != type()) {
    return false;
  }
  auto* other = static_cast<ClosureEventListener*>(listener);

  // When closure_type_ is not kNone, we need to match the listener by
  // lepus_object_ instead of closure_, because we cannot get the same closure_.
  bool listener_match = closure_type_ != ClosureType::kNone
                            ? lepus_object_ == other->lepus_object()
                            : &closure_ == &(other->closure_);
  bool listener_config_match = options_.flags == other->GetOptions().flags &&
                               closure_type_ == other->closure_type();
  return listener_match && listener_config_match;
}

}  // namespace event
}  // namespace lynx
