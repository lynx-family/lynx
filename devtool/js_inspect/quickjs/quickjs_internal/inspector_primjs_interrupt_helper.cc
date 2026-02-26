// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/js_inspect/quickjs/quickjs_internal/inspector_primjs_interrupt_helper.h"

#include <mutex>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"

namespace lynx {
namespace devtool {

int InspectorPrimjsInterruptHelper::InterruptHandler(LEPUSRuntime* rt,
                                                     void* opaque) {
  auto* self = static_cast<InspectorPrimjsInterruptHelper*>(opaque);
  if (self == nullptr) {
    return 0;
  }

  std::vector<base::closure> local_tasks;
  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    local_tasks.swap(self->tasks_);
  }
  for (auto& task : local_tasks) {
    if (task) {
      task();
    }
  }
  return 0;
}

InspectorPrimjsInterruptHelper::InspectorPrimjsInterruptHelper(
    LEPUSRuntime* runtime)
    : runtime_(runtime) {
  if (runtime_ != nullptr) {
    LEPUS_SetInterruptHandler(
        runtime_, &InspectorPrimjsInterruptHelper::InterruptHandler, this);
  }
}

InspectorPrimjsInterruptHelper::~InspectorPrimjsInterruptHelper() {
  if (runtime_ != nullptr) {
    LEPUS_SetInterruptHandler(runtime_, nullptr, nullptr);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.clear();
  }
  runtime_ = nullptr;
}

void InspectorPrimjsInterruptHelper::Request(base::closure closure) {
  if (runtime_ == nullptr) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(closure));
  }
}

}  // namespace devtool
}  // namespace lynx
