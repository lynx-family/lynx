// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/staggered_grid_layout_manager.h"

#include <algorithm>

#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

StaggeredGridLayoutManager::StaggeredGridLayoutManager(
    ListContainerImpl* list_container_impl)
    : ListLayoutManager(list_container_impl) {}

void StaggeredGridLayoutManager::InitLayoutState() {
  column_indexes_.resize(span_count_);
  std::fill(column_indexes_.begin(), column_indexes_.begin() + span_count_,
            std::vector<int>());
}

void StaggeredGridLayoutManager::UpdateStartAndEndLinesStatus(
    LayoutState& layout_state) {
  // Use attached_children to calculate start/end lines
  // 1. Calculate start_item_holders and end_item_holders from attached_children
  std::vector<ItemHolder*> start_item_holders(
      span_count_, list_container_->GetItemHolderForIndex(
                       list_container_->GetDataCount() - 1));
  std::vector<ItemHolder*> end_item_holders(
      span_count_, list_container_->GetItemHolderForIndex(0));
  if (list_children_helper_->attached_children().size() > 0) {
    list_children_helper_->ForEachChild(
        list_children_helper_->attached_children(),
        [this, &start_item_holders, &end_item_holders,
         list_adapter =
             list_container_->list_adapter()](ItemHolder* item_holder) {
          // span index of current item holder.
          int span_index = item_holder->item_col_index();
          int item_index = item_holder->index();
          int current_start_item_index =
              start_item_holders[span_index]->index();
          int current_end_item_index = end_item_holders[span_index]->index();
          // 1. Update start_item_holders vector.
          if (item_holder->item_full_span()) {
            // 1.1 If item holder is full span.
            for (int i = 0; i < span_count_; ++i) {
              // If index of item holder in start_item_holders larger than
              // item_index, we should set item_holder to the position i in
              // start_item_holders.
              if (start_item_holders[i]->index() > item_index) {
                start_item_holders[i] = item_holder;
              }
            }
          } else if (current_start_item_index > item_index) {
            // 1.2 If item holder is not full span and has less index, we just
            // need to set it to start_item_holders.
            start_item_holders[span_index] = item_holder;
          }
          // 2. Update end_item_holders vector.
          if (item_holder->item_full_span()) {
            // 2.1 If item holder is full span.
            for (int i = 0; i < span_count_; ++i) {
              // If index of item holder in end_item_holders smaller than
              // item_index, we should set item_holder to the position i in
              // end_item_holders.
              if (end_item_holders[i]->index() < item_index) {
                end_item_holders[i] = item_holder;
              }
            }
          } else if (current_end_item_index < item_index) {
            // 2.2 If item holder is not full span and has larger index, we
            // just need to set it to end_item_holders.
            end_item_holders[span_index] = item_holder;
          }
          return false;
        });
    layout_state.is_start_full_span_ = true;
    layout_state.is_end_full_span_ = true;
    ItemHolder* item_holder = nullptr;
    // 2. Use start_item_holders update start lines and start indexes.
    for (int i = 0; i < span_count_; ++i) {
      item_holder = start_item_holders[i];
      layout_state.start_index[i] = item_holder->index();
      layout_state.start_lines[i] =
          list_orientation_helper_->GetDecoratedStart(item_holder);
      // Note: if all items in start_item_holders are full span, we mark
      // is_start_full_span_ to true.
      layout_state.is_start_full_span_ =
          item_holder->item_full_span() && layout_state.is_start_full_span_;
    }
    // 3. Use end_item_holders update end lines and end indexes.
    for (int i = 0; i < span_count_; ++i) {
      item_holder = end_item_holders[i];
      layout_state.end_index[i] = item_holder->index();
      layout_state.end_lines[i] =
          list_orientation_helper_->GetDecoratedEnd(item_holder);
      // Note: if all items in end_item_holders are full span, we mark
      // is_end_full_span_ to true.
      layout_state.is_end_full_span_ =
          item_holder->item_full_span() && layout_state.is_end_full_span_;
    }
  }
}

