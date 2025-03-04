// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_event_manager.h"

#include <algorithm>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {

ListEventManager::ListEventManager(ListContainerImpl *list_container_impl)
    : list_container_(list_container_impl) {
  if (!list_container_) {
    LOGE("[EventManager] error: "
         << "list_container_ is nullptr");
  }
}

void ListEventManager::OnScroll(float distance,
                                list::EventSource event_source) {
  if (!list_container_ || !(list_container_->element_manager())) {
    LOGE("[EventManager] onScroll() error: "
         << " list_container or element_manager is nullptr");
    return;
  }

  if (base::IsZero(distance)) {
    return;
  }

  // sendScrollEvent
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::microseconds>(
          now - last_scroll_event_time_)
          .count() > scroll_event_throttle_ms_) {
    SendCustomScrollEvent(list::kScroll, distance, event_source);
    last_scroll_event_time_ = now;
  }
}

void ListEventManager::DetectScrollToThresholdAndSend(
    float distance, float original_offset, list::EventSource event_source) {
  if (!list_container_ || !list_container_->list_layout_manager()) {
    LOGE("[EventManager] DetectScrollToThresholdAndSend() error: "
         << " list_container or list_layout_manager is nullptr");
    return;
  }
  auto list_layout_manager = list_container_->list_layout_manager();
  bool is_upper = false;
  bool is_lower = false;
  bool is_lower_edge = false;
  bool is_upper_edge = false;

  // calculate the firstItemIndex & lastItemIndex
  int first_index = INT_MAX;
  int end_index = INT_MIN;
  auto item_holder_list = children_helper_->on_screen_children();
  for (auto it = item_holder_list.begin(); it != item_holder_list.end(); ++it) {
    auto item_holder = *it;
    if (item_holder) {
      first_index = std::min(item_holder->index(), first_index);
      end_index = std::max(item_holder->index(), end_index);
    }
  }

  float content_offset =
      list_container_->list_layout_manager()->content_offset();
  float content_size = list_container_->list_layout_manager()->content_size();
  float list_size = list_container_->list_layout_manager()
                        ->list_orientation_helper_->GetMeasurement();
  // sendUpperScrollEvent
  if (first_index < upper_threshold_item_count_) {
    is_upper = true;
  }
  if (upper_threshold_item_count_ == 0 &&
      base::FloatsLargerOrEqual(0, content_offset)) {
    // come to the top edge
    is_upper = true;
  }
  if (base::FloatsLarger(list_size, content_size)) {
    is_upper_edge = true;
    is_lower_edge = true;
  } else {
    if (base::FloatsLargerOrEqual(content_offset + list_size, content_size)) {
      is_lower_edge = true;
    }
    if (base::FloatsLargerOrEqual(0, content_offset)) {
      is_upper_edge = true;
    }
  }

  // sendLowerScrollEvent
  int bottom_border_item_index =
      children_helper_->GetChildCount() - lower_threshold_item_count_ - 1;
  if (end_index > bottom_border_item_index) {
    is_lower = true;
  }

  if (lower_threshold_item_count_ == 0 &&
      base::FloatsLargerOrEqual(
          content_offset +
              list_layout_manager->list_orientation_helper_->GetMeasurement(),
          list_layout_manager->content_size())) {
    // come to the bottom edge
    is_lower = true;
  }

  // Special case. The content can not fill the list
  if (base::FloatsLargerOrEqual(
          list_layout_manager->list_orientation_helper_->GetMeasurement(),
          list_layout_manager->content_size())) {
    is_lower = true;
    is_upper = true;
  }

  // Send scroll to upper/lower event.
  if (event_source == list::EventSource::kDiff ||
      event_source == list::EventSource::kLayout) {
    // 1. Force sending lower/upper event after diff or layout
    if (is_upper) {
      SendCustomScrollEvent(list::kScrollToUpper, distance, event_source);
    }
    if (is_lower) {
      SendCustomScrollEvent(list::kScrollToLower, distance, event_source);
    }
  } else if (event_source == list::EventSource::kScroll) {
    // 2. Handle event from scroll.
    list::ListScrollState previous_state = previous_scroll_state_;
    if (is_upper && (previous_state != list::ListScrollState::kUpper &&
                     previous_state != list::ListScrollState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kScrollToUpper, distance, event_source);
    }
    if (is_lower && (previous_state != list::ListScrollState::kLower &&
                     previous_state != list::ListScrollState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kScrollToLower, distance, event_source);
    }
    UpdatePreviousScrollState(is_lower, is_upper);
  }

  // Send scroll to upper/lower edge event.
  if (is_lower_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(list::kScrollToLowerEdge, 0, event_source);
  }
  if (is_upper_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(list::kScrollToUpperEdge, 0, event_source);
  }
  if (!is_lower_edge && !is_upper_edge) {
    SendCustomScrollEvent(list::kScrollToNormalState, 0, event_source);
  }
}

