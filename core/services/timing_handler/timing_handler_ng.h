// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_NG_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_NG_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "base/include/fml/thread.h"
#include "base/include/vector.h"
#include "core/public/pipeline_option.h"
#include "core/services/performance/performance_event_sender.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_handler_delegate.h"
#include "core/services/timing_handler/timing_info_ng.h"

namespace lynx {
namespace shell {
class NativeFacade;
}

namespace tasm {
namespace timing {
class TimingHandlerNg {
 public:
  explicit TimingHandlerNg(performance::PerformanceEventSender *sender);

  // Methods for setting timing information.
  void SetTiming(const TimestampKey &timing_key, const TimestampUs us_timestamp,
                 const PipelineID &pipeline_id);
  void SetFrameworkTiming(const TimestampKey &timing_key,
                          const TimestampUs us_timestamp,
                          const PipelineID &pipeline_id);

  // for framework to store the extra info like dsl, stage, etc.
  void SetFrameworkExtraTimingInfo(const PipelineID &pipeline_id,
                                   const std::string &key,
                                   const std::string &value);

  void SetHostPlatformTiming(TimestampKey &timing_key, TimestampUs us_timestamp,
                             const PipelineID &pipeline_id);

  void SetHostPlatformTimingExtraInfo(const PipelineID &pipeline_id,
                                      const std::string &key,
                                      const std::string &value);

  void OnPipelineStart(const PipelineID &pipeline_id,
                       const PipelineOrigin &pipeline_origin,
                       const TimestampUs pipeline_start_timestamp);

  void BindPipelineIDWithTimingFlag(const PipelineID &pipeline_id,
                                    const TimingFlag &timing_flag);

  // Clear timing information related to PipelineEntry.
  void ClearPipelineTimingInfo();
  // Clear timing information related to InitContainerEntry.
  void ClearContainerTimingInfo();

  // Setter methods for various properties related to timing.
  inline void SetEnableEngineCallback(bool enable_engine_callback) {
    timing_info_.SetEnableEngineCallback(enable_engine_callback);
  };

  inline void SetEnableBackgroundRuntime(bool enable_background_runtime) {
    timing_info_.SetEnableBackgroundRuntime(enable_background_runtime);
  }

 private:
  // Internal storage and delegate for timing information.
  TimingInfoNg timing_info_;
  performance::PerformanceEventSender *sender_;
  std::unordered_map<PipelineID, base::InlineVector<TimingFlag, 2>>
      pipeline_id_to_timing_flags_map_;

  std::unordered_set<TimingFlag> has_dispatched_timing_flags_;
  bool is_background_runtime_ready_ = false;
  bool is_main_thread_runtime_ready_ = false;
  base::InlineVector<std::unique_ptr<lynx::pub::Value>, 3>
      pending_dispatched_performance_entries_;

  // Internal methods for processing timing information.
  void ProcessPipelineTiming(const TimestampKey &timing_key,
                             const TimestampUs us_timestamp,
                             const PipelineID &pipeline_id);
  void ProcessInitTiming(const TimestampKey &timing_key,
                         const TimestampUs us_timestamp);

  // Each time the timingKey is updated, DispatchPerformanceEventIfNeeded needs
  // to be used to determine whether a PerformanceEntry element is complete and
  // whether the sending conditions are met. It will call other Dispatch
  // functions to complete this task.
  void DispatchPerformanceEventIfNeeded(const TimestampKey &current_key,
                                        const PipelineID &pipeline_id = "");
  void DispatchInitContainerEntryIfNeeded(const TimestampKey &current_key);
  void DispatchInitLynxViewEntryIfNeeded(const TimestampKey &current_key);
  void DispatchInitBackgroundRuntimeEntryIfNeeded(
      const TimestampKey &current_key);
  void DispatchMetricFcpEntryIfNeeded(const TimestampKey &current_key,
                                      const PipelineID &pipeline_id);
  void DispatchMetricFmpEntryIfNeeded(const TimestampKey &current_key,
                                      const PipelineID &pipeline_id);
  void DispatchPipelineEntryIfNeeded(const TimestampKey &current_key,
                                     const PipelineID &pipeline_id);
  // Send all pending performance entries. Note that the pending queue will be
  // cleared after flushing.
  void FlushPendingPerformanceEntries();

  // Internal methods for checking which pipeline type.
  bool IsLoadBundlePipeline(const PipelineID &pipeline_id) const;
  bool IsReloadBundlePipeline(const PipelineID &pipeline_id) const;
  bool ReadyToDispatch() const;
  void SendOrPendingPerformanceEntry(std::unique_ptr<lynx::pub::Value> entry);
  void SendPerformanceEntry(std::unique_ptr<lynx::pub::Value> entry);

  void ReleasePipelineTiming(const PipelineID &pipeline_id);
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_NG_H_
