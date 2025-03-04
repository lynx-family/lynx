// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/linear_layout_manager.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include "base/include/float_comparison.h"
#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

LinearLayoutManager::LinearLayoutManager(ListContainerImpl* list_container_impl)
    : ListLayoutManager(list_container_impl) {}

void LinearLayoutManager::OnBatchLayoutChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::OnBatchLayoutChildren");

  OnPrepareForLayoutChildren();

  // Note: To avoid nested invoking OnBatchLayoutChildren,
  // StartInterceptListElementUpdated() and StopInterceptListElementUpdated()
  // need to be invoked at the begin or end of OnBatchLayoutChildren().
  list_container_->StartInterceptListElementUpdated();

  LayoutState layout_state;
  layout_state.latest_updated_content_offset_ = content_offset_;

  // step 1. Update anchor info and layout all item_holders
  ListAnchorManager::AnchorInfo anchor_info;
  InitLayoutAndAnchor(anchor_info, list::kInvalidIndex);
  SendAnchorDebugInfo(anchor_info);

  // step 2. Invoke batch render.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "BatchRender");
  LayoutInvalidItemHolder(0);
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);
  list_container_->list_adapter()->BindItemHolders(
      list_children_helper_->on_screen_children());
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 3. Invoke OnLayoutChildren after batch render.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnLayoutChildrenInternal");
  OnLayoutChildrenInternal(anchor_info, layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 4. Handle layout result: recycle and update layout to platform.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnLayoutAfter");
  OnLayoutAfter(layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void LinearLayoutManager::OnLayoutChildren(
    bool is_component_finished /* = false */, int component_index /* = -1 */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LinearLayoutManager::OnLayoutChildren");
  if (!list_container_ || !list_children_helper_) {
    return;
  }

  OnPrepareForLayoutChildren();

  // Note: To avoid nested invoking OnLayoutChildren,
  // StartInterceptListElementUpdated() and StopInterceptListElementUpdated()
  // need to be invoked at the begin or end of OnLayoutChildren().
  list_container_->StartInterceptListElementUpdated();

  LayoutState layout_state;
  layout_state.latest_updated_content_offset_ = content_offset_;

  // step 1. Update anchor info and layout all item_holders
  ListAnchorManager::AnchorInfo anchor_info;
  InitLayoutAndAnchor(anchor_info, component_index);
  SendAnchorDebugInfo(anchor_info);

  // step 2. Fill after find anchor.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnLayoutChildrenInternal");
  OnLayoutChildrenInternal(anchor_info, layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 3. Handle layout result: recycle and update layout to platform.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnLayoutAfter");
  OnLayoutAfter(layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void LinearLayoutManager::OnLayoutChildrenInternal(
    ListAnchorManager::AnchorInfo& anchor_info, LayoutState& layout_state) {
  // Handle empty data source.
  if (list_container_->GetDataCount() == 0) {
    content_size_ = GetTargetContentSize();
    // Reset content offset to 0.
    SetContentOffset(0.f);
    FlushContentSizeAndOffsetToPlatform(
        layout_state.latest_updated_content_offset_);
    layout_state.latest_updated_content_offset_ = content_offset_;
    // Note: need update on screen children.
    list_children_helper_->UpdateOnScreenChildren(
        list_orientation_helper_.get(), content_offset_);
    return;
  }

  // step 1. Fill from anchor.
  if (anchor_info.valid_) {
    FillWithAnchor(layout_state, anchor_info);
  }

  // step 2. Update content size and offset
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateContentSizeAndOffset");
  LayoutInvalidItemHolder(0);
  content_size_ = GetTargetContentSize();
  list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                      content_offset_);

  // step 2.5 Update sticky items
  UpdateStickyItemsAfterLayout(anchor_info);
  FlushContentSizeAndOffsetToPlatform(
      layout_state.latest_updated_content_offset_);
  layout_state.latest_updated_content_offset_ = content_offset_;

  // The previous AdjustOffsetWithAnchor was called twice(the second one is
  // caused by sticky), so the scrolled value should be set only when both of
  // these calls have finished
  list_anchor_manager_->MarkScrolledInitialScrollIndex();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 3. Handle Preload.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "HandlePreloadIfNeeded");
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);
  if (enable_preload_section_) {
    PreloadSectionOnNextFrame();
  } else {
    HandlePreloadIfNeeded(layout_state, anchor_info);
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void LinearLayoutManager::HandlePreloadIfNeeded(
    LayoutState& layout_state, ListAnchorManager::AnchorInfo& anchor_info) {
  if (ValidPreloadBufferCount() && Preload(layout_state)) {
    LayoutInvalidItemHolder(0);
    content_size_ = GetTargetContentSize();
    list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                        content_offset_);
    FlushContentSizeAndOffsetToPlatform(
        layout_state.latest_updated_content_offset_);
    layout_state.latest_updated_content_offset_ = content_offset_;
    // Note: need re-update on screen children after preload
    list_children_helper_->UpdateOnScreenChildren(
        list_orientation_helper_.get(), content_offset_);
  }
}

void LinearLayoutManager::OnLayoutAfter(LayoutState& layout_state) {
  HandleLayoutOrScrollResult(layout_state, true);
  // Send layout events.
  // Note: Events has to be called after StopInterceptListElementUpdated to
  // avoid reenter in worklet
  list_container_->StopInterceptListElementUpdated();
  float scroll_delta = content_offset_ - last_content_offset_;
  last_content_offset_ = content_offset_;
  list_container_->RecordVisibleItemIfNeeded(false);
  list::EventSource event_source = list_container_->has_valid_diff()
                                       ? list::EventSource::kDiff
                                       : list::EventSource::kLayout;
  SendLayoutCompleteEvent(scroll_delta);
  SendScrollEvents(scroll_delta, content_offset_, event_source);
  list_container_->ClearValidDiff();
}

void LinearLayoutManager::HandleLayoutOrScrollResult(LayoutState& layout_state,
                                                     bool is_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "HandlePlatformOperation");
  if (list_container_->enable_batch_render()) {
    ListLayoutManager::HandleLayoutOrScrollResult(is_layout);
  } else {
    // No Batch Render.
    ListAdapter* list_adapter = list_container_->list_adapter();
    // 1. Recycle off-screen or off-preload's item holder.
    if (!ValidPreloadBufferCount()) {
      // No preload case.
      RecycleOffScreenItemHolders();
    } else if (layout_state.ValidPreload()) {
      // valid preload case.
      if (layout_state.preload_min_index_ != list::kInvalidIndex) {
        RecycleOffPreloadItemHolders(false, layout_state.preload_min_index_);
      }
      if (layout_state.preload_max_index_ != list::kInvalidIndex) {
        RecycleOffPreloadItemHolders(true, layout_state.preload_max_index_);
      }
    }
    if (is_layout) {
      // 2. Recycle all removed child.
      list_adapter->RecycleRemovedItemHolders();
    }
    // 3. Update layout info to platform.
    list_children_helper_->ForEachChild(
        [this, list_adapter](ItemHolder* item_holder) {
          item_holder->UpdateLayoutToPlatform(
              content_size_, GetWidth(),
              list_adapter->GetListItemElement(item_holder));
          return false;
        });
    list_container_->FlushPatching();
  }
}

void LinearLayoutManager::PreloadSectionOnNextFrame() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::PreloadSectionOnNextFrame");
  if (list_container_ && list_container_->need_preload_section_on_next_frame_) {
    list_container_->element()->RequestNextFrame();
  }
}

void LinearLayoutManager::PreloadSection() {
  if (list_container_ && list_container_->need_preload_section_on_next_frame_) {
    list_container_->need_preload_section_on_next_frame_ = false;
    LayoutState layout_state;
    PreloadSection(layout_state);
  }
}

/**
 * @description: The main linear layout fill steps are as follows:
 *
 * step 1. Fill to end: From anchor's index and coordinate, calculate
 * available space to end, then try to render children to fill all available
 * space.
 *
 * step 2. Fill to start: From anchor's index and coordinate,
 * calculate available space to start. Considering if has remaining space that
 * not be filled in step2, we also add it to available space, then try to render
 * children to fill.
 *
 * step 3. Fill extra: Considering if has remaining space in
 * step3 but not in step2, we also needs render children to end again to fill
 * all remaining space.
 */
void LinearLayoutManager::FillWithAnchor(
    LayoutState& layout_state,
    const ListAnchorManager::AnchorInfo& anchor_info) {
  // step1. Fill to end from anchor_info's index
  float extra_for_start = list_orientation_helper_->GetStartAfterPadding();
  float extra_for_end = list_orientation_helper_->GetEndPadding();
  UpdateLayoutStateToFillEnd(layout_state, anchor_info);
  layout_state.extra_ = extra_for_end;
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    "LinearLayoutManager::FillWithAnchor.FillToEnd",
                    "anchor_index", std::to_string(anchor_info.index_));
  Fill(layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step2. Fill to start from anchor_info's index - 1
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    "LinearLayoutManager::FillWithAnchor.FillToStart");
  if (layout_state.available_ > 0.f) {
    extra_for_start += layout_state.available_;
  }
  layout_state.extra_ = extra_for_start;
  UpdateLayoutStateToFillStart(layout_state, anchor_info);
  Fill(layout_state);
  // Record the min laid out index after this fill.
  layout_state.min_layout_chunk_index_ = layout_state.next_bind_index_;
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step3. Fill extra from anchor index instead of the index which is recorded
  // after filling to end, which can avoid the situation that the available
  // space calculated incorrectly.
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::FillWithAnchor.FillExtra");
  if (layout_state.available_ > 0.f) {
    layout_state.extra_ = layout_state.available_;
    UpdateLayoutStateToFillEnd(layout_state, anchor_info);
    Fill(layout_state);
  }
}

