// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/jsi/jsvm/jsvm_exception.h"

#include <ark_runtime/jsvm_types.h>

#include "core/runtime/js/jsi/jsvm/jsvm_dyn_load.h"

namespace lynx {
namespace runtime {
namespace js {
bool JSVMException::ReportExceptionIfNeeded(JSVMRuntime* rt) {
  if (!rt) {
    return false;
  }
  JSVM_Value error = JSVMException::TryCatch(rt->getEnv());
  if (error != nullptr) {
    rt->reportJSIException(JSVMException(*rt, error));
    return false;
  }
  return true;
}

JSVM_Value JSVMException::TryCatch(JSVM_Env env) {
  bool isPending = false;
  DynamicLoader::GetFuncTable()->OH_JSVM_IsExceptionPending(env, &isPending);
  if (isPending) {
    JSVM_Value error;
    DynamicLoader::GetFuncTable()->OH_JSVM_GetAndClearLastException(env,
                                                                    &error);
    return error;
  }
  return nullptr;
}
}  // namespace js
}  // namespace runtime
}  // namespace lynx
