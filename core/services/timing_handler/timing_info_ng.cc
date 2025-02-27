// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_info_ng.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/include/log/logging.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/services/timing_handler/timing_utils.h"

namespace lynx {
namespace tasm {
namespace timing {

void TimingInfoNg::ClearAllTimingInfo() { pipeline_timing_info_.clear(); }

bool TimingInfoNg::SetFrameworkTiming(
    const lynx::tasm::timing::TimestampKey& timing_key,
    lynx::tasm::timing::TimestampUs us_timestamp,
    const lynx::tasm::PipelineID& pipeline_id) {
  return framework_timing_info_[pipeline_id].SetTimestamp(timing_key,
                                                          us_timestamp);
}

bool TimingInfoNg::SetTimingWithTimingFlag(
    const tasm::timing::TimingFlag& timing_flag,
    const std::string& timestamp_key,
    const tasm::timing::TimestampUs timestamp) {
  return timing_infos_with_timing_flag_[timing_flag].SetTimestamp(timestamp_key,
                                                                  timestamp);
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
  return entry;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetLoadBundleEntry(
    const TimestampKey& current_key,
    const lynx::tasm::PipelineID& pipeline_id) {
  static const std::initializer_list<std::string> pick_keys = {
      kLoadBundleStart, kLoadBundleEnd,       kParseStart,
      kParseEnd,        kLoadBackgroundStart, kLoadBackgroundEnd};
  // check is and get base pipeline entry
  std::unique_ptr<lynx::pub::Value> pipeline_entry = nullptr;
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    // not a key of loadBundleEntry, try to get base entry
    pipeline_entry = GetPipelineEntry(current_key, pipeline_id);
  } else {
    // is a key of loadBundleEntry, just get a base entry
    pipeline_entry = GetPipelineEntry(kPipelineStart, pipeline_id);
  }
  if (pipeline_entry == nullptr) {
    return nullptr;
  }
  // get timing map
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  const auto& timing_map = it->second;
  // check ready
  static const std::initializer_list<std::string> check_keys = {
      kLoadBundleEnd, kLoadBackgroundEnd};
  bool ready = timing_map.CheckAllKeysExist(check_keys);
  if (!ready) {
    return nullptr;
  }
  // pick timing
  TimingMap load_bundle_map = timing_map.GetSubMap(pick_keys);
  // store load_bundle_timing_infos_ for metric calc
  load_bundle_timing_map_ = load_bundle_map;

  auto load_bundle_entry = load_bundle_map.ToPubMap(false, value_factory_);
  (*pipeline_entry)
      .ForeachMap([&load_bundle_entry](const lynx::pub::Value& key,
                                       const lynx::pub::Value& value) {
        if (value.IsUInt64()) {
          (*load_bundle_entry).PushUInt64ToMap(key.str(), value.UInt64());
        } else if (value.IsDouble()) {
          (*load_bundle_entry).PushDoubleToMap(key.str(), value.Double());
        } else {
          (*load_bundle_entry).PushValueToMap(key.str(), value);
        }
      });

  return load_bundle_entry;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetPipelineEntry(
    const TimestampKey& current_key, const lynx::tasm::PipelineID& pipeline_id,
    const tasm::timing::TimingFlag& timing_flag) {
  static const std::initializer_list<std::string> pick_keys = {
      kPipelineStart,
      kPipelineEnd,
      kMtsRenderStart,
      kMtsRenderEnd,
      kResolveStart,
      kResolveEnd,
      kLayoutStart,
      kLayoutEnd,
      kPaintingUiOperationExecuteStart,
      kPaintingUiOperationExecuteEnd,
      kLayoutUiOperationExecuteStart,
      kLayoutUiOperationExecuteEnd,
      kPaintEnd};
  // check is
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // get timing mao
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  const auto& timing_map = it->second;
  // check ready
  static const std::initializer_list<std::string> check_keys = {
      kPaintEnd, kLayoutEnd, kLayoutUiOperationExecuteEnd, kPipelineEnd};
  bool ready = timing_map.CheckAllKeysExist(check_keys);
  if (!ready) {
    return nullptr;
  }
  // pick timing
  TimingMap pipeline_map = timing_map.GetSubMap(pick_keys);
  // make entry
  auto entry = pipeline_map.ToPubMap(false, value_factory_);
  auto flag_iter = timing_infos_with_timing_flag_.find(timing_flag);
  if (flag_iter != timing_infos_with_timing_flag_.end()) {
    entry->PushDoubleToMap(
        kMtsRenderStart,
        ConvertUsToDouble(
            flag_iter->second.GetTimestamp(kUpdateDiffVdomStart).value_or(0)));
    entry->PushDoubleToMap(
        kMtsRenderEnd,
        ConvertUsToDouble(
            flag_iter->second.GetTimestamp(kUpdateDiffVdomEnd).value_or(0)));
    entry->PushDoubleToMap(
        kPipelineStart,
        ConvertUsToDouble(flag_iter->second.GetTimestamp(kUpdateSetStateTrigger)
                              .value_or(0)));
  }
  // merge framework
  it = framework_timing_info_.find(pipeline_id);
  if (it != framework_timing_info_.end()) {
    // framework-pipeline may don't have item.
    const auto& framework_infos = it->second;
    auto framework_value = framework_infos.ToPubMap(false, value_factory_);
    (*entry).PushValueToMap(kFrameworkPipelineTiming,
                            std::move(framework_value));
  }
  return entry;
}

bool TimingInfoNg::UpdateMetrics(const std::string& name,
                                 const std::string& start_name,
                                 const std::string& end_name,
                                 uint64_t start_time, uint64_t end_time) {
  auto duration = end_time - start_time;
  auto metric_map = value_factory_->CreateMap();
  metric_map->PushStringToMap(kName, name);
  metric_map->PushStringToMap(kStartTimestampName, start_name);
  metric_map->PushDoubleToMap(kStartTimestamp, ConvertUsToDouble(start_time));
  metric_map->PushStringToMap(kEndTimestampName, end_name);
  metric_map->PushDoubleToMap(kEndTimestamp, ConvertUsToDouble(end_time));
  metric_map->PushDoubleToMap(kDuration, ConvertUsToDouble(duration));
  auto result = metrics_.emplace(name, std::move(metric_map));
  if (result.second) {
    return true;
  }
  return false;
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

  // If there's a pipeline ID, retrieve the timing map associated with the
  // current ID. If there's no pipeline ID, check if metrics_[kLynxFCP] exists.
  //      If not, exit; if it exists, extract kPaintEnd for calculation.
  if (pipeline_id.empty()) {
    // check ready
    static const std::initializer_list<std::string> check_keys = {
        kPrepareTemplateStart, kOpenTime};
    if (std::find(check_keys.begin(), check_keys.end(), current_key) ==
        check_keys.end()) {
      return nullptr;
    }
    auto it = metrics_.find(kLynxFCP);
    if (it == metrics_.end() || it->second == nullptr) {
      return nullptr;
    }

    // calcualate fcp and totalFcp
    uint64_t load_bundle_paint_end =
        metrics_[kLynxFCP]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    if (current_key == kPrepareTemplateStart &&
        metrics_.find(kFCP) == metrics_.end()) {
      has_update_metrics |=
          UpdateMetrics(kFCP, kPrepareTemplateStart, kPaintEnd,
                        *init_timing_info_.GetTimestamp(kPrepareTemplateStart),
                        load_bundle_paint_end);
    }
    if (current_key == kOpenTime &&
        metrics_.find(kTotalFCP) == metrics_.end()) {
      has_update_metrics |= UpdateMetrics(
          kTotalFCP, kOpenTime, kPaintEnd,
          *init_timing_info_.GetTimestamp(kOpenTime), load_bundle_paint_end);
    }
  } else {
    // check ready
    static const std::initializer_list<std::string> check_keys = {kPaintEnd};
    if (std::find(check_keys.begin(), check_keys.end(), current_key) ==
        check_keys.end()) {
      return nullptr;
    }
    auto it = pipeline_timing_info_.find(pipeline_id);
    if (it == pipeline_timing_info_.end()) {
      return nullptr;
    }
    auto& timing_map = it->second;
    auto paint_end = timing_map.GetTimestamp(kPaintEnd);
    auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
    if (!paint_end.has_value() || !load_bundle_start.has_value()) {
      return nullptr;
    }

    has_update_metrics |= UpdateMetrics(kLynxFCP, kLoadBundleStart, kPaintEnd,
                                        *load_bundle_start, *paint_end);

    // try to calc more metrics
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      has_update_metrics |=
          UpdateMetrics(kFCP, kPrepareTemplateStart, kPaintEnd,
                        *prepare_template_start, *paint_end);
    }
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      has_update_metrics |= UpdateMetrics(kTotalFCP, kOpenTime, kPaintEnd,
                                          *open_time, *paint_end);
    }
  }

