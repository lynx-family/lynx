// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/shell/host_script/runtime/interceptor.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx::shell {

// The runtime owner installs this into an existing environment, on its owning
// thread. This component never creates a VM, thread or cross-thread JS handle.
class HostScriptInterceptor final
    : public Interceptor,
      public std::enable_shared_from_this<HostScriptInterceptor> {
 public:
  using Guard = std::function<bool()>;
  using Reporter = std::function<void(const std::string&)>;
  static std::shared_ptr<HostScriptInterceptor> Install(
      napi_env env, Domain domain, Reporter reporter = nullptr,
      Guard guard = {});
  bool IsAttached() const override {
    return Interceptor::IsAttached() && (!guard_ || guard_());
  }
  void Uninstall() override;
  bool HasHandlers(InterceptKind kind) const override;
  InterceptResult Dispatch(InterceptKind kind,
                           const lepus::Value& event) override;
  void ReportError(const std::string& message) override;

 private:
  struct CallbackData {
    std::weak_ptr<HostScriptInterceptor> provider;
    uint64_t id;
  };
  struct Entry {
    uint64_t id;
    InterceptKind kind;
    Napi::FunctionReference function;
  };
  HostScriptInterceptor(napi_env env, Domain domain, Reporter reporter)
      : env_(env), domain_(domain), reporter_(std::move(reporter)) {}
  Napi::Function Bind(Napi::Env env, const char* name,
                      Napi::Value (*callback)(const Napi::CallbackInfo&),
                      uint64_t id = 0);
  static Napi::Value On(const Napi::CallbackInfo& info);
  static Napi::Value Dispose(const Napi::CallbackInfo& info);
  Napi::Value Add(const Napi::CallbackInfo& info);
  void Remove(uint64_t id);
  napi_env env_;
  Domain domain_;
  Reporter reporter_;
  Guard guard_;
  uint64_t next_id_ = 1;
  bool active_ = true;
  bool dispatching_ = false;
  Napi::ObjectReference api_;
  std::vector<std::shared_ptr<Entry>> entries_;
};
}  // namespace lynx::shell
#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_H_
