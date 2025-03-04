// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_handler_ng.h"

#include <unordered_set>
#include <utility>

#include "base/include/log/logging.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {

TimingHandlerNg::TimingHandlerNg(TimingHandlerDelegate* delegate)
    : delegate_(delegate) {
  if (delegate_) {
    timing_info_.SetValueFactory(delegate_->GetValueFactory());
  }
}

void TimingHandlerNg::OnPipelineStart(
    const PipelineID& pipeline_id, const PipelineOrigin& pipeline_origin,
    const TimestampUs pipeline_start_timestamp) {
  pipeline_id_to_origin_map_.emplace(pipeline_id, pipeline_origin);
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
  timing_info_.SetFrameworkTiming(timing_key, us_timestamp, pipeline_id);
}

void TimingHandlerNg::SetTiming(const TimestampKey& timing_key,
                                const TimestampUs us_timestamp,
                                const PipelineID& pipeline_id) {
  if (timing_key.empty() || us_timestamp == 0) {
    LOGE("Invalid timing key or timestamp in TimingHandlerNg::SetTiming");
    return;
  }
  if (pipeline_id.empty()) {
    ProcessInitTiming(timing_key, us_timestamp);
  } else {
    ProcessPipelineTiming(timing_key, us_timestamp, pipeline_id);
  }
}

void TimingHandlerNg::SetTimingWithTimingFlag(
    const tasm::timing::TimingFlag& timing_flag,
    const std::string& timestamp_key,
    const tasm::timing::TimestampUs timestamp) {
  timing_info_.SetTimingWithTimingFlag(timing_flag, timestamp_key, timestamp);
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
    DispatchPerformanceEventIfNeeded(timing_key, pipeline_id);
  }
  // TODO(zhangkaijie.9): temporarily regard PaintEnd as PipelineEnd.
  if (timing_key == kPaintEnd) {
    ProcessPipelineTiming(kPipelineEnd, us_timestamp, pipeline_id);
  }
}

bool TimingHandlerNg::IsLoadBundlePipeline(
    const PipelineID& pipeline_id) const {
  const auto& it = pipeline_id_to_origin_map_.find(pipeline_id);
  if (it != pipeline_id_to_origin_map_.end()) {
    return it->second == kLoadTemplate || it->second == kReloadTemplate ||
           it->second == kReloadBundle || it->second == kLoadBundle;
  }
  return false;
}

/*
 * Reset all timing information.
 */
void TimingHandlerNg::ClearAllTimingInfo() {
  timing_info_.ClearAllTimingInfo();
  has_dispatched_timing_flags_.clear();
}

/*
 * Dispatch PerformanceEntry
 */
void TimingHandlerNg::DispatchPerformanceEventIfNeeded(
    const TimestampKey& timing_key, const lynx::tasm::PipelineID& pipeline_id) {
  if (!pipeline_id.empty()) {
    DispatchPipelineEntryIfNeeded(timing_key, pipeline_id);
    DispatchMetricFcpEntryIfNeeded(timing_key, pipeline_id);
    DispatchMetricTtiEntryIfNeeded(timing_key, pipeline_id);
    DispatchMetricFmpEntryIfNeeded(timing_key, pipeline_id);
  } else {
    DispatchInitContainerEntryIfNeeded(timing_key);
    DispatchInitLynxViewEntryIfNeeded(timing_key);
    DispatchInitBackgroundRuntimeEntryIfNeeded(timing_key);
  }
}

void TimingHandlerNg::DispatchInitContainerEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_container_entry = timing_info_.GetInitContainerEntry(current_key);
  if (init_container_entry != nullptr) {
    init_container_entry->PushStringToMap(kEntryType, kEntryTypeInit);
    init_container_entry->PushStringToMap(kEntryName, kEntryNameContainer);
    if (delegate_) {
      delegate_->OnPerformanceEvent(std::move(init_container_entry),
                                    timing_info_.GetEnableEngineCallback());
    }
  }
}

void TimingHandlerNg::DispatchInitLynxViewEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_lynxview_entry = timing_info_.GetInitLynxViewEntry(current_key);
  if (init_lynxview_entry != nullptr) {
    init_lynxview_entry->PushStringToMap(kEntryType, kEntryTypeInit);
    init_lynxview_entry->PushStringToMap(kEntryName, kEntryNameLynxView);
    if (delegate_) {
      delegate_->OnPerformanceEvent(std::move(init_lynxview_entry),
                                    timing_info_.GetEnableEngineCallback());
    }
  }
}

void TimingHandlerNg::DispatchInitBackgroundRuntimeEntryIfNeeded(
    const TimestampKey& current_key) {
  auto init_background_runtime_entry =
      timing_info_.GetInitBackgroundRuntimeEntry(current_key);
  if (init_background_runtime_entry != nullptr) {
    init_background_runtime_entry->PushStringToMap(kEntryType, kEntryTypeInit);
    init_background_runtime_entry->PushStringToMap(kEntryName,
                                                   kEntryNameBackgroundRuntime);
    if (delegate_) {
      delegate_->OnPerformanceEvent(std::move(init_background_runtime_entry),
                                    timing_info_.GetEnableEngineCallback());
    }
  }
}

