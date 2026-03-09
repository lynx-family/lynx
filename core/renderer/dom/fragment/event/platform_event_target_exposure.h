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

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "core/value_wrapper/value_impl_lepus.h"

struct CommonAncestorRect {
  int target_count{0};
  bool rect_updated{false};
  float rect[4] = {0, 0, 0, 0};
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
    ExposureTargetDetail(int32_t id, std::string unique_id,
                         std::string exposure_id, std::string exposure_scene,
                         lepus::Value dataset);

    int32_t ID() const;
    lepus::Value ExposedParams() const;
    bool operator==(const ExposureTargetDetail& other) const;
    bool operator!=(const ExposureTargetDetail& other) const;
    bool operator<(const ExposureTargetDetail& other) const;

   private:
    int32_t id_{0};
    std::string unique_id_;
    std::string exposure_id_;
    std::string exposure_scene_;
    lepus::Value dataset_;
  };

  explicit PlatformEventTargetExposure(
      NativePaintingCtxPlatformRef* platform_ref)
      : platform_ref_(platform_ref) {}

  void SetTaskRunner(const fml::RefPtr<fml::TaskRunner>& task_runner);
  void SetIntervalMs(int interval_ms);
  void AddExposureTarget(int32_t id, const std::string& unique_id,
                         const std::string& exposure_id,
                         const std::string& exposure_scene,
                         const lepus::Value& dataset);
  void RemoveExposureTarget(int32_t id, const std::string& unique_id,
                            const std::string& exposure_id,
                            const std::string& exposure_scene);
  void StartExposureCheck();
  void StopExposureCheck();

 private:
  void AddCommonAncestorRectMap(int32_t id);
  void RemoveCommonAncestorRectMap(int32_t id);
  void ResetCommonAncestorRectMap();
  void ScheduleNextExposureCheck();
  void DoExposureCheck();
  void SendEvent(const std::set<ExposureTargetDetail>& targets,
                 const std::string& event_name) const;
  std::string BuildExposureUniqueKey(int32_t id, const std::string& unique_id,
                                     const std::string& exposure_id,
                                     const std::string& exposure_scene) const;

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
