// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_EXCEPTION_H_
#define CORE_RUNTIME_JSI_V8_V8_EXCEPTION_H_

#include <stdexcept>
#include <string>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/v8/v8_runtime.h"
#include "v8.h"

namespace lynx {
namespace piper {

class V8Exception : public JSError {
 public:
  explicit V8Exception(V8Runtime &rt, v8::Local<v8::Value> value)
      : JSError(rt, detail::V8Helper::createValue(value, rt.getContext())) {}

  static bool ReportExceptionIfNeeded(V8Runtime &rt, v8::TryCatch &try_catch);
  static std::optional<JSError> TryCatch(V8Runtime &rt,
                                         v8::TryCatch &try_catch);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_EXCEPTION_H_
