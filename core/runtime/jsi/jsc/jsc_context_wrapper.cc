// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/jsc/jsc_context_wrapper.h"

#include <JavaScriptCore/JavaScript.h>

#include <memory>
#include <unordered_map>

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/jsi/jsc/jsc_context_group_wrapper.h"
#include "core/runtime/jsi/jsc/jsc_helper.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

JSCContextWrapper::JSCContextWrapper(std::shared_ptr<VMInstance> vm)
    : JSIContext(vm), ctx_(nullptr), ctx_invalid_(false), objectCounter_(0) {}

void JSCContextWrapper::init() {
  std::shared_ptr<JSCContextGroupWrapper> context_group_wrapper =
      std::static_pointer_cast<JSCContextGroupWrapper>(vm_);
  JSContextGroupRef jsc_context_group =
      context_group_wrapper->GetContextGroup();
  if (!jsc_context_group) {
    LOGE("JSCContextWrapper::init, jsc_context_group is null");
    ctx_invalid_ = true;
    return;
  }
  ctx_ = JSGlobalContextCreateInGroup(jsc_context_group, nullptr);
  if (!ctx_) {
    LOGE("JSCContextWrapper::init, JSGlobalContextCreateInGroup failed");
    ctx_invalid_ = true;
    return;
  }

  // register webassembly here, on ctx.global
  RegisterWasmFunc()(ctx_, &ctx_invalid_);
  auto name = JSStringCreateWithUTF8CString("Lynx");
  JSGlobalContextSetName(ctx_, name);
  JSStringRelease(name);
}

JSCContextWrapper::~JSCContextWrapper() {
  if (!ctx_) {
    LOGE("JSCContextWrapper::~JSCContextWrapper, ctx is invalid");
    return;
  }
  ctx_invalid_ = true;
  JSGlobalContextRelease(ctx_);

#if defined(DEBUG) || (defined(LYNX_UNIT_TEST) && LYNX_UNIT_TEST)
  if (objectCounter_ != 0) {
    LOGF("Error: " << __FILE__ << ":" << __LINE__ << ":"
                   << "JSCRuntime destroyed with a dangling API object, count:"
                   << objectCounter_);
  }

#endif

  LOGI("~JSCContextWrapper " << this);
}

std::atomic<intptr_t>& JSCContextWrapper::objectCounter() const {
  return objectCounter_;
}

JSGlobalContextRef JSCContextWrapper::getContext() const { return ctx_; }

// static
RegisterWasmFuncType JSCContextWrapper::register_wasm_func_ = [](void*, void*) {
};
// static
RegisterWasmFuncType& JSCContextWrapper::RegisterWasmFunc() {
  static RegisterWasmFuncType RegisterWebAssembly = register_wasm_func_;
  return RegisterWebAssembly;
}

}  // namespace piper
}  // namespace lynx
