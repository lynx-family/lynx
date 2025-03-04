// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_runtime_proxy_v8.h"

#include <utility>

#include "core/runtime/bindings/napi/shim/shim_napi_env_v8.h"
#include "core/runtime/jsi/v8/v8_context_wrapper.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "core/runtime/jsi/v8/v8_runtime.h"
#include "v8.h"

namespace lynx {
namespace piper {

// static
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyV8::Create(
    std::shared_ptr<V8ContextWrapper> context,
    runtime::TemplateDelegate *delegate) {
  return std::unique_ptr<NapiRuntimeProxy>(
      new NapiRuntimeProxyV8(std::move(context), delegate));
}

NapiRuntimeProxyV8::NapiRuntimeProxyV8(
    std::shared_ptr<V8ContextWrapper> context,
    runtime::TemplateDelegate *delegate)
    : NapiRuntimeProxy(delegate), context_(context) {}

void NapiRuntimeProxyV8::Attach() {
  auto ctx = context_.lock();
  if (ctx) {
    ctx->getIsolate()->Enter();
    v8::HandleScope handle_scope(ctx->getIsolate());
    napi_attach_v8(env_, ctx->getContext());
  }
}

void NapiRuntimeProxyV8::Detach() {
  NapiRuntimeProxy::Detach();
  auto ctx = context_.lock();
  if (ctx) {
    ctx->getIsolate()->Exit();
    napi_detach_v8(env_);
  }
}

std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxyV8FactoryImpl::Create(
    std::shared_ptr<Runtime> runtime, runtime::TemplateDelegate *delegate) {
  auto v8_runtime = std::static_pointer_cast<V8Runtime>(runtime);
  auto context = v8_runtime->getSharedContext();
  auto v8_context = std::static_pointer_cast<V8ContextWrapper>(context);
  return NapiRuntimeProxyV8::Create(v8_context, delegate);
}

}  // namespace piper
}  // namespace lynx
