// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_OBSERVER_H_
#define PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_OBSERVER_H_

#include <string>

#include "platform/embedder/public/lynx_value.h"

namespace lynx {
namespace embedder {

/**
 * @brief The interface class used to receive reported events
 */
class LynxEventReporterObserver {
 public:
  LynxEventReporterObserver() = default;
  virtual ~LynxEventReporterObserver() = default;

  /**
   * @brief Interface used to receive reported events
   * @param instance_id The optional lynx instance ID associated with the event
   * @param event_name Name of reported event
   * @param props Content of reported event in key-value format
   */
  virtual void OnReportEvent(int32_t instance_id, const std::string& event_name,
                             const lynx::pub::LynxValue& props) = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_PERFORMANCE_LYNX_EVENT_REPORTER_OBSERVER_H_
