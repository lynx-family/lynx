// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/events/closure_event_listener.h"

#include <utility>

#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"

namespace lynx {
namespace event {

ClosureEventListener::ClosureEventListener(ClosureListener&& closure)
    : event::EventListener(event::EventListener::Type::kClosureEventListener),
      closure_(std::move(closure)) {}

void ClosureEventListener::Invoke(event::Event* event) {
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    closure_(message_event->message());
  }
}

bool ClosureEventListener::Matches(EventListener* listener) {
  if (listener->type() != type()) {
    return false;
  }
  auto* other = static_cast<ClosureEventListener*>(listener);

  return &closure_ == &(other->closure_);
}

}  // namespace event
}  // namespace lynx
