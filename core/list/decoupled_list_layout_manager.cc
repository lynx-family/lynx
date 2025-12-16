// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_layout_manager.h"

#include <algorithm>
#include <vector>

#include "base/include/log/logging.h"
#include "core/list/decoupled_list_anchor_manager.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

ListLayoutManager::ListLayoutManager(ListContainerImpl* list_container_impl)
    : list_container_(list_container_impl) {
  if (!list_container_) {
    DLIST_LOGE(
        "ListLayoutManager::ListLayoutManager: list_container_ is nullptr");
  }
}

// When create ListContainerImpl or change list-type prop, we will invoke this
// function to init LayoutManager.
void ListLayoutManager::InitLayoutManager(
    ListChildrenHelper* list_children_helper, Orientation list_orientation) {
  list_children_helper_ = list_children_helper;
  // create list_orientation_helper_.
  SetOrientation(list_orientation);
  // create list_anchor_manager_.
  CreateOrUpdateListAnchorManager();
}

// Set layout orientation, and if list_orientation_helper_ == nullptr or
// orientation changed, create new list_orientation_helper_.
void ListLayoutManager::SetOrientation(Orientation orientation) {
  if (orientation_ == orientation && list_orientation_helper_ != nullptr) {
    return;
  }
  orientation_ = orientation;
  list_orientation_helper_ =
      ListOrientationHelper::CreateListOrientationHelper(this, orientation);
}

void ListLayoutManager::CreateOrUpdateListAnchorManager() {
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
  ElementDelegate* list_delegate = nullptr;
  if (list_container_ && (list_delegate = list_container_->list_delegate())) {
    return list_delegate->GetWidth() -
           list_delegate
               ->GetBorders()[static_cast<uint32_t>(FrameDirection::kLeft)] -
           list_delegate
               ->GetBorders()[static_cast<uint32_t>(FrameDirection::kRight)];
  }
  return 0.f;
}

