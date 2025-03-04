// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_layout_manager.h"

#include <algorithm>
#include <vector>

#include "base/include/log/logging.h"
#include "core/renderer/ui_component/list/list_anchor_manager.h"
#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

ListLayoutManager::ListLayoutManager(ListContainerImpl* list_container_impl)
    : list_container_(list_container_impl) {
  if (!list_container_) {
    LOGE("[ListLayoutManager] error: "
         << "list_container_ is nullptr");
  }
}

void ListLayoutManager::SetListAnchorManager(
    ListChildrenHelper* list_children_helper) {
  if (!list_anchor_manager_) {
    list_anchor_manager_ = std::make_unique<ListAnchorManager>(this);
  }
  list_anchor_manager_->SetListOrientationHelper(
      list_orientation_helper_.get());
  list_anchor_manager_->SetListAdapter(list_container_->list_adapter());
  list_anchor_manager_->SetListChildrenHelper(list_children_helper_);
  list_anchor_manager_->SetListContainer(list_container_);
}

float ListLayoutManager::GetWidth() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()->width() -
           list_container_->element()
               ->borders()[static_cast<uint32_t>(list::FrameDirection::kLeft)] -
           list_container_->element()
               ->borders()[static_cast<uint32_t>(list::FrameDirection::kRight)];
  }
  return 0.f;
}

float ListLayoutManager::GetHeight() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()->height() -
           list_container_->element()
               ->borders()[static_cast<uint32_t>(list::FrameDirection::kTop)] -
           list_container_->element()->borders()[static_cast<uint32_t>(
               list::FrameDirection::kBottom)];
  }
  return 0.f;
}

