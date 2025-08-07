// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_handler_ng.h"

#include <unordered_set>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {

TimingHandlerNg::TimingHandlerNg(performance::PerformanceEventSender* sender)
    : sender_(sender) {
  if (sender_) {
    timing_info_.SetValueFactory(sender_->GetValueFactory());
  }
}

void TimingHandlerNg::OnPipelineStart(
    const PipelineID& pipeline_id, const PipelineOrigin& pipeline_origin,
    const TimestampUs pipeline_start_timestamp) {
  timing_info_.BindPipelineOriginWithPipelineId(pipeline_id, pipeline_origin);
  if (sender_) {
    timing_info_.SetHostPlatformTimingExtraInfo(pipeline_id, kHostPlatformType,
                                                Config::Platform());
  }
  if (pipeline_origin == kLoadBundle ||
      pipeline_origin == kReloadBundleFromNative ||
      pipeline_origin == kReloadBundleFromBts) {
    timing_info_.SetLoadBundlePipelineId(pipeline_id);
  }
}

void TimingHandlerNg::BindPipelineIDWithTimingFlag(
    const PipelineID& pipeline_id, const TimingFlag& timing_flag) {
  if (timing_flag.empty() || pipeline_id.empty()) {
    return;
  }
  pipeline_id_to_timing_flags_map_[pipeline_id].emplace_back(timing_flag);
}

// Methods for setting timing information.
void TimingHandlerNg::SetFrameworkTiming(const TimestampKey& timing_key,
                                         const TimestampUs us_timestamp,
                                         const PipelineID& pipeline_id) {
  if (timing_key.empty() || us_timestamp == 0) {
    LOGE("Invalid timing key or timestamp");
    return;
  }
  timing_info_.SetFrameworkTiming(timing_key, us_timestamp, pipeline_id);
}

void TimingHandlerNg::SetFrameworkExtraTimingInfo(const PipelineID& pipeline_id,
                                                  const std::string& key,
                                                  const std::string& value) {
  timing_info_.SetFrameworkExtraTimingInfo(pipeline_id, key, value);
}

void TimingHandlerNg::SetHostPlatformTiming(TimestampKey& timing_key,
                                            TimestampUs us_timestamp,
                                            const PipelineID& pipeline_id) {
  if (timing_key.empty() || us_timestamp == 0) {
    LOGE("Invalid timing key or timestamp");
    return;
  }
  timing_info_.SetHostPlatformTiming(timing_key, us_timestamp, pipeline_id);
}

void TimingHandlerNg::SetHostPlatformTimingExtraInfo(
    const PipelineID& pipeline_id, const std::string& key,
    const std::string& value) {
  timing_info_.SetHostPlatformTimingExtraInfo(pipeline_id, key, value);
}

void TimingHandlerNg::SetTiming(const TimestampKey& timing_key,
                                const TimestampUs us_timestamp,
                                const PipelineID& pipeline_id) {
  if (timing_key.empty() || us_timestamp == 0) {
    LOGE("Invalid timing key or timestamp");
    return;
  }
  if (pipeline_id.empty()) {
    ProcessInitTiming(timing_key, us_timestamp);
  } else {
    ProcessPipelineTiming(timing_key, us_timestamp, pipeline_id);
  }
}

void TimingHandlerNg::ProcessInitTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    const lynx::tasm::timing::TimestampUs us_timestamp) {
  if (timing_info_.SetInitTiming(timing_key, us_timestamp)) {
    DispatchPerformanceEventIfNeeded(timing_key);
  }
}

