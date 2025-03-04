// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_NG_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_NG_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/public/pipeline_option.h"
#include "core/public/pub_value.h"
#include "core/services/timing_handler/timing_map.h"

namespace lynx {
namespace tasm {
namespace timing {

using TimingFlag = std::string;

class TimingInfoNg {
 public:
  TimingInfoNg() = default;
  ~TimingInfoNg() = default;

  TimingInfoNg(TimingInfoNg&) = delete;
  TimingInfoNg(const TimingInfoNg&) = delete;
  TimingInfoNg(TimingInfoNg&&) = delete;
  TimingInfoNg& operator=(const TimingInfoNg&) = delete;
  TimingInfoNg& operator=(TimingInfoNg&&) = delete;

  // Setter methods for various properties.
  inline void SetValueFactory(
      const std::shared_ptr<pub::PubValueFactory>& value_factory) {
    value_factory_ = value_factory;
  }
  // In some case does not have backgroundRuntime, we need to know whether this
  // message needs to be sent to LynxEngine.
  inline void SetEnableEngineCallback(bool enable_engine_callback) {
    enable_engine_callback_ = enable_engine_callback;
  }
  inline bool GetEnableEngineCallback() const {
    return enable_engine_callback_;
  }

  // This logic is to ensure compatibility with the old js_app markTiming
  // API. The old js_app markTiming API takes TimingFlag as a parameter and
  // uses it as the dimension for marking.
  // Now, we need to mark Timing using pipeline_id as the dimension.
  // However, The old js_app markTiming lacks the context related to
  // pipeline_id, so we can only mark using TimingFlag as the dimension. We
  // will additionally store this data using TimingFlag and later associate
  // it. In the long term, this API will be deprecated after most of the
  // business front-end frameworks are upgraded.
  bool SetTimingWithTimingFlag(const tasm::timing::TimingFlag& timing_flag,
                               const std::string& timestamp_key,
                               const tasm::timing::TimestampUs timestamp);
  // If your data is generated within a pipeline, meaning you have a specific
  // pipelineID, you should use SetPipelineTiming to update this tracking point.
  bool SetPipelineTiming(const TimestampKey& timing_key,
                         const TimestampUs us_timestamp,
                         const PipelineID& pipeline_id);
  // If your data is generated independently of a specific pipeline, such as the
  // initialization of a container or lynxview, you should use SetInitTiming to
  // update this tracking point.
  bool SetInitTiming(const TimestampKey& timing_key,
                     const TimestampUs us_timestamp);
  // If your data comes from a front-end framework, you should use
  // SetFrameworkTiming to update this tracking point.
  bool SetFrameworkTiming(const TimestampKey& timing_key,
                          const TimestampUs us_timestamp,
                          const PipelineID& pipeline_id);

  // Send the time consumption of the Init phase. They will be sent when entry
  // is ready.
  std::unique_ptr<lynx::pub::Value> GetInitContainerEntry(
      const TimestampKey& current_key);
  std::unique_ptr<lynx::pub::Value> GetInitLynxViewEntry(
      const TimestampKey& current_key);
  std::unique_ptr<lynx::pub::Value> GetInitBackgroundRuntimeEntry(
      const TimestampKey& current_key);
  // Send metrics. They will all be sent when kPaintEnd. However, actualFmp
  // will only be sent if the timing_flag: __lynx_timing_flag_actual_fmp exists.
  std::unique_ptr<lynx::pub::Value> GetMetricFcpEntry(
      const TimestampKey& current_key, const PipelineID& pipeline_id);
  std::unique_ptr<lynx::pub::Value> GetMetricTtiEntry(
      const TimestampKey& current_key, const PipelineID& pipeline_id);
  std::unique_ptr<lynx::pub::Value> GetMetricFmpEntry(
      const TimestampKey& current_key, const PipelineID& pipeline_id);
  // Send the time consumption of the Pipeline phase. They will be sent after
  // PipelineEnd occurs. When pipeline.origin is loadBundle, reloadBundle, etc.,
  // LoadBundleEntry will be sent particularly.
  std::unique_ptr<lynx::pub::Value> GetLoadBundleEntry(
      const TimestampKey& current_key, const PipelineID& pipeline_id);
  std::unique_ptr<lynx::pub::Value> GetPipelineEntry(
      const TimestampKey& current_key, const PipelineID& pipeline_id,
      const tasm::timing::TimingFlag& timing_flag = "");

  void ClearAllTimingInfo();

 private:
  // Note: All data is not meant to be overwritten! If you need to overwrite any
  // data, you must clear it first using ClearInitTimingInfo or
  // ClearPipelineTimingInfo before reconfiguring it.

  // load_bundle_timing_map_ stores loadBundleEntry. The cache is used so that
  // pipelines other than the LoadBundlePipeline can also obtain
  // loadBundle-related timing, such as for the calculation of metrics like FMP.
  TimingMap load_bundle_timing_map_;
  // pipeline_timing_info_ stores all the related data of each pipeline, from
  // loadBundleStart to paintEnd, indexed by pipelineId.
  std::unordered_map<PipelineID, TimingMap> pipeline_timing_info_;
  // framework_timing_info stores the tracking data from the front-end
  // framework. Note that TimingHandler does not concern itself with the
  // specific [key, value] pairs within the framework_timing_info_ structure.
  // They will be directly merged when dispatching the pipelineEntry.
  std::unordered_map<PipelineID, TimingMap> framework_timing_info_;
  // init_timing_info_ stores the initialization durations for lynxview,
  // container, and backgroundRuntime. These duration data are not related to
  // any specific pipelineId. If there are other data unrelated to pipeline,
  // they should also be stored here.
  TimingMap init_timing_info_;
  // Save all metric calculation results, because metrics depend on container
  // processing time, so some time calculations will be delayed. At this point,
  // the calculations must ensure that previous results are not affected. This
  // data structure is also used to control the frequency of metric sending.
  std::unordered_map<TimestampKey, std::unique_ptr<lynx::pub::Value>> metrics_;

  // timing_infos_with_timing_flag_ is used to ensure compatibility with the
  // old js_app markTiming API. We store the data using TimingFlag and
  // later associate it in PrepareBeforeDispatchUpdate.
  // In the long term, this data structure will be deprecated after most of the
  // business front-end frameworks are upgraded.
  std::unordered_map<TimingFlag, TimingMap> timing_infos_with_timing_flag_;

  // Other properties for tracking state and configuration.
  bool enable_engine_callback_{false};
  std::shared_ptr<pub::PubValueFactory> value_factory_ = nullptr;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_NG_H_
