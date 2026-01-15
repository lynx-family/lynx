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
#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {

class App;
class Runtime;

class ContextProxyInJS : public HostObject, public runtime::ContextProxy {
 public:
  ContextProxyInJS(runtime::ContextProxy::Delegate&,
                   runtime::ContextProxy::Type, std::weak_ptr<App>);
  virtual ~ContextProxyInJS() override = default;

  fml::RefPtr<runtime::MessageEvent> CreateMessageEvent(
      Runtime& rt, std::shared_ptr<App> native_app, const piper::Value& event);

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

  // clear js object
  void Destroy();

 protected:
  std::weak_ptr<App> native_app_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_EVENT_CONTEXT_PROXY_IN_JS_H_
