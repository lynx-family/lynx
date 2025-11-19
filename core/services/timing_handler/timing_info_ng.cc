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
  host_platform_timing_info_.clear();
  metrics_.clear();
  load_bundle_pipeline_id_ = "";
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
  host_platform_timing_info_.erase(pipeline_id);
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

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetInitContainerEntry(
    const TimestampKey& current_key) {
  static const std::initializer_list<std::string> pick_keys = {
      kOpenTime, kContainerInitStart, kContainerInitEnd, kPrepareTemplateStart,
      kPrepareTemplateEnd};
  // check is
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // check ready
  static const std::initializer_list<std::string> check_keys = {
      kOpenTime, kPrepareTemplateEnd};
  bool ready = init_timing_info_.CheckAllKeysExist(check_keys);
  if (!ready) {
    return nullptr;
  }

  // pick timing
  TimingMap entry_map = init_timing_info_.GetSubMap(pick_keys);
  // make entry
  auto entry = entry_map.ToPubMap(false, value_factory_);
  entry->PushStringToMap(kEntryType, kEntryTypeInit);
  entry->PushStringToMap(kEntryName, kEntryNameContainer);

  return entry;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetInitLynxViewEntry(
    const TimestampKey& current_key) {
  static const std::initializer_list<std::string> pick_keys = {kCreateLynxStart,
                                                               kCreateLynxEnd};
  // check is
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // check ready
  static const std::initializer_list<std::string> check_keys = {kCreateLynxEnd};
  bool ready = init_timing_info_.CheckAllKeysExist(check_keys);
  if (!ready) {
    return nullptr;
  }
  // pick timing
  TimingMap entry_map = init_timing_info_.GetSubMap(pick_keys);
  // make entry
  auto entry = entry_map.ToPubMap(false, value_factory_);
  entry->PushStringToMap(kEntryType, kEntryTypeInit);
  entry->PushStringToMap(kEntryName, kEntryNameLynxView);

  return entry;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetInitBackgroundRuntimeEntry(
    const TimestampKey& current_key) {
  static const std::initializer_list<std::string> pick_keys = {kLoadCoreStart,
                                                               kLoadCoreEnd};
  // check is
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // check ready
  static const std::initializer_list<std::string> check_keys = {kLoadCoreEnd};
  bool ready = init_timing_info_.CheckAllKeysExist(check_keys);
  if (!ready) {
    return nullptr;
  }
  // pick timing
  TimingMap entry_map = init_timing_info_.GetSubMap(pick_keys);
  // make entry
  auto entry = entry_map.ToPubMap(false, value_factory_);
  entry->PushStringToMap(kEntryType, kEntryTypeInit);
  entry->PushStringToMap(kEntryName, kEntryNameBackgroundRuntime);

  return entry;
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
  }
  // 1.0 make entry
  auto entry = timing_map.ToPubMap(false, value_factory_);
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
  entry->PushValueToMap(kFrameworkRenderingTiming,
                        std::move(framework_info_value));
  entry->PushValueToMap(kHostPlatformTiming,
                        std::move(host_platform_info_value));
  entry->PushStringToMap(kEntryType, kEntryTypePipeline);
  entry->PushStringToMap(kEntryName, pipeline_origin);

  return entry;
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

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetMetricFcpEntry(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!value_factory_) {
    LOGE(
        "PerformanceObserver. GetMetricFcpEntry failed. The ValueFactory is "
        "empty.")
    return nullptr;
  }
  bool has_update_metrics = false;

  auto it = pipeline_timing_info_.find(load_bundle_pipeline_id_);
  if (it == pipeline_timing_info_.end()) {
    LOGE("TimingInfoNg: fcp must be calculated after loadBundle/reloadBundle.")
    return nullptr;
  }
  // get stop time for all fcp and start timg for lynxFcp
  auto& load_bundle_timing_map = it->second;
  uint64_t fcp_stop_time =
      load_bundle_timing_map.GetTimestamp(kPaintEnd).value_or(0);
  if (fcp_stop_time == 0) {
    LOGE(
        "TimingHandlerNg: loadBundle pipeline has not yet ended when fcp is "
        "calculated.")
    return nullptr;
  }

  /* Calculation formula:
   * lynxFcp = (Re)LoadBundleEntry.paintEnd -
   * (Re)LoadBundleEntry.loadBundleStart fcp = (Re)LoadBundleEntry.paintEnd -
   * InitContainerEntry.prepareTemplateStart totalFcp =
   * (Re)LoadBundleEntry.paintEnd - InitContainerEntry.openTime
   */
  if (metrics_.find(kLynxFCP) == metrics_.end()) {
    // determine use reload or load
    std::string lynx_fcp_start_name;
    auto pipeline_origin =
        pipeline_id_to_origin_map_.find(load_bundle_pipeline_id_);
    if (pipeline_origin != pipeline_id_to_origin_map_.end()) {
      if (pipeline_origin->second == kLoadBundle) {
        lynx_fcp_start_name = kLoadBundleStart;
      } else if (pipeline_origin->second == kReloadBundleFromBts ||
                 pipeline_origin->second == kReloadBundleFromNative) {
        lynx_fcp_start_name = kReloadBundleStart;
      } else {
        LOGE("TimingInfoNg: only loadBundle/reloadBundle could calc fcp.")
      }
    } else {
      LOGE(
          "TimingInfoNg: fcp must be calculated after loadBundle/reloadBundle.")
    }
    uint64_t lynx_fcp_start_time =
        load_bundle_timing_map.GetTimestamp(lynx_fcp_start_name).value_or(0);

    if (lynx_fcp_start_time != 0) {
      has_update_metrics |=
          UpdateMetrics(kLynxFCP, lynx_fcp_start_name, kPaintEnd,
                        lynx_fcp_start_time, fcp_stop_time);
    }
  }
  if (metrics_.find(kFCP) == metrics_.end()) {
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      has_update_metrics |=
          UpdateMetrics(kFCP, kPrepareTemplateStart, kPaintEnd,
                        *prepare_template_start, fcp_stop_time);
    }
  }
  if (metrics_.find(kTotalFCP) == metrics_.end()) {
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      has_update_metrics |= UpdateMetrics(kTotalFCP, kOpenTime, kPaintEnd,
                                          *open_time, fcp_stop_time);
    }
  }

  if (has_update_metrics) {
    const char* keys[] = {kLynxFCP, kFCP, kTotalFCP};
    auto entry = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        entry->PushValueToMap(key, *it->second);
      }
    }
    entry->PushStringToMap(kEntryType, kEntryTypeMetric);
    entry->PushStringToMap(kEntryName, kEntryNameFCP);
    return entry;
  }
  return nullptr;
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

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetMetricFmpEntry(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!value_factory_) {
    LOGE(
        "PerformanceObserver. GetMetricFmpEntry failed. The ValueFactory is "
        "empty.")
    return nullptr;
  }
  bool has_update_metrics = false;

  uint64_t paint_end = 0;
  if (pipeline_id.empty()) {
    // If there's no pipeline ID, check if metrics_[kLynxActualFMP] exists.
    //      If not, exit; if it exists, extract kPaintEnd for calculation.
    auto it = metrics_.find(kLynxActualFMP);
    if (it == metrics_.end() || it->second == nullptr) {
      return nullptr;
    }
    paint_end =
        metrics_[kLynxActualFMP]->GetValueForKey(kEndTimestamp)->Double() *
        1000;
  } else {
    // If there's a pipeline ID, retrieve the timing map associated with the
    // current ID.
    auto it = pipeline_timing_info_.find(pipeline_id);
    if (it == pipeline_timing_info_.end()) {
      return nullptr;
    }
    auto& timing_map = it->second;
    paint_end = timing_map.GetTimestamp(kPaintEnd).value_or(0);
  }
  if (paint_end == 0) {
    LOGE(
        "TimingHandlerNg: loadBundle pipeline has not yet ended when fmp is "
        "calculated.")
    return nullptr;
  }

  /* Calculation formula:
   * lynxActualFmp = PipelineEntry.paintEnd -
   (Re)LoadBundleEntry.loadBundleStart
   * actualFmp = PipelineEntry.paintEnd
                 - InitContainerEntry.prepareTemplateStart
   * totalActualFmp = PipelineEntry.paintEnd - InitContainerEntry.openTime
   */
  if (metrics_.find(kLynxActualFMP) == metrics_.end()) {
    // LynxActualFmp require LoadBundleStart or ReloadBundleStart
    // Since reloadBundle typically follows a loadBundle, we prioritize using
    // kReloadBundleStart
    std::string lynx_actual_fmp_start_name;
    auto pipeline_origin =
        pipeline_id_to_origin_map_.find(load_bundle_pipeline_id_);
    if (pipeline_origin != pipeline_id_to_origin_map_.end()) {
      if (pipeline_origin->second == kLoadBundle) {
        lynx_actual_fmp_start_name = kLoadBundleStart;
      } else if (pipeline_origin->second == kReloadBundleFromBts ||
                 pipeline_origin->second == kReloadBundleFromNative) {
        lynx_actual_fmp_start_name = kReloadBundleStart;
      } else {
        LOGE("TimingInfoNg: only loadBundle/reloadBundle could calc actualFmp.")
      }
    } else {
      LOGE(
          "TimingInfoNg: actualFmp must be calculated after "
          "loadBundle/reloadBundle.")
    }

    auto it = pipeline_timing_info_.find(load_bundle_pipeline_id_);

    if (it != pipeline_timing_info_.end() && lynx_actual_fmp_start_name != "") {
      auto& timing_map = it->second;
      uint64_t start_time =
          timing_map.GetTimestamp(lynx_actual_fmp_start_name).value_or(0);
      if (start_time != 0) {
        has_update_metrics |=
            UpdateMetrics(kLynxActualFMP, lynx_actual_fmp_start_name, kPaintEnd,
                          start_time, paint_end);
      }
    }
  }
  if (metrics_.find(kActualFMP) == metrics_.end()) {
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      has_update_metrics |=
          UpdateMetrics(kActualFMP, kPrepareTemplateStart, kPaintEnd,
                        *prepare_template_start, paint_end);
    }
  }
  if (metrics_.find(kTotalActualFMP) == metrics_.end()) {
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      has_update_metrics |= UpdateMetrics(kTotalActualFMP, kOpenTime, kPaintEnd,
                                          *open_time, paint_end);
    }
  }

  if (has_update_metrics) {
    const char* keys[] = {kLynxActualFMP, kActualFMP, kTotalActualFMP};
    auto entry = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        entry->PushValueToMap(key, *it->second);
      }
    }
    entry->PushStringToMap(kEntryType, kEntryTypeMetric);
    entry->PushStringToMap(kEntryName, kEntryNameActualFMP);
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
