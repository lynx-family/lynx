// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_H_

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
/**
 * @brief The TimingInfo class serves as a timing data manager for storing
 * timing-related data.
 *
 * This class is responsible for storing timestamps at various stages of
 * rendering pipeline. It provides methods to set and retrieve specific event
 * timestamps and can differentiate between various update timings using
 * TimingFlag. The timing data is stored as microseconds internally by
 * TimingMap. But it can be retrieved as either microseconds or milliseconds.
 */
class TimingInfo {
 public:
  TimingInfo() = default;
  ~TimingInfo() = default;

  TimingInfo(TimingInfo&) = delete;
  TimingInfo(const TimingInfo&) = delete;
  TimingInfo(TimingInfo&&) = delete;
  TimingInfo& operator=(const TimingInfo&) = delete;
  TimingInfo& operator=(TimingInfo&&) = delete;

  // Methods to set timing information, accepts a key (TimestampKey) and a
  // timestamp in microseconds (TimestampUs).
  void SetInitTiming(const TimestampKey& timing_key, TimestampUs us_timestamp);
  void SetExtraTiming(const TimestampKey& timing_key, TimestampUs us_timestamp);
  void SetPipelineTiming(const TimestampKey& timing_key,
                         TimestampUs us_timestamp,
                         const PipelineID& pipeline_id);
  // This logic is to ensure compatibility with the old js_app markTiming
  // API. The old js_app markTiming API takes TimingFlag as a parameter and
  // uses it as the dimension for marking.
  // Now, we need to mark Timing using pipeline_id as the dimension.
  // However, The old js_app markTiming lacks the context related to
  // pipeline_id, so we can only mark using TimingFlag as the dimension. We
  // will additionally store this data using TimingFlag and later associate
  // it. In the long term, this API will be deprecated after most of the
  // business front-end frameworks are upgraded.
  void SetTimingWithTimingFlag(const tasm::timing::TimingFlag& timing_flag,
                               const std::string& timestamp_key,
                               tasm::timing::TimestampUs timestamp);
  // TODO(kechenglong): merge SetPipelineOrSSRTiming & SetPipelineTiming.
  void SetPipelineOrSSRTiming(const TimestampKey& timing_key,
                              TimestampUs us_timestamp,
                              const PipelineID& pipeline_id);

  // Methods to retrieve timing information in milliseconds or microseconds.
  std::unique_ptr<lynx::pub::Value> GetAllTimingInfoAsMillisecond() const;
  std::unique_ptr<lynx::pub::Value> GetAllTimingInfoAsMicrosecond() const;
  std::unique_ptr<lynx::pub::Value> GetUpdateTimingInfoAsMillisecond(
      const TimingFlag& update_flag) const;
  std::unique_ptr<lynx::pub::Value> GetUpdateTimingInfoAsMicrosecond(
      const TimingFlag& update_flag) const;

  // Methods to check if the setup or update is ready.
  bool IsSetupReady(const PipelineID& pipeline_id) const;
  bool IsUpdateReady(const PipelineID& pipeline_id) const;

  // Methods to prepare timing information before dispatching events.
  void PrepareBeforeDispatchSetup(const PipelineID& pipeline_id);
  void PrepareBeforeDispatchUpdate(const PipelineID& pipeline_id,
                                   const TimingFlag& update_flag);

  // Methods related to SSR (Server-Side Rendering) timing data.
  inline void SetSSRTimingData(const std::string& url, uint64_t data_size) {
    use_ssr_ = true;
    ssr_url_ = url;
    ssr_data_size_ = data_size;
  }

  // Setter methods for various properties.
  inline void SetValueFactory(
      const std::shared_ptr<pub::PubValueFactory>& value_factory) {
    value_factory_ = value_factory;
  }
  inline void SetThreadStrategy(
      base::ThreadStrategyForRendering thread_strategy) {
    thread_strategy_ = thread_strategy;
  }
  inline void SetEnableJSRuntime(bool enable_js_runtime) {
    enable_js_runtime_ = enable_js_runtime;
  }
  inline void SetEnableAirStrictMode(bool enable_air_strict_mode) {
    enable_air_strict_mode_ = enable_air_strict_mode;
  }
  inline void SetURL(const std::string& url) { url_ = url; }
  inline void SetHasReload(bool has_reload) { has_reload_ = has_reload; }
  inline bool GetEnableAirStrictMode() const { return enable_air_strict_mode_; }

