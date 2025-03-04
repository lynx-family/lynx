// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_info.h"

#include <algorithm>

#include "base/include/string/string_utils.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {

// Timing Setter
void TimingInfo::SetInitTiming(const TimestampKey& timing_key,
                               TimestampUs us_timestamp) {
  init_timing_infos_.SetTimestamp(timing_key, us_timestamp);
}

void TimingInfo::SetExtraTiming(const TimestampKey& timing_key,
                                TimestampUs us_timestamp) {
  extra_timing_infos_.SetTimestamp(timing_key, us_timestamp);
}

void TimingInfo::SetTimingWithTimingFlag(
    const tasm::timing::TimingFlag& timing_flag,
    const std::string& timestamp_key, tasm::timing::TimestampUs timestamp) {
  timing_infos_with_timing_flag_[timing_flag].SetTimestamp(timestamp_key,
                                                           timestamp);
}

void TimingInfo::SetPipelineOrSSRTiming(const TimestampKey& timing_key,
                                        TimestampUs us_timestamp,
                                        const PipelineID& pipeline_id) {
  if (!use_ssr_) {
    pipeline_timing_infos_[pipeline_id].SetTimestamp(timing_key, us_timestamp);
  } else {
    SetSSRSetupTiming(timing_key, us_timestamp, pipeline_id);
  }
}

void TimingInfo::SetPipelineTiming(const TimestampKey& timing_key,
                                   TimestampUs us_timestamp,
                                   const PipelineID& pipeline_id) {
  pipeline_timing_infos_[pipeline_id].SetTimestamp(timing_key, us_timestamp);
}

void TimingInfo::SetSSRSetupTiming(const std::string& timing_key,
                                   TimestampUs us_timestamp,
                                   const PipelineID& pipeline_id) {
  // TODO(kechenglong): should find a better way to set SSR timing data?
  static const base::NoDestructor<std::unordered_set<TimestampKey>>
      ssr_reused_keys{{kPaintEndPolyfill,
                       kPaintingUiOperationExecuteStartPolyfill,
                       kLayoutUiOperationExecuteEndPolyfill,
                       kLayoutStartPolyfill, kLayoutEndPolyfill}};
  if (lynx::base::EndsWith(timing_key, kSSRSuffix)) {
    ssr_setup_timing_infos_.SetTimestamp(timing_key, us_timestamp);
    return;
  }
  // If no SSR suffix, set the timing info in the standard pipeline timings.
  pipeline_timing_infos_[pipeline_id].SetTimestamp(timing_key, us_timestamp);
  // If the key is in the set of reused keys, append the SSR suffix and set the
  // SSR timing info.
  if (ssr_reused_keys->find(timing_key) != ssr_reused_keys->end()) {
    ssr_setup_timing_infos_.SetTimestamp(timing_key + kSSRSuffix, us_timestamp);
  }
}

// Timing Getter
std::unique_ptr<lynx::pub::Value> TimingInfo::GetUpdateTimingInfoInner(
    const std::string& update_flag, bool as_milliseconds) const {
  if (!value_factory_) {
    LOGE("GetUpdateTimingInfoInner failed. The ValueFactory is empty");
    return nullptr;
  }
  auto dict = value_factory_->CreateMap();
  auto it = update_timing_infos_.find(update_flag);
  if (it == update_timing_infos_.end()) {
    LOGE("The update_flag hasn't been ready to be reported, update_flag: "
         << update_flag);
    return dict;
  }
  const auto& update_timing_infos = it->second;
  dict->PushValueToMap(update_flag, update_timing_infos.ToPubMap(
                                        as_milliseconds, value_factory_));

  return dict;
}

std::unique_ptr<lynx::pub::Value> TimingInfo::GetUpdateTimingInfoAsMicrosecond(
    const std::string& update_flag) const {
  return GetUpdateTimingInfoInner(update_flag, false);
}

std::unique_ptr<lynx::pub::Value> TimingInfo::GetUpdateTimingInfoAsMillisecond(
    const std::string& update_flag) const {
  return GetUpdateTimingInfoInner(update_flag, true);
}

