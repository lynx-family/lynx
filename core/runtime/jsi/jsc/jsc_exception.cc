// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsc/jsc_exception.h"

#include <cstring>
#include <utility>

#include "base/include/compiler_specific.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
using detail::JSCHelper;
bool JSCException::ReportExceptionIfNeeded(JSGlobalContextRef ctx,
                                           JSCRuntime& rt, JSValueRef exc) {
  return ReportExceptionIfNeeded(ctx, rt, nullptr, exc);
}

bool JSCException::ReportExceptionIfNeeded(JSGlobalContextRef ctx,
                                           JSCRuntime& rt, JSValueRef res,
                                           JSValueRef exc) {
  auto maybe_error = TryCatch(ctx, rt, res, exc);
  if (maybe_error.has_value()) {
    rt.reportJSIException(std::move(*maybe_error));
    return false;
  }
  return true;
}

std::optional<JSCException> JSCException::TryCatch(JSGlobalContextRef ctx,
                                                   JSCRuntime& rt,
                                                   JSValueRef res,
                                                   JSValueRef exc) {
  if (!res && UNLIKELY((bool)exc)) {
    JSCException error(rt, exc);
    return error;
  }
  return std::nullopt;
}

}  // namespace piper
}  // namespace lynx