/**
 * @description: This function can be used in OnLayoutChildren() or
 * ScrollByInternal(), and the main steps of preloading are as follows:
 *
 * step 1. Find first or last visible item holder according to on screen
 * children.
 *
 * step 2. Update layout state, calculate target index and preload to start
 * or end.
 */
bool LinearLayoutManager::Preload(LayoutState& layout_state) {
  const auto& on_screen_children = list_children_helper_->on_screen_children();
  layout_state.ResetPreloadIndex();
  list_children_helper_->ClearInPreloadChildren();
  if (on_screen_children.empty()) {
    NLIST_LOGE("LinearLayoutManager::Preload: empty on screen children");
  } else {
    const auto* first_visible_item_holder = *(on_screen_children.cbegin());
    const auto* last_visible_item_holder = *(on_screen_children.crbegin());
    if (!first_visible_item_holder || !last_visible_item_holder) {
      NLIST_LOGE(
          "LinearLayoutManager::Preload: visible item holder is nullptr");
    } else {
      const ItemHolderSet& in_preload_children =
          list_children_helper_->in_preload_children();
      int first_visible_index = first_visible_item_holder->index();
      int last_visible_index = last_visible_item_holder->index();
      int start_index =
          first_visible_index +
          static_cast<int32_t>(list::LayoutDirection::kLayoutToStart);
      int end_index = last_visible_index +
                      static_cast<int32_t>(list::LayoutDirection::kLayoutToEnd);
      int target_start_index = GetTargetIndexForPreloadBuffer(
          start_index, list::LayoutDirection::kLayoutToStart);
      int target_end_index = GetTargetIndexForPreloadBuffer(
          end_index, list::LayoutDirection::kLayoutToEnd);
      // Fill to end for preload
      TRACE_EVENT_BEGIN(
          LYNX_TRACE_CATEGORY, "LinearLayoutManager::PreloadToEnd", "info",
          base::FormatString("[%d -> %d]", end_index, target_end_index));
      NLIST_LOGD(
          "LinearLayoutManager::Preload: preload to end, "
          "last_visible_index = "
          << last_visible_index << ", preload range = [" << end_index << " -> "
          << target_end_index << "]");
      if (end_index != list::kInvalidIndex &&
          target_end_index != list::kInvalidIndex &&
          end_index <= target_end_index) {
        UpdateLayoutStateToFillPreloadBuffer(
            layout_state, end_index,
            list_orientation_helper_->GetDecoratedEnd(last_visible_item_holder),
            list::LayoutDirection::kLayoutToEnd);
        // Fill preload buffer item holders
        PreloadInternal(layout_state, target_end_index);
        layout_state.preload_max_index_ = target_end_index;
        for (int i = end_index; i <= target_end_index; ++i) {
          list_children_helper_->AddChild(
              in_preload_children, list_container_->GetItemHolderForIndex(i));
        }
      }
      TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
      // Fill to start for preload
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, "LinearLayoutManager::PreloadToStart", "info",
          base::FormatString("[%d -> %d]", start_index, target_start_index));
      NLIST_LOGD(
          "LinearLayoutManager::Preload: preload to start, "
          "first_visible_index = "
          << first_visible_index << ", preload range = [" << start_index
          << " -> " << target_start_index << "]");
      if (start_index != list::kInvalidIndex &&
          target_start_index != list::kInvalidIndex &&
          target_start_index <= start_index) {
        UpdateLayoutStateToFillPreloadBuffer(
            layout_state, start_index,
            list_orientation_helper_->GetDecoratedStart(
                first_visible_item_holder),
            list::LayoutDirection::kLayoutToStart);
        // Fill preload buffer item holders
        PreloadInternal(layout_state, target_start_index);
        // Record the min laid out index after this fill.
        layout_state.min_layout_chunk_index_ = layout_state.next_bind_index_;
        layout_state.preload_min_index_ = target_start_index;
        for (int i = target_start_index; i <= start_index; ++i) {
          list_children_helper_->AddChild(
              in_preload_children, list_container_->GetItemHolderForIndex(i));
        }
      }
    }
  }
  return layout_state.preload_max_index_ != list::kInvalidIndex ||
         layout_state.preload_min_index_ != list::kInvalidIndex;
}