void StaggeredGridLayoutManager::OnBatchLayoutChildren() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "StaggeredGridLayoutManager::OnBatchLayoutChildren");
  OnPrepareForLayoutChildren();

  // Note: To avoid nested invoking OnBatchLayoutChildren,
  // StartInterceptListElementUpdated() and StopInterceptListElementUpdated()
  // need to be invoked at the begin or end of OnBatchLayoutChildren().
  list_container_->StartInterceptListElementUpdated();

  LayoutState layout_state(span_count_);
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
  OnLayoutAfter();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void StaggeredGridLayoutManager::OnLayoutChildren(
    bool is_component_finished /* = false */, int component_index /* = -1 */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "StaggeredGridLayoutManager::OnLayoutChildren");
  if (!list_container_ || !list_children_helper_) {
    return;
  }

  OnPrepareForLayoutChildren();

  list_container_->StartInterceptListElementUpdated();
  LayoutState layout_state(span_count_);
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
  OnLayoutAfter();
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void StaggeredGridLayoutManager::OnLayoutChildrenInternal(
    ListAnchorManager::AnchorInfo& anchor_info, LayoutState& layout_state) {
  // Handle empty data source.
  if (list_container_->GetDataCount() == 0) {
    content_size_ = GetTargetContentSize();
    SetContentOffset(0.f);
    FlushContentSizeAndOffsetToPlatform(
        layout_state.latest_updated_content_offset_);
    layout_state.latest_updated_content_offset_ = content_offset_;
    // Note: need update on screen children.
    list_children_helper_->UpdateOnScreenChildren(
        list_orientation_helper_.get(), content_offset_);
    return;
  }

  // step 1. Bind all visible ItemHolders.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "BindAllVisibleItemHolders",
                    "anchor_index", anchor_info.index_);
  bool should_fill = BindAllVisibleItemHolders();
  while (should_fill) {
    LayoutInvalidItemHolder(0);
    content_size_ = GetTargetContentSize();
    list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                        content_offset_);
    should_fill = BindAllVisibleItemHolders();
  }
  LayoutInvalidItemHolder(0);
  content_size_ = GetTargetContentSize();
  list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                      content_offset_);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  if (!list_container_->enable_batch_render()) {
    // TODO(dingwang.wxx): If use initial-scroll-index, this may lead to fill
    // from the item 0 to initial-scroll-index. So we can remove this logic.
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "Fill");
    layout_state.Reset(span_count_);
    layout_state.layout_direction_ = list::LayoutDirection::kLayoutToStart;
    UpdateStartAndEndLinesStatus(layout_state);
    Fill(layout_state);
  }

  // step 2. Update content size and content offset.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "UpdateContentSizeAndOffset");
  LayoutInvalidItemHolder(0);
  content_size_ = GetTargetContentSize();
  list_anchor_manager_->AdjustContentOffsetWithAnchor(anchor_info,
                                                      content_offset_);
  // step 2.5 Update sticky items.
  UpdateStickyItemsAfterLayout(anchor_info);
  FlushContentSizeAndOffsetToPlatform(
      layout_state.latest_updated_content_offset_);
  layout_state.latest_updated_content_offset_ = content_offset_;
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // The previous AdjustOffsetWithAnchor was called twice(the second one is
  // caused by sticky), so the scrolled value should be set only when both of
  // these calls have finished
  list_anchor_manager_->MarkScrolledInitialScrollIndex();

  // step 3. Handle Preload.
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "HandlePreloadIfNeeded");
  // Note: need update on screen children.
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);
  // TODO(dingwang.wxx): impl.
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void StaggeredGridLayoutManager::OnLayoutAfter() {
  HandleLayoutOrScrollResult(true);

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

void StaggeredGridLayoutManager::HandleLayoutOrScrollResult(bool is_layout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "HandlePlatformOperation");
  if (list_container_->enable_batch_render()) {
    // batch render.
    ListLayoutManager::HandleLayoutOrScrollResult(is_layout);
  } else {
    // No batch render.
    ListAdapter* list_adapter = list_container_->list_adapter();
    // 1. Recycle off-screen or off-preload's item holder.
    RecycleOffScreenItemHolders();
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

/**
 Fill
 This algorithm first searches for a valid layout trunk where all item_holders
 are either bound or binding in multi-thread mode. Afterward, we calculate the
 top and bottom positions to determine if there is a need to fill any blank
 space. If the top section needs to be filled, we call FillToStart. If there is
 a gap in the bottom section, we call FillToEnd. Note that calling the
 FillToStart function may cause layout changes since the layout always starts
 from index 0. Binding may also cause height changes, which can result in
 column-index and offset adjustments. Therefore, after calling FillToStart, we
 must always call FillToEnd to ensure that no extra gaps are created during the
 FillToStart process.
 */
void StaggeredGridLayoutManager::Fill(LayoutState& layout_state) {
  if (layout_state.layout_direction_ == list::LayoutDirection::kLayoutToEnd) {
    // If layoutToEnd, only need to fill to end.
    FillToEnd(layout_state);
    LayoutInvalidItemHolder(0);
  } else {
    FillToStart(layout_state);
    // Fill to end if end is not completely filled.
    layout_state.layout_direction_ = list::LayoutDirection::kLayoutToEnd;
    UpdateStartAndEndLinesStatus(layout_state);
    FillToEnd(layout_state);
    // Need layout all item_holder to avoid discontinuous layout which may cause
    // white space
    LayoutInvalidItemHolder(0);
  }
}

void StaggeredGridLayoutManager::FillToEnd(LayoutState& layout_state) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "StaggeredGridLayoutManager::FillToEnd");
  int next_item_index = *std::max_element(layout_state.end_index.begin(),
                                          layout_state.end_index.end());
  next_item_index += static_cast<int>(layout_state.layout_direction_);
  const int data_count = list_container_->GetDataCount();
  bool need_to_fill = next_item_index >= 0 && next_item_index < data_count &&
                      HasRemainSpaceToFillEnd(next_item_index, layout_state);
  while (need_to_fill) {
    LayoutChunkToEnd(next_item_index, layout_state, false);
    next_item_index += static_cast<int>(layout_state.layout_direction_);
    need_to_fill = next_item_index >= 0 && next_item_index < data_count &&
                   HasRemainSpaceToFillEnd(next_item_index, layout_state);
  }
}

