// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_info_ng.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/services/timing_handler/timing_utils.h"

namespace lynx {
namespace tasm {
namespace timing {

void TimingInfoNg::ClearPipelineTimingInfo() {
  pipeline_timing_info_.clear();
  framework_timing_info_.clear();
  framework_extra_info_.clear();
  host_platform_timing_info_.clear();
  host_platform_extra_info_.clear();
  metrics_.clear();
  load_bundle_pipeline_id_ = "";
  has_reload_ = false;
  pipeline_id_to_origin_map_.clear();
  fsp_info_.clear();
  fsp_end_timing_ = 0;
}

void TimingInfoNg::ClearContainerTimingInfo() {
  std::initializer_list<TimestampKey> container_keys = {
      kContainerInitStart, kContainerInitEnd, kPrepareTemplateStart,
      kPrepareTemplateEnd, kOpenTime};
  for (const TimestampKey& key : container_keys) {
    init_timing_info_.Erase(key);
  }
}

void TimingInfoNg::ReleasePipelineTiming(const PipelineID& pipeline_id) {
  pipeline_timing_info_.erase(pipeline_id);
  framework_timing_info_.erase(pipeline_id);
  framework_extra_info_.erase(pipeline_id);
  host_platform_timing_info_.erase(pipeline_id);
  host_platform_extra_info_.erase(pipeline_id);
  framework_extra_info_.erase(pipeline_id);
  pipeline_id_to_origin_map_.erase(pipeline_id);
}

bool TimingInfoNg::SetFrameworkTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    lynx::tasm::timing::TimestampUs us_timestamp,
    const lynx::tasm::PipelineID& pipeline_id) {
  return framework_timing_info_[pipeline_id].SetTimestamp(timing_key,
                                                          us_timestamp);
}

bool TimingInfoNg::SetFrameworkExtraTimingInfo(
    const lynx::tasm::PipelineID& pipeline_id, const std::string& info_key,
    const std::string& info_value) {
  return framework_extra_info_[pipeline_id]
      .emplace(info_key, info_value)
      .second;
}

bool TimingInfoNg::SetHostPlatformTiming(TimestampKey& timing_key,
                                         TimestampUs us_timestamp,
                                         const PipelineID& pipeline_id) {
  return host_platform_timing_info_[pipeline_id].SetTimestamp(timing_key,
                                                              us_timestamp);
}

bool TimingInfoNg::SetHostPlatformTimingExtraInfo(
    const lynx::tasm::PipelineID& pipeline_id, const std::string& info_key,
    const std::string& info_value) {
  return host_platform_extra_info_[pipeline_id]
      .emplace(info_key, info_value)
      .second;
}

void TimingInfoNg::BindPipelineOriginWithPipelineId(
    const PipelineID& pipeline_id, const PipelineOrigin& pipeline_origin) {
  pipeline_id_to_origin_map_.emplace(pipeline_id, pipeline_origin);
  if (pipeline_origin == kLoadBundle) {
    load_bundle_pipeline_id_ = pipeline_id;
    has_reload_ = false;
  } else if (pipeline_origin == kReloadBundleFromNative ||
             pipeline_origin == kReloadBundleFromBts) {
    load_bundle_pipeline_id_ = pipeline_id;
    has_reload_ = true;
  }
}

bool TimingInfoNg::SetPipelineTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    const lynx::tasm::timing::TimestampUs us_timestamp,
    const lynx::tasm::PipelineID& pipeline_id) {
  auto& timing_info = pipeline_timing_info_[pipeline_id];

  return timing_info.SetTimestamp(timing_key, us_timestamp);
}

bool TimingInfoNg::SetInitTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    const lynx::tasm::timing::TimestampUs us_timestamp) {
  return init_timing_info_.SetTimestamp(timing_key, us_timestamp);
}

bool TimingInfoNg::SetFSPTiming(const TimestampUs us_timestamp) {
  if (fsp_end_timing_ == 0) {
    fsp_end_timing_ = us_timestamp;
    return true;
  } else {
    return false;
  }
}