/**
 * @description: Preload item holders to target index.
 */
void LinearLayoutManager::PreloadInternal(LayoutState& layout_state,
                                          int target_index,
                                          bool preload_section /* = false */) {
  static LayoutChunkResult result;
  while (HasMore(layout_state, target_index)) {
    result.Reset();
    LayoutChunk(result, layout_state, preload_section);
    if (result.finished_) {
      break;
    }
    layout_state.next_layout_offset_ +=
        result.consumed_ * static_cast<int32_t>(layout_state.layout_direction_);
  }
}

/**
 * @description: Recycle all item holders out of preload buffer
 */
void LinearLayoutManager::RecycleOffPreloadItemHolders(bool recycle_to_end,
                                                       int target_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::RecycleOffPreloadItemHolders");
  if (target_index != list::kInvalidIndex) {
    list_children_helper_->ForEachChild(
        [this, target_index, recycle_to_end](ItemHolder* item_holder) {
          if (item_holder) {
            int index = item_holder->index();
            if (recycle_to_end && index > target_index &&
                IsItemHolderNotSticky(item_holder)) {
              list_container_->list_adapter()->RecycleItemHolder(item_holder);
            } else if (!recycle_to_end && index < target_index &&
                       IsItemHolderNotSticky(item_holder)) {
              list_container_->list_adapter()->RecycleItemHolder(item_holder);
            }
          }
          return false;
        });
  }
}