  // Method to clear all timing information.
  // TODO(kechenglong): Temporary API, will be removed after pipelineOptions
  // finished pre-created. We don't need reset anymore.
  void ClearAllTiming();

 private:
  // TODO(kechenglong): Currently we re-use TimingMap as TimingDurationMap.
  // Consider refactoring for clearer distinction if possible.
  using TimingDurationMap = TimingMap;

  // Internal storage for timing information as a TimingMap, with values in
  // microseconds.
  // pipeline_timing_infos_, extra_timing_infos_, and init_timing_infos_
  // all store raw data.
  // Among them, pipeline_timing_infos_ stores the timing information
  // for each pipeline. The data format is as follows:
  //
  // {
  //     "pipeline_id_12345": {
  //         "create_vdom_end": 1716882079374531,
  //         "create_vdom_start": 1716882079216397,
  //         "data_processor_end": 1716882079215598,
  //         "data_processor_start": 1716882079215550,
  //         "decode_end": 1716882079214880,
  //         ...
  //     }
  // }
  std::unordered_map<PipelineID, TimingMap> pipeline_timing_infos_;
  // extra_timing_infos_ stores data for all container layers.
  // {
  //     "open_time": 1716882079374531,
  //     "container_init_start": 1716882079216397,
  //     "container_init_end": 1716882079215598,
  //     "prepare_template_start": 1716882079215550,
  //     "prepare_template_end": 1716882079214880,
  // }
  TimingMap extra_timing_infos_;
  // init_timing_infos_ stores data for the initialization of lynxview
  // {
  //     "create_lynx_start": 1716882079374531,
  //     "create_lynx_end": 1716882079216397,
  //     "load_core_start": 1716882079215598,
  //     "load_core_end": 1716882079215550,
  // }
  TimingMap init_timing_infos_;

  // setup_timing_infos_, update_timing_infos_, and metrics_ store
  // temporary data structures used for handling reporting content.
  // These are processed data for FE and client callback.
  TimingMap setup_timing_infos_;
  std::unordered_map<TimingFlag, TimingMap> update_timing_infos_;
  // timing_infos_with_timing_flag_ is used to ensure compatibility with the
  // old js_app markTiming API. We store the data using TimingFlag and
  // later associate it in PrepareBeforeDispatchUpdate.
  // In the long term, this data structure will be deprecated after most of the
  // business front-end frameworks are upgraded.
  std::unordered_map<TimingFlag, TimingMap> timing_infos_with_timing_flag_;
  TimingDurationMap metrics_;

  // Internal methods for retrieving timing information.
  std::unique_ptr<lynx::pub::Value> GetAllTimingInfoInner(
      bool as_milliseconds) const;
  std::unique_ptr<lynx::pub::Value> GetUpdateTimingInfoInner(
      const std::string& update_flag, bool as_milliseconds) const;

  // Other properties for tracking state and configuration.
  bool enable_js_runtime_{true};
  bool enable_air_strict_mode_{false};
  bool has_reload_{false};
  // TODO(kechenglong): these params should be removed from timing?
  std::string url_;
  base::ThreadStrategyForRendering thread_strategy_;

  // SSR-related information and methods.
  bool use_ssr_{false};
  std::string ssr_url_;
  uint64_t ssr_data_size_;
  TimingMap ssr_setup_timing_infos_;
  TimingDurationMap ssr_metrics_;
  std::shared_ptr<pub::PubValueFactory> value_factory_ = nullptr;
  void SetSSRSetupTiming(const TimestampKey& timing_key,
                         TimestampUs us_timestamp,
                         const PipelineID& pipeline_id);
  void PrepareBeforeDispatchSetupForSSR();
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_INFO_H_
