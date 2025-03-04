// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/common/event/context_proxy.h"

#include <utility>

#include "core/event/event.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"

namespace lynx {
namespace runtime {

std::string ContextProxy::ConvertContextTypeToString(ContextProxy::Type type) {
  if (type == ContextProxy::Type::kJSContext) {
    return kJSContext;
  } else if (type == ContextProxy::Type::kCoreContext) {
    return kCoreContext;
  } else if (type == ContextProxy::Type::kUIContext) {
    return kUIContext;
  } else if (type == ContextProxy::Type::kDevTool) {
    return kDevTool;
  }
  return kUnknown;
}

ContextProxy::Type ContextProxy::ConvertStringToContextType(
    const std::string& type_str) {
  if (type_str == kJSContext) {
    return ContextProxy::Type::kJSContext;
  } else if (type_str == kCoreContext) {
    return ContextProxy::Type::kCoreContext;
  } else if (type_str == kUIContext) {
    return ContextProxy::Type::kUIContext;
  } else if (type_str == kDevTool) {
    return ContextProxy::Type::kDevTool;
  }
  return ContextProxy::Type::kUnknown;
}

void ContextProxy::PostMessage(const lepus::Value& message) {
  MessageEvent event(origin_type_, target_type_, message);
  DispatchEvent(event);
}

void ContextProxy::SetListenerBeforePublishEvent(
    std::unique_ptr<event::EventListener> listener) {
  event_listener_.swap(listener);
}

event::EventListener* ContextProxy::GetListenerBeforePublishEvent() {
  return event_listener_.get();
}

event::DispatchEventResult ContextProxy::DispatchEvent(event::Event& event) {
  if (event.event_type() != event::Event::EventType::kMessageEvent) {
    return event::DispatchEventResult::kNotCanceled;
  }
  MessageEvent& message_event = static_cast<MessageEvent&>(event);
  if (message_event.GetTargetType() == origin_type_) {
    if (event_listener_ != nullptr) {
      event_listener_->Invoke(&event);
    }
    EventTarget::DispatchEvent(message_event);
    return event::DispatchEventResult::kNotCanceled;
  }
  return delegate_.DispatchMessageEvent(std::move(message_event));
}

}  // namespace runtime
}  // namespace lynx