void TimingHandlerNg::ProcessPipelineTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    const lynx::tasm::timing::TimestampUs us_timestamp,
    const lynx::tasm::PipelineID& pipeline_id) {
  if (timing_info_.SetPipelineTiming(timing_key, us_timestamp, pipeline_id)) {
    if (timing_key == kLoadBackgroundEnd ||
        timing_key == kReloadBackgroundEnd) {
      is_background_runtime_ready_ = true;
      FlushPendingPerformanceEntries();
    } else if (timing_key == kLoadBundleEnd ||
               timing_key == kReloadBackgroundEnd) {
      is_main_thread_runtime_ready_ = true;
      FlushPendingPerformanceEntries();
    }
    DispatchPerformanceEventIfNeeded(timing_key, pipeline_id);
  }
  // TODO(zhangkaijie.9): temporarily regard PaintEnd as PipelineEnd.
  if (timing_key == kPaintEnd) {
    ProcessPipelineTiming(kPipelineEnd, us_timestamp, pipeline_id);
  }
}

bool TimingHandlerNg::IsLoadBundlePipeline(
    const PipelineID& pipeline_id) const {
  return pipeline_id == timing_info_.GetLoadBundlePipelineId();
}

/*
 * Reset all timing information.
 */
void TimingHandlerNg::ClearPipelineTimingInfo() {
  timing_info_.ClearPipelineTimingInfo();
  has_dispatched_timing_flags_.clear();
  pending_dispatched_performance_entries_.clear();
  is_background_runtime_ready_ = false;
  is_main_thread_runtime_ready_ = false;
}

void TimingHandlerNg::ClearContainerTimingInfo() {
  timing_info_.ClearContainerTimingInfo();
}

/*
 * Dispatch PerformanceEntry
 */
void TimingHandlerNg::DispatchPerformanceEventIfNeeded(
    const TimestampKey& timing_key, const lynx::tasm::PipelineID& pipeline_id) {
  if (!pipeline_id.empty()) {
    DispatchPipelineEntryIfNeeded(timing_key, pipeline_id);
  } else {
    DispatchInitContainerEntryIfNeeded(timing_key);
    DispatchInitLynxViewEntryIfNeeded(timing_key);
    DispatchInitBackgroundRuntimeEntryIfNeeded(timing_key);
  }
  DispatchMetricFcpEntryIfNeeded(timing_key, pipeline_id);
  DispatchMetricFmpEntryIfNeeded(timing_key, pipeline_id);
}

void TimingHandlerNg::DispatchInitContainerEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_container_entry = timing_info_.GetInitContainerEntry(current_key);
  if (init_container_entry == nullptr) {
    return;
  }
  SendOrPendingPerformanceEntry(std::move(init_container_entry));
}

void TimingHandlerNg::DispatchInitLynxViewEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_lynxview_entry = timing_info_.GetInitLynxViewEntry(current_key);
  if (init_lynxview_entry == nullptr) {
    return;
  }
  SendOrPendingPerformanceEntry(std::move(init_lynxview_entry));
}

void TimingHandlerNg::DispatchInitBackgroundRuntimeEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_background_runtime_entry =
      timing_info_.GetInitBackgroundRuntimeEntry(current_key);
  if (init_background_runtime_entry == nullptr) {
    return;
  }
  SendOrPendingPerformanceEntry(std::move(init_background_runtime_entry));
}

void TimingHandlerNg::DispatchMetricFcpEntryIfNeeded(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!pipeline_id.empty() && !IsLoadBundlePipeline(pipeline_id)) {
    return;
  }
  auto entry = timing_info_.GetMetricFcpEntry(current_key, pipeline_id);
  if (entry == nullptr) {
    return;
  }
  SendOrPendingPerformanceEntry(std::move(entry));
}

void TimingHandlerNg::DispatchMetricFmpEntryIfNeeded(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  if (!pipeline_id.empty()) {
    auto it = pipeline_id_to_timing_flags_map_.find(pipeline_id);
    if (it == pipeline_id_to_timing_flags_map_.end()) {
      return;
    }
    bool is_fmp_pipeline =
        false;  // Iterate over the vector of TimingFlags for the specific ID
    for (const TimingFlag& flag : it->second) {
      if (flag == kLynxTimingActualFMPFlag) {
        is_fmp_pipeline = true;
        break;
      }
    }
    if (!is_fmp_pipeline) {
      return;
    }
  }

  auto entry = timing_info_.GetMetricFmpEntry(current_key, pipeline_id);
  if (entry == nullptr) {
    return;
  }
  SendOrPendingPerformanceEntry(std::move(entry));
}