void TimingHandlerNg::DispatchMetricFcpEntryIfNeeded(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!IsLoadBundlePipeline(pipeline_id)) {
    return;
  }
  auto entry = timing_info_.GetMetricFcpEntry(current_key, pipeline_id);
  if (entry == nullptr) {
    return;
  }
  entry->PushStringToMap(kEntryType, kEntryTypeMetric);
  entry->PushStringToMap(kEntryName, kEntryNameFCP);
  delegate_->OnPerformanceEvent(std::move(entry),
                                timing_info_.GetEnableEngineCallback());
}

void TimingHandlerNg::DispatchMetricTtiEntryIfNeeded(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!IsLoadBundlePipeline(pipeline_id)) {
    return;
  }
  auto entry = timing_info_.GetMetricTtiEntry(current_key, pipeline_id);
  if (entry == nullptr) {
    return;
  }
  entry->PushStringToMap(kEntryType, kEntryTypeMetric);
  entry->PushStringToMap(kEntryName, kEntryNameTTI);
  delegate_->OnPerformanceEvent(std::move(entry),
                                timing_info_.GetEnableEngineCallback());
}

void TimingHandlerNg::DispatchMetricFmpEntryIfNeeded(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  auto it = pipeline_id_to_timing_flags_map_.find(pipeline_id);
  if (it == pipeline_id_to_timing_flags_map_.end()) {
    return;
  }
  // Iterate over the vector of TimingFlags for the specific ID
  for (const TimingFlag& flag : it->second) {
    if (flag != kLynxTimingActualFMPFlag) {
      continue;
    }
    auto entry = timing_info_.GetMetricFmpEntry(current_key, pipeline_id);
    if (entry == nullptr) {
      return;
    }
    entry->PushStringToMap(kEntryType, kEntryTypeMetric);
    entry->PushStringToMap(kEntryName, kEntryNameActualFMP);
    delegate_->OnPerformanceEvent(std::move(entry),
                                  timing_info_.GetEnableEngineCallback());
    break;
  }
}

void TimingHandlerNg::DispatchLoadBundleEntryIfNeeded(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  const auto& name = pipeline_id_to_origin_map_.find(pipeline_id);
  auto timing_flag_iter = pipeline_id_to_timing_flags_map_.find(pipeline_id);
  bool is_timing_flags_empty =
      (timing_flag_iter == pipeline_id_to_timing_flags_map_.end());

  auto process_load_bundle_entry = [&](auto load_bundle_entry,
                                       const std::string& flag = {}) {
    load_bundle_entry->PushStringToMap(kEntryType, kEntryTypePipeline);
    if (name != pipeline_id_to_origin_map_.end()) {
      load_bundle_entry->PushStringToMap(kEntryName, name->second);
    } else {
      load_bundle_entry->PushStringToMap(kEntryName, kEntryNameLoadBundle);
    }
    if (!flag.empty()) {
      load_bundle_entry->PushStringToMap(kIdentifier, flag);
    }
    if (delegate_) {
      delegate_->OnPerformanceEvent(std::move(load_bundle_entry),
                                    timing_info_.GetEnableEngineCallback());
    }
  };

  if (is_timing_flags_empty) {
    auto load_bundle_entry =
        timing_info_.GetLoadBundleEntry(current_key, pipeline_id);
    if (load_bundle_entry != nullptr) {
      process_load_bundle_entry(std::move(load_bundle_entry));
    }
    return;
  } else {
    // Iterate over the vector of TimingFlags for the specific ID
    for (const TimingFlag& flag : timing_flag_iter->second) {
      if (has_dispatched_timing_flags_.count(flag)) {
        continue;
      }
      auto load_bundle_entry =
          timing_info_.GetLoadBundleEntry(current_key, pipeline_id);
      if (load_bundle_entry == nullptr) {
        continue;
      }
      process_load_bundle_entry(std::move(load_bundle_entry), flag);
      has_dispatched_timing_flags_.emplace(flag);
    }
  }
}

void TimingHandlerNg::DispatchPipelineEntryIfNeeded(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  if (IsLoadBundlePipeline(pipeline_id)) {
    DispatchLoadBundleEntryIfNeeded(current_key, pipeline_id);
    return;
  }

  auto timing_flag_iter = pipeline_id_to_timing_flags_map_.find(pipeline_id);
  if (timing_flag_iter == pipeline_id_to_timing_flags_map_.end()) {
    return;
  }
  const auto& name = pipeline_id_to_origin_map_.find(pipeline_id);
  // pipeline
  // Iterate over the vector of TimingFlags for the specific ID
  for (const TimingFlag& flag : timing_flag_iter->second) {
    if (has_dispatched_timing_flags_.count(flag)) {
      continue;
    }
    auto pipeline_entry =
        timing_info_.GetPipelineEntry(current_key, pipeline_id, flag);
    if (pipeline_entry == nullptr) {
      continue;
    }

    pipeline_entry->PushStringToMap(kEntryType, kEntryTypePipeline);
    if (name != pipeline_id_to_origin_map_.end()) {
      pipeline_entry->PushStringToMap(kEntryName, name->second);
    } else {
      pipeline_entry->PushStringToMap(kEntryName, kEntryTypePipeline);
    }
    pipeline_entry->PushStringToMap(kIdentifier, flag);
    if (delegate_) {
      delegate_->OnPerformanceEvent(std::move(pipeline_entry),
                                    timing_info_.GetEnableEngineCallback());
    }
    has_dispatched_timing_flags_.emplace(flag);
  }
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