/**
 * @description: Calculate target index with start index and preload
 * buffer count
 */
int LinearLayoutManager::GetTargetIndexForPreloadBuffer(
    int start_index, list::LayoutDirection layout_direction) {
  const int data_count = list_container_->GetDataCount();
  int target_index = list::kInvalidIndex;
  if (start_index >= 0 && start_index < data_count) {
    target_index = layout_direction == list::LayoutDirection::kLayoutToEnd
                       ? start_index + preload_buffer_count_ - 1
                       : start_index - preload_buffer_count_ + 1;
    target_index = std::max(0, std::min(target_index, data_count));
  }
  return target_index;
}

/**
 * @description: The main handling scroll steps are as follows:
 * step 1. Update anchor info in scroll using the latest content offset.
 *
 * step 2. Fill from anchor item holder.
 *
 * step 3. Update content offset and size: After filling, we
 * need calculate new content offset and content size.
 *
 * step 4. Handle sticky /
 * Recycle / Flush: Handle sticky, recycle ItemHolder out of list's visible
 * range, and flush children's layout infos to platform in OnLayoutCompleted.
 */
void LinearLayoutManager::ScrollByInternal(float content_offset,
                                           float original_offset,
                                           bool from_platform) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LinearLayoutManager::ScrollByInternal");
  if (!list_container_ || !list_children_helper_) {
    return;
  }
  list_container_->StartInterceptListElementUpdated();

  // step1. Update anchor info in scroll.
  LayoutState layout_state;
  layout_state.latest_updated_content_offset_ =
      from_platform ? content_offset : content_offset_;
  content_offset_ = content_offset;

  // Note: use the latest content offset to update on screen children first.
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);
  const auto& on_screen_children = list_children_helper_->on_screen_children();
  if (on_screen_children.empty()) {
    NLIST_LOGE(
        "LinearLayoutManager::ScrollByInternal: empty on screen children");
    list_container_->StopInterceptListElementUpdated();
    return;
  }
  ListAnchorManager::AnchorInfo anchor_info;
  UpdateScrollAnchorInfo(anchor_info, on_screen_children, content_offset_);
  if (!anchor_info.valid_) {
    NLIST_LOGE(
        "LinearLayoutManager::ScrollByInternal: null anchor item holder");
    list_container_->StopInterceptListElementUpdated();
    return;
  }

  // step 2. Fill
  FillWithAnchor(layout_state, anchor_info);

  // step 3. Update content size and offset
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "FlushContentSizeAndOffsetToPlatform");
  LayoutInvalidItemHolder(
      layout_state.min_layout_chunk_index_ -
      static_cast<int32_t>(list::LayoutDirection::kLayoutToStart));
  content_size_ = GetTargetContentSize();
  list_anchor_manager_->AdjustContentOffsetWithAnchor(
      anchor_info, layout_state.latest_updated_content_offset_);
  FlushContentSizeAndOffsetToPlatform(
      layout_state.latest_updated_content_offset_);
  layout_state.latest_updated_content_offset_ = content_offset_;
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 3.5. Handle sticky.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateStickyItems");
  UpdateStickyItems();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 4. Handle preload
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "HandlePreloadIfNeeded");
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);
  HandlePreloadIfNeeded(layout_state, anchor_info);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // step 5. Handle scroll result.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnScrollAfter");
  OnScrollAfter(layout_state, original_offset);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void LinearLayoutManager::OnScrollAfter(LayoutState& layout_state,
                                        float original_offset) {
  HandleLayoutOrScrollResult(layout_state, false);
  // Send scroll event
  // Events has to be called after StopInterceptListElementUpdated to avoid
  // reenter in worklet
  list_container_->StopInterceptListElementUpdated();
  float scroll_delta = content_offset_ - last_content_offset_;
  last_content_offset_ = content_offset_;
  SendScrollEvents(scroll_delta, original_offset, list::EventSource::kScroll);
}

