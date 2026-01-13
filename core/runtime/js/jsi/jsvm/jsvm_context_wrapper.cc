// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_context_wrapper.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime_wrapper.h"
#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace runtime {
namespace js {
JSVMContextWrapper::JSVMContextWrapper(std::shared_ptr<VMInstance> vm)
    : JSIContext(vm) {
  if (!vm) {
    LOGE("JSVMContextWrapper constructor vm is nullptr");
    return;
  }
}

JSVMContextWrapper::~JSVMContextWrapper() {
  JSVM_CALL(nullptr, OH_JSVM_DestroyEnv, env_);
}

void JSVMContextWrapper::Init() {
  if (!vm_) {
    LOGE("JSVMContextWrapper Init VMInstance is nullptr");
    return;
  }

  auto jsvm_instance = std::static_pointer_cast<JSVMRuntimeInstance>(vm_);
  JSVM_VM vm = jsvm_instance->GetVM();
  if (!vm) {
    LOGE("JSVMContextWrapper Init JSVM_VM is nullptr");
    return;
  }

  JSVM_CALL_NO_ENV(OH_JSVM_CreateEnv, vm, 0, nullptr, &env_);
}

JSVM_VM JSVMContextWrapper::getJSVM() const {
  if (!vm_) {
    return nullptr;
  }
  auto jsvm_instance = std::static_pointer_cast<JSVMRuntimeInstance>(vm_);
  return jsvm_instance->GetVM();
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
