// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/performance/performance_controller_impl.h"

#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/performance_controller.h"
#include "platform/embedder/core/performance/lynx_event_reporter.h"
#include "platform/embedder/public/capi/lynx_event_reporter_service_capi.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"

namespace lynx {
namespace embedder {

void PerformanceControllerImpl::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry_map) {
  auto entry = PreparePerformanceData(entry_map);
  lynx_service_center_t* service_center = lynx_service_get_center_instance();
  bool has_reported = false;
  if (service_center) {
    lynx_event_reporter_service_t* reporter_service =
        reinterpret_cast<lynx_event_reporter_service_t*>(
            lynx_service_get_service(service_center,
                                     kServiceTypeEventReporter));
    if (reporter_service) {
      has_reported = true;
      lynx_event_reporter_service_on_performance_event(reporter_service,
                                                       entry.value());
    }
  }
  if (!has_reported) {
    SendPerformanceData(std::move(entry));
  }
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
