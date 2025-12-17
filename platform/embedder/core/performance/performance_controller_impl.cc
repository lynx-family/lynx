// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/performance/performance_controller_impl.h"

#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/performance_controller.h"
#include "platform/embedder/core/performance/lynx_event_reporter.h"

namespace lynx {
namespace embedder {

void PerformanceControllerImpl::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry_map) {
  auto entry = PreparePerformanceData(entry_map);
  SendPerformanceData(std::move(entry));
}

lepus::Value PerformanceControllerImpl::PreparePerformanceData(
    const std::unique_ptr<pub::Value>& entry_map) {
  if (!entry_map || !entry_map->IsMap()) {
    return lepus::Value(lepus::Value::kCreateAsNanTag);
  }

  // 1. parse instanceid
  auto value = entry_map->GetValueForKey("instanceId");
  if (value && value->IsInt32()) {
    auto instance_id = value->Int32();
    auto lepus_entry_map =
        pub::ValueUtils::ConvertValueToLepusValue(*entry_map);

    auto entry =
        LynxEventReporter::GetAllGenericInfosInReportThread(instance_id);
    if (entry) {
      auto generic_infos_lepus_entry_map =
          pub::ValueUtils::ConvertValueToLepusValue(*entry);

      // 2. merge generic infos
      lepus::Value::MergeValue(lepus_entry_map, generic_infos_lepus_entry_map);
    }
    return lepus_entry_map;
  }
  return lepus::Value(lepus::Value::kCreateAsNanTag);
}

void PerformanceControllerImpl::SendPerformanceData(
    const lepus::Value& lepus_entry_map) {
  if (lepus_entry_map.IsNaN()) {
    return;  // not valid data
  }

  // post to ui thread and report
  // NOTE: the lynx view can only be used in ui thread in pc for now
  fml::TaskRunner::RunNowOrPostTask(
      base::UIThread::GetRunner(),
      [weak_renderer = weak_renderer_, lepus_entry_map]() mutable {
        if (auto renderer = weak_renderer.lock()) {
          renderer->renderer->OnPerformanceEvent(lepus_entry_map);
        }
      });
}

}  // namespace embedder
}  // namespace lynx
