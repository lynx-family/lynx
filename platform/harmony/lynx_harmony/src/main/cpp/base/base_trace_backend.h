// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_BASE_BASE_TRACE_BACKEND_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_BASE_BASE_TRACE_BACKEND_H_

#include "base/include/base_trace/trace_event_utils.h"

namespace lynx {
namespace base {
void BaseTraceEvent(const char* category, const char* name,
                    trace::BaseTraceEventType phase);
}  // namespace base
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_BASE_BASE_TRACE_BACKEND_H_
