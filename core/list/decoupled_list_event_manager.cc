// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_event_manager.h"

#include <algorithm>
#include <utility>

#include "core/list/decoupled_list_container_impl.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

ListEventManager::ListEventManager(ListContainerImpl* list_container_impl)
    : list_container_(list_container_impl) {
  if (!list_container_) {
    DLIST_LOGE("[EventManager] error: "
               << "list_container_ is nullptr");
  }
}

void ListEventManager::SendLayoutCompleteEvent() {
  if (!list_container_ || !list_container_->value_factory() ||
      !list_container_->list_delegate()->HasBoundEvent(
          list::kEventLayoutComplete)) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  if (layout_complete_info_) {
    // Move and clear layout_complete_info_
    std::unique_ptr<pub::Value> layout_complete_info =
        std::move(layout_complete_info_);
    // push layout id
    layout_complete_info->PushInt32ToMap(list::kLayoutInfoLayoutId,
                                         list_container_->layout_id());
    std::unique_ptr<pub::Value> scroll_info;
    if (need_layout_complete_info_ &&
        (scroll_info = GenerateScrollInfo(0.f, 0.f))) {
      layout_complete_info->PushValueToMap(list::kLayoutInfoScrollInfo,
                                           *scroll_info);
    }
    list_container_->ResetLayoutID();
    list_container_->list_delegate()->SendCustomEvent(
        list::kEventLayoutComplete, list::kEventParamDetail,
        std::move(layout_complete_info));
  }
}

void ListEventManager::RecordVisibleItemIfNeeded(bool is_layout_before) {
  if (!need_layout_complete_info_ || !list_container_->value_factory()) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  if (layout_complete_info_) {
    std::unique_ptr<pub::Value> visible_cells_info;
    if ((visible_cells_info = GenerateVisibleCellsInfo(0.f, 0.f, false))) {
      layout_complete_info_->PushValueToMap(
          is_layout_before ? list::kLayoutInfoVisibleItemBeforeUpdate
                           : list::kLayoutInfoVisibleItemAfterUpdate,
          *visible_cells_info);
    }
  }
}

void ListEventManager::RecordDiffResultIfNeeded() {
  if (!need_layout_complete_info_ || !list_container_->value_factory()) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  std::unique_ptr<pub::Value> diff_result;
  if (layout_complete_info_ &&
      (diff_result = list_container_->list_adapter()->GenerateDiffResult())) {
    layout_complete_info_->PushValueToMap(list::kLayoutInfoDiffResult,
                                          *diff_result);
  }
}

void ListEventManager::SendScrollEvent(float distance,
                                       list::EventSource event_source) {
  if (base::IsZero(distance)) {
    return;
  }
  // sendScrollEvent
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - last_scroll_event_time_)
                      .count();
  if (duration > scroll_event_throttle_ms_) {
    SendCustomScrollEvent(list::kEventScroll, distance, event_source);
    last_scroll_event_time_ = now;
  }
}

void ListEventManager::DetectScrollToThresholdAndSend(
    float distance, float original_offset, list::EventSource event_source) {
  if (!list_container_ || !list_container_->list_layout_manager() ||
      !list_container_->value_factory()) {
    return;
  }
  ListLayoutManager* list_layout_manager =
      list_container_->list_layout_manager();
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
  float content_offset = list_layout_manager->content_offset();
  float content_size = list_layout_manager->content_size();
  float list_size =
      list_layout_manager->list_orientation_helper_->GetMeasurement();

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
      base::FloatsLargerOrEqual(content_offset + list_size, content_size)) {
    // come to the bottom edge
    is_lower = true;
  }

  // Special case. The content can not fill the list
  if (base::FloatsLargerOrEqual(list_size, content_size)) {
    is_lower = true;
    is_upper = true;
  }

  // Send scroll to upper/lower event.
  if (event_source == list::EventSource::kDiff ||
      event_source == list::EventSource::kLayout) {
    // 1. Force sending lower/upper event after diff or layout
    if (is_upper) {
      SendCustomScrollEvent(list::kEventScrollToUpper, distance, event_source);
    }
    if (is_lower) {
      SendCustomScrollEvent(list::kEventScrollToLower, distance, event_source);
    }
  } else if (event_source == list::EventSource::kScroll) {
    // 2. Handle event from scroll.
    list::ListScrollState previous_state = previous_scroll_state_;
    if (is_upper && (previous_state != list::ListScrollState::kUpper &&
                     previous_state != list::ListScrollState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kEventScrollToUpper, distance, event_source);
    }
    if (is_lower && (previous_state != list::ListScrollState::kLower &&
                     previous_state != list::ListScrollState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kEventScrollToLower, distance, event_source);
    }
    UpdatePreviousScrollState(is_lower, is_upper);
  }

  // Send scroll to upper/lower edge event.
  if (is_lower_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(list::kEventScrollToLowerEdge, 0, event_source);
  }
  if (is_upper_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(list::kEventScrollToUpperEdge, 0, event_source);
  }
  if (!is_lower_edge && !is_upper_edge) {
    SendCustomScrollEvent(list::kEventScrollToNormalState, 0, event_source);
  }
}

