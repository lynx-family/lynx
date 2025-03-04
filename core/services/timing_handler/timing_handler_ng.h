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
#include "core/services/timing_handler/timing.h"
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
  TimingHandlerNg(TimingHandlerDelegate *delegate = nullptr);

  // Methods for setting timing information.
  void SetTiming(const TimestampKey &timing_key, const TimestampUs us_timestamp,
                 const PipelineID &pipeline_id);
  void SetFrameworkTiming(const TimestampKey &timing_key,
                          const TimestampUs us_timestamp,
                          const PipelineID &pipeline_id);

  // This logic is to ensure compatibility with the old js_app markTiming
  // API. The old js_app markTiming API takes TimingFlag as a parameter and
  // uses it as the dimension for marking.
  // Now, we need to mark Timing using pipeline_id as the dimension.
  // However, The old js_app markTiming lacks the context related to
  // pipeline_id, so we can only mark using TimingFlag as the dimension. We
  // will additionally store this data using TimingFlag and later associate
  // it. In the long term, this API will be deprecated after most of the
  // business front-end frameworks are upgraded.
  void SetTimingWithTimingFlag(const tasm::timing::TimingFlag &timing_flag,
                               const std::string &timestamp_key,
                               const tasm::timing::TimestampUs timestamp);

  void OnPipelineStart(const PipelineID &pipeline_id,
                       const PipelineOrigin &pipeline_origin,
                       const TimestampUs pipeline_start_timestamp);

  void BindPipelineIDWithTimingFlag(const PipelineID &pipeline_id,
                                    const TimingFlag &timing_flag);

  // Clears all stored timing information.
  void ClearAllTimingInfo();

  // Setter methods for various properties related to timing.
  inline void SetEnableEngineCallback(bool enable_engine_callback) {
    timing_info_.SetEnableEngineCallback(enable_engine_callback);
  };

 private:
  // Internal storage and delegate for timing information.
  TimingInfoNg timing_info_;
  TimingHandlerDelegate *delegate_;
  std::unordered_map<PipelineID, base::InlineVector<TimingFlag, 2>>
      pipeline_id_to_timing_flags_map_;
  std::unordered_map<PipelineID, PipelineOrigin> pipeline_id_to_origin_map_;

  std::unordered_set<TimingFlag> has_dispatched_timing_flags_;

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
  void DispatchMetricTtiEntryIfNeeded(const TimestampKey &current_key,
                                      const PipelineID &pipeline_id);
  void DispatchMetricFmpEntryIfNeeded(const TimestampKey &current_key,
                                      const PipelineID &pipeline_id);
  void DispatchLoadBundleEntryIfNeeded(const TimestampKey &current_key,
                                       const PipelineID &pipeline_id);
  void DispatchPipelineEntryIfNeeded(const TimestampKey &current_key,
                                     const PipelineID &pipeline_id);

  // Internal methods for checking which pipeline type.
  bool IsLoadBundlePipeline(const PipelineID &pipeline_id) const;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_NG_H_
