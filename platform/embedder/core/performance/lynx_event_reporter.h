// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_H_
#define PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "core/public/pub_value.h"
#include "core/services/event_report/event_tracker.h"
#include "platform/embedder/core/performance/lynx_event_reporter_observer.h"

namespace lynx {
namespace embedder {

class LynxEventReporter {
 public:
  /**
   * @brief
   * @param type The specific host platform type.
   */
  static void OnEvent(int32_t instance_id, tasm::report::MoveOnlyEvent&& event);

  static void UpdateGenericInfo(const std::string& key,
                                const std::string& value, int32_t instance_id);

  static void UpdateGenericInfo(
      std::unordered_map<std::string, std::string>&& infos,
      int32_t instance_id);

  static void UpdateGenericInfo(const std::string& key, float value,
                                int32_t instance_id);

  static void UpdateGenericInfo(std::unordered_map<std::string, float>&& infos,
                                int32_t instance_id);

  static void UpdateGenericInfo(const std::string& key, int64_t value,
                                int32_t instance_id);

  static void RemoveGenericInfo(const std::string& key, int32_t instance_id);

  static void GetAllGenericInfosInReportThread(
      int32_t instance_id,
      base::MoveOnlyClosure<void, std::unique_ptr<const pub::Value>>
          on_get_generic_infos_cb);

  // make sure that this method is called in report thread
  static std::unique_ptr<const pub::Value> GetAllGenericInfosInReportThread(
      int32_t instance_id);

  static void ClearCache(int32_t instance_id);

  static void ClearAll();

  /**
   * @brief
   * @param type The specific host platform type.
   */
  static uint32_t AddEventReporterObserver(
      std::weak_ptr<LynxEventReporterObserver> observer);
  static void RemoveEventReporterObserver(uint32_t token);

 private:
  static void InitLynxReporterServiceObserverIfNeeded();
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_H_
