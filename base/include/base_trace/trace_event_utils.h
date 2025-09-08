// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BASE_TRACE_TRACE_EVENT_UTILS_H_
#define BASE_INCLUDE_BASE_TRACE_TRACE_EVENT_UTILS_H_

#include <cstdint>
#include <functional>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
namespace trace {
enum class BaseTraceEventType : int32_t {
  TYPE_UNSPECIFIED = 0,
  TYPE_SLICE_BEGIN = 1,
  TYPE_SLICE_END = 2,
  TYPE_INSTANT = 3,
  TYPE_COUNTER = 4,
};
using trace_backend_ptr = void (*)(const char* category, const char* name,
                                   BaseTraceEventType phase);
BASE_EXPORT void SetTraceBackend(trace_backend_ptr backend);
void TraceEventBegin(const char* category, const char* name);
void TraceEventEnd(const char* category, const char* name);
}  // namespace trace
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_BASE_TRACE_TRACE_EVENT_UTILS_H_
