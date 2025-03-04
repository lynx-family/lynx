// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/event/context_proxy_in_lepus.h"

#include <memory>

#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/event/lepus_event_listener.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

ContextProxyInLepus::ContextProxyInLepus(
    runtime::ContextProxy::Delegate& delegate, runtime::ContextProxy::Type type)
    : runtime::ContextProxy(delegate, runtime::ContextProxy::Type::kCoreContext,
                            type),
      context_(nullptr),
      proxy_binding_(),
      on_trigger_event_() {}

runtime::MessageEvent ContextProxyInLepus::CreateMessageEvent(
    const lepus::Value& event) {
  return runtime::MessageEvent(
      event.GetProperty(BASE_STATIC_STRING(runtime::kType)).StdString(),
      GetOriginType(), GetTargetType(),
      event.GetProperty(BASE_STATIC_STRING(runtime::kData)));
}

ContextProxyInLepus* ContextProxyInLepus::GetContextProxyFromLepusValue(
    const lepus::Value& binding_object) {
  if (!binding_object.IsObject()) {
    return nullptr;
  }

  auto context_proxy_property = binding_object.GetProperty(
      BASE_STATIC_STRING(runtime::kInnerRuntimeProxy));
  if (!context_proxy_property.IsCPointer()) {
    return nullptr;
  }

  ContextProxyInLepus* context_proxy =
      reinterpret_cast<ContextProxyInLepus*>(context_proxy_property.CPoint());
  return context_proxy;
}

lepus::Value ContextProxyInLepus::GetBinding(lepus::Context* context) {
  if (proxy_binding_.IsEmpty()) {
    proxy_binding_ = lepus::Value::CreateObject(context);
    proxy_binding_.SetProperty(BASE_STATIC_STRING(runtime::kInnerRuntimeProxy),
                               lepus::Value(this));
    if (context->IsVMContext()) {
      // TODO(songshourui.null): There is no implementation of this function
      // when enable lite. Later, we will abstract RegisterMethodToContextProxy
      // as a utility function to solve this problem.
#if !ENABLE_JUST_LEPUSNG
      Utils::RegisterMethodToContextProxy(context, proxy_binding_,
                                          target_type_);
#endif
    } else {
      Utils::RegisterNGMethodToContextProxy(context, proxy_binding_,
                                            target_type_);
    }
  }
  if (context_ == nullptr) {
    context_ = context;
  }
  return proxy_binding_;
}

void ContextProxyInLepus::PostMessage(const lepus::Value& message) {
  EnsureListenerBeforePublishEvent();
  ContextProxy::PostMessage(message);
}

event::DispatchEventResult ContextProxyInLepus::DispatchEvent(
    event::Event& event) {
  EnsureListenerBeforePublishEvent();
  return ContextProxy::DispatchEvent(event);
}

void ContextProxyInLepus::EnsureListenerBeforePublishEvent() {
  auto new_on_trigger_event_ =
      proxy_binding_.GetProperty(BASE_STATIC_STRING(runtime::kOnTriggerEvent));
  if (new_on_trigger_event_.IsEqual(on_trigger_event_)) {
    return;
  }
  if (!new_on_trigger_event_.IsCallable()) {
    return;
  }

  on_trigger_event_ = new_on_trigger_event_;
  SetListenerBeforePublishEvent(
      std::make_unique<LepusClosureEventListener>(context_, on_trigger_event_));
}

}  // namespace tasm
}  // namespace lynx
