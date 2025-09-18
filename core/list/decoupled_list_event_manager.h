// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_EVENT_MANAGER_H_
#define CORE_LIST_DECOUPLED_LIST_EVENT_MANAGER_H_

#include <chrono>
#include <ctime>
#include <memory>
#include <string>
#include <unordered_set>

#include "core/list/decoupled_list_children_helper.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class ListContainerImpl;

class ListEventManager {
 public:
  ListEventManager(ListContainerImpl* list_container_impl);

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

  void SendLayoutCompleteEvent();

  void SendScrollEvent(float distance, list::EventSource event_source);

  void SendExposureEvent(const std::string& event_name,
                         const ItemHolder* item_holder);

  void DetectScrollToThresholdAndSend(float distance, float original_offset,
                                      list::EventSource event_source);

  void RecordVisibleItemIfNeeded(bool is_layout_before);

  void RecordDiffResultIfNeeded();

  void SetNeedLayoutCompleteInfo(bool need_layout_complete_info) {
    need_layout_complete_info_ = need_layout_complete_info;
  }

 private:
  void SendCustomScrollEvent(const std::string& event_name, float distance,
                             list::EventSource event_source);
  void CreateLayoutCompleteInfoIfNeeded();
  std::unique_ptr<pub::Value> GenerateScrollInfo(float deltaX,
                                                 float deltaY) const;
  std::unique_ptr<pub::Value> GenerateVisibleCellsInfo(
      float scroll_left, float scroll_top, bool for_scroll_info) const;
  std::unique_ptr<pub::Value> GenerateNodeExposureInfo(
      const ItemHolder* item_holder) const;
  void UpdatePreviousScrollState(bool is_lower, bool is_upper);
  bool NotAtBouncesArea(float content_offset, float content_size,
                        float list_size);

 private:
  ListChildrenHelper* children_helper_{nullptr};
  ListContainerImpl* list_container_{nullptr};
  std::unique_ptr<pub::Value> layout_complete_info_;
  bool need_visible_cell_{false};
  bool need_layout_complete_info_{false};
  int scroll_event_throttle_ms_{200};
  int lower_threshold_item_count_{0};
  int upper_threshold_item_count_{0};
  std::chrono::time_point<std::chrono::steady_clock,
                          std::chrono::steady_clock::duration>
      last_scroll_event_time_ = std::chrono::steady_clock::now();
  list::ListScrollState previous_scroll_state_{list::ListScrollState::kMiddle};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_EVENT_MANAGER_H_