void StaggeredGridLayoutManager::FillToStart(LayoutState& layout_state) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "StaggeredGridLayoutManager::FillToStart");
  // If layoutToStart, fill to start then fill to end
  LayoutInvalidItemHolder(0);
  // Fill to start first
  UpdateStartAndEndLinesStatus(layout_state);
  auto min_start_lines_height = std::min_element(
      layout_state.start_lines.begin(), layout_state.start_lines.end());
  int min_start_col = static_cast<int>(
      std::distance(layout_state.start_lines.begin(), min_start_lines_height));
  int min_start_index = layout_state.start_index[min_start_col];
  // Store the delta before layout
  ItemHolder* scroll_anchor_item_holder =
      list_container_->GetItemHolderForIndex(min_start_index);
  float start_align_delta =
      list_orientation_helper_->GetDecoratedStart(scroll_anchor_item_holder) -
      content_offset_;
  int next_item_index = list::kInvalidIndex;
  bool need_fill = HasRemainSpaceToFillStart(layout_state) &&
                   (next_item_index = FindNextIndexToFillStart(layout_state)) !=
                       list::kInvalidIndex;
  while (need_fill) {
    bool item_size_changed = false;
    ItemHolder* item_holder =
        list_container_->GetItemHolderForIndex(next_item_index);
    if (item_holder) {
      float before_size =
          list_orientation_helper_->GetDecoratedMeasurement(item_holder);
      list_container_->list_adapter()->BindItemHolder(item_holder,
                                                      next_item_index);
      item_size_changed = base::FloatsNotEqual(
          before_size,
          list_orientation_helper_->GetDecoratedMeasurement(item_holder));
    }
    if (item_size_changed) {
      // If item_holder really bound and size did changed, trigger the layout.
      LayoutInvalidItemHolder(0);
      UpdateStartAndEndLinesStatus(layout_state);
    } else {
      // If size didn't change, only update start lines and start indexes
      if (item_holder->item_full_span()) {
        std::fill(layout_state.start_lines.begin(),
                  layout_state.start_lines.end(),
                  list_orientation_helper_->GetDecoratedStart(item_holder));
        std::fill(layout_state.start_index.begin(),
                  layout_state.start_index.end(), next_item_index);
        // Note: need to update is_start_full_span_ after update start_lines and
        // start_index.
        layout_state.is_start_full_span_ = true;
      } else {
        int span_index = item_holder->item_col_index();
        layout_state.start_lines[span_index] =
            list_orientation_helper_->GetDecoratedStart(item_holder);
        layout_state.start_index[span_index] = next_item_index;
        // Note: need to update is_start_full_span_ after update start_lines and
        // start_index.
        layout_state.is_start_full_span_ = false;
      }
    }
    float target_content_offset =
        list_orientation_helper_->GetDecoratedStart(scroll_anchor_item_holder) -
        start_align_delta;
    content_size_ = GetTargetContentSize();
    SetContentOffset(target_content_offset);
    // If still not filled, trigger next fill.
    need_fill = HasRemainSpaceToFillStart(layout_state) &&
                (next_item_index = FindNextIndexToFillStart(layout_state)) !=
                    list::kInvalidIndex;
  }
}

