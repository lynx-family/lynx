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
    : JSIContext(vm), ctx_invalid_(false), objectCounter_(0) {}

void JSCContextWrapper::init() {
  std::shared_ptr<JSCContextGroupWrapper> context_group_wrapper =
      std::static_pointer_cast<JSCContextGroupWrapper>(vm_);
  JSContextGroupRef jsc_context_group =
      context_group_wrapper->GetContextGroup();
  ctx_ = JSGlobalContextCreateInGroup(jsc_context_group, nullptr);

  // register webassembly here, on ctx.global
  RegisterWasmFunc()(ctx_, &ctx_invalid_);
  auto name = JSStringCreateWithUTF8CString("Lynx");
  JSGlobalContextSetName(ctx_, name);
  JSStringRelease(name);
}

JSCContextWrapper::~JSCContextWrapper() {
  // remove all global object
  JSObjectRef global = JSContextGetGlobalObject(ctx_);
  JSPropertyNameArrayRef names = JSObjectCopyPropertyNames(ctx_, global);
  size_t count = JSPropertyNameArrayGetCount(names);
  for (size_t i = 0; i < count; i++) {
    JSStringRef name = JSPropertyNameArrayGetNameAtIndex(names, i);
    JSObjectDeleteProperty(ctx_, global, name, nullptr);
  }
  ctx_invalid_ = true;
  JSGlobalContextRelease(ctx_);

#ifdef DEBUG
  // assert(objectCounter_ == 0 &&
  //       "JSCRuntime destroyed with a dangling API object");
  if (objectCounter_ != 0) {
    LOGE("Error: " << __FILE__ << ":" << __LINE__ << ":"
                   << "JSCRuntime destroyed with a dangling API object");
  }

#endif

  LOGI("~JSCContextWrapper " << this);
}

const std::atomic<bool>& JSCContextWrapper::contextInvalid() const {
  return ctx_invalid_;
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
