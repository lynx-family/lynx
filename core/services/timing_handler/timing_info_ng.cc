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

std::unique_ptr<lynx::pub::Value> TimingInfoNg::GetMetricFcpEntry(
    const TimestampKey& current_key, const PipelineID& pipeline_id) {
  if (!value_factory_) {
    LOGE(
        "PerformanceObserver. GetMetricFcpEntry failed. The ValueFactory is "
        "empty.")
    return nullptr;
  }
  // check is
  static const std::initializer_list<std::string> pick_keys = {kLoadBundleStart,
                                                               kPaintEnd};
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // get timing map
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  auto& timing_map = it->second;
  // check ready
  static const std::initializer_list<std::string> check_keys = {kPaintEnd};
  bool ready = timing_map.CheckAllKeysExist(pick_keys);
  if (!ready) {
    // When the Pipeline is not a LoadBundlePipeline, kLoadBundleStart will not
    // exist in the timing_map. We need to check if kPaintEnd exists and whether
    // LoadBundleEntry has been sent.
    ready = timing_map.CheckAllKeysExist(check_keys) &&
            !load_bundle_timing_map_.Empty();
    if (!ready) {
      return nullptr;
    }
  }

  bool has_update_metrics = false;
  auto dict = value_factory_->CreateMap();
  auto metric_fcp = value_factory_->CreateMap();
  auto metric_lynx_fcp = value_factory_->CreateMap();
  auto metric_total_fcp = value_factory_->CreateMap();

  auto paint_end = timing_map.GetTimestamp(kPaintEnd);
  auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
  if (load_bundle_start.has_value()) {
    auto lynx_fcp = *paint_end - *load_bundle_start;
    metric_lynx_fcp->PushStringToMap(kName, kLynxFCP);
    metric_lynx_fcp->PushStringToMap(kStartTimestampName, kLoadBundleStart);
    metric_lynx_fcp->PushDoubleToMap(kStartTimestamp,
                                     ConvertUsToDouble(*load_bundle_start));
    metric_lynx_fcp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_lynx_fcp->PushDoubleToMap(kEndTimestamp,
                                     ConvertUsToDouble(*paint_end));
    metric_lynx_fcp->PushDoubleToMap(kDuration, ConvertUsToDouble(lynx_fcp));
    auto result = metrics_.emplace(kLynxFCP, std::move(metric_lynx_fcp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kLynxFCP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kLynxFCP, *metrics_[kLynxFCP]);
    }
  }

  auto prepare_template_start =
      init_timing_info_.GetTimestamp(kPrepareTemplateStart);
  if (prepare_template_start.has_value() && metrics_[kLynxFCP] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxFCP]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    auto fcp = load_bundle_paint_end - *prepare_template_start;
    metric_fcp->PushStringToMap(kName, kFCP);
    metric_fcp->PushStringToMap(kStartTimestampName, kPrepareTemplateStart);
    metric_fcp->PushDoubleToMap(kStartTimestamp,
                                ConvertUsToDouble(*prepare_template_start));
    metric_fcp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_fcp->PushDoubleToMap(kEndTimestamp,
                                ConvertUsToDouble(load_bundle_paint_end));
    metric_fcp->PushDoubleToMap(kDuration, ConvertUsToDouble(fcp));
    auto result = metrics_.emplace(kFCP, std::move(metric_fcp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kFCP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kFCP, *metrics_[kFCP]);
    }
  }

  auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
  if (open_time.has_value() && metrics_[kLynxFCP] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxFCP]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    auto total_fcp = load_bundle_paint_end - *open_time;
    metric_total_fcp->PushStringToMap(kName, kTotalFCP);
    metric_total_fcp->PushStringToMap(kStartTimestampName, kOpenTime);
    metric_total_fcp->PushDoubleToMap(kStartTimestamp,
                                      ConvertUsToDouble(*open_time));
    metric_total_fcp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_total_fcp->PushDoubleToMap(kEndTimestamp,
                                      ConvertUsToDouble(load_bundle_paint_end));
    metric_total_fcp->PushDoubleToMap(kDuration, ConvertUsToDouble(total_fcp));
    auto result = metrics_.emplace(kTotalFCP, std::move(metric_total_fcp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kTotalFCP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kTotalFCP, *metrics_[kTotalFCP]);
    }
  }

  if (has_update_metrics) {
    return dict;
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
  // check is
  static const std::initializer_list<std::string> pick_keys = {
      kLoadBundleStart, kLoadBackgroundEnd, kPaintEnd};
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // get timing map
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  auto& timing_map = it->second;
  // check ready
  static const std::initializer_list<std::string> check_keys = {
      kLoadBackgroundEnd, kPaintEnd};
  bool ready = timing_map.CheckAllKeysExist(pick_keys);
  if (!ready) {
    // When the Pipeline is not a LoadBundlePipeline, kLoadBundleStart will not
    // exist in the timing_map. We need to check if kPaintEnd exists and whether
    // LoadBundleEntry has been sent.
    ready = timing_map.CheckAllKeysExist(check_keys) &&
            !load_bundle_timing_map_.Empty();
    if (!ready) {
      return nullptr;
    }
  }

  bool has_update_metrics = false;
  auto dict = value_factory_->CreateMap();
  auto metric_tti = value_factory_->CreateMap();
  auto metric_lynx_tti = value_factory_->CreateMap();
  auto metric_total_tti = value_factory_->CreateMap();

  auto load_app_end = timing_map.GetTimestamp(kLoadBackgroundEnd);
  auto paint_end = timing_map.GetTimestamp(kPaintEnd);
  auto load_app_end_val = load_app_end.value_or(0);
  auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
  if (load_bundle_start.has_value()) {
    auto lynx_tti = std::max(*paint_end, load_app_end_val) - *load_bundle_start;
    metric_lynx_tti->PushStringToMap(kName, kLynxTTI);
    metric_lynx_tti->PushStringToMap(kStartTimestampName, kLoadBundleStart);
    metric_lynx_tti->PushDoubleToMap(kStartTimestamp,
                                     ConvertUsToDouble(*load_bundle_start));
    if (*paint_end > load_app_end_val) {
      metric_lynx_tti->PushStringToMap(kEndTimestampName, kPaintEnd);
      metric_lynx_tti->PushDoubleToMap(kEndTimestamp,
                                       ConvertUsToDouble(*paint_end));
    } else {
      metric_lynx_tti->PushStringToMap(kEndTimestampName, kLoadBackgroundEnd);
      metric_lynx_tti->PushDoubleToMap(kEndTimestamp,
                                       ConvertUsToDouble(load_app_end_val));
    }
    metric_lynx_tti->PushDoubleToMap(kDuration, ConvertUsToDouble(lynx_tti));
    auto result = metrics_.emplace(kLynxTTI, std::move(metric_lynx_tti));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kLynxTTI, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kLynxTTI, *metrics_[kLynxTTI]);
    }
  }

  auto prepare_template_start =
      init_timing_info_.GetTimestamp(kPrepareTemplateStart);
  if (prepare_template_start.has_value() && metrics_[kLynxTTI] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxTTI]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    auto tti = std::max(load_bundle_paint_end, load_app_end_val) -
               *prepare_template_start;
    metric_tti->PushStringToMap(kName, kTTI);
    metric_tti->PushDoubleToMap(kDuration, tti);
    metric_tti->PushStringToMap(kStartTimestampName, kPrepareTemplateStart);
    metric_tti->PushDoubleToMap(kStartTimestamp,
                                ConvertUsToDouble(*prepare_template_start));
    if (load_bundle_paint_end > load_app_end_val) {
      metric_tti->PushStringToMap(kEndTimestampName, kPaintEnd);
      metric_tti->PushDoubleToMap(kEndTimestamp,
                                  ConvertUsToDouble(load_bundle_paint_end));
    } else {
      metric_tti->PushStringToMap(kEndTimestampName, kLoadBackgroundEnd);
      metric_tti->PushDoubleToMap(kEndTimestamp,
                                  ConvertUsToDouble(load_app_end_val));
    }
    metric_tti->PushDoubleToMap(kDuration, ConvertUsToDouble(tti));
    auto result = metrics_.emplace(kTTI, std::move(metric_tti));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kTTI, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kTTI, *metrics_[kTTI]);
    }
  }

  auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
  if (open_time.has_value() && metrics_[kLynxTTI] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxTTI]->GetValueForKey(kEndTimestamp)->Double() * 1000;
    auto total_tti =
        std::max(load_bundle_paint_end, load_app_end_val) - *open_time;
    metric_total_tti->PushStringToMap(kName, kTotalTTI);
    metric_total_tti->PushStringToMap(kStartTimestampName, kOpenTime);
    metric_total_tti->PushDoubleToMap(kStartTimestamp,
                                      ConvertUsToDouble(*open_time));
    if (load_bundle_paint_end > load_app_end_val) {
      metric_total_tti->PushStringToMap(kEndTimestampName, kPaintEnd);
      metric_total_tti->PushDoubleToMap(
          kEndTimestamp, ConvertUsToDouble(load_bundle_paint_end));
    } else {
      metric_total_tti->PushStringToMap(kEndTimestampName, kLoadBackgroundEnd);
      metric_total_tti->PushDoubleToMap(kEndTimestamp,
                                        ConvertUsToDouble(load_app_end_val));
    }
    metric_total_tti->PushDoubleToMap(kDuration, ConvertUsToDouble(total_tti));
    dict->PushValueToMap(kTotalTTI, *metric_total_tti);
    auto result = metrics_.emplace(kTotalTTI, std::move(metric_total_tti));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kTotalTTI, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kTotalTTI, *metrics_[kTotalTTI]);
    }
  }

  if (has_update_metrics) {
    return dict;
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
  // check is
  static const std::initializer_list<std::string> pick_keys = {kLoadBundleStart,
                                                               kPaintEnd};
  if (std::find(pick_keys.begin(), pick_keys.end(), current_key) ==
      pick_keys.end()) {
    return nullptr;
  }
  // get timing map
  auto it = pipeline_timing_info_.find(pipeline_id);
  if (it == pipeline_timing_info_.end()) {
    return nullptr;
  }
  auto& timing_map = it->second;
  // check ready
  static const std::initializer_list<std::string> check_keys = {kPaintEnd};
  bool ready = timing_map.CheckAllKeysExist(pick_keys);
  if (!ready) {
    // When the Pipeline is not a LoadBundlePipeline, kLoadBundleStart will not
    // exist in the timing_map. We need to check if kPaintEnd exists and whether
    // LoadBundleEntry has been sent.
    ready = timing_map.CheckAllKeysExist(check_keys) &&
            !load_bundle_timing_map_.Empty();
    if (!ready) {
      return nullptr;
    }
  }

  bool has_update_metrics = false;
  auto dict = value_factory_->CreateMap();
  auto metric_fmp = value_factory_->CreateMap();
  auto metric_lynx_fmp = value_factory_->CreateMap();
  auto metric_total_fmp = value_factory_->CreateMap();

  // calculate actual_fmp if needed.
  auto paint_end = timing_map.GetTimestamp(kPaintEnd);
  auto load_bundle_start = timing_map.GetTimestamp(kLoadBundleStart);
  if (!load_bundle_timing_map_.Empty()) {
    load_bundle_start = load_bundle_timing_map_.GetTimestamp(kLoadBundleStart);
  }
  if (load_bundle_start.has_value()) {
    auto lynx_actual_fmp = *paint_end - *load_bundle_start;
    metric_lynx_fmp->PushStringToMap(kName, kLynxActualFMP);
    metric_lynx_fmp->PushDoubleToMap(kDuration, lynx_actual_fmp);
    metric_lynx_fmp->PushStringToMap(kStartTimestampName, kLoadBundleStart);
    metric_lynx_fmp->PushDoubleToMap(kStartTimestamp,
                                     ConvertUsToDouble(*load_bundle_start));
    metric_lynx_fmp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_lynx_fmp->PushDoubleToMap(kEndTimestamp,
                                     ConvertUsToDouble(*paint_end));
    metric_lynx_fmp->PushDoubleToMap(kDuration,
                                     ConvertUsToDouble(lynx_actual_fmp));
    auto result = metrics_.emplace(kLynxActualFMP, std::move(metric_lynx_fmp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kLynxActualFMP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kLynxActualFMP, *metrics_[kLynxActualFMP]);
    }
  }

  auto prepare_template_start =
      init_timing_info_.GetTimestamp(kPrepareTemplateStart);
  if (prepare_template_start.has_value() &&
      metrics_[kLynxActualFMP] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxActualFMP]->GetValueForKey(kEndTimestamp)->Double() *
        1000;
    auto actual_fmp = load_bundle_paint_end - *prepare_template_start;
    metric_fmp->PushStringToMap(kName, kActualFMP);
    metric_fmp->PushStringToMap(kStartTimestampName, kPrepareTemplateStart);
    metric_fmp->PushDoubleToMap(kStartTimestamp,
                                ConvertUsToDouble(*prepare_template_start));
    metric_fmp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_fmp->PushDoubleToMap(kEndTimestamp,
                                ConvertUsToDouble(load_bundle_paint_end));
    metric_fmp->PushDoubleToMap(kDuration, ConvertUsToDouble(actual_fmp));
    auto result = metrics_.emplace(kActualFMP, std::move(metric_fmp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kActualFMP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kActualFMP, *metrics_[kActualFMP]);
    }
  }

  auto open_time = init_timing_info_.GetTimestamp(kOpenTime);
  if (open_time.has_value() && metrics_[kLynxActualFMP] != nullptr) {
    uint64_t load_bundle_paint_end =
        metrics_[kLynxActualFMP]->GetValueForKey(kEndTimestamp)->Double() *
        1000;
    auto total_actual_fmp = load_bundle_paint_end - *open_time;
    metric_total_fmp->PushStringToMap(kName, kTotalActualFMP);
    metric_total_fmp->PushStringToMap(kStartTimestampName, kOpenTime);
    metric_total_fmp->PushDoubleToMap(kStartTimestamp,
                                      ConvertUsToDouble(*open_time));
    metric_total_fmp->PushStringToMap(kEndTimestampName, kPaintEnd);
    metric_total_fmp->PushDoubleToMap(kEndTimestamp,
                                      ConvertUsToDouble(load_bundle_paint_end));
    metric_total_fmp->PushDoubleToMap(kDuration,
                                      ConvertUsToDouble(total_actual_fmp));
    auto result =
        metrics_.emplace(kTotalActualFMP, std::move(metric_total_fmp));
    if (result.second) {
      has_update_metrics = true;
      dict->PushValueToMap(kTotalActualFMP, *metrics_[result.first->first]);
    } else {
      dict->PushValueToMap(kTotalActualFMP, *metrics_[kTotalActualFMP]);
    }
  }

  if (has_update_metrics) {
    return dict;
  }
  return nullptr;
}
}  // namespace timing
}  // namespace tasm
}  // namespace lynx