void LinearLayoutManager::UpdateScrollAnchorInfo(
    ListAnchorManager::AnchorInfo& anchor_info,
    const ItemHolderSet& on_screen_children, const float content_offset) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::UpdateScrollAnchorInfo");
  ItemHolder* first_visible_item_holder = list_children_helper_->GetFirstChild(
      on_screen_children,
      [this, list_adapter = list_container_->list_adapter()](
          const ItemHolder* item_holder) {
        return !list_adapter->IsDirty(item_holder) &&
               list_adapter->GetListItemElement(item_holder) &&
               IsItemHolderNotSticky(item_holder);
      });
  ItemHolder* last_visible_item_holder = list_children_helper_->GetLastChild(
      on_screen_children,
      [this, list_adapter = list_container_->list_adapter()](
          const ItemHolder* item_holder) {
        return !list_adapter->IsDirty(item_holder) &&
               list_adapter->GetListItemElement(item_holder) &&
               IsItemHolderNotSticky(item_holder);
      });
  ItemHolder* anchor_item_holder = nullptr;
  if (!first_visible_item_holder || !last_visible_item_holder) {
    anchor_item_holder = list_children_helper_->GetFirstChild(
        on_screen_children, [this](const ItemHolder* item_holder) {
          return base::FloatsLargerOrEqual(
                     list_orientation_helper_->GetStart(item_holder),
                     content_offset_) &&
                 IsItemHolderNotSticky(item_holder);
        });
    if (!anchor_item_holder) {
      anchor_item_holder = *(on_screen_children.begin());
    }
  } else {
    anchor_item_holder = first_visible_item_holder;
  }
  if (anchor_item_holder) {
    anchor_info.item_holder_ = anchor_item_holder;
    anchor_info.index_ = anchor_item_holder->index();
    anchor_info.start_offset_ =
        list_orientation_helper_->GetStart(anchor_item_holder);
    anchor_info.start_alignment_delta_ =
        anchor_info.start_offset_ - content_offset;
    anchor_info.valid_ = true;
  } else {
    anchor_info.valid_ = false;
  }
}

