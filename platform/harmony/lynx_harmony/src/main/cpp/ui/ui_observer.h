// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OBSERVER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OBSERVER_H_

#include <memory>
#include <set>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_exposure.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_intersection_observer.h"

namespace lynx {
namespace tasm {
namespace harmony {
class UIOwner;
class UIObserverCallback;

class UIObserver {
 public:
  explicit UIObserver(UIOwner* ui_owner);

  void NotifyUILayout();
  void NotifyUIScroll();
  void NotifyUIPropsChange();

  void AddUILayoutObserver(UIObserverCallback* callback);
  void RemoveUILayoutObserver(UIObserverCallback* callback);
  void AddUIScrollObserver(UIObserverCallback* callback);
  void RemoveUIScrollObserver(UIObserverCallback* callback);
  void AddUIPropsChangeObserver(UIObserverCallback* callback);
  void RemoveUIPropsChangeObserver(UIObserverCallback* callback);

  void AddUIToExposedMap(UIBase* ui, std::string unique_id,
                         lepus::Value extra_data, bool is_custom_event);
  void RemoveUIFromExposedMap(UIBase* ui, std::string unique_id);
  void TriggerExposureCheck();
  void StopExposure(const lepus::Value& options);
  void ResumeExposure();
  void SetObserverFrameRate(const lepus::Value& options);

  void CreateUIIntersectionObserver(int intersection_observer_id,
                                    const std::string& js_component_id,
                                    const lepus::Value& options);
  UIIntersectionObserver* GetUIIntersectionObserver(
      int intersection_observer_id);

 private:
  std::shared_ptr<UIExposure> ui_exposure_;
  std::shared_ptr<UIIntersectionObserverManager>
      ui_intersection_observer_manager_;
  std::set<UIObserverCallback*> ui_layout_observers_;
  std::set<UIObserverCallback*> ui_scroll_observers_;
  std::set<UIObserverCallback*> ui_props_change_observers_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_OBSERVER_H_
