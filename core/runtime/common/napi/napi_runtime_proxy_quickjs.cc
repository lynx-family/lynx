// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"

#include <utility>

#include "core/runtime/common/napi/shim/shim_napi_env_quickjs.h"

namespace lynx {
namespace runtime {
namespace js {
// static
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyQuickjs::Create(
    LEPUSContext* context,
    std::shared_ptr<DelegateObserver> delegate_observer) {
  return std::unique_ptr<NapiRuntimeProxy>(
      new NapiRuntimeProxyQuickjs(context, std::move(delegate_observer)));
}

NapiRuntimeProxyQuickjs::NapiRuntimeProxyQuickjs(
    LEPUSContext* context, std::shared_ptr<DelegateObserver> delegate_observer)
    : NapiRuntimeProxy(std::move(delegate_observer)), context_(context) {}

void NapiRuntimeProxyQuickjs::Attach() { napi_attach_quickjs(env_, context_); }

void NapiRuntimeProxyQuickjs::Detach() {
  NapiRuntimeProxy::Detach();
  napi_detach_quickjs(env_);
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
