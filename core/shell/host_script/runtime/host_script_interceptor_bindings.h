// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_BINDINGS_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_BINDINGS_H_

#include <memory>

#include "core/shell/host_script/runtime/process_runtime.h"

namespace lynx {
namespace runtime {
namespace js {
class NapiEnvironment;
}  // namespace js
}  // namespace runtime
namespace shell {

class HostScriptInterceptor;

// Exposes `interceptor` in one process runtime context. Each binding owns the
// N-API environment of its context and the provider attached on its runner.
class HostScriptInterceptorBindings final
    : public ProcessRuntime::RuntimeBindings {
 public:
  // Returns null for a domain without interceptable events.
  static std::unique_ptr<ProcessRuntime::RuntimeBindings> Create(
      ProcessRuntime::Domain domain);
  ~HostScriptInterceptorBindings() override;

  // Failing to install leaves the context usable without interception.
  bool Attach(base::UnsafeWeakPtr<runtime::js::Runtime> runtime,
              fml::RefPtr<fml::TaskRunner> runner) override;
  void NotifyViewCreated(const ProcessRuntime::ViewCreated& event) override;
  void NotifyViewError(const ProcessRuntime::ViewError& event) override;

 private:
  explicit HostScriptInterceptorBindings(ProcessRuntime::Domain domain);

  ProcessRuntime::Domain domain_;
  std::unique_ptr<runtime::js::NapiEnvironment> napi_;
  std::shared_ptr<HostScriptInterceptor> provider_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_INTERCEPTOR_BINDINGS_H_
