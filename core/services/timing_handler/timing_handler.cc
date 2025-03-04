// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_handler.h"

#include <unordered_set>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/services/timing_handler/timing_handler_delegate.h"
#include "core/services/timing_handler/timing_utils.h"

namespace lynx {
namespace tasm {
namespace timing {

TimingHandler::TimingHandler(std::unique_ptr<TimingHandlerDelegate> delegate)
    : handler_ng_(delegate.get()), delegate_(std::move(delegate)) {
  if (delegate_) {
    timing_info_.SetValueFactory(delegate_->GetValueFactory());
  }
}

void TimingHandler::OnPipelineStart(const PipelineID& pipeline_id,
                                    const PipelineOrigin& pipeline_origin,
                                    TimestampUs pipeline_start_timestamp) {
  pipeline_id_to_origin_map_.emplace(pipeline_id, pipeline_origin);
  handler_ng_.OnPipelineStart(pipeline_id, pipeline_origin,
                              pipeline_start_timestamp);

  std::string start_time_key(kPipelineStart);
  SetTiming(start_time_key, pipeline_start_timestamp, pipeline_id);
}

void TimingHandler::BindPipelineIDWithTimingFlag(
    const PipelineID& pipeline_id, const TimingFlag& timing_flag) {
  if (timing_flag.empty() || pipeline_id.empty()) {
    return;
  }
  pipeline_id_to_timing_flags_map_[pipeline_id].emplace_back(timing_flag);

  handler_ng_.BindPipelineIDWithTimingFlag(pipeline_id, timing_flag);
}

// Methods for setting timing information.
void TimingHandler::SetTiming(tasm::Timing timing) {
  for (auto& [timing_key, timestamp] : timing.framework_timings_) {
    SetTiming(timing_key, timestamp, timing.pipeline_id_);

    handler_ng_.SetFrameworkTiming(timing_key, timestamp, timing.pipeline_id_);
  }
  for (auto& [timing_key, timestamp] : timing.timings_) {
    SetTiming(timing_key, timestamp, timing.pipeline_id_);
  }
}

void TimingHandler::SetTiming(TimestampKey& timing_key,
                              TimestampUs us_timestamp,
                              const PipelineID& pipeline_id) {
  if (timing_key.empty() || us_timestamp == 0) {
    LOGE("Invalid timing key or timestamp in TimingHandler::SetTiming");
    return;
  }
  TimestampKey polyfillKey = GetPolyfillTimingKey(timing_key);
  if (IsInitTiming(polyfillKey)) {
    ProcessInitTiming(polyfillKey, us_timestamp);
  } else if (IsExtraTiming(polyfillKey)) {
    ProcessExtraTiming(polyfillKey, us_timestamp);
  } else if (!pipeline_id.empty()) {
    ProcessPipelineTiming(polyfillKey, us_timestamp, pipeline_id);
  }

  handler_ng_.SetTiming(timing_key, us_timestamp, pipeline_id);
}

void TimingHandler::SetTimingWithTimingFlag(
    const tasm::timing::TimingFlag& timing_flag,
    const std::string& timestamp_key, tasm::timing::TimestampUs timestamp) {
  TimestampKey polyfillKey = GetPolyfillTimingKey(timestamp_key);
  timing_info_.SetTimingWithTimingFlag(timing_flag, polyfillKey, timestamp);

  handler_ng_.SetTimingWithTimingFlag(timing_flag, timestamp_key, timestamp);
}

// Internal methods for checking which timing type.
bool TimingHandler::IsInitTiming(const TimestampKey& timing_key) const {
  // These are the only init timing keys we are looking for
  static const base::NoDestructor<std::unordered_set<TimestampKey>>
      initTimingKeys{{kCreateLynxStartPolyfill, kCreateLynxEndPolyfill,
                      kLoadCoreStartPolyfill, kLoadCoreEndPolyfill,
                      kTemplateBundleParseStartPolyfill,
                      kTemplateBundleParseEndPolyfill}};
  return initTimingKeys->find(timing_key) != initTimingKeys->end();
}

bool TimingHandler::IsExtraTiming(const TimestampKey& timing_key) const {
  // These are the only extra timing keys we are looking for
  static const base::NoDestructor<std::unordered_set<TimestampKey>>
      extraTimingKeys{{kOpenTimePolyfill, kContainerInitStartPolyfill,
                       kContainerInitEndPolyfill, kPrepareTemplateStartPolyfill,
                       kPrepareTemplateEndPolyfill}};
  return extraTimingKeys->find(timing_key) != extraTimingKeys->end();
}

bool TimingHandler::IsSetupPipeline(const PipelineID& pipeline_id) const {
  const auto& it = pipeline_id_to_origin_map_.find(pipeline_id);
  if (it != pipeline_id_to_origin_map_.end()) {
    // TODO(kechenglong): split kLoadSSRData to a special pipeline type?
    return it->second == kLoadTemplate || it->second == kReloadTemplate ||
           it->second == kLoadSSRData || it->second == kLoadBundle ||
           it->second == kReloadBundle;
  }
  return false;
}

// Internal methods for checking if timing information can be dispatched.
bool TimingHandler::IsSetupReady(const PipelineID& pipeline_id) const {
  return timing_info_.IsSetupReady(pipeline_id);
}

bool TimingHandler::IsUpdateReady(const PipelineID& pipeline_id) const {
  return timing_info_.IsUpdateReady(pipeline_id);
}

// Internal methods for processing timing information.
void TimingHandler::ProcessInitTiming(TimestampKey& timing_key,
                                      TimestampUs us_timestamp) {
  timing_info_.SetInitTiming(timing_key, us_timestamp);
}

void TimingHandler::ProcessPipelineTiming(TimestampKey& timing_key,
                                          TimestampUs us_timestamp,
                                          const PipelineID& pipeline_id) {
  // The FE framework has added kSetupPrefix & kUpdatePrefix to the timing key.
  // So we still need the ReplaceAll logic here.
  // TODO(kechenglong): these ReplaceAll logic should be removed after the user
  // used the new FE framework package version.
  lynx::base::ReplaceAll(timing_key, kSetupPrefix, "");
  lynx::base::ReplaceAll(timing_key, kUpdatePrefix, "");

  if (IsSetupPipeline(pipeline_id)) {
    // TODO(kechenglong): merge SetPipelineOrSSRTiming & SetPipelineTiming.
    timing_info_.SetPipelineOrSSRTiming(timing_key, us_timestamp, pipeline_id);
    DispatchSetupTimingIfNeeded(pipeline_id);
    // The setup pipeline might also have an attributeTimingFlag,
    // so we also need to check if DispatchUpdateTiming is needed.
    DispatchUpdateTimingIfNeeded(pipeline_id);
  } else {
    timing_info_.SetPipelineTiming(timing_key, us_timestamp, pipeline_id);
    DispatchUpdateTimingIfNeeded(pipeline_id);
  }
}

void TimingHandler::ProcessExtraTiming(const TimestampKey& timing_key,
                                       TimestampUs us_timestamp) {
  timing_info_.SetExtraTiming(timing_key, us_timestamp);
}

// Internal methods for dispatching timing information.
void TimingHandler::DispatchSetupTimingIfNeeded(const PipelineID& pipeline_id) {
  if (has_dispatched_setup_timing_) {
    LOGE("Setup timing has already been dispatched before.");
    return;
  }
  if (!IsSetupReady(pipeline_id)) {
    return;
  }
  DispatchSetupTiming(pipeline_id);
  DispatchPendingPipelineIDIfNeeded();
}

void TimingHandler::DispatchUpdateTimingIfNeeded(
    const PipelineID& pipeline_id) {
  if (!IsUpdateReady(pipeline_id)) {
    return;
  }
  if (has_dispatched_setup_timing_) {
    DispatchUpdateTiming(pipeline_id);
  } else {
    pending_dispatched_pipeline_id_.emplace_back(pipeline_id);
  }
}

void TimingHandler::DispatchSetupTiming(const PipelineID& pipeline_id) {
  has_dispatched_setup_timing_ = true;
  timing_info_.PrepareBeforeDispatchSetup(pipeline_id);
  if (delegate_) {
    delegate_->OnTimingSetup(timing_info_);
  }
}

void TimingHandler::DispatchUpdateTiming(const PipelineID& pipeline_id) {
  // Check if the specific ID exists in the map
  auto it = pipeline_id_to_timing_flags_map_.find(pipeline_id);
  if (it == pipeline_id_to_timing_flags_map_.end()) {
    return;
  }
  // Iterate over the vector of TimingFlags for the specific ID
  for (const TimingFlag& flag : it->second) {
    if (has_dispatched_timing_flags_.count(flag)) {
      continue;
    }
    has_dispatched_timing_flags_.emplace(flag);
    timing_info_.PrepareBeforeDispatchUpdate(pipeline_id, flag);
    if (delegate_) {
      delegate_->OnTimingUpdate(timing_info_, flag);
    }
  }
}

void TimingHandler::DispatchPendingPipelineIDIfNeeded() {
  for (const auto& pipeline_id : pending_dispatched_pipeline_id_) {
    DispatchUpdateTiming(pipeline_id);
  }
  pending_dispatched_pipeline_id_.clear();
}

// Retrieves all timing information.
std::unique_ptr<lynx::pub::Value> TimingHandler::GetAllTimingInfo() const {
  return timing_info_.GetAllTimingInfoAsMicrosecond();
}

// Reset all timing information.
void TimingHandler::ResetTimingBeforeReload(const PipelineID& pipeline_id) {
  ClearAllTimingInfo();
  timing_info_.SetHasReload(true);
}

void TimingHandler::ClearAllTimingInfo() {
  timing_info_.ClearAllTiming();
  has_dispatched_setup_timing_ = false;
  has_dispatched_timing_flags_.clear();

  handler_ng_.ClearAllTimingInfo();
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
