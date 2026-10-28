// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/table.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/common/napi/napi_runtime_proxy.h"
#include "core/shell/host_script/runtime/host_script_interceptor.h"
#include "core/shell/host_script/runtime/host_script_resources.h"

namespace lynx {
namespace shell {
namespace {
BASE_STATIC_STRING_DECL(kViewId, "viewId");
BASE_STATIC_STRING_DECL(kUrl, "url");
}  // namespace

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
  if (resources_) resources_->Detach();
  resources_.reset();
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
  resources_ = HostScriptResources::Install(napi_->proxy()->Env(),
                                            std::move(runner), resource_root_);
  provider_ = HostScriptInterceptor::Install(napi_->proxy()->Env(), domain_);
  if (!provider_) {
    LOGE("Host Script interceptor: install failed for "
         << HostScriptDomainName(domain_));
  }
  return true;
}

void HostScriptInterceptorBindings::NotifyViewCreated(
    const ProcessRuntime::ViewCreated& event) {
  if (!provider_) return;
  lepus::Value snapshot(lepus::Dictionary::Create());
  snapshot.SetProperty(kViewId, lepus::Value(event.view_id));
  snapshot.SetProperty(kUrl, lepus::Value(event.url));
  provider_->Dispatch(InterceptKind::kCreated, snapshot);
}

}  // namespace shell
}  // namespace lynx