void StaggeredGridLayoutManager::LayoutChunkToEnd(int current_index,
                                                  LayoutState& layout_state,
                                                  bool skip_bind) {
  if (!list_container_ || !list_children_helper_ || !list_orientation_helper_) {
    return;
  }
  // Init current layout item_holder
  ItemHolder* item_holder =
      list_container_->GetItemHolderForIndex(current_index);
  if (!item_holder || list_container_->list_adapter()->IsRemoved(item_holder)) {
    return;
  }
  item_holder->SetOrientation(orientation());
  if (!skip_bind) {
    list_container_->list_adapter()->BindItemHolder(item_holder, current_index);
  }
  float main_axis_position =
      CalculateMainAxisPosition(item_holder, layout_state);
  float cross_axis_position = CalculateCrossAxisPosition(item_holder);
  if (orientation_ == list::Orientation::kVertical) {
    item_holder->UpdateLayoutFromManager(cross_axis_position,
                                         main_axis_position);
  } else {
    item_holder->UpdateLayoutFromManager(main_axis_position,
                                         cross_axis_position);
  }
}

bool StaggeredGridLayoutManager::BindAllVisibleItemHolders() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "StaggeredGridLayoutManager::BindAllVisibleItemHolders");
  // Bind all visible item_holders
  bool should_fill = false;
  list_children_helper_->ForEachChild(
      [this, &should_fill](ItemHolder* item_holder) {
        if (IntersectVisibleArea(item_holder)) {
          should_fill = list_container_->list_adapter()->BindItemHolder(
                            item_holder, item_holder->index()) ||
                        should_fill;
        }
        return false;
      });
  return should_fill;
}

void StaggeredGridLayoutManager::LayoutInvalidItemHolder(
    int first_invalid_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "StaggeredGridLayoutManager::LayoutInvalidItemHolder",
              "first_invalid_index", first_invalid_index);
  if (list_container_->GetDataCount() == 0) {
    for (auto& column_index : column_indexes_) {
      column_index.clear();
    }
    return;
  }
  // (TODO)fangzhou.fz: here should not be default first_invalid_index 0.
  // Optimize it to improve performance.
  const int data_count = list_container_->GetDataCount();
  if (first_invalid_index < 0 || first_invalid_index > data_count - 1) {
    return;
  }
  for (auto& column_index : column_indexes_) {
    if (!column_index.empty()) {
      int erase_index = static_cast<int>(column_index.size());
      for (int i = erase_index - 1; i >= 0; --i) {
        if (column_index[i] < first_invalid_index) {
          break;
        }
        erase_index = i;
      }
      column_index.erase(column_index.begin() + erase_index,
                         column_index.end());
    }
  }
  LayoutState layout_state(span_count_);
  for (int i = first_invalid_index; i < data_count; ++i) {
    LayoutChunkToEnd(i, layout_state, true);
  }
}

