// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JSB_CALL_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JSB_CALL_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/public/pub_value.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx::runtime::js {
// Per-call BTS context for explicit callbacks; native Promises are unsupported.
class HostScriptJSBCall final {
 public:
  static std::shared_ptr<HostScriptJSBCall> Create(const std::string& module,
                                                   const std::string& method,
                                                   base::LynxEntityId view);
  // Mock decisions are invocation-local and final: skip OnResult and callback
  // interception; recording still observes the mock values.
  lepus::Value OnCall(Runtime& rt, const Value* args, size_t count,
                      std::vector<Value>& replacement);
  // Returns true only when a result patch was applied.
  bool OnResult(Runtime& rt, Value& result);
  // Skip callbacks with any value that cannot be copied without loss.
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
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JSB_CALL_H_
