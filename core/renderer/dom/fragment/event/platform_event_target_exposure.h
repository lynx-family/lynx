// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_EXPOSURE_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_EXPOSURE_H_

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "core/value_wrapper/value_impl_lepus.h"

struct CommonAncestorRect {
  int target_count{0};
  bool rect_updated{false};
  float rect[4] = {0, 0, 0, 0};
};

struct ExposureTargetMeta {
  std::string unique_id;
  bool is_custom_event{false};
  bool is_global_event{false};
  bool intercept_global_event{false};
};

namespace lynx::tasm {

class NativePaintingCtxPlatformRef;
class PlatformEventTarget;
class PlatformEventTargetHelper;

class PlatformEventTargetExposure
    : public std::enable_shared_from_this<PlatformEventTargetExposure> {
 public:
  class ExposureTargetDetail {
   public:
    ExposureTargetDetail() = default;
    ExposureTargetDetail(const fml::RefPtr<PlatformEventTarget>& target,
                         std::string unique_id, bool is_custom_event,
                         bool is_global_event, bool intercept_global_event);

    const fml::RefPtr<PlatformEventTarget>& Target() const { return target_; }
    const std::string& UniqueId() const { return unique_id_; }
    bool IsCustomEvent() const { return is_custom_event_; }
    void SetIsCustomEvent(bool is_custom_event) {
      is_custom_event_ = is_custom_event;
    }
    bool IsGlobalEvent() const { return is_global_event_; }
    void SetIsGlobalEvent(bool is_global_event) {
      is_global_event_ = is_global_event;
    }
    bool InterceptGlobalEvent() const { return intercept_global_event_; }
    lepus::Value ExposedParams() const;
    bool operator==(const ExposureTargetDetail& other) const;
    bool operator!=(const ExposureTargetDetail& other) const;
    bool operator<(const ExposureTargetDetail& other) const;

   private:
    fml::RefPtr<PlatformEventTarget> target_;
    std::string unique_id_;
    bool is_custom_event_{false};
    bool is_global_event_{false};
    bool intercept_global_event_{false};
  };

  explicit PlatformEventTargetExposure(
      NativePaintingCtxPlatformRef* platform_ref,
      fml::RefPtr<fml::TaskRunner> task_runner)
      : platform_ref_(platform_ref), task_runner_(std::move(task_runner)) {}

  void SetIntervalMs(int interval_ms);
  void AddExposureTarget(const fml::RefPtr<PlatformEventTarget>& target,
                         const lepus::Value& option);
  void RemoveExposureTarget(const fml::RefPtr<PlatformEventTarget>& target,
                            const lepus::Value& option);
  void ClearExposureTargetMap();
  void StartExposureCheck();
  void StopExposureCheck();

 private:
  void AddCommonAncestorRectMap(PlatformEventTarget* target);
  void RemoveCommonAncestorRectMap(PlatformEventTarget* target);
  void ResetCommonAncestorRectMap();
  void ScheduleNextExposureCheck();
  void DoExposureCheck();
  void SendEvent(const std::set<ExposureTargetDetail>& targets,
                 const std::string& event_name) const;
  ExposureTargetMeta ParseExposureTargetMeta(const lepus::Value& detail);

  NativePaintingCtxPlatformRef* platform_ref_{nullptr};
  fml::RefPtr<fml::TaskRunner> task_runner_;
  std::unordered_map<std::string, ExposureTargetDetail> exposure_target_map_;
  std::unordered_map<int32_t, CommonAncestorRect> common_ancestor_rect_map_;
  std::set<ExposureTargetDetail> visible_target_before_;
  float window_rect_[4] = {0, 0, 0, 0};
  bool window_rect_valid_{false};
  int interval_ms_{50};
  bool scheduled_exposure_check_{false};
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_EXPOSURE_H_
