// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/intersection_observer_manager.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/public/value.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/expose_manager/expose_observer.h"
#include "clay/ui/component/intersection_observer.h"

namespace clay {

void IntersectionObserverManager::StopExposure(bool send_event) {
  for (auto& it : expose_observers_map_) {
    it.second->StopExposure(send_event);
  }
}

void IntersectionObserverManager::ResumeExposure() {
  for (auto& it : expose_observers_map_) {
    it.second->ResumeExposure();
  }
}

void IntersectionObserverManager::AddObserver(
    std::unique_ptr<IntersectionObserver> observer) {
  if (observer->IsOfType(IntersectionObserver::kIntersectionObserver)) {
    intersection_observers_.emplace_back(std::move(observer));
  } else if (observer->IsOfType(IntersectionObserver::kExposeObserver)) {
    std::unique_ptr<ExposeObserver> p(
        static_cast<ExposeObserver*>(observer.release()));
    expose_observers_map_.emplace(p.get()->GetAttachedView(), std::move(p));
  }
}

void IntersectionObserverManager::EraseExposeObserver(
    const BaseView* view, const ExposeObserver* target) {
  if (view) {
    expose_observers_map_.erase(view);
  } else if (target) {
    for (auto iter = expose_observers_map_.begin();
         iter != expose_observers_map_.end();) {
      if ((iter->second).get() == target) {
        iter = expose_observers_map_.erase(iter);
        return;
      } else {
        ++iter;
      }
    }
  }
}

void IntersectionObserverManager::RemoveObserver(
    const IntersectionObserver* target) {
  if (target->IsOfType(IntersectionObserver::kIntersectionObserver)) {
    EraseObserver(intersection_observers_, nullptr, target);
  } else if (target->IsOfType(IntersectionObserver::kExposeObserver)) {
    expose_observers_map_.erase(target->GetAttachedView());
    EraseExposeObserver(nullptr, static_cast<const ExposeObserver*>(target));
  }
}

void IntersectionObserverManager::RemoveAll(BaseView* view) {
  EraseObserver(intersection_observers_, view, nullptr);
  EraseExposeObserver(view);
}

void IntersectionObserverManager::RemoveIntersectionObserver(BaseView* view) {
  EraseObserver(intersection_observers_, view, nullptr);
}

void IntersectionObserverManager::RemoveExposeObserver(BaseView* view) {
  EraseExposeObserver(view);
}

void IntersectionObserverManager::SetExposureFrequency(int freq) {
  expose_min_time_gap_ms_ = 1000 / std::min(1, std::max(freq, 60));
}

void IntersectionObserverManager::SetExposureUIMarginEnabled(bool enabled) {
  exposure_ui_margin_enabled_ = enabled;
}

bool IntersectionObserverManager::UpdateExposeData(const char* attr_key,
                                                   const clay::Value& value,
                                                   BaseView* target_view) {
  auto target = expose_observers_map_.find(target_view);
  if (target != expose_observers_map_.end()) {
    return ((target->second).get())->UpdateExposeData(attr_key, value);
  }
  return false;
}

bool IntersectionObserverManager::HasExposeObserver(BaseView* view) {
  return expose_observers_map_.count(view) > 0;
}

void IntersectionObserverManager::NotifyObservers() {
  NotifyObserver(intersection_observers_,
                 &IntersectionObserver::CheckForIntersectionWithTarget,
                 nullptr);
  auto now = fml::TimePoint::Now().ToEpochDelta().ToMicroseconds();
  if (last_expose_time_ == -1 ||
      (now - last_expose_time_ > expose_min_time_gap_ms_)) {
    last_expose_time_ = now;
    NotifyExposures(&IntersectionObserver::CheckForIntersectionWithTarget);
  }
}

void IntersectionObserverManager::NotifyTargetAttached(BaseView* view) {
  NotifyAllObserver(&IntersectionObserver::OnAttach, view);
}

void IntersectionObserverManager::NotifyTargetDetached(BaseView* view) {
  NotifyAllObserver(&IntersectionObserver::OnDetach, view);
}

void IntersectionObserverManager::NotifyExposures(
    void (IntersectionObserver::*ptr)(), BaseView* view) {
  for (auto& it : expose_observers_map_) {
    if (view == nullptr || (it.second->GetAttachedView() == view)) {
      (it.second.get()->*ptr)();
    }
  }
}

void IntersectionObserverManager::NotifyAllObserver(
    void (IntersectionObserver::*ptr)(), BaseView* view) {
  NotifyObserver(intersection_observers_, ptr, view);
  NotifyExposures(ptr, view);
}

}  // namespace clay