float StaggeredGridLayoutManager::CalculateMainAxisPosition(
    ItemHolder* item_holder, LayoutState& layout_state) {
  // Use min and max elements to layout current item_holder and update end_lines
  auto& end_lines = layout_state.end_lines;
  auto& end_indexes = layout_state.end_index;
  if (end_lines.empty() || end_indexes.empty()) {
    return 0.f;
  }
  float item_size = 0.f;
  float main_axis_position = 0.f;
  int item_index = item_holder->index();
  if (item_holder->item_full_span()) {
    // Handle full_span items
    main_axis_position = *std::max_element(end_lines.begin(), end_lines.end());
    float top_inset = 0.f;

    if (item_index > 0) {
      top_inset = main_axis_gap_;
    } else {
      main_axis_position += list_orientation_helper_->GetStartAfterPadding();
    }
    // update top inset
    item_holder->SetTopInset(top_inset);
    // Note: After updating top_inset, we can get new item_size because
    // item_size contains the top_inset of item holder.
    item_size = list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    // Update end_lines and end_indexes.
    std::fill(end_lines.begin(), end_lines.end(),
              main_axis_position + item_size);
    std::fill(end_indexes.begin(), end_indexes.end(), item_index);
    layout_state.is_end_full_span_ = true;
    // Note: don't forget to add top_inset to main_axis_position.
    main_axis_position += top_inset;
    item_holder->SetItemColIndex(0);
    for (auto& info : column_indexes_) {
      info.push_back(item_index);
    }
  } else {
    int min_span_index =
        GetMinEndSpanItemInfoForEndLines(layout_state).span_index_;
    main_axis_position = end_lines[min_span_index];
    // Only add topInset to non_zero item_holder
    // When layout_manager changed, the previous top_inset setting should be
    // overrided
    float top_inset = 0;
    if (end_lines[min_span_index] > 0) {
      top_inset = main_axis_gap_;
    } else {
      main_axis_position += list_orientation_helper_->GetStartAfterPadding();
    }
    // update top inset
    item_holder->SetTopInset(top_inset);
    // Note: After updating top_inset, we can get new item_size because
    // item_size contains the top_inset of item holder.
    item_size = list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    // Update end_lines and end_indexes.
    end_lines[min_span_index] = main_axis_position + item_size;
    end_indexes[min_span_index] = item_index;
    layout_state.is_end_full_span_ = false;
    // Note: don't forget to add top_inset to main_axis_position.
    main_axis_position += top_inset;
    item_holder->SetItemColIndex(min_span_index);
    column_indexes_[min_span_index].push_back(item_index);
  }
  return main_axis_position;
}

float StaggeredGridLayoutManager::CalculateCrossAxisPosition(
    const ItemHolder* item_holder) {
  // padding + column_index * (column_size + cross_axis_gap)
  float column_size =
      list_orientation_helper_->GetDecoratedMeasurementInOther(item_holder) > 0
          ? list_orientation_helper_->GetDecoratedMeasurementInOther(
                item_holder)
          : list_orientation_helper_->GetMeasurementInOther() / span_count_;
  return list_orientation_helper_->GetStartAfterPaddingInOther() +
         item_holder->item_col_index() * (column_size + cross_axis_gap_);
}

/**
 scroll
 */
