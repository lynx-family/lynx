// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#define CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/closure.h"
#include "core/public/pipeline_option.h"
#include "core/public/timing_key.h"

namespace lynx {
namespace shell {

/**
 * @brief Event struct containing different types of property
 */
struct ReportEvent {
  std::string event_name;
  std::unordered_map<std::string, std::string> string_props;
  std::unordered_map<std::string, int> int_props;
  std::unordered_map<std::string, double> double_props;
};

class PerfControllerProxy {
 public:
  virtual ~PerfControllerProxy() = default;

  /**
   * @brief Support setting HostPlatformType field (such as 'windowsClay'),
   * which will affect the platform type tag in HostPlatformTiming data in
   * PipelineEntry
   * @param type The specific host platform type.
   */
  virtual void SetHostPlatformType(const std::string& type) = 0;

  /**
   * @brief Marks a timing event with the specified key and pipeline ID.
   * @param timing_key The key that uniquely identifies the timing event.
   * @param pipeline_id The optional pipeline ID associated with the timing
   * event.
   */
  virtual void MarkTiming(tasm::TimingKey timing_key,
                          const tasm::PipelineID& pipeline_id) = 0;
  virtual void SetTiming(tasm::TimingKey timing_key, uint64_t timestamp_us,
                         const tasm::PipelineID& pipeline_id) = 0;

  /**
   * @brief Set a host-platform-timing event with the specified key and
   * pipeline ID.
   * @param timing_key The key that uniquely identifies the timing event.
   * @param timestamp_us Timestamp of the timing event occurrence
   * @param pipeline_id The optional pipeline ID associated with the timing
   * event.
   */
  virtual void SetHostPlatformTiming(tasm::TimingKey timing_key,
                                     uint64_t timestamp_us,
                                     const tasm::PipelineID& pipeline_id) = 0;

  /**
   * @brief Get the currently running platform
   * @return eg. 'windows' 'macOS' 'iOS' 'Android' 'Linux'
   */
  virtual std::string GetPlatform() const = 0;

  /**
   * @brief Post an async task to the report thread
   * @param task The async task
   */
  virtual void RunTaskInReportThread(base::closure task) = 0;

  /**
   * @brief Interface to report an event
   * @param instance_id The instanceId of a lynx view
   * @param event The event to be reported
   */
  virtual void OnEvent(int32_t instance_id, ReportEvent& event) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_PUBLIC_PERF_CONTROLLER_PROXY_H_
