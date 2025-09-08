// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/base/base_trace_backend.h"

#include "base/trace/native/trace_event.h"

namespace lynx {
namespace base {
void BaseTraceEvent(const char* category, const char* name,
                    trace::BaseTraceEventType phase) {
  switch (phase) {
    case trace::BaseTraceEventType::TYPE_SLICE_BEGIN:
      TRACE_EVENT_BEGIN(category, name);
      break;
    case trace::BaseTraceEventType::TYPE_SLICE_END:
      TRACE_EVENT_END(category);
      break;
    case trace::BaseTraceEventType::TYPE_INSTANT:
      TRACE_EVENT_INSTANT(category, name);
      break;
    default:
      break;
  }
}
}  // namespace base
}  // namespace lynx