  if (has_update_metrics) {
    std::string keys[] = {kLynxFCP, kFCP, kTotalFCP};
    auto result_dict = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        result_dict->PushValueToMap(key, *it->second);
      }
    }
    return result_dict;
  }
  return nullptr;
}

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetMetricTtiEntry(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!value_factory_) {
    LOGE(
        "PerformanceObserver. GetMetricTtiEntry failed. The ValueFactory is "
        "empty.")
    return nullptr;
  }
  bool has_update_metrics = false;

  // If there's a pipeline ID, retrieve the timing map associated with the
  // current ID. If there's no pipeline ID, check if metrics_[kLynxFCP] exists.
  //      If not, exit; if it exists, extract kPaintEnd for calculation.
  if (pipeline_id.empty()) {
    // check ready
    static const std::initializer_list<std::string> pick_keys = {
        kPrepareTemplateStart, kOpenTime};
    if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
        pick_keys.end()) {
      return nullptr;
    }
    auto it = metrics_.find(kLynxTTI);
    if (it == metrics_.end() || it->second == nullptr) {
      return nullptr;
    }

    // calculate tti and totalTti
    uint64_t load_bundle_paint_end_val =
        metrics_[kLynxTTI]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    uint64_t load_background_end_val =
        load_bundle_timing_map_.GetTimestamp(kLoadBackgroundEnd).value_or(0);
    if (current_key == kPrepareTemplateStart &&
        metrics_.find(kTTI) == metrics_.end()) {
      if (load_bundle_paint_end_val < load_background_end_val) {
        has_update_metrics |= UpdateMetrics(
            kTTI, kPrepareTemplateStart, kPaintEnd,
            *init_timing_info_.GetTimestamp(kPrepareTemplateStart),
            load_bundle_paint_end_val);
      } else {
        has_update_metrics |= UpdateMetrics(
            kTTI, kPrepareTemplateStart, kLoadBackgroundEnd,
            *init_timing_info_.GetTimestamp(kPrepareTemplateStart),
            load_background_end_val);
      }
    }
    if (current_key == kOpenTime &&
        metrics_.find(kTotalTTI) == metrics_.end()) {
      if (load_bundle_paint_end_val < load_background_end_val) {
        has_update_metrics |=
            UpdateMetrics(kTotalTTI, kOpenTime, kPaintEnd,
                          *init_timing_info_.GetTimestamp(kOpenTime),
                          load_bundle_paint_end_val);
      } else {
        has_update_metrics |=
            UpdateMetrics(kTotalTTI, kOpenTime, kLoadBackgroundEnd,
                          *init_timing_info_.GetTimestamp(kOpenTime),
                          load_background_end_val);
      }
    }
  } else {
    // check ready
    static const std::initializer_list<std::string> pick_keys = {kPaintEnd};
    if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
        pick_keys.end()) {
      return nullptr;
    }
    auto it = pipeline_timing_info_.find(pipeline_id);
    if (it == pipeline_timing_info_.end()) {
      return nullptr;
    }
    auto& timing_map = it->second;
    auto paint_end = timing_map.GetTimestamp(kPaintEnd);
    auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
    if (!paint_end.has_value() || !load_bundle_start.has_value()) {
      return nullptr;
    }

    // calculate lynxTti
    uint64_t load_background_end_val =
        timing_map.GetTimestamp(kLoadBackgroundEnd).value_or(0);
    if (*paint_end < load_background_end_val) {
      has_update_metrics |= UpdateMetrics(kLynxTTI, kLoadBundleStart, kPaintEnd,
                                          *load_bundle_start, *paint_end);
    } else {
      has_update_metrics |=
          UpdateMetrics(kLynxTTI, kLoadBundleStart, kLoadBackgroundEnd,
                        *load_bundle_start, load_background_end_val);
    }

    // try to calc tti and totalTti
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      if (*paint_end < load_background_end_val) {
        has_update_metrics |=
            UpdateMetrics(kTTI, kPrepareTemplateStart, kPaintEnd,
                          *prepare_template_start, *paint_end);
      } else {
        has_update_metrics |=
            UpdateMetrics(kTTI, kPrepareTemplateStart, kLoadBackgroundEnd,
                          *prepare_template_start, load_background_end_val);
      }
    }
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      if (*paint_end < load_background_end_val) {
        has_update_metrics |= UpdateMetrics(kTotalTTI, kOpenTime, kPaintEnd,
                                            *open_time, *paint_end);
      } else {
        has_update_metrics |=
            UpdateMetrics(kTotalTTI, kOpenTime, kLoadBackgroundEnd, *open_time,
                          load_background_end_val);
      }
    }
  }

  if (has_update_metrics) {
    std::string keys[] = {kLynxTTI, kTTI, kTotalTTI};
    auto result_dict = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        result_dict->PushValueToMap(key, *it->second);
      }
    }
    return result_dict;
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

  // If there's a pipeline ID, retrieve the timing map associated with the
  // current ID. If there's no pipeline ID, check if metrics_[kLynxFCP] exists.
  //      If not, exit; if it exists, extract kPaintEnd for calculation.
  if (pipeline_id.empty()) {
    // check ready
    static const std::initializer_list<std::string> check_keys = {
        kPrepareTemplateStart, kOpenTime};
    if (std::find(check_keys.begin(), check_keys.end(), current_key) ==
        check_keys.end()) {
      return nullptr;
    }
    auto it = metrics_.find(kLynxActualFMP);
    if (it == metrics_.end() || it->second == nullptr) {
      return nullptr;
    }

    // calcualate fcp and totalFcp
    uint64_t paint_end =
        metrics_[kLynxActualFMP]->GetValueForKey(kEndTimestamp)->Double() *
        1000;
    if (current_key == kPrepareTemplateStart &&
        metrics_.find(kActualFMP) == metrics_.end()) {
      has_update_metrics |= UpdateMetrics(
          kActualFMP, kPrepareTemplateStart, kPaintEnd,
          *init_timing_info_.GetTimestamp(kPrepareTemplateStart), paint_end);
    }
    if (current_key == kOpenTime &&
        metrics_.find(kTotalActualFMP) == metrics_.end()) {
      has_update_metrics |=
          UpdateMetrics(kTotalActualFMP, kOpenTime, kPaintEnd,
                        *init_timing_info_.GetTimestamp(kOpenTime), paint_end);
    }
  } else {
    // check ready
    static const std::initializer_list<std::string> check_keys = {kPaintEnd};
    if (std::find(check_keys.begin(), check_keys.end(), current_key) ==
        check_keys.end()) {
      return nullptr;
    }
    auto it = pipeline_timing_info_.find(pipeline_id);
    if (it == pipeline_timing_info_.end()) {
      return nullptr;
    }
    auto& timing_map = it->second;
    auto paint_end = timing_map.GetTimestamp(kPaintEnd);
    auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
    if (!load_bundle_timing_map_.Empty()) {
      load_bundle_start =
          load_bundle_timing_map_.GetTimestamp(kLoadBundleStart);
    }
    if (!paint_end.has_value() || !load_bundle_start.has_value()) {
      return nullptr;
    }

    has_update_metrics |=
        UpdateMetrics(kLynxActualFMP, kLoadBundleStart, kPaintEnd,
                      *load_bundle_start, *paint_end);

    // try to calc more metrics
    auto prepare_template_start =
        init_timing_info_.GetTimestamp(kPrepareTemplateStart);
    if (prepare_template_start.has_value()) {
      has_update_metrics |=
          UpdateMetrics(kActualFMP, kPrepareTemplateStart, kPaintEnd,
                        *prepare_template_start, *paint_end);
    }
    auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
    if (open_time.has_value()) {
      has_update_metrics |= UpdateMetrics(kTotalActualFMP, kOpenTime, kPaintEnd,
                                          *open_time, *paint_end);
    }
  }

  if (has_update_metrics) {
    std::string keys[] = {kLynxActualFMP, kActualFMP, kTotalActualFMP};
    auto result_dict = value_factory_->CreateMap();
    for (const auto& key : keys) {
      auto it = metrics_.find(key);
      if (it != metrics_.end() && it->second != nullptr) {
        result_dict->PushValueToMap(key, *it->second);
      }
    }
    return result_dict;
  }
  return nullptr;
}
}  // namespace timing
}  // namespace tasm
}  // namespace lynx
