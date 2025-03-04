// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_H_
#define CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_H_

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
#include "core/services/timing_handler/timing_handler_ng.h"
#include "core/services/timing_handler/timing_info.h"

namespace lynx {
namespace shell {
class NativeFacade;
}
namespace tasm {
namespace timing {

enum PipelineType { Setup, Update };

/**
 * @class TimingHandler
 *
 * This class is responsible for managing and dispatching timing events.
 * It is not thread-safe and should be used in conjunction with TimingInfo,
 * TimingMediator and TimingMap classes within the 'LynxTiming' thread context.
 * Currently, this timing thread context is provided by reusing the
 * 'LynxReporter' thread.
 *
 * Usage of this class, along with TimingInfo, TimingMediator and TimingMap,
 * outside of the 'LynxTiming' thread context (i.e., the 'LynxReporter' thread),
 * may lead to undefined behavior due to thread safety issues.
 *
 * @warning This class, along with TimingInfo, TimingMediator and
 * TimingMap, is not thread-safe and is intended for use only within the
 * 'timing' thread context, which is currently the 'reporter' thread.
 *
 * Here will give an example of the lifecycle of TimingHandler.
 *  1. LynxView init -> LynxTimingHandler init
 *  2. Set Timing
 *     During the process of setup or update, the timing info will be stored in
 * setupTiming or updateTimings Dictionary.
 *                /--> IsSetupTiming  ->  timing_info_.SetSetupTiming
 *     setTiming ----> IsUpdateTiming -> timing_info_.SetUpdateTiming
 *                \--> IsExtraTiming  ->   timing_info_.SetExtraTiming
 *  3. Dispatch Timing
 *     Every time after timing is set, we need to check whether the timing info
 * should be dispatched.
 * a. setup: SetSetupTiming   -> DispatchForSetupIfNeeded:
 * IsSetupReady  ?-> DispatchForSetup    -> PrepareForSetup
 * ->  Dispatch -> TimingMediator.OnTimingSetup
 * b. update: SetUpdateTiming  ->  DispatchForUpdateIfNeeded:
 *  IsUpdateReady ?-> DispatchForUpdate   -> PrepareForUpdate
 * ->  Dispatch -> TimingMediator.OnTimingUpdate
 */
class TimingHandler {
 public:
  // Delegate interface for handling timing events.
  TimingHandler(std::unique_ptr<TimingHandlerDelegate> delegate = nullptr);

  // Methods for setting timing information.
  void SetTiming(tasm::Timing timing);
  void SetTiming(TimestampKey& timing_key, TimestampUs us_timestamp,
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

  // TODO(kechenglong): remove this API.
  void ResetTimingBeforeReload(const PipelineID& pipeline_id);

  void OnPipelineStart(const PipelineID& pipeline_id,
                       const PipelineOrigin& pipeline_origin,
                       TimestampUs pipeline_start_timestamp);

  void BindPipelineIDWithTimingFlag(const PipelineID& pipeline_id,
                                    const TimingFlag& timing_flag);

  // Clears all stored timing information.
  void ClearAllTimingInfo();

  // Retrieves all timing information.
  std::unique_ptr<lynx::pub::Value> GetAllTimingInfo() const;

  // Setter methods for various properties related to timing.
  inline void SetEnableJSRuntime(bool enable_js_runtime) {
    timing_info_.SetEnableJSRuntime(enable_js_runtime);
  }
  inline void SetEnableAirStrictMode(bool enable_air_strict_mode) {
    timing_info_.SetEnableAirStrictMode(enable_air_strict_mode);
    handler_ng_.SetEnableEngineCallback(enable_air_strict_mode);
  }
  inline void SetThreadStrategy(
      base::ThreadStrategyForRendering thread_strategy) {
    timing_info_.SetThreadStrategy(thread_strategy);
  }
  inline void SetSSRTimingData(const std::string& url, uint64_t data_size) {
    timing_info_.SetSSRTimingData(url, data_size);
  };
  inline void SetURL(const std::string& url) { timing_info_.SetURL(url); }

  // Don't store this raw ptr
  // Used in RuntimeMediator::AttachToLynxShell only
  TimingHandlerDelegate* GetDelegate() { return delegate_.get(); }

 private:
  TimingHandlerNg handler_ng_;
  // Internal storage and delegate for timing information.
  TimingInfo timing_info_;
  std::unique_ptr<TimingHandlerDelegate> delegate_;
  bool has_dispatched_setup_timing_{false};
  std::unordered_map<PipelineID, base::InlineVector<TimingFlag, 2>>
      pipeline_id_to_timing_flags_map_;
  std::unordered_map<PipelineID, PipelineOrigin> pipeline_id_to_origin_map_;
  base::InlineVector<PipelineID, 16> pending_dispatched_pipeline_id_;
  std::unordered_set<TimingFlag> has_dispatched_timing_flags_;

  // Internal methods for checking which timing type.
  bool IsInitTiming(const TimestampKey& timing_key) const;
  bool IsExtraTiming(const TimestampKey& timing_key) const;

  // Internal methods for checking which pipeline type.
  bool IsSetupPipeline(const PipelineID& pipeline_id) const;

  // Internal methods for processing timing information.
  void ProcessInitTiming(TimestampKey& timing_key, TimestampUs us_timestamp);
  void ProcessPipelineTiming(TimestampKey& timing_key, TimestampUs us_timestamp,
                             const PipelineID& pipeline_id);
  void ProcessExtraTiming(const TimestampKey& timing_key,
                          TimestampUs us_timestamp);

  // Internal methods for checking if timing information can be dispatched.
  bool IsSetupReady(const PipelineID& pipeline_id) const;
  bool IsUpdateReady(const PipelineID& pipeline_id) const;
  // TODO(kechenglong): Maybe IsSetupReady & IsUpdateReady can
  // be merged to IsPipelineReady.
  // bool IsPipelineReady(const PipelineID& pipeline_id) const;

  // Internal methods for dispatching timing information.
  void DispatchSetupTimingIfNeeded(const PipelineID& pipeline_id);
  void DispatchUpdateTimingIfNeeded(const PipelineID& pipeline_id);

  void DispatchSetupTiming(const PipelineID& pipeline_id);
  void DispatchUpdateTiming(const PipelineID& pipeline_id);
  void DispatchPendingPipelineIDIfNeeded();
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_TIMING_HANDLER_TIMING_HANDLER_H_