std::unique_ptr<lynx::pub::Value> TimingInfo::GetAllTimingInfoInner(
    bool as_milliseconds) const {
  if (!value_factory_) {
    LOGE("GetAllTimingInfoInner failed. The ValueFactory is empty");
    return nullptr;
  }
  auto dict = value_factory_->CreateMap();
  // SetupTiming
  dict->PushValueToMap(kSetupTiming, setup_timing_infos_.ToPubMap(
                                         as_milliseconds, value_factory_));
  // // ExtraTiming
  dict->PushValueToMap(kExtraTiming, extra_timing_infos_.ToPubMap(
                                         as_milliseconds, value_factory_));
  // // Metrics
  dict->PushValueToMap(kMetrics,
                       metrics_.ToPubMap(as_milliseconds, value_factory_));

  // UpdateTimings
  auto update_dict = value_factory_->CreateMap();
  for (const auto& [timing_flag, update_timing_info] : update_timing_infos_) {
    update_dict->PushValueToMap(
        timing_flag,
        update_timing_info.ToPubMap(as_milliseconds, value_factory_));
  }
  dict->PushValueToMap(kUpdateTimings, std::move(update_dict));

  // SSR Info
  if (use_ssr_) {
    dict->PushValueToMap(
        kSSRMetrics, ssr_metrics_.ToPubMap(as_milliseconds, value_factory_));
    dict->PushValueToMap(kSSRRenderPage, ssr_setup_timing_infos_.ToPubMap(
                                             as_milliseconds, value_factory_));

    auto ssr_extra_info = value_factory_->CreateMap();
    ssr_extra_info->PushStringToMap(kURL, ssr_url_);
    ssr_extra_info->PushUInt64ToMap(kSSRExtraInfoDataSize, ssr_data_size_);
    dict->PushValueToMap(kSSRExtraInfo, std::move(ssr_extra_info));
  }

  // Other Info
  dict->PushStringToMap(kURL, url_);
  dict->PushUInt64ToMap(kThreadStrategy, thread_strategy_);
  dict->PushBoolToMap(kHasReload, has_reload_);
  return dict;
}

std::unique_ptr<lynx::pub::Value> TimingInfo::GetAllTimingInfoAsMicrosecond()
    const {
  return GetAllTimingInfoInner(false);
}

std::unique_ptr<lynx::pub::Value> TimingInfo::GetAllTimingInfoAsMillisecond()
    const {
  return GetAllTimingInfoInner(true);
}

// Check can be dispatched
bool TimingInfo::IsSetupReady(const PipelineID& pipeline_id) const {
  auto it = pipeline_timing_infos_.find(pipeline_id);
  if (it == pipeline_timing_infos_.end()) {
    return false;
  }
  const auto& pipeline_timing_info = it->second;
  if (!enable_js_runtime_) {
    // When JS runtime is not enabled, check draw_end, layout_end,
    // ui_operation_flush_end and load_template_end.
    static const std::initializer_list<std::string> setup_check_keys = {
        kPaintEndPolyfill, kLayoutEndPolyfill,
        kLayoutUiOperationExecuteEndPolyfill, kLoadBundleEndPolyfill};
    return pipeline_timing_info.CheckAllKeysExist(setup_check_keys);
  }
  // When JS runtime is enabled, check draw_end, layout_end,
  // ui_operation_flush_end, load_template_end and load_app_end.
  static const std::initializer_list<std::string> setup_check_keys = {
      kPaintEndPolyfill, kLayoutEndPolyfill,
      kLayoutUiOperationExecuteEndPolyfill, kLoadBundleEndPolyfill,
      kLoadBackgroundEndPolyfill};
  return pipeline_timing_info.CheckAllKeysExist(setup_check_keys);
}

bool TimingInfo::IsUpdateReady(const PipelineID& pipeline_id) const {
  auto it = pipeline_timing_infos_.find(pipeline_id);
  if (it == pipeline_timing_infos_.end()) {
    return false;
  }
  const auto& pipeline_timing_info = it->second;

  // For update, we will check draw_end, layout_end and ui_operation_flush_end
  static const std::initializer_list<std::string> update_check_keys = {
      kPaintEndPolyfill, kLayoutEndPolyfill,
      kLayoutUiOperationExecuteEndPolyfill};
  return pipeline_timing_info.CheckAllKeysExist(update_check_keys);
}

// Prepare before dispatch
void TimingInfo::PrepareBeforeDispatchSetup(const PipelineID& pipeline_id) {
  auto it = pipeline_timing_infos_.find(pipeline_id);
  if (it == pipeline_timing_infos_.end()) {
    return;
  }
  setup_timing_infos_ = it->second;
  // Here we merge init_timing_infos_ into setup_timing_infos_ to
  // prevent breaking the logic.
  setup_timing_infos_.Merge(init_timing_infos_);
  if (use_ssr_) {
    PrepareBeforeDispatchSetupForSSR();
  }
  auto draw_end = setup_timing_infos_.GetTimestamp(kPaintEndPolyfill);
  if (!draw_end.has_value()) {
    return;
  }
  auto load_app_end =
      setup_timing_infos_.GetTimestamp(kLoadBackgroundEndPolyfill);
  auto load_app_end_val = load_app_end.value_or(0);

  auto load_template_start =
      setup_timing_infos_.GetTimestamp(kLoadBundleStartPolyfill);
  if (load_template_start.has_value()) {
    auto lynx_fcp = *draw_end - *load_template_start;
    metrics_.SetTimestamp(kLynxFCPPolyfill, lynx_fcp);
    auto lynx_tti =
        std::max(*draw_end, load_app_end_val) - *load_template_start;
    metrics_.SetTimestamp(kLynxTTIPolyfill, lynx_tti);
  }

  auto prepare_template_start =
      extra_timing_infos_.GetTimestamp(kPrepareTemplateStartPolyfill);
  if (prepare_template_start.has_value()) {
    auto fcp = *draw_end - *prepare_template_start;
    metrics_.SetTimestamp(kFCP, fcp);
    auto tti = std::max(*draw_end, load_app_end_val) - *prepare_template_start;
    metrics_.SetTimestamp(kTTI, tti);
  }

  auto open_time = extra_timing_infos_.GetTimestamp(kOpenTimePolyfill);
  if (open_time.has_value()) {
    auto total_fcp = *draw_end - *open_time;
    metrics_.SetTimestamp(kTotalFCPPolyfill, total_fcp);
    auto total_tti = std::max(*draw_end, load_app_end_val) - *open_time;
    metrics_.SetTimestamp(kTotalTTIPolyfill, total_tti);
  }
}

