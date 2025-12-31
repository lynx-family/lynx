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
    DLIST_LOGE(
        "ListEventManager::ListEventManager: list_container_ is nullptr");
  }
}

void ListEventManager::SendLayoutCompleteEvent() {
  if (!list_container_ || !list_container_->value_factory() ||
      !list_container_->list_delegate()->HasBoundEvent(kEventLayoutComplete)) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  if (layout_complete_info_) {
    // Move and clear layout_complete_info_
    std::unique_ptr<pub::Value> layout_complete_info =
        std::move(layout_complete_info_);
    // push layout id
    layout_complete_info->PushInt32ToMap(kLayoutInfoLayoutId,
                                         list_container_->layout_id());
    // push scroll info if needed
    std::unique_ptr<pub::Value> scroll_info;
    if (need_layout_complete_info_ &&
        (scroll_info = GenerateScrollInfo(0.f, 0.f))) {
      layout_complete_info->PushValueToMap(kLayoutInfoScrollInfo, *scroll_info);
    }
    list_container_->ResetLayoutID();
    list_container_->list_delegate()->SendCustomEvent(
        kEventLayoutComplete, kEventParamDetail,
        std::move(layout_complete_info));
  }
}

void ListEventManager::RecordVisibleItemIfNeeded(bool is_layout_before) {
  if (!need_layout_complete_info_ || !list_container_->value_factory()) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  if (layout_complete_info_) {
    // push eventUnit
    layout_complete_info_->PushStringToMap(kLayoutInfoEventUnit,
                                           kLayoutInfoEventUnitValuePx);
    // push visibleItemBeforeUpdate or visibleItemAfterUpdate
    std::unique_ptr<pub::Value> visible_cells_info;
    if ((visible_cells_info = GenerateVisibleCellsInfo(0.f, 0.f, false))) {
      layout_complete_info_->PushValueToMap(
          is_layout_before ? kLayoutInfoVisibleItemBeforeUpdate
                           : kLayoutInfoVisibleItemAfterUpdate,
          *visible_cells_info);
    }
  }
}

void ListEventManager::RecordDiffResultIfNeeded() {
  if (!need_layout_complete_info_ || !list_container_->value_factory()) {
    return;
  }
  CreateLayoutCompleteInfoIfNeeded();
  if (layout_complete_info_) {
    std::unique_ptr<pub::Value> diff_result;
    if ((diff_result = list_container_->list_adapter()->GenerateDiffResult())) {
      layout_complete_info_->PushValueToMap(kLayoutInfoDiffResult,
                                            *diff_result);
    }
  }
}

void ListEventManager::SendScrollEvent(float distance,
                                       EventSource event_source) {
  if (base::IsZero(distance)) {
    return;
  }
  // sendScrollEvent
  auto now = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now - last_scroll_event_time_)
                      .count();
  if (duration > scroll_event_throttle_ms_) {
    SendCustomScrollEvent(kEventScroll, distance, event_source);
    last_scroll_event_time_ = now;
  }
}

void ListEventManager::DetectScrollToThresholdAndSend(
    float distance, float original_offset, list::EventSource event_source) {
  if (!list_container_) {
    return;
  }
  ListLayoutManager* list_layout_manager =
      list_container_->list_layout_manager();
  ListChildrenHelper* list_children_helper =
      list_container_->list_children_helper();
  bool is_upper = false;
  bool is_lower = false;
  bool is_lower_edge = false;
  bool is_upper_edge = false;

  // calculate the firstItemIndex & lastItemIndex
  int first_index = INT_MAX;
  int end_index = INT_MIN;
  auto item_holder_list = list_children_helper->on_screen_children();
  for (auto it = item_holder_list.begin(); it != item_holder_list.end(); ++it) {
    auto item_holder = *it;
    if (item_holder) {
      first_index = std::min(item_holder->index(), first_index);
      end_index = std::max(item_holder->index(), end_index);
    }
  }
  float content_offset = list_layout_manager->content_offset();
  float content_size = list_layout_manager->content_size();
  float list_size = list_layout_manager->main_axis_size();

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
      list_children_helper->GetChildCount() - lower_threshold_item_count_ - 1;
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
  if (event_source == EventSource::kDiff ||
      event_source == EventSource::kLayout) {
    // 1. Force sending lower/upper event after diff or layout
    if (is_upper) {
      SendCustomScrollEvent(kEventScrollToUpper, distance, event_source);
    }
    if (is_lower) {
      SendCustomScrollEvent(kEventScrollToLower, distance, event_source);
    }
  } else if (event_source == EventSource::kScroll) {
    // 2. Handle event from scroll.
    ScrollPositionState previous_state = previous_scroll_pos_state_;
    if (is_upper && (previous_state != ScrollPositionState::kUpper &&
                     previous_state != ScrollPositionState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kEventScrollToUpper, distance, event_source);
    }
    if (is_lower && (previous_state != ScrollPositionState::kLower &&
                     previous_state != ScrollPositionState::kBothEdge)) {
      // Update previous_status and valid_diff flag before sending event to
      // avoid reenter in worklet.
      UpdatePreviousScrollState(is_lower, is_upper);
      SendCustomScrollEvent(list::kEventScrollToLower, distance, event_source);
    }
    UpdatePreviousScrollState(is_lower, is_upper);
  }

  // Send scroll to upper/lower edge event.
  if (is_upper_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(kEventScrollToUpperEdge, 0, event_source);
  }
  if (is_lower_edge &&
      NotAtBouncesArea(original_offset, content_size, list_size)) {
    SendCustomScrollEvent(kEventScrollToLowerEdge, 0, event_source);
  }
  if (!is_lower_edge && !is_upper_edge) {
    SendCustomScrollEvent(kEventScrollToNormalState, 0, event_source);
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
    previous_scroll_pos_state_ = ScrollPositionState::kBothEdge;
  } else if (is_lower) {
    previous_scroll_pos_state_ = ScrollPositionState::kLower;
  } else if (is_upper) {
    previous_scroll_pos_state_ = ScrollPositionState::kUpper;
  } else {
    previous_scroll_pos_state_ = ScrollPositionState::kMiddle;
  }
}