float ListLayoutManager::GetHeightAfterPadding() const {
  if (list_container_ && list_container_->element()) {
    return list_orientation_helper_->GetEndAfterPadding() -
           list_orientation_helper_->GetStartAfterPadding();
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingLeft() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()
        ->paddings()[static_cast<uint32_t>(list::FrameDirection::kLeft)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingRight() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()
        ->paddings()[static_cast<uint32_t>(list::FrameDirection::kRight)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingTop() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()
        ->paddings()[static_cast<uint32_t>(list::FrameDirection::kTop)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingBottom() const {
  if (list_container_ && list_container_->element()) {
    return list_container_->element()
        ->paddings()[static_cast<uint32_t>(list::FrameDirection::kBottom)];
  }
  return 0.f;
}

void ListLayoutManager::SendAnchorDebugInfo(
    ListAnchorManager::AnchorInfo& anchor_info) {
  if (list_container_->ShouldGenerateDebugInfo(
          list::ListDebugInfoLevel::kListDebugInfoLevelInfo)) {
    auto detail = lepus::Dictionary::Create();
    auto anchor_info_map = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kAnchorIndex, "anchor_index");
    BASE_STATIC_STRING_DECL(kStartOffset, "start_offset");
    BASE_STATIC_STRING_DECL(kStartAlignmentDelta, "start_alignment_delta");
    BASE_STATIC_STRING_DECL(kDirty, "dirty");
    BASE_STATIC_STRING_DECL(kBinding, "binding");
    BASE_STATIC_STRING_DECL(kAnchorInfo, "anchor_info");
    if (!anchor_info.valid_) {
      anchor_info_map->SetValue(kAnchorIndex, list::kInvalidIndex);
    } else {
      ListAdapter* list_adapter = list_container_->list_adapter();
      anchor_info_map->SetValue(kAnchorIndex, anchor_info.index_);
      anchor_info_map->SetValue(kStartOffset, anchor_info.start_offset_);
      anchor_info_map->SetValue(kStartAlignmentDelta,
                                anchor_info.start_alignment_delta_);
      anchor_info_map->SetValue(
          kDirty, list_adapter->IsDirty(anchor_info.item_holder_));
      anchor_info_map->SetValue(
          kBinding, list_adapter->IsBinding(anchor_info.item_holder_));
    }
    auto detail_info = lepus::Dictionary::Create();
    detail_info->SetValue(kAnchorInfo, anchor_info_map);
    detail->SetValue(BASE_STATIC_STRING(list::kListDebugInfoLevelInfo),
                     detail_info);
    list_container_->SendDebugEvent(detail);
  }
}

void ListLayoutManager::InitLayoutAndAnchor(
    ListAnchorManager::AnchorInfo& anchor_info, int finishing_binding_index) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "RetrieveAnchorInfoBeforeLayout");
  // Record the current anchor information BEFORE laying out the item_holders as
  // the layout result should be connected to the previous onScreen status.
  list_anchor_manager_->RetrieveAnchorInfoBeforeLayout(anchor_info,
                                                       finishing_binding_index);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  if (!anchor_info.valid_) {
    return;
  }
  LayoutInvalidItemHolder(0);
  content_size_ = GetTargetContentSize();
  // After LayoutInvalidItemHolder, the anchor item_holder's top or left may
  // changed so it has to be adjusted.
  list_anchor_manager_->AdjustAnchorInfoAfterLayout(anchor_info);
}

void ListLayoutManager::SetListLayoutInfoToAllItemHolders() {
  if (!list_children_helper_ || !list_orientation_helper_) {
    LOGE(
        "ListLayoutManager::SetListLayoutInfoToAllItemHolders: "
        "list_children_helper_ or list_orientation_helper_ is nullptr");
    return;
  }
  list_children_helper_->ForEachChild(
      [container_size = list_orientation_helper_->GetMeasurement(),
       is_rtl = list_container_->IsRTL()](ItemHolder* item_holder) {
        item_holder->SetContainerSize(container_size);
        item_holder->SetDirection(is_rtl ? list::Direction::kRTL
                                         : list::Direction::kNormal);
        return false;
      });
}

// Set layout orientation, and if list_orientation_helper_ == nullptr or
// orientation changed, create new list_orientation_helper_.
void ListLayoutManager::SetOrientation(list::Orientation orientation) {
  if (orientation_ == orientation && list_orientation_helper_ != nullptr) {
    return;
  }
  orientation_ = orientation;
  list_orientation_helper_ =
      ListOrientationHelper::CreateListOrientationHelper(this, orientation);
}

void ListLayoutManager::SetSpanCount(int span_count) {
  span_count_ = span_count;
  InitLayoutState();
}

// Receives scrolling events from the platform.
void ListLayoutManager::ScrollByPlatformContainer(float content_offset_x,
                                                  float content_offset_y,
                                                  float original_x,
                                                  float original_y) {
  ScrollByInternal(
      orientation_ == list::Orientation::kHorizontal ? content_offset_x
                                                     : content_offset_y,
      orientation_ == list::Orientation::kHorizontal ? original_x : original_y,
      true);
}

// Platform UI will invoke this function when scrollToPosition UI method is
// invoked and pass parameters to ListLayoutManager.
void ListLayoutManager::ScrollToPosition(int index, float offset, int align,
                                         bool smooth) {
  ItemHolder* item_holder = nullptr;
  if (!list_container_ || !list_orientation_helper_ ||
      !(item_holder = list_container_->GetItemHolderForIndex(index)) ||
      !list_anchor_manager_) {
    return;
  }
  list_anchor_manager_->InitScrollToPositionParam(item_holder, index, offset,
                                                  align, smooth);
  NLIST_LOGI("[list_container=" << list_container_ << "] ScrollToPosition: "
                                << item_holder << ", " << index << ", "
                                << offset << ", " << align << ", " << smooth);
  if (smooth) {
    float target_offset =
        list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
    list_container_->UpdateScrollInfo(target_offset, smooth, false);
  } else {
    // scroll to index by layout, by initial-scroll-index
    // is_scroll_to_position_ will block layout_complete event
    is_scroll_to_position_ = true;
    OnLayoutChildren();
    is_scroll_to_position_ = false;

    // Invalidate consumed index to avoid double calculation
    list_anchor_manager_->InvalidateScrollInfoPosition();
    float target_offset =
        list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
    // scroll to additional offset
    if (base::FloatsNotEqual(0, offset) ||
        align != static_cast<int>(list::ScrollingInfoAlignment::kTop)) {
      ScrollByInternal(target_offset, target_offset, false);
    }
  }
}

// Platform UI will invoke this function when scrollToPosition UI method is
// finished to clear ListLayoutManager's related scrolling info.
void ListLayoutManager::ScrollStopped() {
  NLIST_LOGI("[list_container=" << list_container_ << "] ScrollStopped");
  list_anchor_manager_->ResetScrollInfo();
}

// Determine whether the current ItemHolder needs to be recycled.
bool ListLayoutManager::ShouldRecycleItemHolder(ItemHolder* item_holder) {
  if (!item_holder || !list_orientation_helper_) {
    return false;
  }
  return !ItemHolderVisibleInList(item_holder);
}

bool ListLayoutManager::ItemHolderVisibleInList(ItemHolder* item_holder) {
  if (!item_holder) {
    return false;
  }
  return item_holder->VisibleInList(list_orientation_helper_.get(),
                                    content_offset_);
}

// Recycle all off-screen ItemHolders. It will be invoked after layouting
// children or handling scroll events.
void ListLayoutManager::RecycleOffScreenItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListLayoutManager::RecycleOffScreenItemHolders");
  if (!list_children_helper_) {
    return;
  }
  std::vector<ItemHolder*> off_screen_item_holders;
  list_children_helper_->ForEachChild(
      list_children_helper_->attached_children(),
      [this, &off_screen_item_holders](ItemHolder* item_holder) {
        if (item_holder && ShouldRecycleItemHolder(item_holder) &&
            IsItemHolderNotSticky(item_holder)) {
          off_screen_item_holders.push_back(item_holder);
        }
        return false;
      });
  ListAdapter* list_adapter = list_container_->list_adapter();
  for (auto item_holder : off_screen_item_holders) {
    Element* list_item = list_adapter->GetListItemElement(item_holder);
    if (list_item && list_container_->should_request_state_restore()) {
      list_container_->element_manager()->painting_context()->ListCellDisappear(
          list_item->impl_id(), true, item_holder->item_key());
    }
    list_container_->list_adapter()->RecycleItemHolder(item_holder);
  }
}

// Update content size and content offset and flush to platform by invoking
// ListContainer::UpdateContentOffsetAndSizeToPlatform().
void ListLayoutManager::FlushContentSizeAndOffsetToPlatform(
    float content_offset_before_adjustment) {
  content_offset_ = ClampContentOffsetToEdge(content_offset_, content_size_);
  float delta_x = orientation_ == list::Orientation::kVertical
                      ? 0.f
                      : content_offset_ - content_offset_before_adjustment;
  float delta_y = orientation_ == list::Orientation::kVertical
                      ? content_offset_ - content_offset_before_adjustment
                      : 0.f;
  if (list_container_) {
    list_container_->UpdateContentOffsetAndSizeToPlatform(
        content_size_, delta_x, delta_y,
        list_anchor_manager_->initial_scroll_index_status() ==
            list::InitialScrollIndexStatus::kSet);
  }
  FlushScrollInfoToPlatformIfNeeded();
}

void ListLayoutManager::FlushScrollInfoToPlatformIfNeeded() {
  if (list_container_ && list_anchor_manager_->IsValidSmoothScrollInfo()) {
    const ListAnchorManager::ScrollingInfo& scrolling_info =
        list_anchor_manager_->scrolling_info();
    ItemHolder* item_holder = list_container_->GetItemHolderForIndex(
        scrolling_info.scrolling_target_);
    if (item_holder) {
      if (item_holder != scrolling_info.item_holder_) {
        NLIST_LOGE(
            "FlushScrollInfoToPlatformIfNeeded: target item holder in "
            "scrolling_info_ is not exist: "
            << scrolling_info.item_holder_ << ", " << item_holder);
      }
      float target_offset =
          list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
      list_container_->UpdateScrollInfo(target_offset, true, true);
    } else {
      list_anchor_manager_->ResetScrollInfo();
    }
  }
}

// Callback before layout.
void ListLayoutManager::OnPrepareForLayoutChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListLayoutManager::OnPrepareForLayoutChildren");
  list_container_->RecordVisibleItemIfNeeded(true);
}

