// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/quickjs/quickjs_exception.h"

#include <optional>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/quickjs/quickjs_helper.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {

bool QuickjsException::ReportExceptionIfNeeded(QuickjsRuntime& rt,
                                               LEPUSValue value) {
  auto maybe_error = TryCatch(rt, value);
  if (maybe_error) {
    rt.reportJSIException(std::move(*maybe_error));
    return false;
  }
  return true;
}

std::optional<QuickjsException> QuickjsException::TryCatch(QuickjsRuntime& rt,
                                                           LEPUSValue value) {
  if (UNLIKELY(LEPUS_IsException(value))) {
    auto ctx = rt.getJSContext();
    LEPUSValue exception_val = LEPUS_GetException(ctx);
    HandleScope block_scope(ctx, &exception_val, HANDLE_TYPE_LEPUS_VALUE);
    return QuickjsException(rt, exception_val);
  }
  return std::nullopt;
}

}  // namespace piper
}  // namespace lynx
