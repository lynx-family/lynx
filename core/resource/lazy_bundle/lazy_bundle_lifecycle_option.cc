// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/lazy_bundle_lifecycle_option.h"

#include <utility>

#include "core/renderer/template_assembler.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {

LazyBundleLifecycleOption::LazyBundleLifecycleOption(const std::string& url,
                                                     int instance_id)
    : component_url(url), instance_id(instance_id) {
  // to prevent from reading Env too frequently
  static bool enable_report_event =
      lynx::tasm::LynxEnv::GetInstance().GetBoolEnv(
          lynx::tasm::LynxEnv::Key::ENABLE_REPORT_DYNAMIC_COMPONENT_EVENT,
          enable_report_event_);
  enable_report_event_ = enable_report_event;
};

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

LazyBundleLifecycleOption::~LazyBundleLifecycleOption() {
  if (!enable_report_event_) {
    return;
  }
  // mode is cache means that this lifecycle did not really send a request, so
  // do not report event in this case
  if (mode == LazyBundleState::STATE_CACHE) {
    return;
  }
  // do some reporter.
  report::EventTracker::OnEvent(
      [component_url = std::move(component_url), mode = mode,
       is_success = is_success, binary_size = (double)binary_size, sync = sync,
       decode_time = (double)(end_decode_time - start_decode_time),
       require_time = (double)(end_require_time - start_require_time)](
          report::MoveOnlyEvent& event) {
        event.SetName("lynxsdk_lazy_bundle_timing");
        event.SetProps("component_url", component_url);
        event.SetProps("mode", lazy_bundle::GenerateModeInfo(mode));
        event.SetProps("is_success", is_success);
        event.SetProps("size", binary_size);
        event.SetProps("sync", sync);
        event.SetProps("decode_time", decode_time);
        event.SetProps("require_time", require_time);
      });
}

void LazyBundleLifecycleOption::SyncOption(
    const LazyBundleLifecycleOption& option) {
  start_require_time = option.start_require_time;
}

}  // namespace tasm
}  // namespace lynx
