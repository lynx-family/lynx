// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/performance_controller.h"

#include <string>
#include <utility>

#include "core/public/performance_controller_platform_impl.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/performance_event_sender.h"

namespace lynx {
namespace tasm {
namespace performance {

namespace {
bool IsDevtoolRecordedPerformanceEntry(const std::string& entry) {
  return entry == timing::kEntryTypePipeline;
}
}  // namespace

fml::RefPtr<fml::TaskRunner> PerformanceController::GetTaskRunner() {
  return report::EventTrackerPlatformImpl::GetReportTaskRunner();
}

void PerformanceController::SetPlatformImpl(
    std::unique_ptr<PerformanceControllerPlatformImpl> platform_impl) {
  platform_impl_ = std::move(platform_impl);
}

void PerformanceController::OnPerformanceEvent(
    std::unique_ptr<pub::Value> entry, EventType type) {
  entry->PushInt32ToMap("instanceId", instance_id_);
  auto entry_type = entry->GetValueForKey(kPerformanceEventType);
  if (entry_type && entry_type->IsString() &&
      IsDevtoolRecordedPerformanceEntry(entry_type->str()) &&
      tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    performance_entries_.emplace_back(
        pub::ValueUtils::ConvertValueToLepusValue(*entry));
  }
  if ((type & kEventTypePlatform) && platform_impl_) {
    platform_impl_->OnPerformanceEvent(entry);
  }
  if (js_blocking_monitor_) {
    js_blocking_monitor_->OnPerformanceEvent(entry);
  }
  delegate_->OnPerformanceEvent(std::move(entry), type);
}

std::unique_ptr<pub::Value> PerformanceController::GetAllPerformanceEntries()
    const {
  auto entries = value_factory_->CreateArray();
  if (!tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    return entries;
  }
  for (const auto& entry : performance_entries_) {
    entries->PushValueToArray(pub::ValueImplLepus(entry));
  }
  return entries;
}

void PerformanceController::ResetStateBeforeReload() {
  timing_handler_.ResetTimingBeforeReload();
  performance_entries_.clear();
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