void LinearLayoutManager::LayoutInvalidItemHolder(int first_invalid_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "LinearLayoutManager::LayoutInvalidItemHolder",
              "first_invalid_index", std::to_string(first_invalid_index));
  if (!list_container_ || !list_children_helper_ || first_invalid_index < 0 ||
      first_invalid_index >= list_container_->GetDataCount()) {
    return;
  }
  float offset = 0.f;
  list_children_helper_->ForEachChild(
      [this, &offset, first_invalid_index](ItemHolder* item_holder) {
        if (item_holder->index() >= first_invalid_index) {
          item_holder->SetOrientation(orientation());
          if (item_holder->index() > 0) {
            item_holder->SetTopInset(main_axis_gap_);
          } else {
            offset += list_orientation_helper_->GetStartAfterPadding();
          }
          offset += item_holder->top_inset();
          float main_axis = 0.f, cross_axis = 0.f;
          main_axis =
              offset +
              list_orientation_helper_->GetItemHolderMainMargin(item_holder);
          cross_axis = list_orientation_helper_->GetStartAfterPaddingInOther();
          if (orientation_ == list::Orientation::kVertical) {
            item_holder->UpdateLayoutFromManager(cross_axis, main_axis);
          } else {
            item_holder->UpdateLayoutFromManager(main_axis, cross_axis);
          }
        }
        offset = list_orientation_helper_->GetDecoratedEnd(item_holder);
        return false;
      });
}

float LinearLayoutManager::GetTargetContentSize() {
  // Note: content size == padding top + sum of children's height + padding
  // bottom
  if (!list_container_ || !list_orientation_helper_) {
    return 0.f;
  }
  if (list_container_->list_adapter()->GetDataCount() == 0) {
    return list_orientation_helper_->GetStartAfterPadding() +
           list_orientation_helper_->GetEndPadding();
  } else {
    int last_element_index =
        list_container_->list_adapter()->GetDataCount() - 1;
    // Last ItemHolder's end + list's padding-bottom or padding-right
    return list_orientation_helper_->GetDecoratedEnd(
               list_container_->GetItemHolderForIndex(last_element_index)) +
           list_orientation_helper_->GetEndPadding();
  }
}

// Render and layout one ItemHolder, GridLayoutManager overrides this function
// to render column-count ItemHolders or a full-span ItemHolder.
void LinearLayoutManager::LayoutChunk(LayoutChunkResult& result,
                                      LayoutState& layout_state,
                                      bool preload_section) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LinearLayoutManager::LayoutChunk", "index",
              base::FormatString("%d", layout_state.next_bind_index_));
  if (!list_container_ || !list_children_helper_ || !list_orientation_helper_) {
    result.consumed_ = 0.f;
    return;
  }
  int index = layout_state.next_bind_index_;
  ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);
  if (item_holder) {
    list_container_->list_adapter()->BindItemHolder(item_holder, index,
                                                    preload_section);
    result.consumed_ =
        list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    float left = 0.f, top = 0.f;
    if (orientation_ == list::Orientation::kVertical) {
      // vertical
      left = list_orientation_helper_->GetStartAfterPaddingInOther() +
             item_holder->GetMargin(list::FrameDirection::kLeft);
      if (layout_state.layout_direction_ ==
          list::LayoutDirection::kLayoutToEnd) {
        // fill to end
        top = layout_state.next_layout_offset_ +
              item_holder->GetMargin(list::FrameDirection::kTop);
      } else {
        // fill to start
        top = layout_state.next_layout_offset_ - result.consumed_ +
              item_holder->top_inset() +
              item_holder->GetMargin(list::FrameDirection::kTop);
      }
    } else {
      // horizontal
      top = list_orientation_helper_->GetStartAfterPaddingInOther() +
            item_holder->GetMargin(list::FrameDirection::kTop);
      if (layout_state.layout_direction_ ==
          list::LayoutDirection::kLayoutToEnd) {
        // fill to end
        left = layout_state.next_layout_offset_ +
               item_holder->GetMargin(list::FrameDirection::kLeft);
      } else {
        // fill to start
        left = layout_state.next_layout_offset_ - result.consumed_ +
               item_holder->top_inset() +
               item_holder->GetMargin(list::FrameDirection::kLeft);
      }
    }
    item_holder->UpdateLayoutFromManager(left, top);
  }
  layout_state.next_bind_index_ +=
      static_cast<int32_t>(layout_state.layout_direction_);
}