bool ListEventManager::NotAtBouncesArea(float content_offset,
                                        float content_size, float list_size) {
  // content_offset is smaller than 0
  if (base::FloatsLarger(0, content_offset)) {
    return false;
  }
  // list can not be scrolled and content_offset is not zero
  if (base::FloatsLargerOrEqual(list_size, content_size) &&
      base::FloatsLarger(content_offset, 0)) {
    return false;
  }
  // list is scrollable and content_offset is beyond end edge
  if (base::FloatsLarger(content_size, list_size) &&
      base::FloatsLarger(content_offset + list_size, content_size)) {
    return false;
  }
  return true;
}

void ListEventManager::UpdatePreviousScrollState(bool is_lower, bool is_upper) {
  if (is_lower && is_upper) {
    previous_scroll_state_ = list::ListScrollState::kBothEdge;
  } else if (is_lower) {
    previous_scroll_state_ = list::ListScrollState::kLower;
  } else if (is_upper) {
    previous_scroll_state_ = list::ListScrollState::kUpper;
  } else {
    previous_scroll_state_ = list::ListScrollState::kMiddle;
  }
}

bool ListEventManager::IsDebugEventBound() {
  auto it = events_.find(list::kListDebugInfoEvent);
  if (it == events_.end()) {
    return false;
  }
  return true;
}

void ListEventManager::SendDebugEvent(
    const fml::RefPtr<lepus::Dictionary> &detail) {
  if (list_container_ && list_container_->element_manager()) {
    ElementManager *element_manager = list_container_->element_manager();
    element_manager->SendNativeCustomEvent(
        list::kListDebugInfoEvent, list_container_->element()->impl_id(),
        lepus_value(std::move(detail)), "detail");
  }
}

float ListEventManager::LayoutUnitPerPx() const {
  if (list_container_ && list_container_->element_manager()) {
    return list_container_->element_manager()
        ->GetLynxEnvConfig()
        .LayoutsUnitPerPx();
  }
  return 0.f;
}

fml::RefPtr<lepus::Dictionary> ListEventManager::GenerateScrollInfo(
    float deltaX, float deltaY) const {
  auto scroll_info = lepus::Dictionary::Create();
  if (!list_container_ || !(list_container_->list_layout_manager()) ||
      !(list_container_->element())) {
    LOGE("[EventManager] GenerateScrollInfo() error: "
         << " list_container or element or list_layout_manager is nullptr");
    return scroll_info;
  }
  ListLayoutManager *list_layout_manager =
      list_container_->list_layout_manager();
  Element *list_element = list_container_->element();
  float layouts_unit_per_px = LayoutUnitPerPx();
  if (base::FloatsLarger(layouts_unit_per_px, 0.f)) {
    bool is_vertical = list_layout_manager->CanScrollVertically();
    float content_offset =
        list_layout_manager->content_offset() / layouts_unit_per_px;
    float content_size =
        list_layout_manager->content_size() / layouts_unit_per_px;
    float list_width = list_element->width() / layouts_unit_per_px;
    float list_height = list_element->height() / layouts_unit_per_px;
    scroll_info->SetValue(BASE_STATIC_STRING(list::kScrollLeft),
                          is_vertical ? 0.f : content_offset);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kScrollTop),
                          !is_vertical ? 0.f : content_offset);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kScrollWith),
                          is_vertical ? list_width : content_size);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kScrollHeight),
                          !is_vertical ? list_height : content_size);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kListWidth), list_width);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kListHeight), list_height);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kDeltaX),
                          deltaX / layouts_unit_per_px);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kDeltaY),
                          deltaY / layouts_unit_per_px);
  }
  return scroll_info;
}

void ListEventManager::SendCustomScrollEvent(const std::string &event_name,
                                             float distance,
                                             list::EventSource event_source) {
  if (!list_container_ || !list_container_->element_manager() ||
      !list_container_->list_layout_manager()) {
    LOGE("[EventManager] SendCustomScrollEvent() error: "
         << " list_container or element or list_layout_manager is nullptr");
    return;
  }
  auto it = events_.find(event_name);
  // the switch of this event is not opened!
  if (it == events_.end()) {
    return;
  }
  auto list_layout_manager = list_container_->list_layout_manager();
  float scroll_left = list_layout_manager->CanScrollHorizontally()
                          ? list_layout_manager->content_offset()
                          : 0.f;
  float scroll_top = list_layout_manager->CanScrollVertically()
                         ? list_layout_manager->content_offset()
                         : 0.f;
  float dx = list_layout_manager->CanScrollHorizontally() ? distance : 0.f;
  float dy = list_layout_manager->CanScrollVertically() ? distance : 0.f;
  float layouts_unit_per_px = LayoutUnitPerPx();
  if (base::FloatsLarger(layouts_unit_per_px, 0.f)) {
    ElementManager *element_manager = list_container_->element_manager();
    auto scroll_info = GenerateScrollInfo(dx, dy);
    scroll_info->SetValue(BASE_STATIC_STRING(list::kEventSource),
                          static_cast<int>(event_source));
    if (need_visible_cell_) {
      scroll_info->SetValue(BASE_STATIC_STRING(list::kAttachedCells),
                            GetVisibleCellInfo(scroll_left, scroll_top));
    }
    element_manager->SendNativeCustomEvent(
        event_name, list_container_->element()->impl_id(),
        lepus_value(std::move(scroll_info)), "detail");
  }
}

