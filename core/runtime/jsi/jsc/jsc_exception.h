// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSC_JSC_EXCEPTION_H_
#define CORE_RUNTIME_JSI_JSC_JSC_EXCEPTION_H_

#include <JavaScriptCore/JavaScript.h>

#include <string>

#include "core/runtime/jsi/jsc/jsc_helper.h"
#include "core/runtime/jsi/jsc/jsc_runtime.h"

namespace lynx {
namespace piper {

class JSCException : public JSError {
 public:
  JSCException(JSCRuntime& rt, JSValueRef value)
      : JSError(rt, detail::JSCHelper::createValue(rt, value)) {}

  static bool ReportExceptionIfNeeded(JSGlobalContextRef, JSCRuntime&,
                                      JSValueRef);
  static bool ReportExceptionIfNeeded(JSGlobalContextRef, JSCRuntime&,
                                      JSValueRef, JSValueRef);
  static std::optional<JSCException> TryCatch(JSGlobalContextRef, JSCRuntime&,
                                              JSValueRef, JSValueRef);
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_EXCEPTION_H_
