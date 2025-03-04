// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_H_

#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/vm/lepus/context.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class ContextProxyInLepus : public runtime::ContextProxy {
 public:
  ContextProxyInLepus(runtime::ContextProxy::Delegate& delegate,
                      runtime::ContextProxy::Type type);
  virtual ~ContextProxyInLepus() override = default;

  static ContextProxyInLepus* GetContextProxyFromLepusValue(
      const lepus::Value& binding_object);

  runtime::MessageEvent CreateMessageEvent(const lepus::Value& event);

  lepus::Value GetBinding(lepus::Context* context);

  virtual void PostMessage(const lepus::Value& message) override;

  virtual event::DispatchEventResult DispatchEvent(
      event::Event& event) override;

 protected:
  void EnsureListenerBeforePublishEvent();

  lepus::Context* context_;
  lepus::Value proxy_binding_;
  lepus::Value on_trigger_event_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_EVENT_CONTEXT_PROXY_IN_LEPUS_H_
