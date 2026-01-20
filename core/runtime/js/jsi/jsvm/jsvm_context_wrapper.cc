// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_context_wrapper.h"

#include <memory>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime_wrapper.h"

namespace lynx {
namespace runtime {
namespace js {
JSVMContextWrapper::JSVMContextWrapper(std::shared_ptr<VMInstance> vm)
    : JSIContext(vm) {
  if (!vm) {
    LOGE("vm is nullptr");
    return;
  }
  auto jsvm_instance = std::static_pointer_cast<JSVMRuntimeInstance>(vm);
  env_ = jsvm_instance->Env();
}
}  // namespace js
}  // namespace runtime
}  // namespace lynx