void StaggeredGridLayoutManager::ScrollByInternal(float content_offset,
                                                  float original_offset,
                                                  bool from_platform) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "StaggeredGridLayoutManager::ScrollByInternal");
  float delta = content_offset - last_content_offset_;
  if (!list_container_ || fabs(delta) < 10e-6) {
    FlushContentSizeAndOffsetToPlatform(content_offset);
    last_content_offset_ = content_offset_;
    return;
  }
  float content_offset_before_adjustment =
      from_platform ? content_offset : content_offset_;
  list_container_->StartInterceptListElementUpdated();
  // Fill
  LayoutState layout_state(span_count_,
                           delta > 0 ? list::LayoutDirection::kLayoutToEnd
                                     : list::LayoutDirection::kLayoutToStart);
  UpdateStartAndEndLinesStatus(layout_state);
  SetContentOffset(content_offset);
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    "StaggeredGridLayoutManager::ScrollByInternal.Fill");
  Fill(layout_state);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  content_size_ = GetTargetContentSize();
  UpdateStickyItems();
  // TODO(fangzhou.fz) adjust offset for sticky-item
  FlushContentSizeAndOffsetToPlatform(content_offset_before_adjustment);
  list_children_helper_->UpdateOnScreenChildren(list_orientation_helper_.get(),
                                                content_offset_);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "OnScrollAfter");
  OnScrollAfter(original_offset);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
}

void StaggeredGridLayoutManager::OnScrollAfter(float original_offset) {
  HandleLayoutOrScrollResult(false);
  // Send scroll event
  // Events has to be called after StopInterceptListElementUpdated to avoid
  // reenter in worklet
  list_container_->StopInterceptListElementUpdated();
  float scroll_delta = content_offset_ - last_content_offset_;
  last_content_offset_ = content_offset_;
  SendScrollEvents(scroll_delta, original_offset, list::EventSource::kScroll);
}

float StaggeredGridLayoutManager::GetTargetContentSize() {
  // Note: content size == padding top + sum of children's height + padding
  // bottom
  float content_size = 0.f;
  for (const auto& column_index : column_indexes_) {
    if (!column_index.empty()) {
      ItemHolder* item_holder =
          list_container_->GetItemHolderForIndex(column_index.back());
      content_size = std::max(
          content_size,
          list_orientation_helper_->GetDecoratedStart(item_holder) +
              list_orientation_helper_->GetDecoratedMeasurement(item_holder));
    }
  }
  // Note: end padding in main axis should be considered.
  return content_size + list_orientation_helper_->GetEndPadding();
}

bool StaggeredGridLayoutManager::IntersectVisibleArea(
    const ItemHolder* item_holder) const {
  float container_size = list_orientation_helper_->GetMeasurement();
  return base::FloatsLargerOrEqual(
             content_offset_ + container_size,
             list_orientation_helper_->GetDecoratedStart(item_holder)) &&
         base::FloatsLargerOrEqual(
             list_orientation_helper_->GetDecoratedEnd(item_holder),
             content_offset_);
}

bool StaggeredGridLayoutManager::CurrentLineHasRemainSpaceToFillEnd(
    float end_line) const {
  return end_line <
         std::min(content_offset_ + list_orientation_helper_->GetMeasurement(),
                  content_size_ - list_orientation_helper_->GetEndPadding());
}

