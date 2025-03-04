// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_EXCEPTION_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_EXCEPTION_H_

#include <string>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"

namespace lynx {
namespace piper {
class QuickjsException : public JSError {
 public:
  QuickjsException(QuickjsRuntime& rt, LEPUSValue value)
      : JSError(rt, detail::QuickjsHelper::createValue(value, &rt)) {}

  static bool ReportExceptionIfNeeded(QuickjsRuntime& rt, LEPUSValue value);

  static std::optional<QuickjsException> TryCatch(QuickjsRuntime& rt,
                                                  LEPUSValue value);
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_EXCEPTION_H_
