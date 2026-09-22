// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_NATIVE_MODULE_INTERCEPTION_H_
#define CORE_RUNTIME_COMMON_NATIVE_MODULE_INTERCEPTION_H_

#include <memory>
#include <string>

#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx::runtime {
// Immutable identity shared by interception and observation of one invocation.
struct NativeModuleInvocationInfo {
  NativeModuleInvocationInfo(std::string module, std::string method,
                             base::LynxEntityId view);

  const int64_t id;
  const std::string module;
  const std::string method;
  const base::LynxEntityId view;
};

// Native snapshots and a weak environment identity; no script handles.
class NativeModuleInterception {
 public:
  static std::shared_ptr<NativeModuleInterception> Create(
      const std::shared_ptr<shell::Interceptor>& provider,
      const std::string& module, const std::string& method,
      base::LynxEntityId view);
  static std::shared_ptr<NativeModuleInterception> Create(
      const std::shared_ptr<shell::Interceptor>& provider,
      std::shared_ptr<const NativeModuleInvocationInfo> info);
  bool IsAlive() const;
  bool HasHandlers(shell::InterceptKind kind) const {
    return Provider(kind) != nullptr;
  }
  shell::InterceptResult Call(const lepus::Value& args,
                              const lepus::Value& callbacks,
                              const lepus::Value& opaque);
  lepus::Value Result(const lepus::Value& value);
  lepus::Value Callback(int argument_index, const lepus::Value& args);

 private:
  lepus::Value BuildEvent() const;
  std::shared_ptr<shell::Interceptor> Provider(shell::InterceptKind kind) const;
  std::shared_ptr<const NativeModuleInvocationInfo> info_;
  std::weak_ptr<shell::Interceptor> owner_;
};
}  // namespace lynx::runtime
#endif  // CORE_RUNTIME_COMMON_NATIVE_MODULE_INTERCEPTION_H_