bool StaggeredGridLayoutManager::HasRemainSpaceToFillEnd(
    int next_valid_item_index, LayoutState& layout_state) const {
  const auto& end_indexes = layout_state.end_index;
  const auto& end_lines = layout_state.end_lines;
  if (end_indexes.empty() || end_lines.empty()) {
    return false;
  }
  if (layout_state.is_end_full_span_) {
    // Case 1: end index is full span.
    //        col-0            col-1          col-2
    //   +--------------++--------------++--------------+
    //   | Item0(200px) || Item1(100px) || Item2(150px) |
    //   |              ||              ||              |
    //   |              |+______________+|              |
    //   |              |                +______________+
    //   +______________+                               |
    //   +----------------------------------------------+
    //   |          FullSpanItem3(end index)            |
    //   +----------------------------------------------+
    return CurrentLineHasRemainSpaceToFillEnd(end_lines[0]);
  } else {
    // Case 2: item in end indexes is not full span.
    ListAdapter* list_adapter = list_container_->list_adapter();
    if (list_adapter->IsFullSpanAtIndex(next_valid_item_index)) {
      // Case 2.1 the next item is full span.
      //        col-0            col-1          col-2
      //   +--------------++--------------++--------------+
      //   | Item0(200px) || Item1(100px) || Item2(150px) |
      //   |              ||              ||              |
      //   |              |+______________+|              |
      //   |              |                +______________+
      //   +______________+                               |
      //   +----------------------------------------------+
      //   |          FullSpanItem3(next item)            |
      //   +----------------------------------------------+
      float max_end_line = *std::max_element(layout_state.end_lines.begin(),
                                             layout_state.end_lines.end());
      return CurrentLineHasRemainSpaceToFillEnd(max_end_line);
    } else {
      // Case 2.2 the next item is not full span.
      //        col-0            col-1          col-2
      //   +--------------++--------------++--------------+
      //   | Item0(200px) || Item1(100px) || Item2(200px) |
      //   |              ||              ||              |
      //   |              |+______________+|              |
      //   |              |+______________+|              |
      //   +______________+|              |+______________+
      //                   |    Item3     |
      //                   | (next item)  |
      //                   |              |
      //                   +______________+
      float min_end_line = *std::min_element(layout_state.end_lines.begin(),
                                             layout_state.end_lines.end());
      return CurrentLineHasRemainSpaceToFillEnd(min_end_line);
    }
  }
}

bool StaggeredGridLayoutManager::HasRemainSpaceToFillStart(
    LayoutState& layout_state) const {
  const auto& start_indexes = layout_state.start_index;
  const auto& start_lines = layout_state.start_lines;
  if (start_indexes.empty() || start_lines.empty()) {
    return false;
  }
  const float start_after_padding =
      list_orientation_helper_->GetStartAfterPadding();
  if (layout_state.is_start_full_span_) {
    // Case 1: start index is full span
    float start_full_span_line = start_lines[0];
    return start_full_span_line > content_offset_ &&
           start_full_span_line > start_after_padding;
  } else {
    // Case 2: start indexes has no full span item
    for (int i = 0; i < static_cast<int>(start_lines.size()) &&
                    i < list_container_->GetDataCount();
         ++i) {
      if (start_lines[i] > content_offset_ &&
          start_lines[i] > start_after_padding) {
        return true;
      }
    }
  }
  return false;
}