float ListLayoutManager::GetHeight() const {
  ElementDelegate* list_delegate = nullptr;
  if (list_container_ && (list_delegate = list_container_->list_delegate())) {
    return list_delegate->GetHeight() -
           list_delegate
               ->GetBorders()[static_cast<uint32_t>(FrameDirection::kTop)] -
           list_delegate
               ->GetBorders()[static_cast<uint32_t>(FrameDirection::kBottom)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingLeft() const {
  if (list_container_ && list_container_->list_delegate()) {
    return list_container_->list_delegate()
        ->GetPaddings()[static_cast<uint32_t>(FrameDirection::kLeft)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingRight() const {
  if (list_container_ && list_container_->list_delegate()) {
    return list_container_->list_delegate()
        ->GetPaddings()[static_cast<uint32_t>(FrameDirection::kRight)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingTop() const {
  if (list_container_ && list_container_->list_delegate()) {
    return list_container_->list_delegate()
        ->GetPaddings()[static_cast<uint32_t>(FrameDirection::kTop)];
  }
  return 0.f;
}

float ListLayoutManager::GetPaddingBottom() const {
  if (list_container_ && list_container_->list_delegate()) {
    return list_container_->list_delegate()
        ->GetPaddings()[static_cast<uint32_t>(FrameDirection::kBottom)];
  }
  return 0.f;
}

void ListLayoutManager::InitLayoutAndAnchor(
    ListAnchorManager::AnchorInfo& anchor_info, int finishing_binding_index) {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    LIST_LAYOUT_MANAGER_RETRIEVE_ANCHOR_INFO);
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
    DLIST_LOGE(
        "ListLayoutManager::SetListLayoutInfoToAllItemHolders: "
        "list_children_helper_ or list_orientation_helper_ is nullptr");
    return;
  }
  float container_size = list_orientation_helper_->GetMeasurement();
  if (!base::FloatsLarger(container_size, 0.f)) {
    DLIST_LOGE(
        "ListLayoutManager::SetListLayoutInfoToAllItemHolders: invalid list "
        "container's size.");
  }
  list_children_helper_->ForEachChild([container_size,
                                       is_rtl = list_container_->IsRTL()](
                                          ItemHolder* item_holder) {
    item_holder->SetContainerSize(container_size);
    item_holder->SetDirection(is_rtl ? Direction::kRTL : Direction::kNormal);
    return false;
  });
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
      orientation_ == Orientation::kHorizontal ? content_offset_x
                                               : content_offset_y,
      orientation_ == Orientation::kHorizontal ? original_x : original_y, true);
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
  DLIST_LOGI("[" << list_container_ << "] ScrollToPosition: " << item_holder
                 << ", " << index << ", " << offset << ", " << align << ", "
                 << smooth);
  if (smooth) {
    float target_offset =
        list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
    list_container_->list_delegate()->UpdateScrollInfo(target_offset, smooth,
                                                       false);
  } else {
    // scroll to index by layout, by initial-scroll-index
    // is_non_smooth_scroll_ will block layout_complete event
    is_non_smooth_scroll_ = true;
    OnLayoutChildren();
    // Invalidate consumed index to avoid double calculation
    list_anchor_manager_->InvalidateScrollInfoPosition();
    float target_offset =
        list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
    // scroll to additional offset
    if (base::FloatsNotEqual(0, offset) ||
        align != static_cast<int>(ScrollingInfoAlignment::kTop)) {
      ScrollByInternal(target_offset, target_offset, false);
    }
    is_non_smooth_scroll_ = false;
  }
}

// Platform UI will invoke this function when scrollToPosition UI method is
// finished to clear ListLayoutManager's related scrolling info.
void ListLayoutManager::ScrollStopped() {
  DLIST_LOGI("[" << list_container_ << "] ScrollStopped");
  list_anchor_manager_->ResetScrollInfo();
}

// Determine whether the current ItemHolder needs to be recycled.
bool ListLayoutManager::ShouldRecycleItemHolder(
    const ItemHolder* item_holder) const {
  if (!item_holder || !item_holder->recyclable() || !list_orientation_helper_) {
    return false;
  }
  return !ItemHolderVisibleInList(item_holder);
}

bool ListLayoutManager::ItemHolderVisibleInList(
    const ItemHolder* item_holder) const {
  if (!item_holder) {
    return false;
  }
  return item_holder->VisibleInList(list_orientation_helper_.get(),
                                    content_offset_);
}

// Recycle all off-screen ItemHolders. It will be invoked after layouting
// children or handling scroll events.
void ListLayoutManager::RecycleOffScreenItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_LAYOUT_MANAGER_RECYCLE_OFF_SCREEN_ITEM);
  if (!list_children_helper_) {
    return;
  }
  std::vector<ItemHolder*> off_screen_item_holders;
  list_children_helper_->ForEachChild(
      list_children_helper_->attached_children(),
      [this, &off_screen_item_holders](ItemHolder* item_holder) {
        if (item_holder && ShouldRecycleItemHolder(item_holder) &&
            ShouldRecycleStickyItemHolder(item_holder)) {
          off_screen_item_holders.push_back(item_holder);
        }
        return false;
      });
  ListAdapter* list_adapter = list_container_->list_adapter();
  ItemElementDelegate* list_item_delegate = nullptr;
  bool should_request_state_restore =
      list_container_->should_request_state_restore();
  for (auto item_holder : off_screen_item_holders) {
    list_item_delegate = list_adapter->GetItemElementDelegate(item_holder);
    if (list_item_delegate && should_request_state_restore) {
      list_item_delegate->OnListItemDisappear(true, item_holder->item_key());
    }
    list_container_->list_adapter()->RecycleItemHolder(item_holder);
  }
}

// Update content size and content offset and flush to platform by invoking
// ListContainer::UpdateContentOffsetAndSizeToPlatform().
void ListLayoutManager::FlushContentSizeAndOffsetToPlatform(
    float content_offset_before_adjustment, bool from_layout) {
  content_offset_ = ClampContentOffsetToEdge(content_offset_, content_size_);
  float delta_x = CanScrollVertically()
                      ? 0.f
                      : content_offset_ - content_offset_before_adjustment;
  float delta_y = CanScrollVertically()
                      ? content_offset_ - content_offset_before_adjustment
                      : 0.f;
  if (list_container_) {
    list_container_->UpdateContentOffsetAndSizeToPlatform(
        content_size_, delta_x, delta_y,
        list_anchor_manager_->IsValidInitialScrollIndex(),
        is_non_smooth_scroll_ || from_layout);
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
        DLIST_LOGE(
            "[" << list_container_
                << "] FlushScrollInfoToPlatformIfNeeded: target item holder in "
                   "scrolling_info_ is not exist: "
                << scrolling_info.item_holder_ << ", " << item_holder);
      }
      float target_offset =
          list_anchor_manager_->CalculateTargetScrollingOffset(item_holder);
      list_container_->list_delegate()->UpdateScrollInfo(target_offset, true,
                                                         true);
    } else {
      list_anchor_manager_->ResetScrollInfo();
    }
  }
}

// Callback before layout.
void ListLayoutManager::OnPrepareForLayoutChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              LIST_LAYOUT_MANAGER_PREPARE_FOR_LAYOUT_CHILDREN);
  SetListLayoutInfoToAllItemHolders();
  list_container_->list_event_manager()->RecordVisibleItemIfNeeded(true);
}

