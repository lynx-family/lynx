// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/shell/host_script/runtime/host_script_interceptor.h"
#include "core/shell/host_script/runtime/host_script_lynx_views.h"
#include "core/shell/host_script/runtime/host_script_resources.h"

namespace lynx {
namespace shell {

std::unique_ptr<ProcessRuntime::RuntimeBindings>
HostScriptInterceptorBindings::Create(ProcessRuntime::Domain domain) {
  if (!HostScriptInterceptor::Supports(domain)) return nullptr;
  return std::unique_ptr<ProcessRuntime::RuntimeBindings>(
      new HostScriptInterceptorBindings(domain));
}

ProcessRuntime::BindingFactory HostScriptInterceptorBindings::CreateFactory(
    std::string resource_root) {
  return [root = std::move(resource_root)](ProcessRuntime::Domain domain) {
    return std::unique_ptr<ProcessRuntime::RuntimeBindings>(
        new HostScriptInterceptorBindings(domain, root));
  };
}

HostScriptInterceptorBindings::HostScriptInterceptorBindings(
    ProcessRuntime::Domain domain, std::string resource_root)
    : domain_(domain), resource_root_(std::move(resource_root)) {}

HostScriptInterceptorBindings::~HostScriptInterceptorBindings() {
  // Settle pending reads and release the provider and proxies before detaching
  // the N-API environment.
  if (resources_) resources_->Detach();
  resources_.reset();
  if (provider_) provider_->Uninstall();
  provider_.reset();
  if (views_) views_->Detach();
  views_.reset();
  if (napi_) napi_->Detach();
}

bool HostScriptInterceptorBindings::Attach(
    base::UnsafeWeakPtr<runtime::js::Runtime> runtime,
    fml::RefPtr<fml::TaskRunner> runner, ProcessRuntime::Guard guard) {
  auto* js_runtime = runtime.Lock();
  if (!js_runtime) return false;
  auto proxy = runtime::js::NapiRuntimeProxy::Create(
      *js_runtime, std::make_shared<runtime::js::DelegateObserver>(runner));
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
  auto env = napi_->proxy()->Env();
  views_ = HostScriptLynxViews::Create(env, runner, guard);
  resources_ =
      HostScriptResources::Install(env, std::move(runner), resource_root_);
  provider_ = HostScriptInterceptor::Install(env, domain_, nullptr,
                                             std::move(guard), views_);
  if (!provider_) {
    LOGE("Host Script interceptor: install failed for "
         << HostScriptDomainName(domain_));
  }
  return true;
}

}  // namespace shell
}  // namespace lynx