void ListLayoutManager::SendLayoutCompleteEvent(float scroll_delta) {
  // The bindlayoutcomplete event always works with a worklet to ensure
  // immediate operation. Since a worklet may change a component's size and
  // trigger another layout process, this event should be sent after the
  // StopInterceptListElementUpdated to ensure that the layout inside it goes
  // without blocking.
  ListEventManager* event_manager = list_container_->list_event_manager();
  if (event_manager && !is_scroll_to_position_) {
    event_manager->SendLayoutCompleteInfo();
  }
}

void ListLayoutManager::SendScrollEvents(float scroll_delta,
                                         float original_offset,
                                         list::EventSource event_source) {
  list_container_->list_event_manager()->OnScroll(scroll_delta, event_source);
  list_container_->list_event_manager()->DetectScrollToThresholdAndSend(
      scroll_delta, content_offset_, event_source);
}

// Callback if layout finished.
void ListLayoutManager::OnLayoutCompleted() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListLayoutManager::OnLayoutCompleted");
  // Recycle all removed child
  if (!list_container_ || !list_children_helper_) {
    return;
  }
  ListAdapter* list_adapter = list_container_->list_adapter();
  if (list_adapter) {
    list_adapter->RecycleRemovedItemHolders();
  }
  // Update layout to platform
  list_children_helper_->ForEachChild([this](ItemHolder* item_holder) {
    item_holder->UpdateLayoutToPlatform(content_size_, GetWidth());
    return false;
  });
  list_container_->RecordVisibleItemIfNeeded(false);
  list_container_->FlushPatching();
}