void TimingHandlerNg::DispatchPipelineEntryIfNeeded(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  auto pipeline_entry = timing_info_.GetPipelineEntry(current_key, pipeline_id);
  if (pipeline_entry == nullptr) {
    return;
  }
  /* To optimize the memory usage of TimingHandlerNg, we have formulated the
   * following rules to release Lynx Pipeline.
   *
   * A PipelineID may correspond to multiple flags. The rule for releasing a
   * PipelineID is that all flags corresponding to this PipelineID have been
   * dispatched.
   *
   * So, if a LynxPipeline can form a PipelineEntry, we will clear the timing
   * associated with it regardless of whether the PipelineEntry is sent. Unless
   * the Pipeline is a LoadBundle or ReloadBundle Pipeline, we cannot clear them
   * because they need to be used for subsequent metric calculations.
   */
  auto timing_flag_iter = pipeline_id_to_timing_flags_map_.find(pipeline_id);
  if (timing_flag_iter != pipeline_id_to_timing_flags_map_.end()) {
    for (const TimingFlag& flag : timing_flag_iter->second) {
      if (has_dispatched_timing_flags_.count(flag)) {
        continue;
      }
      // TODO(zhangkaijie.9): Removed multiple Get behavior after changing
      // PipelineEntry.identifier to string[]
      auto pipeline_entry =
          timing_info_.GetPipelineEntry(current_key, pipeline_id);
      if (pipeline_entry == nullptr) {
        return;
      }
      pipeline_entry->PushStringToMap(kIdentifier, flag);
      SendOrPendingPerformanceEntry(std::move(pipeline_entry));
      has_dispatched_timing_flags_.emplace(flag);
    }
  } else if (IsLoadBundlePipeline(pipeline_id)) {
    SendOrPendingPerformanceEntry(std::move(pipeline_entry));
  }

  if (!IsLoadBundlePipeline(pipeline_id)) {
    ReleasePipelineTiming(pipeline_id);
  }
}

void TimingHandlerNg::FlushPendingPerformanceEntries() {
  if (!sender_) {
    return;
  }

  auto temp_pending_entries =
      std::move(pending_dispatched_performance_entries_);
  for (auto& entry : temp_pending_entries) {
    if (entry) {
      SendPerformanceEntry(std::move(entry));
    }
  }
}

void TimingHandlerNg::SendOrPendingPerformanceEntry(
    std::unique_ptr<lynx::pub::Value> entry) {
  if (ReadyToDispatch()) {
    SendPerformanceEntry(std::move(entry));
  } else {
    pending_dispatched_performance_entries_.emplace_back(std::move(entry));
  }
}

void TimingHandlerNg::SendPerformanceEntry(
    std::unique_ptr<lynx::pub::Value> entry) {
  if (!sender_) {
    return;
  }
  performance::EventType env = performance::kEventTypePlatform;
  if (timing_info_.GetEnableEngineCallback()) {
    env |= performance::kEventTypeMTSEngine;
  }
  if (timing_info_.GetEnableBackgroundRuntime()) {
    env |= performance::kEventTypeBTSEngine;
  }
  sender_->OnPerformanceEvent(std::move(entry), env);
}

bool TimingHandlerNg::ReadyToDispatch() const {
  return is_background_runtime_ready_ ||
         (!timing_info_.GetEnableBackgroundRuntime() &&
          is_main_thread_runtime_ready_);
}

void TimingHandlerNg::ReleasePipelineTiming(const PipelineID& pipeline_id) {
  pipeline_id_to_timing_flags_map_.erase(pipeline_id);
  timing_info_.ReleasePipelineTiming(pipeline_id);
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
