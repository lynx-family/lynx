// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_EXPOSURE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_EXPOSURE_H_

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/ui_observer_callback.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIObserver;
class UIOwner;
class UIBase;

class UIExposure : public UIObserverCallback,
                   public std::enable_shared_from_this<UIExposure> {
 public:
  struct CommonAncestorUIRect {
    int ui_count{0};
    bool ui_rect_updated{false};
    float ui_rect[4] = {0};
  };

  class UIExposureDetail {
   public:
    UIExposureDetail() = default;
    UIExposureDetail(int ui_id, std::string unique_id, std::string exposure_id,
                     std::string exposure_scene, lepus::Value extra_data,
                     lepus::Value dataset, bool is_custom_event);

    int ID() const;
    bool IsCustomEvent() const;
    lepus::Value ExposedParams() const;
    bool operator==(const UIExposureDetail& other) const;
    bool operator!=(const UIExposureDetail& other) const;
    bool operator<(const UIExposureDetail& other) const;

   private:
    int ui_id_;
    std::string unique_id_;
    std::string exposure_id_;
    std::string exposure_scene_;
    lepus::Value extra_data_;
    lepus::Value dataset_;
    bool is_custom_event_{false};
  };

  UIExposure(UIObserver* ui_observer, UIOwner* ui_owner);
  virtual ~UIExposure() = default;

  void RegisterExposureCheckCallBack();
  void UnregisterExposureCheckCallBack();
  void AddUIToExposedMap(UIBase* ui, std::string unique_id,
                         lepus::Value extra_data, bool is_custom_event);
  void RemoveUIFromExposedMap(UIBase* ui, std::string unique_id);
  void StopExposure(const lepus::Value& options);
  void ResumeExposure();
  void SetObserverFrameRate(const lepus::Value& options);
  void ExecExposureCheck();
  void PostTask() override;
  void CheckOnUIThread();

 private:
  void SendEvent(const std::set<UIExposureDetail>& ui_set,
                 const std::string& event_name) const;
  void ScheduleUIExposureCheck();
  bool IsLynxViewChanged();
  void AddCommonAncestorUI(UIBase* ui);
  void RemoveCommonAncestorUI(UIBase* ui);
  void ResetCommonAncestorUIRect();

  UIOwner* ui_owner_{nullptr};
  UIObserver* ui_observer_{nullptr};
  std::unordered_map<std::string, UIExposureDetail> exposed_ui_map_;
  std::unordered_map<int, CommonAncestorUIRect> common_ancestor_ui_rect_map_;
  std::set<UIExposureDetail> ui_visible_before_;
  int time_interval_for_exposure_check_{50};
  long long last_lynxview_check_time_{0};
  int time_interval_for_lynxview_check_{50};
  float old_lynx_origin_rect_[4] = {0};
  bool exposure_check_flag_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_EXPOSURE_H_