void TimingInfo::PrepareBeforeDispatchUpdate(const PipelineID& pipeline_id,
                                             const TimingFlag& update_flag) {
  // move update_timing_info from pipeline_timing_infos to
  // update_timing_infos_.
  auto it = pipeline_timing_infos_.find(pipeline_id);
  if (it != pipeline_timing_infos_.end()) {
    update_timing_infos_.emplace(update_flag, it->second);
  } else {
    LOGE("This pipeline_id doesn't exist when prepare for dispatch: "
         << pipeline_id);
    return;
  }

  auto& update_timing_info = update_timing_infos_[update_flag];
  // This logic is to ensure compatibility with the old js_app markTiming API.
  // We have stored the js_app markTiming data with TimingFlag in
  // timing_infos_with_timing_flag_.
  // Here we merge this data to update_timing_info.
  // In the long term, this logic will be deprecated after most of the business
  // front-end frameworks are upgraded.
  auto flag_iter = timing_infos_with_timing_flag_.find(update_flag);
  if (flag_iter != timing_infos_with_timing_flag_.end()) {
    update_timing_info.Merge(flag_iter->second);
    timing_infos_with_timing_flag_.erase(flag_iter);
  }

  if (update_flag != kLynxTimingActualFMPFlag) {
    return;
  }
  // calculate actual_fmp if needed.
  auto actual_fmp_draw_end = update_timing_info.GetTimestamp(kPaintEndPolyfill);
  if (!actual_fmp_draw_end.has_value()) {
    return;
  }

  auto load_template_start =
      setup_timing_infos_.GetTimestamp(kLoadBundleStartPolyfill);
  if (load_template_start.has_value()) {
    auto lynx_actual_fmp = *actual_fmp_draw_end - *load_template_start;
    metrics_.SetTimestamp(kLynxActualFMPPolyfill, lynx_actual_fmp);
  }

  auto prepare_template_start =
      extra_timing_infos_.GetTimestamp(kPrepareTemplateStartPolyfill);
  if (prepare_template_start.has_value()) {
    auto actual_fmp = *actual_fmp_draw_end - *prepare_template_start;
    metrics_.SetTimestamp(kActualFMPPolyfill, actual_fmp);
  }

  auto open_time = extra_timing_infos_.GetTimestamp(kOpenTimePolyfill);
  if (open_time.has_value()) {
    auto total_actual_fmp = *actual_fmp_draw_end - *open_time;
    metrics_.SetTimestamp(kTotalActualFMPPolyfill, total_actual_fmp);
  }
}

void TimingInfo::PrepareBeforeDispatchSetupForSSR() {
  auto render_page_start_ssr =
      ssr_setup_timing_infos_.GetTimestamp(kRenderPageStartSSR);
  auto draw_end_ssr = ssr_setup_timing_infos_.GetTimestamp(kDrawEndSSR);
  auto load_app_end =
      setup_timing_infos_.GetTimestamp(kLoadBackgroundEndPolyfill);
  if (render_page_start_ssr.has_value() && draw_end_ssr.has_value()) {
    auto ssr_lynx_fcp = *draw_end_ssr - *render_page_start_ssr;
    ssr_metrics_.SetTimestamp(kSSRLynxFCP, ssr_lynx_fcp);
    auto ssr_lynx_tti = std::max(*draw_end_ssr, load_app_end.value_or(0)) -
                        *render_page_start_ssr;
    ssr_metrics_.SetTimestamp(kSSRLynxTTI, ssr_lynx_tti);
  }
}

// Reset
void TimingInfo::ClearAllTiming() {
  setup_timing_infos_.Clear();
  update_timing_infos_.clear();
  metrics_.Clear();
  ssr_setup_timing_infos_.Clear();
  ssr_metrics_.Clear();
  timing_infos_with_timing_flag_.clear();
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