bool ListEventManager::NotAtBouncesArea(float original_offset,
                                        float content_size, float list_size) {
  // original_offset is smaller than 0
  if (base::FloatsLarger(0, original_offset)) {
    return false;
  }
  // list can not be scrolled and content_offset is not zero
  if (base::FloatsLargerOrEqual(list_size, content_size) &&
      base::FloatsLarger(original_offset, 0)) {
    return false;
  }
  // list is scrollable and original_offset is beyond end edge
  if (base::FloatsLarger(content_size, list_size) &&
      base::FloatsLarger(original_offset + list_size, content_size)) {
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

void ListEventManager::SendCustomScrollEvent(const std::string& event_name,
                                             float distance,
                                             list::EventSource event_source) {
  if (!list_container_ || !list_container_->value_factory() ||
      !list_container_->list_delegate()->HasBoundEvent(event_name)) {
    return;
  }
  ListLayoutManager* list_layout_manager =
      list_container_->list_layout_manager();
  bool is_vertical = list_layout_manager->CanScrollVertically();
  float scroll_left =
      !is_vertical ? list_layout_manager->content_offset() : 0.f;
  float scroll_top = is_vertical ? list_layout_manager->content_offset() : 0.f;
  float dx = !is_vertical ? distance : 0.f;
  float dy = is_vertical ? distance : 0.f;
  float layouts_unit_per_px =
      list_container_->list_delegate()->GetLayoutsUnitPerPx();
  if (base::FloatsLarger(layouts_unit_per_px, 0.f)) {
    std::unique_ptr<pub::Value> scroll_info = GenerateScrollInfo(dx, dy);
    if (scroll_info) {
      scroll_info->PushInt32ToMap(list::kScrollInfoEventSource,
                                  static_cast<int>(event_source));
      std::unique_ptr<pub::Value> visible_cells_info;
      if (need_visible_cell_ && (visible_cells_info = GenerateVisibleCellsInfo(
                                     scroll_left, scroll_top, true))) {
        scroll_info->PushValueToMap(list::kScrollInfoAttachedCells,
                                    *visible_cells_info);
      }
      list_container_->list_delegate()->SendCustomEvent(
          event_name, list::kEventParamDetail, std::move(scroll_info));
    }
  }
}

void ListEventManager::SendExposureEvent(const std::string& event_name,
                                         const ItemHolder* item_holder) {
  ItemElementDelegate* list_item_delegate =
      list_container_->list_adapter()->GetItemElementDelegate(item_holder);
  if (!list_item_delegate || !list_item_delegate->HasBoundEvent(event_name)) {
    return;
  }
  std::unique_ptr<pub::Value> exposure_info =
      GenerateNodeExposureInfo(item_holder);
  if (exposure_info) {
    list_item_delegate->SendCustomEvent(event_name, list::kEventParamDetail,
                                        std::move(exposure_info));
  }
}

std::unique_ptr<pub::Value> ListEventManager::GenerateNodeExposureInfo(
    const ItemHolder* item_holder) const {
  std::unique_ptr<pub::Value> exposure_info;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory && (exposure_info = value_factory->CreateMap())) {
    exposure_info->PushInt32ToMap(list::kCellInfoIndex, item_holder->index());
    exposure_info->PushStringToMap(list::kCellInfoItemKey,
                                   item_holder->item_key());
  }
  return exposure_info;
}

std::unique_ptr<pub::Value> ListEventManager::GenerateScrollInfo(
    float deltaX, float deltaY) const {
  std::unique_ptr<pub::Value> scroll_info;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory) {
    scroll_info = value_factory->CreateMap();
    float layouts_unit_per_px =
        list_container_->list_delegate()->GetLayoutsUnitPerPx();
    if (scroll_info && base::FloatsLarger(layouts_unit_per_px, 0.f)) {
      ListLayoutManager* list_layout_manager =
          list_container_->list_layout_manager();
      bool is_vertical = list_layout_manager->CanScrollVertically();
      float content_offset =
          list_layout_manager->content_offset() / layouts_unit_per_px;
      float content_size =
          list_layout_manager->content_size() / layouts_unit_per_px;
      float list_width =
          list_container_->list_delegate()->GetWidth() / layouts_unit_per_px;
      float list_height =
          list_container_->list_delegate()->GetHeight() / layouts_unit_per_px;

      scroll_info->PushDoubleToMap(list::kScrollInfoScrollLeft,
                                   is_vertical ? 0.f : content_offset);
      scroll_info->PushDoubleToMap(list::kScrollInfoScrollTop,
                                   !is_vertical ? 0.f : content_offset);
      scroll_info->PushDoubleToMap(list::kScrollInfoScrollWidth,
                                   is_vertical ? list_width : content_size);
      scroll_info->PushDoubleToMap(list::kScrollInfoScrollHeight,
                                   !is_vertical ? list_height : content_size);
      scroll_info->PushDoubleToMap(list::kScrollInfoListWidth, list_width);
      scroll_info->PushDoubleToMap(list::kScrollInfoListHeight, list_height);
      scroll_info->PushDoubleToMap(list::kScrollInfoDeltaX,
                                   deltaX / layouts_unit_per_px);
      scroll_info->PushDoubleToMap(list::kScrollInfoDeltaY,
                                   deltaY / layouts_unit_per_px);
    }
  }
  return scroll_info;
}

