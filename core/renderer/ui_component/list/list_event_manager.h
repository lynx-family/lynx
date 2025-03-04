// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_EVENT_MANAGER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_EVENT_MANAGER_H_
#include <chrono>
#include <ctime>
#include <string>
#include <unordered_set>

#include "core/renderer/ui_component/list/list_children_helper.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class ListContainerImpl;

class ListEventManager {
 public:
  ListEventManager(ListContainerImpl* list_container_impl);
  ~ListEventManager() { this->events_.clear(); }
  void AddEvent(const std::string& event) { this->events_.insert(event); };

  void ClearEvents() { this->events_.clear(); }

  void SetVisibleCell(bool visible_cell) { need_visible_cell_ = visible_cell; }

  void SetScrollEventThrottleMS(int scroll_event_throttle_ms) {
    scroll_event_throttle_ms_ = scroll_event_throttle_ms;
  }

  void SetLowerThresholdItemCount(int lower_threshold_item_count) {
    lower_threshold_item_count_ = lower_threshold_item_count;
  }

  void SetUpperThresholdItemCount(int upper_threshold_item_count) {
    upper_threshold_item_count_ = upper_threshold_item_count;
  }

  void SetChildrenHelper(ListChildrenHelper* children_helper) {
    children_helper_ = children_helper;
  };

  void OnScroll(float distance, list::EventSource event_source);
  void DetectScrollToThresholdAndSend(float distance, float original_offset,
                                      list::EventSource event_source);
  void SendLayoutCompleteInfo();
  void OnViewAttach(const ItemHolder* item_holder);
  void OnViewDetach(const ItemHolder* item_holder);
  bool IsDebugEventBound();
  void SendDebugEvent(const fml::RefPtr<lepus::Dictionary>& detail);

 private:
  lepus::Value GetVisibleCellInfo(float scroll_left, float scroll_top);
  void SendCustomScrollEvent(const std::string& event_name, float distance,
                             list::EventSource event_source);
  void SendExposureEvent(const std::string& event_name,
                         const ItemHolder* item_holder);
  fml::RefPtr<lepus::Dictionary> GenerateScrollInfo(float deltaX,
                                                    float deltaY) const;
  float LayoutUnitPerPx() const;
  void UpdatePreviousScrollState(bool is_lower, bool is_upper);
  bool NotAtBouncesArea(float content_offset, float content_size,
                        float list_size);

 private:
  ListChildrenHelper* children_helper_{nullptr};
  ListContainerImpl* list_container_{nullptr};
  std::unordered_set<std::string> events_{};
  int scroll_event_throttle_ms_{200};
  int lower_threshold_item_count_{0};
  int upper_threshold_item_count_{0};
  bool need_visible_cell_{false};
  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::steady_clock::duration>
      last_scroll_event_time_ = std::chrono::steady_clock::now();
  list::ListScrollState previous_scroll_state_{list::ListScrollState::kMiddle};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_EVENT_MANAGER_H_