void ListEventManager::SendCustomScrollEvent(const std::string& event_name,
                                             float distance,
                                             EventSource event_source) {
  if (!list_container_ || !list_container_->value_factory() ||
      !list_container_->list_delegate()->HasBoundEvent(event_name)) {
    return;
  }
  ListLayoutManager* list_layout_manager =
      list_container_->list_layout_manager();
  bool is_vertical = list_layout_manager->CanScrollVertically();
  float content_offset = list_layout_manager->content_offset();
  float scroll_left = !is_vertical ? content_offset : 0.f;
  float scroll_top = is_vertical ? content_offset : 0.f;
  float dx = !is_vertical ? distance : 0.f;
  float dy = is_vertical ? distance : 0.f;
  float layouts_unit_per_px =
      list_container_->list_delegate()->GetLayoutsUnitPerPx();
  if (base::FloatsLarger(layouts_unit_per_px, 0.f)) {
    std::unique_ptr<pub::Value> scroll_info = GenerateScrollInfo(dx, dy);
    if (scroll_info) {
      // push eventSource
      scroll_info->PushInt32ToMap(kScrollInfoEventSource,
                                  static_cast<int>(event_source));
      std::unique_ptr<pub::Value> visible_cells_info;
      // push attachedCells if needed.
      if (need_visible_cell_ && (visible_cells_info = GenerateVisibleCellsInfo(
                                     scroll_left, scroll_top, true))) {
        scroll_info->PushValueToMap(kScrollInfoAttachedCells,
                                    *visible_cells_info);
      }
      list_container_->list_delegate()->SendCustomEvent(
          event_name, kEventParamDetail, std::move(scroll_info));
    }
  }
}

void ListEventManager::SendDiffDebugEventIfNeeded() {
  const auto& value_factory = list_container_->value_factory();
  if (value_factory &&
      ShouldGenerateDebugInfo(ListDebugInfoLevel::kListDebugInfoLevelInfo)) {
    std::unique_ptr<pub::Value> debug_info;
    std::unique_ptr<pub::Value> diff_result;
    if ((debug_info = value_factory->CreateMap()) &&
        (diff_result = list_container_->list_adapter()->GenerateDiffResult())) {
      // push diff result
      debug_info->PushValueToMap(kDebugInfoDiffResult, *diff_result);
      list_container_->list_delegate()->SendCustomEvent(
          kEventListDebugInfo, kEventParamDetail, std::move(debug_info));
    }
  }
}

