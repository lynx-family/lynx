// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/shell/host_script/runtime/host_script_interceptor.h"

namespace lynx {
namespace shell {

std::unique_ptr<ProcessRuntime::RuntimeBindings>
HostScriptInterceptorBindings::Create(ProcessRuntime::Domain domain) {
  if (!HostScriptInterceptor::Supports(domain)) return nullptr;
  return std::unique_ptr<ProcessRuntime::RuntimeBindings>(
      new HostScriptInterceptorBindings(domain));
}

HostScriptInterceptorBindings::HostScriptInterceptorBindings(
    ProcessRuntime::Domain domain)
    : domain_(domain) {}

HostScriptInterceptorBindings::~HostScriptInterceptorBindings() {
  // The provider releases its N-API references before the environment.
  if (provider_) provider_->Uninstall();
  provider_.reset();
  if (napi_) napi_->Detach();
}

bool HostScriptInterceptorBindings::Attach(
    base::UnsafeWeakPtr<runtime::js::Runtime> runtime,
    fml::RefPtr<fml::TaskRunner> runner) {
  auto* js_runtime = runtime.Lock();
  if (!js_runtime) return false;
  auto proxy = runtime::js::NapiRuntimeProxy::Create(
      *js_runtime,
      std::make_shared<runtime::js::DelegateObserver>(std::move(runner)));
  if (!proxy) {
    LOGE("Host Script interceptor: no N-API proxy for "
         << HostScriptDomainName(domain_));
    return true;
  }
  proxy->SetJSRuntime(std::move(runtime));
  napi_ = std::make_unique<runtime::js::NapiEnvironment>(
      std::make_unique<runtime::js::NapiEnvironment::Delegate>());
  napi_->SetRuntimeProxy(std::move(proxy));
  napi_->Attach();
  provider_ = HostScriptInterceptor::Install(napi_->proxy()->Env(), domain_);
  if (!provider_) {
    LOGE("Host Script interceptor: install failed for "
         << HostScriptDomainName(domain_));
  }
  return true;
}

}  // namespace shell
}  // namespace lynx
