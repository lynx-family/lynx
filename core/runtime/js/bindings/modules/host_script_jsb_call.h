// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_JSB_CALL_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_JSB_CALL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/public/pub_value.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx::runtime::js {
// State for one BTS invocation, retained by explicit JS callback arguments.
// Implicit native Promise settlement and Promise-preserving mock returns are
// not supported. Promise wrappers using explicit JS callbacks work normally.
class HostScriptJSBCall final {
 public:
  static std::shared_ptr<HostScriptJSBCall> Create(const std::string& module,
                                                   const std::string& method,
                                                   base::LynxEntityId view);
  // Returns a mock decision owned only by the active invocation.
  // A mock decision is final: the caller skips OnResult and does not attach
  // this context to mocked callbacks. Recording still observes the mock values.
  lepus::Value OnCall(Runtime& rt, const Value* args, size_t count,
                      std::vector<Value>& replacement);
  // Returns true only when a result patch was applied.
  bool OnResult(Runtime& rt, Value& result);
  // Keep the original arguments and skip interception if any nested value
  // cannot be represented in the callback snapshot.
  void OnCallback(int argument_index, std::unique_ptr<pub::Value>& args);

 private:
  lepus::Value BuildEvent() const;
  std::shared_ptr<shell::Interceptor> Provider(shell::InterceptKind kind) const;
  std::weak_ptr<shell::Interceptor> owner_;
  int64_t id_ = 0;
  base::LynxEntityId view_ = 0;
  std::string module_;
  std::string method_;
};
}  // namespace lynx::runtime::js
#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_JSB_CALL_H_
