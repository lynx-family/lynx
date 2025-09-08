// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/base_trace/trace_event_utils.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace trace {
namespace {
static trace_backend_ptr trace_backend = nullptr;
}
void TraceEventBegin(const char* category, const char* name) {
  if (trace_backend == nullptr) {
    return;
  }
  trace_backend(category, name, BaseTraceEventType::TYPE_SLICE_BEGIN);
}
void TraceEventEnd(const char* category, const char* name) {
  if (trace_backend == nullptr) {
    return;
  }
  trace_backend(category, name, BaseTraceEventType::TYPE_SLICE_END);
}

void SetTraceBackend(trace_backend_ptr backend) {
  if (backend != nullptr) {
    trace_backend = backend;
    LOGI("base trace init success, backend: "
         << reinterpret_cast<uint64_t>(backend));
    return;
  }
  LOGE("base trace init failed.");
}
}  // namespace trace
}  // namespace base
}  // namespace lynx