// Render sticky nodes if needed.
int ListLayoutManager::UpdateStickyItems() {
  if (!list_container_ || !list_container_->list_adapter() ||
      !list_container_->sticky_enabled()) {
    return list::kInvalidIndex;
  }
  int minimum_layout_changed_item_holder_index = list::kInvalidIndex;
  float sticky_offset = list_container_->sticky_offset();

  // enumerate from end to begin, find the first visible sticky-top item
  const std::vector<int32_t>& sticky_top_items =
      list_container_->list_adapter()->GetStickyTops();

  for (auto iter = sticky_top_items.rbegin(); iter != sticky_top_items.rend();
       iter++) {
    int index = *iter;
    if (UpdateStickyItemsInternal(minimum_layout_changed_item_holder_index,
                                  sticky_offset, index)) {
      break;
    }
  }

  // enumerate from begin to end, find the first visible sticky-bottom item
  const std::vector<int32_t>& sticky_bottom_items =
      list_container_->list_adapter()->GetStickyBottoms();

  for (auto iter = sticky_bottom_items.begin();
       iter != sticky_bottom_items.end(); iter++) {
    int index = *iter;
    if (UpdateStickyItemsInternal(minimum_layout_changed_item_holder_index,
                                  sticky_offset, index)) {
      break;
    }
  }

  return minimum_layout_changed_item_holder_index;
}

