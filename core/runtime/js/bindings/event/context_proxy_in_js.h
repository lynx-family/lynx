// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_EVENT_CONTEXT_PROXY_IN_JS_H_
#define CORE_RUNTIME_JS_BINDINGS_EVENT_CONTEXT_PROXY_IN_JS_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/base/memory/unsafe_owning_ptr.h"
#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
class App;
class Runtime;

class ContextProxyInJSImpl : public runtime::ContextProxy {
 public:
  ContextProxyInJSImpl(runtime::ContextProxy::Delegate&,
                       runtime::ContextProxy::Type, base::UnsafeWeakPtr<App>);
  ~ContextProxyInJSImpl() override = default;

  base::expected<Value, JSINativeException> PostMessageFromJS(
      Runtime& rt, const std::string& method_name, const Value* args,
      size_t count);
  base::expected<Value, JSINativeException> DispatchEventFromJS(
      Runtime& rt, const std::string& method_name, const Value* args,
      size_t count);
  base::expected<Value, JSINativeException> AddEventListenerFromJS(
      Runtime& rt, const std::string& method_name, const Value* args,
      size_t count);
  base::expected<Value, JSINativeException> RemoveEventListenerFromJS(
      Runtime& rt, const std::string& method_name, const Value* args,
      size_t count);
  Value GetOnTriggerEvent(Runtime& rt);
  void SetOnTriggerEvent(Runtime& rt, const Value& value);

  fml::RefPtr<runtime::MessageEvent> CreateMessageEvent(Runtime& rt,
                                                        const Value& event);

 protected:
  void ReportError(base::LynxError error) override;

 private:
  base::UnsafeWeakPtr<App> native_app_;
};

class ContextProxyInJS : public HostObject {
 public:
  ContextProxyInJS(runtime::ContextProxy::Type type, base::UnsafeWeakPtr<App>);
  ~ContextProxyInJS() override = default;

  Value get(Runtime*, const PropNameID& name) override;
  void set(Runtime*, const PropNameID& name, const Value& value) override;
  std::vector<PropNameID> getPropertyNames(Runtime& rt) override;
  runtime::ContextProxy::Type GetTargetType() const;

 private:
  ContextProxyInJSImpl* GetContextProxyImpl() const;

  runtime::ContextProxy::Type type_;
  base::UnsafeWeakPtr<App> native_app_;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_EVENT_CONTEXT_PROXY_IN_JS_H_
