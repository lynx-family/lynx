// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_INTERSECTION_OBSERVER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_INTERSECTION_OBSERVER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/ui_observer_callback.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIObserver;
class UIOwner;
class UIBase;
class UIIntersectionObserverManager;

class UIIntersectionObserverTarget {
 public:
  class UIIntersectionObserverEntry {
   public:
    UIIntersectionObserverEntry() = default;
    UIIntersectionObserverEntry(std::string target_id_selector,
                                float intersection_rect[4],
                                float target_rect[4], float ref_rect[4],
                                long intersection_time);

    float IntersectionRatio();
    void UpdateIntersectionRatio();
    lepus::Value IntersectionParams();
    void GetIntersectionRect(float rect[4]);

   private:
    lepus::Value GetRectDictionary(float rect[4]);

    std::string target_id_selector_;
    bool is_intersecting_{false};
    float intersection_ratio_{0.f};
    float intersection_rect_[4]{0.f};
    float target_rect_[4]{0.f};
    float ref_rect_[4]{0.f};
    long intersection_time_{0};
  };

  UIIntersectionObserverTarget(int target_id, int js_callback_id);
  int TargetID();
  int JSCallbackID();
  void SetUIIntersectionObserverEntry(
      std::unique_ptr<UIIntersectionObserverEntry> intersection_entry);
  UIIntersectionObserverEntry* IntersectionEntry();

 private:
  int target_id_{0};
  int js_callback_id_{0};
  std::unique_ptr<UIIntersectionObserverEntry> intersection_entry_;
};

class UIIntersectionObserver {
 public:
  UIIntersectionObserver(
      UIIntersectionObserverManager* intersection_observer_manager,
      UIOwner* ui_owner, int intersection_observer_id, int js_component_id,
      const lepus::Value& options);

  void RelativeTo(const std::string& ref_id_selector,
                  const lepus::Value& options);
  void RelativeToViewport(const lepus::Value& options);
  void RelativeToScreen(const lepus::Value& options);
  void Observe(const std::string& target_id_selector, int js_callback_id);
  void Disconnect();
  void CheckIntersection();

 private:
  void ParseUIMargin(const lepus::Value& options);
  float GetUIMarginVP(const lepus::Value& value);
  void CheckIntersectionWithTarget(UIIntersectionObserverTarget* target,
                                   bool is_initial_check);
  void ComputeTargetInteraction(UIBase* target, float target_rect[4],
                                float intersection_rect[4]);
  bool HasCrossIntersectionRatioThreshold(
      UIIntersectionObserverTarget::UIIntersectionObserverEntry* old_entry,
      UIIntersectionObserverTarget::UIIntersectionObserverEntry* new_entry);

  UIIntersectionObserverManager* intersection_observer_manager_{nullptr};
  UIOwner* ui_owner_{nullptr};
  int intersection_observer_id_{0};
  int js_component_id_{-1};
  float initial_intersection_ratio_{0.f};
  std::vector<float> intersection_ratio_thresholds_;
  float ui_margin_top_{0.f};
  float ui_margin_right_{0.f};
  float ui_margin_bottom_{0.f};
  float ui_margin_left_{0.f};
  int ref_id_{10};
  float ref_rect_[4]{0.f};
  std::vector<UIIntersectionObserverTarget> intersection_observe_targets_;
};

class UIIntersectionObserverManager
    : public UIObserverCallback,
      public std::enable_shared_from_this<UIIntersectionObserverManager> {
 public:
  UIIntersectionObserverManager(UIObserver* ui_observer, UIOwner* ui_owner);
  virtual ~UIIntersectionObserverManager() = default;

  void PostTask() override;
  void CheckOnUIThread();
  void AddUIIntersectionObserver(
      UIIntersectionObserverManager* intersection_observer_manager,
      int intersection_observer_id, const std::string& js_component_id,
      const lepus::Value& options);
  void RemoveUIIntersectionObserver(int intersection_observer_id);
  UIIntersectionObserver* GetUIIntersectionObserver(
      int intersection_observer_id);
  void ExecIntersectionCheck();
  void CallJSIntersectionObserver(int32_t observer_id, int32_t callback_id,
                                  lepus::Value params) const;
  void ScreenSize(float size[2]);

 private:
  void ScheduleUIIntersectionCheck();

  UIOwner* ui_owner_{nullptr};
  UIObserver* ui_observer_{nullptr};
  std::unordered_map<int, UIIntersectionObserver> intersection_observers_;
  bool intersection_check_flag_{false};
  long long last_intersection_check_time_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_INTERSECTION_OBSERVER_H_
