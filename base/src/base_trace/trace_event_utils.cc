// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/base_trace/trace_event_utils.h"

#include <atomic>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace trace {
namespace {
std::atomic<trace_backend_ptr> trace_backend{nullptr};
}  // namespace

void TraceEventBegin(const char* category, const char* name) {
  trace_backend_ptr backend = trace_backend.load(std::memory_order_acquire);
  if (backend == nullptr) {
    return;
  }
  backend(category, name, BaseTraceEventType::TYPE_SLICE_BEGIN);
}
void TraceEventEnd(const char* category, const char* name) {
  trace_backend_ptr backend = trace_backend.load(std::memory_order_acquire);
  if (backend == nullptr) {
    return;
  }
  backend(category, name, BaseTraceEventType::TYPE_SLICE_END);
}

void SetTraceBackend(trace_backend_ptr backend) {
  if (backend != nullptr) {
    trace_backend.store(backend, std::memory_order_release);
    LOGI("base trace init success, backend: "
         << reinterpret_cast<uint64_t>(backend));
    return;
  }
  LOGE("base trace init failed.");
}
}  // namespace trace
}  // namespace base
}  // namespace lynx