void ListLayoutManager::SendLayoutCompleteEvent() {
  // The bindlayoutcomplete event always works with a worklet to ensure
  // immediate operation. Since a worklet may change a component's size and
  // trigger another layout process, this event should be sent after the
  // StopInterceptListElementUpdated to ensure that the layout inside it goes
  // without blocking.
  ListEventManager* event_manager = list_container_->list_event_manager();
  if (event_manager && !is_non_smooth_scroll_) {
    event_manager->SendLayoutCompleteEvent();
  }
}

void ListLayoutManager::SendScrollEvents(float scroll_delta,
                                         float original_offset,
                                         EventSource event_source) {
  list_container_->list_event_manager()->SendScrollEvent(scroll_delta,
                                                         event_source);
  list_container_->list_event_manager()->DetectScrollToThresholdAndSend(
      scroll_delta, original_offset, event_source);
}

// Render sticky nodes if needed.
int ListLayoutManager::UpdateStickyItems() {
  if (!list_container_ || !list_container_->list_adapter() ||
      !list_container_->sticky_enabled()) {
    return kInvalidIndex;
  }
  int minimum_layout_changed_item_holder_index = kInvalidIndex;
  float sticky_offset = list_container_->sticky_offset();
  // If recycle sticky item, clear sticky item holder set firstly.
  if (list_container_->recycle_sticky_item()) {
    list_children_helper_->InitStickyItemHolderSet(
        list_container_->list_delegate()->GetThreadStrategy());
  }
  // Enumerate from end to begin, find the first visible sticky-top item.
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
  // Enumerate from begin to end, find the first visible sticky-bottom item.
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
          list_orientation_helper_->GetStart(item_holder),
          list_orientation_helper_->GetEnd(item_holder))) {
    float size_before_bind =
        list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    // Bind item_holder if needed.
    list_container_->list_adapter()->BindItemHolder(item_holder, index);
    // Check if size changed.
    if (base::FloatsNotEqual(
            list_orientation_helper_->GetDecoratedMeasurement(item_holder),
            size_before_bind)) {
      if (layout_changed_position == kInvalidIndex ||
          layout_changed_position > item_holder->index()) {
        layout_changed_position = item_holder->index();
      }
    }
    // If recycle sticky item, we need to add item holder to sticky item set and
    // return whether need to bind next sticky item holder.
    return list_container_->recycle_sticky_item()
               ? !list_children_helper_->AddToStickyItemHolderSet(item_holder)
               : true;
  }
  return false;
}

