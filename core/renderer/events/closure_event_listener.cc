// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/events/closure_event_listener.h"

#include <utility>

#include "base/include/value/array.h"
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
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    closure_(
        pub::ValueUtils::ConvertValueToLepusValue(*message_event->message()));
  }
  if (event->event_type() == event::Event::EventType::kTouchEvent ||
      event->event_type() == event::Event::EventType::kCustomEvent) {
    if (event->current_target() == nullptr) {
      return;
    }
    event->HandleEventBaseDetail(closure_type_ == ClosureType::kCore);
    auto args = lepus::CArray::Create();
    args->emplace_back(event->current_target()->GetEventControlInfo(
        event->type(), options_.is_global));
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
  bool listener_config_match =
      options_.capture == other->GetOptions().capture &&
      options_.is_catch == other->GetOptions().is_catch &&
      options_.is_global == other->GetOptions().is_global &&
      closure_type_ == other->closure_type();
  return listener_match && listener_config_match;
}

}  // namespace event
}  // namespace lynx