int StaggeredGridLayoutManager::FindNextIndexToFillStart(
    LayoutState& layout_state) const {
  const auto& start_indexes = layout_state.start_index;
  const auto& start_lines = layout_state.start_lines;
  if (start_indexes.empty() || start_lines.empty()) {
    return list::kInvalidIndex;
  }
  ListAdapter* list_adapter = list_container_->list_adapter();
  if (layout_state.is_start_full_span_) {
    // Case 1 start index is full span
    int start_full_span_index = start_indexes[0];
    if (start_full_span_index == 0) {
      // Case 1.1 start index is first item.
      return list::kInvalidIndex;
    } else if (list_adapter->IsFullSpanAtIndex(start_full_span_index - 1)) {
      // Case 1.2 start index - 1 is full span item.
      return start_full_span_index - 1;
    } else {
      // Case 1.3. Find max end item index before start_full_span_index
      //        col-0            col-1          col-2
      //   +--------------++--------------++--------------+
      //   | Item0(200px) || Item1(100px) || Item2(150px) |
      //   |              ||              ||              |
      //   | (next item)  |+______________+|              |
      //   |              |                +______________+
      //   +______________+                               |
      //   +----------------------------------------------+
      //   |              FullSpanItem3(50)               |
      //   +----------------------------------------------+
      for (const auto& column_index : column_indexes_) {
        int item_index =
            GetItemIndexBeforeTargetIndex(column_index, start_full_span_index);
        if (item_index != list::kInvalidIndex) {
          return item_index;
        }
      }
    }
  } else {
    // Case 2. start index is not full span.
    // Here consider the case:
    //
    //        col-0            col-1          col-2
    //   +--------------++--------------++--------------+
    //   | Item0(200px) || Item1(100px) || Item2(150px) |
    //   |              ||              || (next item)  |
    //   |              |+______________+|              |
    //   |              |                +______________+
    //   +______________+                               |
    //   +----------------------------------------------+
    //   |              FullSpanItem3(50)               |
    //   +----------------------------------------------+
    //
    // If fill to start, the correct bind order should be: Item0, Item2, Item1.
    // After bind Item0, we find that col-1 and col-2 has the same start line
    // that larger than col-0. If we invoke max_element(start_lines) directly,
    // we will get the col-1 is the max start line and use col-1 to fill, but in
    // this case the correct col is col-2.
    std::vector<SpanItemInfo> span_item_infos =
        GetMaxEndSpanItemInfoForStartLines(layout_state);
    if (!span_item_infos.empty()) {
      for (const auto& span_item_info : span_item_infos) {
        if (span_item_info.IsValid()) {
          const auto& column_index =
              column_indexes_[span_item_info.span_index_];
          int next_item_index = GetItemIndexBeforeTargetIndex(
              column_index, span_item_info.item_index_);
          if (next_item_index != list::kInvalidIndex) {
            return next_item_index;
          }
        }
      }
    }
  }
  return list::kInvalidIndex;
}

int StaggeredGridLayoutManager::GetItemIndexBeforeTargetIndex(
    const std::vector<int>& span_indexes, int target_index) const {
  if (!span_indexes.empty()) {
    auto it = std::find(span_indexes.begin(), span_indexes.end(), target_index);
    auto reverse_it = span_indexes.rend();
    if (it != span_indexes.end() &&
        (reverse_it = std::make_reverse_iterator(it)) != span_indexes.rend()) {
      ItemHolder* next_item_holder =
          list_container_->GetItemHolderForIndex(*reverse_it);
      if (next_item_holder &&
          base::FloatsLargerOrEqual(
              list_orientation_helper_->GetDecoratedEnd(next_item_holder),
              content_offset_)) {
        return *reverse_it;
      }
    }
  }
  return list::kInvalidIndex;
}

std::vector<StaggeredGridLayoutManager::SpanItemInfo>
StaggeredGridLayoutManager::GetMaxEndSpanItemInfoForStartLines(
    LayoutState& layout_state) const {
  const auto& start_indexes = layout_state.start_index;
  const auto& start_lines = layout_state.start_lines;
  if (start_lines.empty()) {
    return {{list::kInvalidIndex, list::kInvalidIndex}};
  }
  if (layout_state.is_start_full_span_) {
    return {{0, start_indexes[0]}};
  }
  std::vector<SpanItemInfo> span_item_infos;
  auto max_line_it = std::max_element(start_lines.begin(), start_lines.end());
  for (int i = 0; i < static_cast<int>(start_lines.size()); ++i) {
    if (base::FloatsEqual(start_lines[i], *max_line_it)) {
      span_item_infos.emplace_back(i, start_indexes[i]);
    }
  }
  return span_item_infos;
}

StaggeredGridLayoutManager::SpanItemInfo
StaggeredGridLayoutManager::GetMinEndSpanItemInfoForEndLines(
    LayoutState& layout_state) const {
  const auto& end_indexes = layout_state.end_index;
  const auto& end_lines = layout_state.end_lines;
  if (end_lines.empty()) {
    return {list::kInvalidIndex, list::kInvalidIndex};
  }
  if (layout_state.is_end_full_span_) {
    return {0, end_indexes[0]};
  }
  auto it = std::min_element(end_lines.begin(), end_lines.end());
  int span_index = static_cast<int>(std::distance(end_lines.begin(), it));
  return {span_index, end_indexes[span_index]};
}

}  // namespace tasm
}  // namespace lynx
