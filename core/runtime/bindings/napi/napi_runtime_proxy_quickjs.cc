// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"

#include "core/runtime/bindings/napi/shim/shim_napi_env_quickjs.h"

namespace lynx {
namespace piper {

// static
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyQuickjs::Create(
    LEPUSContext* context, runtime::TemplateDelegate* delegate) {
  return std::unique_ptr<NapiRuntimeProxy>(
      new NapiRuntimeProxyQuickjs(context, delegate));
}

NapiRuntimeProxyQuickjs::NapiRuntimeProxyQuickjs(
    LEPUSContext* context, runtime::TemplateDelegate* delegate)
    : NapiRuntimeProxy(delegate), context_(context) {}

void NapiRuntimeProxyQuickjs::Attach() { napi_attach_quickjs(env_, context_); }

void NapiRuntimeProxyQuickjs::Detach() {
  NapiRuntimeProxy::Detach();
  napi_detach_quickjs(env_);
}

}  // namespace piper
}  // namespace lynx