// Update layout state to fill to start, GridLayoutManager overrides this
// function to handle LayoutState by it self.
void LinearLayoutManager::UpdateLayoutStateToFillStart(
    LayoutState& layout_state,
    const ListAnchorManager::AnchorInfo& anchor_info) {
  // anchor_info.coordinate_ is the top value of anchor item holder, which is
  // including main axis gap. For example, item_holder's height == 100 and
  // main_axis_gap == 10, the top value of item_holder_1 is 110.
  float top_inset =
      anchor_info.item_holder_ ? anchor_info.item_holder_->top_inset() : 0.f;
  float offset = anchor_info.start_offset_ - top_inset;
  int index = anchor_info.index_ +
              static_cast<int32_t>(list::LayoutDirection::kLayoutToStart);
  // Update layout state.
  layout_state.available_ = offset - content_offset_ -
                            list_orientation_helper_->GetStartAfterPadding();
  layout_state.next_bind_index_ = index;
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = list::LayoutDirection::kLayoutToStart;
}

// Update layout state to fill to end, GridLayoutManager overrides this function
// to handle LayoutState by it self.
void LinearLayoutManager::UpdateLayoutStateToFillEnd(
    LayoutState& layout_state,
    const ListAnchorManager::AnchorInfo& anchor_info) {
  float offset = anchor_info.start_offset_;
  int index = anchor_info.index_;
  // Update layout state.
  layout_state.available_ =
      list_orientation_helper_->GetEndAfterPadding() + content_offset_ - offset;
  layout_state.next_bind_index_ = index;
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = list::LayoutDirection::kLayoutToEnd;
}

// Update layout state to fill preload buffer
void LinearLayoutManager::UpdateLayoutStateToFillPreloadBuffer(
    LayoutState& layout_state, int index, float offset,
    list::LayoutDirection layout_direction) {
  layout_state.next_bind_index_ = index;
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = layout_direction;
}

// Try to render as many ItemHolders as possible to fill the specified available
// area. The filling's direction, start index, start coordinate and the size
// of the available area are specified by the LayoutState.
void LinearLayoutManager::Fill(LayoutState& layout_state) {
  float remaining = layout_state.available_ + layout_state.extra_;
  static LayoutChunkResult result;
  while (HasMore(layout_state.next_bind_index_, remaining)) {
    result.Reset();
    LayoutChunk(result, layout_state);
    if (result.finished_) {
      break;
    }
    layout_state.next_layout_offset_ +=
        result.consumed_ * static_cast<int32_t>(layout_state.layout_direction_);
    layout_state.available_ -= result.consumed_;
    remaining -= result.consumed_;
  }
}

// Return whether list has more available space and data source to fill.
bool LinearLayoutManager::HasMore(int next_bind_index, float remaining) const {
  return base::FloatsLarger(remaining, 0.f) && list_container_ &&
         next_bind_index >= 0 &&
         next_bind_index < list_container_->GetDataCount();
}

// Return whether list has more data source to fill.
bool LinearLayoutManager::HasMore(const LayoutState& layout_state,
                                  int target_index) const {
  int next_bind_index = layout_state.next_bind_index_;
  if (layout_state.layout_direction_ == list::LayoutDirection::kLayoutToEnd) {
    return list_container_ && next_bind_index >= 0 &&
           next_bind_index < list_container_->GetDataCount() &&
           next_bind_index <= target_index;
  } else {
    return list_container_ && next_bind_index >= 0 &&
           next_bind_index < list_container_->GetDataCount() &&
           next_bind_index >= target_index;
  }
}

