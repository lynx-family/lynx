// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/event/lepus_event_listener.h"

#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace tasm {

LepusClosureEventListener::LepusClosureEventListener(
    lepus::Context* context, lepus::Value closure,
    const EventListener::Options& options)
    : event::EventListener(
          event::EventListener::Type::kLepusClosureEventListener, options),
      context_(context),
      closure_(closure) {}

void LepusClosureEventListener::Invoke(event::Event* event) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LEPUS_CLOSURE_EVENT_LISTENER_INVOKE, "name",
              event ? event->type() : "");
  LOGI("LepusClosureEventListener::Invoke name: " << (event ? event->type()
                                                            : ""));
  if (context_ == nullptr || !closure_.IsCallable()) {
    LOGE(
        "LepusClosureEventListener::Invoke error: the context is null or "
        "closure isn't callable.");
    return;
  }
  context_->CallClosure(closure_, ConvertEventToLepusValue(event));
}

bool LepusClosureEventListener::Matches(event::EventListener* listener) {
  if (listener == nullptr || listener->type() != type()) {
    return false;
  }
  LepusClosureEventListener* other =
      static_cast<LepusClosureEventListener*>(listener);
  return context_ == other->context_ && closure_.IsEqual(other->closure_) &&
         options_.flags == other->GetOptions().flags;
};

lepus::Value LepusClosureEventListener::ConvertEventToLepusValue(
    event::Event* event) {
  lepus::Value value = lepus::LEPUSValueHelper::CreateObject(context_);
  if (event->event_type() == event::Event::EventType::kMessageEvent) {
    runtime::MessageEvent* message_event =
        static_cast<runtime::MessageEvent*>(event);
    value.SetProperty(BASE_STATIC_STRING(runtime::kType),
                      lepus::Value(message_event->type()));
    value.SetProperty(
        BASE_STATIC_STRING(runtime::kData),
        pub::ValueUtils::ConvertValueToLepusValue(*message_event->message()));
    value.SetProperty(BASE_STATIC_STRING(runtime::kOrigin),
                      lepus::Value(message_event->GetOriginString()));
  }
  if (event->event_type() == event::Event::EventType::kTouchEvent ||
      event->event_type() == event::Event::EventType::kCustomEvent) {
    event->HandleEventBaseDetail();
    return event->detail();
  }
  return value;
}

}  // namespace tasm
}  // namespace lynx
