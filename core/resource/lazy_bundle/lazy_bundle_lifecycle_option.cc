// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"

#include <utility>

#include "base/include/lynx_actor.h"
#include "core/renderer/template_assembler.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/performance_controller.h"
#include "core/services/timing_handler/timing_constants.h"

namespace lynx {
namespace tasm {

LazyBundleLifecycleOption::LazyBundleLifecycleOption(
    const std::string& url, int instance_id, bool enable_event_reporter,
    LazyBundleEntryOrigin entry_origin)
    : component_url(url),
      instance_id(instance_id),
      entry_origin_(entry_origin){};

void LazyBundleLifecycleOption::SetPerfControllerActor(
    std::weak_ptr<shell::LynxActor<performance::PerformanceController>> actor) {
  perf_controller_actor_ = std::move(actor);
}

void LazyBundleLifecycleOption::RecordRequireStart(int64_t timestamp) {
  start_require_time = timestamp;
}

void LazyBundleLifecycleOption::RecordRequireEnd(int64_t timestamp,
                                                 bool is_sync,
                                                 int64_t resource_size) {
  end_require_time = timestamp;
  sync = is_sync;
  binary_size = resource_size;
}

void LazyBundleLifecycleOption::RecordDecodeStart(int64_t timestamp) {
  start_decode_time = timestamp;
}

void LazyBundleLifecycleOption::RecordDecodeEnd(int64_t timestamp) {
  end_decode_time = timestamp;
}

void LazyBundleLifecycleOption::SetLoadResult(bool success) {
  is_success = success;
  if (mode != LazyBundleState::STATE_CACHE &&
      mode != LazyBundleState::STATE_PRELOAD) {
    mode =
        success ? LazyBundleState::STATE_SUCCESS : LazyBundleState::STATE_FAIL;
  }
}

void LazyBundleLifecycleOption::MarkMTSLazyBundleUrl() {
  if (entry_origin_ != LazyBundleEntryOrigin::kMTS) {
    return;
  }
  auto perf_controller_actor = perf_controller_actor_.lock();
  if (!perf_controller_actor) {
    return;
  }
  perf_controller_actor->ActAsync(
      [url = component_url](auto& performance) mutable {
        performance->MarkMTSLazyBundleUrl(url);
      });
}

void LazyBundleLifecycleOption::ReportLazyBundleEntry() {
  if (lazy_bundle_entry_reported_) {
    return;
  }
  lazy_bundle_entry_reported_ = true;

  auto perf_controller_actor = perf_controller_actor_.lock();
  if (!perf_controller_actor) {
    return;
  }
  auto lazy_bundle_entry = GetLazyBundleEntry();
  if (!lazy_bundle_entry) {
    return;
  }
  perf_controller_actor->ActAsync(
      [url = component_url, entry_origin = entry_origin_,
       entry = std::move(lazy_bundle_entry)](auto& performance) mutable {
        if (entry_origin == LazyBundleEntryOrigin::kMTS) {
          performance->OnMTSLazyBundlePerformanceEvent(
              url, std::move(entry), tasm::performance::kEventTypeAll);
        } else {
          performance->OnBTSLazyBundlePerformanceEvent(
              url, std::move(entry), tasm::performance::kEventTypeAll);
        }
      });
}

bool LazyBundleLifecycleOption::HandleLoadFailure(TemplateAssembler* tasm) {
  if (sync && component_instance) {
    component_instance->SetLazyBundleState(LazyBundleState::STATE_FAIL,
                                           message);
    return false;
  }

  // Asynchronous mode.
  int impl_id = 0;
  bool need_dispatch =
      tasm->page_proxy()->OnLazyBundleLoadedFailed(component_uid, impl_id);

  // If asynchronous loading fails, there is no opportunity to send the bind
  // event during the normal component lifecycle. Therefore, the bind event must
  // be sent here.
  tasm->SendLazyBundleBindEvent(component_url, lazy_bundle::kEventFail, message,
                                impl_id);

  return need_dispatch;
}

bool LazyBundleLifecycleOption::HandleLoadSuccess(TemplateAssembler* tasm) {
  if (sync && component_instance) {
    component_instance->SetLazyBundleState(LazyBundleState::STATE_SUCCESS,
                                           message);
    tasm->OnLazyBundlePerfReady(GetPerfEventMessage());
    return false;
  }

  // Asynchronous mode.
  int impl_id;
  bool need_dispatch = tasm->page_proxy()->OnLazyBundleLoadedSuccess(
      tasm, component_url, component_uid, impl_id);

  // If loading is asynchronous, trigger the bind event immediately.
  tasm->SendLazyBundleBindEvent(component_url, lazy_bundle::kEventSuccess,
                                message, impl_id);

  tasm->OnLazyBundlePerfReady(GetPerfEventMessage());
  return need_dispatch;
}

bool LazyBundleLifecycleOption::OnLazyBundleLifecycleEnd(
    TemplateAssembler* tasm) {
  if (enable_fiber_arch) {
    tasm->TriggerLepusClosure(callback, message);

    // No need to trigger dispatch anymore; simply returning false is
    // sufficient.
    return false;
  }

  return is_success ? HandleLoadSuccess(tasm) : HandleLoadFailure(tasm);
}

/**
 * construct perf event message:
 * -url
 *   |-perf_info
 */
lepus::Value LazyBundleLifecycleOption::GetPerfEventMessage() {
  auto perf_value = lepus::Dictionary::Create();
  perf_value->SetValue(component_url, GetPerfInfo());
  return lepus::Value(std::move(perf_value));
}

/**
 * construct perf info:
 * |-sync: bool
 * |-sync_require: bool (compatible with old formats)
 * |-size: int
 * |-decode_time: string
 * |-require_time: string
 * |-timing
 *   |-decode_start_time: int
 *   |-decode_end_time: int
 *   |-require_start_time: int
 *   |- require_end_time: int
 */
lepus::Value LazyBundleLifecycleOption::GetPerfInfo() {
  if (perf_info_.IsNil()) {
    constexpr const static char kSyncRequire[] = "sync_require";
    constexpr const static char kSize[] = "size";
    constexpr const static char kDecodeTime[] = "decode_time";
    constexpr const static char kRequireTime[] = "require_time";
    constexpr const static char kTiming[] = "timing";
    constexpr const static char kDecodeStartTime[] = "decode_start_time";
    constexpr const static char kDecodeEndTime[] = "decode_end_time";
    constexpr const static char kRequireStartTime[] = "require_start_time";
    constexpr const static char kRequireEndTime[] = "require_end_time";

    auto perf_info_dict = lepus::Dictionary::Create();
    perf_info_dict->SetValue(BASE_STATIC_STRING(kSyncRequire), sync);
    perf_info_dict->SetValue(BASE_STATIC_STRING(lazy_bundle::kSync), sync);
    perf_info_dict->SetValue(BASE_STATIC_STRING(kSize), binary_size);
    perf_info_dict->SetValue(BASE_STATIC_STRING(kRequireTime),
                             end_require_time - start_require_time);
    perf_info_dict->SetValue(BASE_STATIC_STRING(kDecodeTime),
                             end_decode_time - start_decode_time);

    auto perf_timing_info = lepus::Dictionary::Create();
    perf_timing_info->SetValue(BASE_STATIC_STRING(kDecodeStartTime),
                               start_decode_time);
    perf_timing_info->SetValue(BASE_STATIC_STRING(kDecodeEndTime),
                               end_decode_time);
    perf_timing_info->SetValue(BASE_STATIC_STRING(kRequireStartTime),
                               start_require_time);
    perf_timing_info->SetValue(BASE_STATIC_STRING(kRequireEndTime),
                               end_require_time);
    perf_info_dict->SetValue(BASE_STATIC_STRING(kTiming),
                             std::move(perf_timing_info));

    perf_info_ = lepus::Value(perf_info_dict);
  }
  return perf_info_;
}

std::unique_ptr<pub::Value> LazyBundleLifecycleOption::GetLazyBundleEntry() {
  if (mode == LazyBundleState::STATE_CACHE) {
    return nullptr;
  }

  constexpr const static char kTypeResource[] = "resource";
  constexpr const static char kNameLazyBundle[] = "lazyBundle";
  constexpr const static char kComponentUrl[] = "componentUrl";
  constexpr const static char kSize[] = "size";
  constexpr const static char kMode[] = "mode";
  constexpr const static char kLoadSuccess[] = "loadSuccess";
  constexpr const static char kDecodeStartTime[] = "decodeStart";
  constexpr const static char kDecodeEndTime[] = "decodeEnd";
  constexpr const static char kRequireStartTime[] = "requireStart";
  constexpr const static char kRequireEndTime[] = "requireEnd";

  auto performance_entry = value_factory_->CreateMap();
  performance_entry->PushStringToMap(tasm::timing::kEntryType, kTypeResource);
  performance_entry->PushStringToMap(tasm::timing::kEntryName, kNameLazyBundle);
  performance_entry->PushStringToMap(kComponentUrl, component_url);
  performance_entry->PushInt64ToMap(kSize, binary_size);
  performance_entry->PushBoolToMap(kLoadSuccess, is_success);
  performance_entry->PushStringToMap(kMode,
                                     lazy_bundle::GenerateModeInfo(mode));
  performance_entry->PushUInt64ToMap(kDecodeStartTime, start_decode_time);
  performance_entry->PushUInt64ToMap(kDecodeEndTime, end_decode_time);
  performance_entry->PushUInt64ToMap(kRequireStartTime, start_require_time);
  performance_entry->PushUInt64ToMap(kRequireEndTime, end_require_time);

  return performance_entry;
}

void LazyBundleLifecycleOption::SyncOption(
    const LazyBundleLifecycleOption& option) {
  start_require_time = option.start_require_time;
}

}  // namespace tasm
}  // namespace lynx