bool TimingInfoNg::SetFSPInfo(const std::string& key,
                              const std::string& value) {
  return fsp_info_.emplace(key, value).second;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetPipelineEntry(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  // get timing map
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  const auto& timing_map = it->second;
  // check ready
  // Different ready conditions are determined based on the origin
  auto pipeline_origin_it = pipeline_id_to_origin_map_.find(pipeline_id);
  PipelineOrigin pipeline_origin = kEntryTypePipeline;
  if (pipeline_origin_it != pipeline_id_to_origin_map_.end()) {
    pipeline_origin = pipeline_origin_it->second;
  }
  // check normal pipeline is ready
  static const std::initializer_list<std::string> normal_pipeline_check_keys = {
      kPaintEnd, kLayoutEnd, kLayoutUiOperationExecuteEnd, kPipelineEnd};
  bool ready = timing_map.CheckAllKeysExist(normal_pipeline_check_keys);
  if (!ready) {
    return nullptr;
  }
  std::unique_ptr<lynx::pub::Value> entry_ptr = nullptr;
  // check special pipeline is ready
  if (pipeline_origin == kLoadBundle) {
    // check loadBundle is ready
    bool ready = false;
    if (enable_background_runtime_) {
      static const std::initializer_list<std::string> check_keys =
          std::initializer_list<std::string>{kLoadBundleEnd,
                                             kLoadBackgroundEnd};
      ready = timing_map.CheckAllKeysExist(check_keys);
    } else {
      static const std::initializer_list<std::string> check_keys =
          std::initializer_list<std::string>{kLoadBundleEnd};
      ready = timing_map.CheckAllKeysExist(check_keys);
    }
    if (!ready) {
      return nullptr;
    }
    entry_ptr = init_timing_info_.ToPubMap(false, value_factory_);
    PushFcpToPubMap(entry_ptr);
  } else if (pipeline_origin == kReloadBundleFromBts ||
             pipeline_origin == kReloadBundleFromNative) {
    // check reloadBundle is ready
    bool ready = false;
    if (enable_background_runtime_) {
      static const std::initializer_list<std::string> check_keys =
          std::initializer_list<std::string>{kReloadBundleEnd,
                                             kReloadBackgroundEnd};
      ready = timing_map.CheckAllKeysExist(check_keys);
    } else {
      static const std::initializer_list<std::string> check_keys =
          std::initializer_list<std::string>{kReloadBundleEnd};
      ready = timing_map.CheckAllKeysExist(check_keys);
    }
    if (!ready) {
      return nullptr;
    }
    entry_ptr = init_timing_info_.ToPubMap(false, value_factory_);
    PushFcpToPubMap(entry_ptr);
  }
  // 1.0 make entry
  if (entry_ptr) {
    timing_map.PushAllToPubMap(false, entry_ptr);
  } else {
    entry_ptr = timing_map.ToPubMap(false, value_factory_);
  }
  // 2.0 merge framework, framework-pipeline may don't have item.
  TimingMap framework_info_map;
  auto framework_info_it = framework_timing_info_.find(pipeline_id);
  if (framework_info_it != framework_timing_info_.end()) {
    framework_info_map.Merge(framework_info_it->second);
  }
  // 2.1 merge framework extra info, like dsl, stage, etc.
  std::unique_ptr<lynx::pub::Value> framework_info_value =
      framework_info_map.ToPubMap(false, value_factory_);
  auto extra_info_iter = framework_extra_info_.find(pipeline_id);
  if (extra_info_iter != framework_extra_info_.end()) {
    const auto& framework_extra_info = extra_info_iter->second;
    for (const auto& [info_key, info_value] : framework_extra_info) {
      framework_info_value->PushStringToMap(info_key, info_value);
    }
  }
  // 3.0 merge host platform timing, if it exists.
  TimingMap host_platform_info_map;
  auto host_platform_info_it = host_platform_timing_info_.find(pipeline_id);
  if (host_platform_info_it != host_platform_timing_info_.end()) {
    host_platform_info_map.Merge(host_platform_info_it->second);
  }
  // 3.1 merge framework extra info, like dsl, stage, etc.
  std::unique_ptr<lynx::pub::Value> host_platform_info_value =
      host_platform_info_map.ToPubMap(false, value_factory_);
  auto host_platform_info_iter = host_platform_extra_info_.find(pipeline_id);
  if (host_platform_info_iter != host_platform_extra_info_.end()) {
    for (const auto& [info_key, info_value] : host_platform_info_iter->second) {
      host_platform_info_value->PushStringToMap(info_key, info_value);
    }
  }
  // 3.2 merge kHostPlatformType
  host_platform_info_value->PushStringToMap(kHostPlatformType,
                                            GetHostPlatformType());
  entry_ptr->PushValueToMap(kFrameworkRenderingTiming,
                            std::move(framework_info_value));
  entry_ptr->PushValueToMap(kHostPlatformTiming,
                            std::move(host_platform_info_value));
  entry_ptr->PushStringToMap(kEntryType, kEntryTypePipeline);
  entry_ptr->PushStringToMap(kEntryName, pipeline_origin);

  return entry_ptr;
}

bool TimingInfoNg::UpdateMetrics(const std::string& name,
                                 const std::string& start_name,
                                 const std::string& end_name,
                                 uint64_t start_time, uint64_t end_time) {
  if (metrics_.find(name) != metrics_.end()) {
    return false;
  }
  auto duration = end_time - start_time;
  auto metric_map = value_factory_->CreateMap();
  metric_map->PushStringToMap(kName, name);
  metric_map->PushStringToMap(kStartTimestampName, start_name);
  metric_map->PushDoubleToMap(kStartTimestamp, ConvertUsToDouble(start_time));
  metric_map->PushStringToMap(kEndTimestampName, end_name);
  metric_map->PushDoubleToMap(kEndTimestamp, ConvertUsToDouble(end_time));
  metric_map->PushDoubleToMap(kDuration, ConvertUsToDouble(duration));
  metrics_.emplace(name, std::move(metric_map));
  return true;
}

void TimingInfoNg::PushMetricToPubMap(
    std::unique_ptr<lynx::pub::Value>& entry_map,
    const std::string& metric_name, const std::string& start_name,
    const std::string& end_name, uint64_t start_time, uint64_t end_time) {
  auto metric_map = value_factory_->CreateMap();
  metric_map->PushStringToMap(kName, metric_name);
  metric_map->PushStringToMap(kStartTimestampName, start_name);
  metric_map->PushDoubleToMap(kStartTimestamp, ConvertUsToDouble(start_time));
  metric_map->PushStringToMap(kEndTimestampName, end_name);
  metric_map->PushDoubleToMap(kEndTimestamp, ConvertUsToDouble(end_time));
  metric_map->PushDoubleToMap(kDuration,
                              CalculateMsDuration(start_time, end_time));
  entry_map->PushValueToMap(metric_name, std::move(metric_map));
}

void TimingInfoNg::PushFcpToPubMap(
    std::unique_ptr<lynx::pub::Value>& entry_map) {
  auto it = pipeline_timing_info_.find(load_bundle_pipeline_id_);
  if (it == pipeline_timing_info_.end()) {
    return;
  }
  // get stop time for all fcp and start time for lynxFcp
  auto& load_bundle_timing_map = it->second;
  uint64_t fcp_stop_time =
      load_bundle_timing_map.GetTimestamp(kPaintEnd).value_or(0);
  if (fcp_stop_time == 0) {
    return;
  }

  // 1. calculate lynxFcp = LoadBundleEntry.paintEnd -
  // loadBundleStart/reloadBundleStart
  const TimestampKey lynx_fcp_start_name =
      has_reload_ ? kReloadBundleStart : kLoadBundleStart;
  uint64_t lynx_fcp_start_time =
      load_bundle_timing_map.GetTimestamp(lynx_fcp_start_name).value_or(0);
  if (lynx_fcp_start_time != 0) {
    PushMetricToPubMap(entry_map, kLynxFCP, lynx_fcp_start_name, kPaintEnd,
                       lynx_fcp_start_time, fcp_stop_time);
  }

  // 2. calculate fcp = LoadBundleEntry.paintEnd - prepareTemplateStart
  auto prepare_template_start =
      init_timing_info_.GetTimestamp(kPrepareTemplateStart).value_or(0);
  if (prepare_template_start != 0) {
    PushMetricToPubMap(entry_map, kFCP, kPrepareTemplateStart, kPaintEnd,
                       prepare_template_start, fcp_stop_time);
  }

  // 3. calculate totalFcp = LoadBundleEntry.paintEnd - openTime
  auto open_time = init_timing_info_.GetTimestamp(kOpenTime).value_or(0);
  if (open_time != 0) {
    PushMetricToPubMap(entry_map, kTotalFCP, kOpenTime, kPaintEnd, open_time,
                       fcp_stop_time);
  }
}

void TimingInfoNg::PushFmpToPubMap(std::unique_ptr<lynx::pub::Value>& entry_map,
                                   const PipelineID& pipeline_id) {
  if (pipeline_id.empty()) {
    return;
  }
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return;
  }
  // If there's a pipeline ID, retrieve the timing map associated with the
  // current ID.
  auto& pipeline_timing_map = it->second;
  uint64_t paint_end = pipeline_timing_map.GetTimestamp(kPaintEnd).value_or(0);
  if (paint_end == 0) {
    return;
  }
  // 1. calculate lynxActualFMP = pipeline.paintEnd -
  // loadBundleEnd/reloadBundleEnd
  auto load_bundle_it = pipeline_timing_info_.find(load_bundle_pipeline_id_);
  if (load_bundle_it != pipeline_timing_info_.end()) {
    auto& load_bundle_timing_map = load_bundle_it->second;
    const TimestampKey lynx_fcp_start_name =
        has_reload_ ? kReloadBundleStart : kLoadBundleStart;
    uint64_t lynx_fcp_start_time =
        load_bundle_timing_map.GetTimestamp(lynx_fcp_start_name).value_or(0);
    if (lynx_fcp_start_time != 0) {
      PushMetricToPubMap(entry_map, kLynxActualFMP, lynx_fcp_start_name,
                         kPaintEnd, lynx_fcp_start_time, paint_end);
    }
  }
  // 2. calculate actualFmp = PipelineEntry.paintEnd - prepareTemplateStart
  auto prepare_template_start =
      init_timing_info_.GetTimestamp(kPrepareTemplateStart).value_or(0);
  if (prepare_template_start != 0) {
    PushMetricToPubMap(entry_map, kActualFMP, kPrepareTemplateStart, kPaintEnd,
                       prepare_template_start, paint_end);
  }

  // 3. calculate totalActualFmp = PipelineEntry.paintEnd - openTime
  auto open_time = init_timing_info_.GetTimestamp(kOpenTime).value_or(0);
  if (open_time != 0) {
    PushMetricToPubMap(entry_map, kTotalActualFMP, kOpenTime, kPaintEnd,
                       open_time, paint_end);
  }
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetMetricFspEntry(
    const TimestampKey& current_key) {
  if (!value_factory_) {
    LOGE(
        "PerformanceObserver. GetMetricFspEntry failed. The ValueFactory is "
        "empty.")
    return nullptr;
  }

  bool has_update_metrics = false;

  static const std::initializer_list<std::string> check_keys = {
      kPrepareTemplateStart, kOpenTime, kFSPEnd};
  if (std::find(check_keys.begin(), check_keys.end(), current_key) ==
      check_keys.end()) {
    return nullptr;
  }

  uint64_t fsp_end = fsp_end_timing_;
  if (fsp_end == 0) {
    return nullptr;
  }

  /* Calculation formula:
   * lynxFsp = fspEnd - (Re)loadBundleEntry.loadBundleStart
   * Fsp = fspEnd - InitContainerEntry.prepareTemplateStart
   * totalFsp = fspEnd - InitContainerEntry.openTime
   */
  if (metrics_.find(kLynxFSP) == metrics_.end()) {
    std::string lynx_fsp_start_name;
    auto pipeline_origin =
        pipeline_id_to_origin_map_.find(load_bundle_pipeline_id_);
    if (pipeline_origin != pipeline_id_to_origin_map_.end()) {
      if (pipeline_origin->second == kLoadBundle) {
        lynx_fsp_start_name = kLoadBundleStart;
      } else if (pipeline_origin->second == kReloadBundleFromBts ||
                 pipeline_origin->second == kReloadBundleFromNative) {
        lynx_fsp_start_name = kReloadBundleStart;
      } else {
        LOGE("TimingInfoNg: only loadBundle/reloadBundle could calc fsp.")
      }
    } else {
      LOGE(
          "TimingInfoNg: fsp must be calculated after loadBundle/reloadBundle.")
    }

    auto it = pipeline_timing_info_.find(load_bundle_pipeline_id_);
    if (it != pipeline_timing_info_.end() && lynx_fsp_start_name != "") {
      auto& load_bundle_timing_map = it->second;
      uint64_t lynx_fsp_start_time =
          load_bundle_timing_map.GetTimestamp(lynx_fsp_start_name).value_or(0);
      has_update_metrics |= UpdateMetrics(
          kLynxFSP, lynx_fsp_start_name, kFSPEnd, lynx_fsp_start_time, fsp_end);
    }
  }
  if (metrics_.find(kFSP) == metrics_.end()) {
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      has_update_metrics |= UpdateMetrics(kFSP, kPrepareTemplateStart, kFSPEnd,
                                          *prepare_template_start, fsp_end);
    }
  }
  if (metrics_.find(kTotalFSP) == metrics_.end()) {
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      has_update_metrics |=
          UpdateMetrics(kTotalFSP, kOpenTime, kFSPEnd, *open_time, fsp_end);
    }
  }

  if (has_update_metrics) {
    const char* keys[] = {kLynxFSP, kFSP, kTotalFSP};
    auto entry = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        entry->PushValueToMap(key, *it->second);
      }
    }
    entry->PushStringToMap(kEntryType, kEntryTypeMetric);
    entry->PushStringToMap(kEntryName, kEntryNameFSP);
    for (const auto& [key, value] : fsp_info_) {
      entry->PushStringToMap(key, value);
    }
    return entry;
  }
  return nullptr;
}

void TimingInfoNg::SetHostPlatformType(const std::string& type) {
  host_platform_type_ = type;
}

std::string TimingInfoNg::GetHostPlatformType() const {
  auto platform_type =
      host_platform_type_.empty() ? Config::Platform() : host_platform_type_;
  return platform_type;
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
