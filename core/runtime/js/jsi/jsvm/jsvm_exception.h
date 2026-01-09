// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_EXCEPTION_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_EXCEPTION_H_

#include <ark_runtime/jsvm_types.h>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/jsvm/jsvm_helper.h"
#include "core/runtime/js/jsi/jsvm/jsvm_runtime.h"

namespace lynx {
namespace piper {
class JSVMException : JSError {
 public:
  JSVMException(JSVMRuntime& rt, JSVM_Value value)
      : JSError(rt, detail::JSVMHelper::createValue(value, &rt)) {}
  static bool ReportExceptionIfNeeded(JSVMRuntime* rt);
  static JSVM_Value TryCatch(JSVM_Env env);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_EXCEPTION_H_
