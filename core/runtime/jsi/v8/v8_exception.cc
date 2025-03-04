// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_exception.h"

#include <optional>
#include <utility>

#include "base/include/compiler_specific.h"
#include "core/runtime/jsi/v8/v8_helper.h"
#include "v8.h"

namespace lynx {
namespace piper {

bool V8Exception::ReportExceptionIfNeeded(V8Runtime &rt,
                                          v8::TryCatch &try_catch) {
  auto maybe_error = TryCatch(rt, try_catch);
  if (maybe_error.has_value()) {
    rt.reportJSIException(std::move(*maybe_error));
    return false;
  }
  return true;
}

std::optional<JSError> V8Exception::TryCatch(V8Runtime &rt,
                                             v8::TryCatch &try_catch) {
  if (UNLIKELY(try_catch.HasCaught())) {
    V8Exception exception(rt, try_catch.Exception());
    return std::move(exception);
  }
  return std::nullopt;
}

}  // namespace piper
}  // namespace lynx
