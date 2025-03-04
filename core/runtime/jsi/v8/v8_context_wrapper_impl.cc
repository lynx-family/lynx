// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_context_wrapper_impl.h"

#include <memory>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/v8/v8_isolate_wrapper_impl.h"
#include "v8.h"

namespace lynx {
namespace piper {

V8ContextWrapperImpl::V8ContextWrapperImpl(std::shared_ptr<VMInstance> vm)
    : V8ContextWrapper(vm) {}

void V8ContextWrapperImpl::Init() {
  LOGI("V8ContextWrapper Init");
  std::shared_ptr<V8IsolateInstance> iso =
      std::static_pointer_cast<V8IsolateInstance>(vm_);

  v8::Isolate* isolate_ = iso.get()->Isolate();

  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  isolate_->SetCaptureStackTraceForUncaughtExceptions(
      true, 100, v8::StackTrace::kOverview);

  auto context = v8::Context::New(isolate_, nullptr);
  v8::Context::Scope context_scope(context);
  ctx_.Reset(isolate_, context);
}

V8ContextWrapperImpl::~V8ContextWrapperImpl() {
  LOGI("~V8ContextWrapper");
  ctx_.Reset();
}

v8::Isolate* V8ContextWrapperImpl::getIsolate() const {
  std::shared_ptr<V8IsolateInstance> v8VM =
      std::static_pointer_cast<V8IsolateInstance>(vm_);
  return v8VM->Isolate();
}

v8::Local<v8::Context> V8ContextWrapperImpl::getContext() const {
  return ctx_.Get(getIsolate());
}

}  // namespace piper
}  // namespace lynx