void ListEventManager::SendLayoutCompleteInfo() {
  // the switch of this event is not opened!
  if (!list_container_ ||
      events_.find(list::kLayoutComplete) == events_.end()) {
    return;
  }
  // layout id
  auto layout_complete_info = list_container_->layout_complete_info();
  if (!layout_complete_info) {
    layout_complete_info = lepus::Dictionary::Create();
  }
  layout_complete_info->SetValue(BASE_STATIC_STRING(list::kLayoutID),
                                 list_container_->layout_id());
  // scroll info
  if (list_container_->need_layout_complete_info()) {
    layout_complete_info->SetValue(BASE_STATIC_STRING(list::kScrollInfo),
                                   GenerateScrollInfo(0.f, 0.f));
  }
  // Since the worklet call inside lauoutcomplete event may trigger another
  // layoutcomplete event, the reset has to be done before sending the event.
  list_container_->ClearLayoutCompleteInfo();
  list_container_->ResetLayoutID();
  list_container_->element_manager()->SendNativeCustomEvent(
      list::kLayoutComplete, list_container_->element()->impl_id(),
      lepus_value(layout_complete_info), "detail");
}

lepus::Value ListEventManager::GetVisibleCellInfo(float scroll_left,
                                                  float scroll_top) {
  auto cell_array = lepus::CArray::Create();
  if (!children_helper_ || !list_container_ ||
      !(list_container_->element_manager())) {
    LOGE(
        "GetVisibleCellInfo error: "
        << " children_helper_ or list_container or element_manager is nullptr");
    return lepus_value(std::move(cell_array));
  }
  BASE_STATIC_STRING_DECL(kId, "id");
  BASE_STATIC_STRING_DECL(kItemKey, "itemKey");
  BASE_STATIC_STRING_DECL(kIndex, "index");
  // for legacy API
  BASE_STATIC_STRING_DECL(kPosition, "position");
  BASE_STATIC_STRING_DECL(kTop, "top");
  BASE_STATIC_STRING_DECL(kBottom, "bottom");
  BASE_STATIC_STRING_DECL(kLeft, "left");
  BASE_STATIC_STRING_DECL(kRight, "right");
  const float layouts_unit_per_px =
      list_container_->element_manager()->GetLynxEnvConfig().LayoutsUnitPerPx();
  ListAdapter *list_adapter = list_container_->list_adapter();
  for (const auto &child_item_holder : children_helper_->on_screen_children()) {
    Element *list_item = nullptr;
    if (child_item_holder &&
        (list_item = list_adapter->GetListItemElement(child_item_holder))) {
      auto item_info = lepus::Dictionary::Create();
      float top = child_item_holder->top() - scroll_top;
      float left = child_item_holder->left() - scroll_left;
      item_info->SetValue(kId, list_item->data_model()->idSelector());
      item_info->SetValue(kItemKey, child_item_holder->item_key());
      item_info->SetValue(kIndex, child_item_holder->index());
      // for legacy API
      item_info->SetValue(kPosition, child_item_holder->index());
      item_info->SetValue(kTop, top / layouts_unit_per_px);
      item_info->SetValue(
          kBottom, (top + child_item_holder->height()) / layouts_unit_per_px);
      item_info->SetValue(kLeft, left / layouts_unit_per_px);
      item_info->SetValue(
          kRight, (left + child_item_holder->width()) / layouts_unit_per_px);
      cell_array->emplace_back(std::move(item_info));
    }
  }
  return lepus_value(std::move(cell_array));
}

void ListEventManager::OnViewAttach(const ItemHolder *item_holder) {
  SendExposureEvent(list::kNodeAppear, item_holder);
}

void ListEventManager::OnViewDetach(const ItemHolder *item_holder) {
  SendExposureEvent(list::kNodeDisappear, item_holder);
}

void ListEventManager::SendExposureEvent(const std::string &event_name,
                                         const ItemHolder *item_holder) {
  if (!list_container_ || !list_container_->element_manager() || !item_holder) {
    return;
  }
  Element *element =
      list_container_->list_adapter()->GetListItemElement(item_holder);
  if (!element) {
    return;
  }
  if (element->event_map().find(base::String(event_name)) ==
      element->event_map().end()) {
    return;
  }
  ElementManager *element_manager = list_container_->element_manager();
  auto info = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kIndex, "index");
  BASE_STATIC_STRING_DECL(kKey, "key");
  info->SetValue(kIndex, item_holder->index());
  info->SetValue(kKey, item_holder->item_key());
  element_manager->SendNativeCustomEvent(
      event_name, element->impl_id(), lepus_value(std::move(info)), "detail");
}

}  // namespace tasm
}  // namespace lynx