/**
 * For scroll event:
 *   (1) left / top / right / bottom Relative to the list, in px
 *   (2) position is legacy.
 *   [{
 *     "id": string,
 *     "itemKey": string,
 *     "index": number,
 *     "position": number,
 *     "left": number,
 *     "top": number,
 *     "right": number,
 *     "bottom": number,
 *   }]
 *
 * For layout complete info:
 *   (1) originX and originY is the position of child node relative to the
 * entire scroll area.
 *   [{
 *     "height": number;
 *     "width": number;
 *     "itemKey": string;
 *     "isBinding": boolean;
 *     "originX": number;
 *     "originY": number;
 *     "updated": boolean;
 *    }]
 */
std::unique_ptr<pub::Value> ListEventManager::GenerateVisibleCellsInfo(
    float scroll_left, float scroll_top, bool for_scroll_info) const {
  std::unique_ptr<pub::Value> visible_cells_info;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory) {
    visible_cells_info = value_factory->CreateArray();
    float layouts_unit_per_px =
        list_container_->list_delegate()->GetLayoutsUnitPerPx();
    if (visible_cells_info && base::FloatsLarger(layouts_unit_per_px, 0.f)) {
      ListAdapter* list_adapter = list_container_->list_adapter();
      children_helper_->ForEachChild(
          children_helper_->on_screen_children(),
          [list_adapter, layouts_unit_per_px, scroll_left, scroll_top,
           for_scroll_info, &visible_cells_info,
           &value_factory](ItemHolder* item_holder) {
            ItemElementDelegate* list_item_delegate =
                list_adapter->GetItemElementDelegate(item_holder);
            if (list_item_delegate) {
              auto item_info = value_factory->CreateMap();
              if (item_info) {
                item_info->PushStringToMap(list::kCellInfoItemKey,
                                           item_holder->item_key());
                item_info->PushInt32ToMap(list::kCellInfoIndex,
                                          item_holder->index());
                float left = item_holder->left();
                float top = item_holder->top();
                if (for_scroll_info) {
                  // scroll info
                  item_info->PushStringToMap(
                      list::kCellInfoIdSelector,
                      list_item_delegate->GetIdSelector());
                  item_info->PushDoubleToMap(
                      list::kCellInfoTop,
                      (top - scroll_top) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      list::kCellInfoBottom,
                      (top + item_holder->height()) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      list::kCellInfoLeft,
                      (left - scroll_left) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      list::kCellInfoRight,
                      (left + item_holder->width()) / layouts_unit_per_px);
                  // for legacy API
                  item_info->PushInt32ToMap(list::kCellInfoPosition,
                                            item_holder->index());
                } else {
                  // layout info
                  item_info->PushDoubleToMap(list::kCellInfoOriginX,
                                             left / layouts_unit_per_px);
                  item_info->PushDoubleToMap(list::kCellInfoOriginY,
                                             top / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      list::kCellInfoWidth,
                      item_holder->width() / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      list::kCellInfoHeight,
                      item_holder->height() / layouts_unit_per_px);
                  item_info->PushBoolToMap(
                      list::kCellInfoIsBinding,
                      list_adapter->IsBinding(item_holder));
                  item_info->PushBoolToMap(
                      list::kCellInfoUpdated,
                      list_adapter->IsUpdated(item_holder));
                }
                visible_cells_info->PushValueToArray(*item_info);
              }
            }
            return false;
          });
    }
  }
  return visible_cells_info;
}

void ListEventManager::CreateLayoutCompleteInfoIfNeeded() {
  if (!layout_complete_info_) {
    layout_complete_info_ = list_container_->value_factory()->CreateMap();
  }
}

}  // namespace list
}  // namespace lynx
