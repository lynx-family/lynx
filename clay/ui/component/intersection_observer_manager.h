// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_
#define CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/expose_manager/expose_observer.h"
#include "clay/ui/component/intersection_observer.h"

namespace clay {
class IntersectionObserverManager {
 public:
  explicit IntersectionObserverManager(BaseView* page_view)
      : page_view_(page_view) {}
  ~IntersectionObserverManager() = default;

  void AddObserver(std::unique_ptr<IntersectionObserver> observer);
  void RemoveIntersectionObserver(BaseView* view);

  void RemoveAll(BaseView* view);
  void RemoveObserver(const IntersectionObserver* target);

  void NotifyObservers();
  void NotifyTargetAttached(BaseView* view);
  void NotifyTargetDetached(BaseView* view);

  void RemoveExposeObserver(BaseView* view);
  bool UpdateExposeData(const char* attr_key, const clay::Value& value,
                        BaseView* target_view);
  bool HasExposeObserver(BaseView* view);

  void StopExposure(bool send_event);
  void ResumeExposure();

  void SetExposureFrequency(int freq);
  void SetExposureUIMarginEnabled(bool enabled);
  bool GetExposureUIMarginEnabled() { return exposure_ui_margin_enabled_; }

  BaseView* page_view() { return page_view_; }

 private:
  int64_t expose_min_time_gap_ms_ = 1000 / 20;
  bool exposure_ui_margin_enabled_ = false;
  int64_t last_expose_time_ = -1;

  BaseView* page_view_;
  std::list<std::unique_ptr<IntersectionObserver>> intersection_observers_;
  std::unordered_map<const BaseView*, std::unique_ptr<ExposeObserver>>
      expose_observers_map_;

  void EraseExposeObserver(const BaseView* view = nullptr,
                           const ExposeObserver* target = nullptr);

  template <class T>
  void EraseObserver(std::list<T>& container, BaseView* attached_view,
                     const IntersectionObserver* ptr = nullptr) {
    for (auto iter = container.begin(); iter != container.end();) {
      if (((*iter).get() == ptr) ||
          ((*iter)->GetAttachedView() == attached_view)) {
        iter = container.erase(iter);
        return;
      } else {
        ++iter;
      }
    }
  }

  template <typename T, typename = std::enable_if_t<
                            std::is_base_of<IntersectionObserver, T>::value>>
  void NotifyObserver(std::list<std::unique_ptr<T>>& container,
                      void (IntersectionObserver::*ptr)(), BaseView* view) {
    for (auto& observer : container) {
      if (view == nullptr || (observer->GetAttachedView() == view)) {
        (observer.get()->*ptr)();
      }
    }
  }

  void NotifyExposures(void (IntersectionObserver::*ptr)(),
                       BaseView* view = nullptr);

  void NotifyAllObserver(void (IntersectionObserver::*ptr)(),
                         BaseView* view = nullptr);
};
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_
