// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_runtime_proxy_jsc.h"

#include <JavaScriptCore/JavaScript.h>

#include <utility>

#include "core/runtime/bindings/napi/shim/shim_napi_env_jsc.h"

namespace lynx {
namespace piper {

// static
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyJSC::Create(
    std::shared_ptr<JSCContextWrapper> context,
    runtime::TemplateDelegate *delegate) {
  return std::unique_ptr<NapiRuntimeProxy>(
      new NapiRuntimeProxyJSC(std::move(context), delegate));
}

NapiRuntimeProxyJSC::NapiRuntimeProxyJSC(
    std::shared_ptr<JSCContextWrapper> context,
    runtime::TemplateDelegate *delegate)
    : NapiRuntimeProxy(delegate), context_(context) {}

void NapiRuntimeProxyJSC::Attach() {
  auto ctx = context_.lock();
  if (ctx) {
    napi_attach_jsc(env_, ctx->getContext());
  }
}

void NapiRuntimeProxyJSC::Detach() {
  NapiRuntimeProxy::Detach();
  napi_detach_jsc(env_);
}

}  // namespace piper
}  // namespace lynx