// Return first ItemHolder intersected with specified line.
ItemHolder* LinearLayoutManager::FindFirstIntersectItemHolder(
    float line) const {
  ItemHolder* intersect_item_holder = nullptr;
  // Find attached child first
  list_children_helper_->ForEachChild(
      list_children_helper_->attached_children(),
      [this, line, &intersect_item_holder](ItemHolder* item_holder) {
        if (IsItemHolderIntersectsWithLine(line, item_holder)) {
          intersect_item_holder = item_holder;
          return true;
        }
        return false;
      });
  if (intersect_item_holder) {
    return intersect_item_holder;
  }
  // Fallback logic. find all item_holder as backup logic
  list_children_helper_->ForEachChild(
      [this, line, &intersect_item_holder](ItemHolder* item_holder) {
        if (IsItemHolderIntersectsWithLine(line, item_holder)) {
          intersect_item_holder = item_holder;
          return true;
        }
        return false;
      });
  return intersect_item_holder;
}

bool LinearLayoutManager::IsItemHolderIntersectsWithLine(
    float line, ItemHolder* item_holder) const {
  if (!item_holder || !list_orientation_helper_) {
    return false;
  }
  return base::FloatsLargerOrEqual(
             line, list_orientation_helper_->GetDecoratedStart(item_holder)) &&
         base::FloatsLargerOrEqual(
             list_orientation_helper_->GetDecoratedEnd(item_holder), line);
}

void LinearLayoutManager::PreloadSection(LayoutState& layout_state) {
  if (!enable_preload_section_) {
    return;
  }
  const auto& on_screen_children = list_children_helper_->on_screen_children();
  if (on_screen_children.empty()) {
    NLIST_LOGE("LinearLayoutManager::PreloadSection: empty on screen children");
  } else {
    const auto* first_visible_item_holder = *(on_screen_children.cbegin());
    const auto* last_visible_item_holder = *(on_screen_children.crbegin());
    if (!first_visible_item_holder || !last_visible_item_holder) {
      NLIST_LOGE(
          "LinearLayoutManager::PreloadSection: visible item holder is "
          "nullptr");
    } else {
      int first_visible_index = first_visible_item_holder->index();
      int last_visible_index = last_visible_item_holder->index();
      int start_index =
          first_visible_index +
          static_cast<int32_t>(list::LayoutDirection::kLayoutToStart);
      int end_index = last_visible_index +
                      static_cast<int32_t>(list::LayoutDirection::kLayoutToEnd);
      const int data_count = list_container_->GetDataCount();
      int target_end_index = end_index;
      int section_count = last_visible_index - first_visible_index + 1;
      if (section_count <= 0) {
        NLIST_LOGE("LinearLayoutManager::PreloadSection: invalid section count "
                   << section_count);
        return;
      }
      while (end_index >= 0 && end_index < data_count &&
             last_visible_item_holder) {
        target_end_index = std::min(end_index + section_count, data_count - 1);
        TRACE_EVENT(
            LYNX_TRACE_CATEGORY, "LinearLayoutManager::PreloadSectionToEnd",
            "info",
            base::FormatString("[%d -> %d]", end_index, target_end_index));
        UpdateLayoutStateToFillPreloadBuffer(
            layout_state, end_index,
            list_orientation_helper_->GetDecoratedEnd(last_visible_item_holder),
            list::LayoutDirection::kLayoutToEnd);
        // Fill preload section item holders
        PreloadInternal(layout_state, target_end_index, true);
        // Recycle
        RecycleOffScreenItemHolders();
        last_visible_item_holder =
            list_container_->GetItemHolderForIndex(target_end_index);
        end_index = target_end_index + 1;
      }
      int target_start_index = start_index;
      while (start_index >= 0 && start_index < data_count &&
             first_visible_item_holder) {
        target_start_index = std::max(start_index - section_count, 0);
        TRACE_EVENT(
            LYNX_TRACE_CATEGORY, "LinearLayoutManager::PreloadSectionToStart",
            "info",
            base::FormatString("[%d -> %d]", start_index, target_start_index));
        UpdateLayoutStateToFillPreloadBuffer(
            layout_state, start_index,
            list_orientation_helper_->GetDecoratedStart(
                first_visible_item_holder),
            list::LayoutDirection::kLayoutToStart);
        // Fill preload section item holders
        PreloadInternal(layout_state, target_start_index, true);
        // Recycle
        RecycleOffScreenItemHolders();
        first_visible_item_holder =
            list_container_->GetItemHolderForIndex(target_start_index);
        start_index = target_start_index - 1;
      }
    }
  }
}

}  // namespace tasm
}  // namespace lynx
