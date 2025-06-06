// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/performance_controller.h"

#include <utility>

#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/performance_entry_sender.h"

namespace lynx {
namespace tasm {
namespace performance {

fml::RefPtr<fml::TaskRunner> PerformanceController::GetTaskRunner() {
  return report::EventTrackerPlatformImpl::GetReportTaskRunner();
}

void PerformanceController::SetPlatformImpl(
    std::unique_ptr<PerformanceControllerPlatform<PerformanceController>>
        platform_impl) {
  platform_impl_ = std::move(platform_impl);
}

void PerformanceController::OnPerformanceEvent(
    std::unique_ptr<pub::Value> entry, PerformanceObserverEnv env) {
  entry->PushInt32ToMap("instanceId", instance_id_);
  if ((env & kPerformanceObserverOfPlatform) && platform_impl_) {
    platform_impl_->OnPerformanceEvent(entry);
  }
  delegate_->OnPerformanceEvent(std::move(entry), env);
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
