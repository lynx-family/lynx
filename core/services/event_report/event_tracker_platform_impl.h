// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_PLATFORM_IMPL_H_
#define CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_PLATFORM_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/thread.h"
#include "core/services/event_report/event_tracker.h"

// Name of report thread.
constexpr const static char* kLynxReportThread = "lynx_report_thread";

namespace lynx {
namespace tasm {
namespace report {

class EventTrackerPlatformImpl {
 public:
  /// Send single event of template instance to platform.
  /// @param instance_id The unique id of template instance.
  /// @param event The event.
  static void OnEvent(int32_t instance_id, MoveOnlyEvent&& event);

  /// Send events of template instance to platform.
  /// @param instance_id The unique id of template instance.
  /// @param stack The stack of events.
  static void OnEvents(int32_t instance_id, std::vector<MoveOnlyEvent> stack);

  /// Update the generic info of template instance to platform.
  /// @param instance_id The unique id of template instance.
  /// @param generic_info The generic info of template instance.
  static void UpdateGenericInfo(
      int32_t instance_id,
      std::unordered_map<std::string, std::string> generic_info);

  /// Update the generic info of template instance to platform.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value string value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                const std::string& value);

  /// Update the generic info of template instance.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value int64_t value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                int64_t value);

  /// Update the generic info of template instance to platform.
  /// @param instance_id The unique id of template instance.
  /// @param key key of the generic info
  /// @param value float value of the generic info
  static void UpdateGenericInfo(int32_t instance_id, const std::string& key,
                                const float value);

  /// Clear the cache, which includes extra parameters and generic info directly
  /// mapped by instance id.
  /// @param instance_id The unique id of template instance.
  static void ClearCache(int32_t instance_id);
  // Get Task Runner of report thread.
  static fml::RefPtr<fml::TaskRunner> GetReportTaskRunner() {
    static base::NoDestructor<fml::Thread> event_report_thread_t_(
        fml::Thread::ThreadConfig(kLynxReportThread,
                                  fml::Thread::ThreadPriority::NORMAL));
    return event_report_thread_t_->GetTaskRunner();
  }
};

}  // namespace report
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_EVENT_REPORT_EVENT_TRACKER_PLATFORM_IMPL_H_