void ListLayoutManager::UpdateStickyItemsAfterLayout(
    ListAnchorManager::AnchorInfo& anchor_info) {
  // If the list has sticky items, the sticky items should be updated after
  // the first adjustment to obtain information about which sticky items will
  // enter their sticky mode. Since new sticky items may trigger extra
  // bindings and cause additional layout changes, which requires an update
  // to the layout afterwards.
  if (list_container_->sticky_enabled()) {
    int minimum_layout_updated_index = UpdateStickyItems();
    minimum_layout_updated_index = std::max(minimum_layout_updated_index, 0);
    // Layout and adjust scroll status again
    LayoutInvalidItemHolder(minimum_layout_updated_index);
    content_size_ = GetTargetContentSize();
    list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                        content_offset_);
  }
}

void ListLayoutManager::HandleLayoutOrScrollResult(bool is_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              LIST_LAYOUT_MANAGER_HANDLE_PLATFORM_OPERATION);
  // The Handler to update layout info to platform.
  auto update_layout_handler = [this](ItemHolder* item_holder) {
    item_holder->UpdateLayoutToPlatform(
        content_size_, GetWidth(),
        list_container_->list_adapter()->GetItemElementDelegate(item_holder));
    return false;
  };
  if (list_container_->sticky_enabled() &&
      !list_container_->recycle_sticky_item()) {
    list_children_helper_->UpdateInStickyChildren(
        list_orientation_helper_.get(), content_offset_, content_size_,
        list_container_->sticky_offset());
  }
  // The Handler to insert platform view and update layout info to platform ui
  // including:
  //   (1) on screen children
  //   (2) in preload children
  //   (3) sticky children
  auto insert_handler = [this](ItemHolder* item_holder) {
    ItemElementDelegate* list_item_delegate =
        list_container_->list_adapter()->GetItemElementDelegate(item_holder);
    if (list_item_delegate) {
      list_container_->list_delegate()->InsertListItemPaintingNode(
          list_item_delegate->GetImplId());
    }
    return false;
  };
  // The Handler to recycle off-screen or off-preload's item holder.
  auto recycle_handler = [this](ItemHolder* item_holder) {
    list_container_->list_adapter()->RecycleItemHolder(item_holder);
    return false;
  };
  list_children_helper_->HandleLayoutOrScrollResult(
      insert_handler, recycle_handler, update_layout_handler);
  // Recycle all removed child.
  if (is_layout) {
    list_container_->list_adapter()->RecycleRemovedItemHolders();
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

bool ListLayoutManager::ShouldRecycleStickyItemHolder(
    const ItemHolder* item_holder) const {
  if (list_container_->recycle_sticky_item()) {
    return !list_container_->sticky_enabled() || !item_holder->sticky() ||
           !list_children_helper_->InStickyItemHolderSet(item_holder);
  } else {
    return IsItemHolderNotAtStickyPosition(item_holder);
  }
}

bool ListLayoutManager::IsItemHolderNotAtStickyPosition(
    const ItemHolder* item_holder) const {
  int sticky_offset = list_container_->sticky_offset();
  bool sticky_enabled = list_container_->sticky_enabled();
  return !sticky_enabled || !item_holder->sticky() ||
         !item_holder->IsAtStickyPosition(
             content_offset_, GetHeight(), content_size_, sticky_offset,
             list_orientation_helper_->GetStart(item_holder),
             list_orientation_helper_->GetEnd(item_holder));
}

#if ENABLE_TRACE_PERFETTO
void ListLayoutManager::UpdateTraceDebugInfo(TraceEvent* event) const {
  auto* content_size_info = event->add_debug_annotations();
  content_size_info->set_name("content_size");
  content_size_info->set_string_value(std::to_string(content_size_));

  auto* content_offset_info = event->add_debug_annotations();
  content_offset_info->set_name("content_offset");
  content_offset_info->set_string_value(std::to_string(content_offset_));
}
#endif

}  // namespace list
}  // namespace lynx