bool ListLayoutManager::UpdateStickyItemsInternal(int& layout_changed_position,
                                                  float sticky_offset,
                                                  int index) {
  ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);

  if (item_holder->IsAtStickyPosition(
          content_offset_, GetHeight(), content_size_, sticky_offset,
          list_orientation_helper_->GetDecoratedStart(item_holder),
          list_orientation_helper_->GetDecoratedEnd(item_holder))) {
    float size_before_bind =
        list_orientation_helper_->GetDecoratedMeasurement(item_holder);

    // bind it
    list_container_->list_adapter()->BindItemHolder(item_holder, index);

    // check if size changed
    if (base::FloatsNotEqual(
            list_orientation_helper_->GetDecoratedMeasurement(item_holder),
            size_before_bind)) {
      if (layout_changed_position == list::kInvalidIndex ||
          layout_changed_position > item_holder->index()) {
        layout_changed_position = item_holder->index();
      }
    }
    return true;
  }
  return false;
}

void ListLayoutManager::UpdateStickyItemsAfterLayout(
    ListAnchorManager::AnchorInfo& anchor_info) {
  //    If the list has sticky items, the sticky items should be updated after
  //    the first adjustment to obtain information about which sticky items will
  //    enter their sticky mode. Since new sticky items may trigger extra
  //    bindings and cause additional layout changes, which requires an update
  //    to the layout afterwards.
  if (list_container_->sticky_enabled()) {
    int minimum_layout_updated_index = UpdateStickyItems();
    minimum_layout_updated_index =
        std::max(minimum_layout_updated_index - 1, 0);
    // Layout and adjust scroll status again
    LayoutInvalidItemHolder(minimum_layout_updated_index);
    content_size_ = GetTargetContentSize();
    list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                        content_offset_);
  }
}

void ListLayoutManager::HandleLayoutOrScrollResult(bool is_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ListLayoutManager::HandlePlatformOperation");
  ListAdapter* list_adapter = list_container_->list_adapter();
  // The Handler to update layout info to platform.
  auto update_layout_handler = [this, list_adapter](ItemHolder* item_holder) {
    item_holder->UpdateLayoutToPlatform(
        content_size_, GetWidth(),
        list_adapter->GetListItemElement(item_holder));
    return false;
  };
  if (list_container_->sticky_enabled()) {
    list_children_helper_->UpdateInStickyChildren(
        list_orientation_helper_.get(), content_offset_, content_size_,
        list_container_->sticky_offset());
  }
  // The Handler to insert platform view and update layout info to platform ui
  // including:
  //   (1) on screen children
  //   (2) in preload children
  //   (3) sticky children
  auto insert_handler =
      [list_adapter,
       list_element = list_container_->element()](ItemHolder* item_holder) {
        Element* list_item = list_adapter->GetListItemElement(item_holder);
        if (list_item) {
          list_element->element_manager()
              ->painting_context()
              ->InsertListItemPaintingNode(list_element->impl_id(),
                                           list_item->impl_id());
        }
        return false;
      };
  // The Handler to recycle off-screen or off-preload's item holder.
  auto recycle_handler = [list_adapter](ItemHolder* item_holder) {
    list_adapter->RecycleItemHolder(item_holder);
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_layout_handler);
  // Recycle all removed child.
  if (is_layout) {
    list_adapter->RecycleRemovedItemHolders();
  }
  list_container_->FlushPatching();
}

// Clamp content offset within scrollable range.
float ListLayoutManager::ClampContentOffsetToEdge(float content_offset,
                                                  float content_size) {
  if (!list_orientation_helper_) {
    return content_offset;
  }
  float scroll_range =
      content_size - list_orientation_helper_->GetMeasurement();
  return std::max(0.f, std::min(content_offset, scroll_range));
}

bool ListLayoutManager::IsItemHolderNotSticky(
    const ItemHolder* item_holder) const {
  int sticky_offset = list_container_->sticky_offset();

  bool sticky_enabled = list_container_->sticky_enabled();
  return !sticky_enabled || !item_holder->sticky() ||
         !item_holder->IsAtStickyPosition(
             content_offset_, GetHeight(), content_size_, sticky_offset,
             list_orientation_helper_->GetDecoratedStart(item_holder),
             list_orientation_helper_->GetDecoratedEnd(item_holder));
}

}  // namespace tasm
}  // namespace lynx