void ListEventManager::SendAnchorDebugInfoIfNeeded(
    const ListAnchorManager::AnchorInfo& anchor_info) {
  const auto& value_factory = list_container_->value_factory();
  if (value_factory &&
      ShouldGenerateDebugInfo(ListDebugInfoLevel::kListDebugInfoLevelInfo)) {
    std::unique_ptr<pub::Value> debug_info;
    std::unique_ptr<pub::Value> anchor_info_map;
    if ((debug_info = value_factory->CreateMap()) &&
        (anchor_info_map = value_factory->CreateMap())) {
      if (!anchor_info.valid_) {
        anchor_info_map->PushInt32ToMap(kDebugInfoAnchorIndex, kInvalidIndex);
      } else {
        anchor_info_map->PushInt32ToMap(kDebugInfoAnchorIndex,
                                        anchor_info.index_);
        anchor_info_map->PushDoubleToMap(kDebugInfoAnchorStartOffset,
                                         anchor_info.start_offset_);
        anchor_info_map->PushDoubleToMap(kDebugInfoAnchorStartAlignmentDelta,
                                         anchor_info.start_alignment_delta_);
        anchor_info_map->PushBoolToMap(
            kDebugInfoAnchorDirty,
            list_container_->list_adapter()->IsDirty(anchor_info.item_holder_));
        anchor_info_map->PushBoolToMap(
            kDebugInfoAnchorBinding, list_container_->list_adapter()->IsBinding(
                                         anchor_info.item_holder_));
      }
      debug_info->PushValueToMap(kDebugInfoAnchorInfo, *anchor_info_map);
      list_container_->list_delegate()->SendCustomEvent(
          kEventListDebugInfo, kEventParamDetail, std::move(debug_info));
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
    list_item_delegate->SendCustomEvent(event_name, kEventParamDetail,
                                        std::move(exposure_info));
  }
}

std::unique_ptr<pub::Value> ListEventManager::GenerateNodeExposureInfo(
    const ItemHolder* item_holder) const {
  std::unique_ptr<pub::Value> exposure_info;
  const auto& value_factory = list_container_->value_factory();
  if (value_factory && (exposure_info = value_factory->CreateMap())) {
    // push index
    exposure_info->PushInt32ToMap(kCellInfoIndex, item_holder->index());
    // push key
    exposure_info->PushStringToMap(kCellInfoItemKey, item_holder->item_key());
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

      scroll_info->PushDoubleToMap(kScrollInfoScrollLeft,
                                   is_vertical ? 0.f : content_offset);
      scroll_info->PushDoubleToMap(kScrollInfoScrollTop,
                                   !is_vertical ? 0.f : content_offset);
      scroll_info->PushDoubleToMap(kScrollInfoScrollWidth,
                                   is_vertical ? list_width : content_size);
      scroll_info->PushDoubleToMap(kScrollInfoScrollHeight,
                                   !is_vertical ? list_height : content_size);
      scroll_info->PushDoubleToMap(kScrollInfoListWidth, list_width);
      scroll_info->PushDoubleToMap(kScrollInfoListHeight, list_height);
      scroll_info->PushDoubleToMap(kScrollInfoDeltaX,
                                   deltaX / layouts_unit_per_px);
      scroll_info->PushDoubleToMap(kScrollInfoDeltaY,
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
      ListChildrenHelper* list_children_helper =
          list_container_->list_children_helper();
      list_children_helper->ForEachChild(
          list_children_helper->on_screen_children(),
          [list_adapter, layouts_unit_per_px, scroll_left, scroll_top,
           for_scroll_info, &visible_cells_info,
           &value_factory](ItemHolder* item_holder) {
            ItemElementDelegate* list_item_delegate =
                list_adapter->GetItemElementDelegate(item_holder);
            if (list_item_delegate) {
              auto item_info = value_factory->CreateMap();
              if (item_info) {
                item_info->PushStringToMap(kCellInfoItemKey,
                                           item_holder->item_key());
                item_info->PushInt32ToMap(kCellInfoIndex, item_holder->index());
                float left = item_holder->left();
                float top = item_holder->top();
                if (for_scroll_info) {
                  // scroll info
                  item_info->PushStringToMap(
                      kCellInfoIdSelector, list_item_delegate->GetIdSelector());
                  item_info->PushDoubleToMap(
                      kCellInfoTop, (top - scroll_top) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      kCellInfoBottom,
                      (top + item_holder->height()) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      kCellInfoLeft,
                      (left - scroll_left) / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      kCellInfoRight,
                      (left + item_holder->width()) / layouts_unit_per_px);
                  // for legacy API
                  item_info->PushInt32ToMap(kCellInfoPosition,
                                            item_holder->index());
                } else {
                  // layout info
                  item_info->PushDoubleToMap(kCellInfoOriginX,
                                             left / layouts_unit_per_px);
                  item_info->PushDoubleToMap(kCellInfoOriginY,
                                             top / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      kCellInfoWidth,
                      item_holder->width() / layouts_unit_per_px);
                  item_info->PushDoubleToMap(
                      kCellInfoHeight,
                      item_holder->height() / layouts_unit_per_px);
                  item_info->PushBoolToMap(
                      kCellInfoIsBinding, list_adapter->IsBinding(item_holder));
                  item_info->PushBoolToMap(
                      kCellInfoUpdated, list_adapter->IsUpdated(item_holder));
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
  if (!layout_complete_info_ && list_container_->value_factory()) {
    layout_complete_info_ = list_container_->value_factory()->CreateMap();
  }
}

bool ListEventManager::ShouldGenerateDebugInfo(
    ListDebugInfoLevel target_level) const {
  ElementDelegate* list_delegate = list_container_->list_delegate();
  return list_delegate->IsInDebugMode() &&
         list_delegate->HasBoundEvent(kEventListDebugInfo) &&
         debug_info_level_ >= target_level;
}

}  // namespace list
}  // namespace lynx
