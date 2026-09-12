// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_
#define CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_

#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/expose_manager/expose_observer.h"
#include "clay/ui/component/intersection_observer.h"

namespace clay {

// Trace-only identity for a custom event produced by one synchronous Clay
// exposure episode. It never enters the business event payload.
struct ExposureEventTraceInfo {
  uint64_t exposure_episode_id = 0;
  uint32_t exposure_transition_index = 0;
  uint32_t exposure_event_index = 0;
  int clay_page_view_id = -1;
  bool direct_page_child = false;
  int page_child_index = -1;
  const char* exposure_trigger_source = "unscoped";
  const char* exposure_transition_reason = "unknown";
};

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
  void ReconcileExposureForTarget(
      BaseView* view, const char* trigger_source = "target_reconcile");
  bool TryReconcileDirectPageChildOnFirstAdd(BaseView* view);
  bool TryReconcileLargeExposureTargetAfterLayout(BaseView* view);

  void RemoveExposeObserver(BaseView* view);
  bool UpdateExposeData(const char* attr_key, const clay::Value& value,
                        BaseView* target_view);
  bool HasExposeObserver(BaseView* view);

  void StopExposure(bool send_event);
  void ResumeExposure();
  void SetExposureHostVisible(bool visible);

  void SetExposureFrequency(int freq);
  void SetExposureUIMarginEnabled(bool enabled);
  bool GetExposureUIMarginEnabled() { return exposure_ui_margin_enabled_; }

  BaseView* page_view() { return page_view_; }

  ExposureEventTraceInfo RecordExposureTransition(
      BaseView* view, bool appear, bool has_custom_event,
      const char* transition_reason);

 private:
  int64_t expose_min_time_gap_ms_ = 1000 / 20;
  bool exposure_ui_margin_enabled_ = false;
  bool exposure_stopped_ = false;
  bool exposure_host_visible_ = true;
  int64_t last_expose_time_ = -1;

  BaseView* page_view_;
  std::list<std::unique_ptr<IntersectionObserver>> intersection_observers_;
  std::unordered_map<const BaseView*, std::unique_ptr<ExposeObserver>>
      expose_observers_map_;
  std::unordered_set<BaseView*> layout_fast_path_candidate_views_;
  std::unordered_set<const BaseView*> layout_fast_path_attempted_views_;

  struct ExposureTraceEpisodeState {
    uint64_t id = 0;
    const char* trigger_source = "unscoped";
    int target_view_id = -1;
    uint32_t transition_count = 0;
    uint32_t custom_event_count = 0;
    uint32_t uiappear_count = 0;
    uint32_t uidisappear_count = 0;
    int first_direct_page_child_event_index = -1;
    int first_page_child_event_index = -1;
  } exposure_trace_episode_;
  uint64_t next_exposure_trace_episode_id_ = 0;
  bool pending_resume_scan_ = false;
  bool pending_host_visible_scan_ = false;

  void EraseExposeObserver(BaseView* view);

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

  void BeginExposureTraceEpisode(const char* trigger_source, BaseView* view);
  void EndExposureTraceEpisode(size_t observer_visit_count);
  void NotifyExposures(void (IntersectionObserver::*ptr)(), BaseView* view,
                       const char* trigger_source);

  void NotifyAllObserver(void (IntersectionObserver::*ptr)(), BaseView* view,
                         const char* trigger_source);
};
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_INTERSECTION_OBSERVER_MANAGER_H_
