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
  // 1. Calculate top_item and end_item from attached_children
  std::vector<ItemHolder*> top_item_holder(
      span_count_, list_container_->GetItemHolderForIndex(
                       list_container_->GetDataCount() - 1));
  std::vector<ItemHolder*> end_item_holder(
      span_count_, list_container_->GetItemHolderForIndex(0));
  if (list_children_helper_->attached_children().size() > 0) {
    list_children_helper_->ForEachChild(
        list_children_helper_->attached_children(),
        [this, &top_item_holder, &end_item_holder,
         list_adapter =
             list_container_->list_adapter()](ItemHolder* item_holder) {
          if (IntersectVisibleArea(item_holder) &&
              !list_adapter->IsRemoved(item_holder)) {
            int column_index = item_holder->item_col_index();
            if (top_item_holder[column_index]->index() > item_holder->index()) {
              top_item_holder[column_index] = item_holder;
            }
            if (end_item_holder[column_index]->index() < item_holder->index()) {
              end_item_holder[column_index] = item_holder;
            }
          }
          return false;
        });
    // 2. Use top_item and end_item to update start_lines and start_indexes
    for (int i = 0; i < span_count_; i++) {
      if (top_item_holder[i]->item_full_span()) {
        std::fill(
            layout_state.start_lines.begin(), layout_state.start_lines.end(),
            list_orientation_helper_->GetDecoratedStart(top_item_holder[i]));
        std::fill(layout_state.start_index.begin(),
                  layout_state.start_index.end(), top_item_holder[i]->index());
        break;
      } else {
        layout_state.start_lines[i] =
            list_orientation_helper_->GetDecoratedStart(top_item_holder[i]);
        layout_state.start_index[i] = top_item_holder[i]->index();
      }
    }
    for (int i = 0; i < span_count_; i++) {
      if (end_item_holder[i]->item_full_span()) {
        std::fill(
            layout_state.end_lines.begin(), layout_state.end_lines.end(),
            list_orientation_helper_->GetDecoratedStart(end_item_holder[i]) +
                list_orientation_helper_->GetDecoratedMeasurement(
                    end_item_holder[i]));
        std::fill(layout_state.end_index.begin(), layout_state.end_index.end(),
                  end_item_holder[i]->index());
        break;
      } else {
        layout_state.end_lines[i] =
            list_orientation_helper_->GetDecoratedStart(end_item_holder[i]) +
            list_orientation_helper_->GetDecoratedMeasurement(
                end_item_holder[i]);
        layout_state.end_index[i] = end_item_holder[i]->index();
      }
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
  bool need_fill = HasRemainSpace(layout_state);
  if (!need_fill || layout_state.end_index.empty() ||
      layout_state.end_lines.empty()) {
    return;
  }
  int current_last_index = *std::max_element(layout_state.end_index.begin(),
                                             layout_state.end_index.end());
  current_last_index += static_cast<int>(layout_state.layout_direction_);
  while (need_fill && current_last_index >= 0 &&
         current_last_index < list_container_->GetDataCount()) {
    LayoutChunkToEnd(current_last_index, layout_state, false);
    need_fill = HasRemainSpace(layout_state);
    current_last_index += static_cast<int>(layout_state.layout_direction_);
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
  float delta =
      list_orientation_helper_->GetDecoratedStart(scroll_anchor_item_holder) -
      content_offset_;
  bool need_fill = HasRemainSpace(layout_state);
  while (need_fill) {
    int next_start_index_to_fill = FindNextIndexToBindToStart(layout_state);
    int max_start_col = BiggestColumn(layout_state.start_lines);
    ItemHolder* item_holder =
        list_container_->GetItemHolderForIndex(next_start_index_to_fill);
    // If no valid next index, break;
    if (!item_holder) {
      break;
    }
    float decorated_size =
        list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    list_container_->list_adapter()->BindItemHolder(item_holder,
                                                    next_start_index_to_fill);
    if (fabs(list_orientation_helper_->GetDecoratedMeasurement(item_holder) -
             decorated_size) > 10e-6) {
      // If item_holder really bound and size did changed, trigger the layout,
      // or just use the size in cache.
      LayoutInvalidItemHolder(0);
      UpdateStartAndEndLinesStatus(layout_state);
    } else {
      // If size didn't change, only update start_lines
      if (item_holder->item_full_span()) {
        std::fill(layout_state.start_lines.begin(),
                  layout_state.start_lines.end(),
                  list_orientation_helper_->GetDecoratedStart(item_holder));
        std::fill(layout_state.start_index.begin(),
                  layout_state.start_index.end(), item_holder->index());
      } else {
        layout_state.start_lines[max_start_col] =
            list_orientation_helper_->GetDecoratedStart(item_holder);
        layout_state.start_index[max_start_col] = item_holder->index();
      }
    }
    float target_content_offset =
        list_orientation_helper_->GetDecoratedStart(scroll_anchor_item_holder) -
        delta;
    content_size_ = GetTargetContentSize();
    SetContentOffset(target_content_offset);
    // If still not filled, trigger next fill
    need_fill = HasRemainSpace(layout_state);
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
  if (layout_state.end_index.empty() || layout_state.end_lines.empty()) {
    return 0.f;
  }
  auto min_end_line = std::min_element(layout_state.end_lines.begin(),
                                       layout_state.end_lines.end());
  auto max_end_line = std::max_element(layout_state.end_lines.begin(),
                                       layout_state.end_lines.end());
  float item_size = 0.f;
  float main_axis_position = 0.f;
  if (item_holder->item_full_span()) {
    // Handle full_span items
    float top_inset = 0.f;
    main_axis_position = *max_end_line;
    if (item_holder->index() > 0) {
      top_inset = main_axis_gap_;
    } else {
      main_axis_position += list_orientation_helper_->GetStartAfterPadding();
    }
    // update top inset
    item_holder->SetTopInset(top_inset);
    // Note: After updating top_inset, we can get new item_size because
    // item_size contains the top_inset of item holder.
    item_size = list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    std::fill(layout_state.end_lines.begin(), layout_state.end_lines.end(),
              main_axis_position + item_size);
    // Note: don't forget to add top_inset to main_axis_position.
    main_axis_position += top_inset;
    item_holder->SetItemColIndex(0);
    for (auto& info : column_indexes_) {
      info.push_back(item_holder->index());
    }
  } else {
    auto min_col_span_index = static_cast<int>(
        std::distance(layout_state.end_lines.begin(), min_end_line));
    main_axis_position = layout_state.end_lines[min_col_span_index];
    // Only add topInset to non_zero item_holder
    // When layout_manager changed, the previous top_inset setting should be
    // overrided
    float top_inset = 0;
    if (layout_state.end_lines[min_col_span_index] > 0) {
      top_inset = main_axis_gap_;
    } else {
      main_axis_position += list_orientation_helper_->GetStartAfterPadding();
    }
    // update top inset
    item_holder->SetTopInset(top_inset);
    // Note: After updating top_inset, we can get new item_size because
    // item_size contains the top_inset of item holder.
    item_size = list_orientation_helper_->GetDecoratedMeasurement(item_holder);
    layout_state.end_lines[min_col_span_index] = main_axis_position + item_size;
    // Note: don't forget to add top_inset to main_axis_position.
    main_axis_position += top_inset;
    layout_state.end_index[min_col_span_index] = item_holder->index();
    item_holder->SetItemColIndex(min_col_span_index);
    column_indexes_[min_col_span_index].push_back(item_holder->index());
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
    const ItemHolder* item_holder) {
  float container_size = list_orientation_helper_->GetMeasurement();
  return base::FloatsLargerOrEqual(
             content_offset_ + container_size,
             list_orientation_helper_->GetDecoratedStart(item_holder)) &&
         base::FloatsLargerOrEqual(
             list_orientation_helper_->GetDecoratedEnd(item_holder),
             content_offset_);
}

int StaggeredGridLayoutManager::SmallestColumn(
    const std::vector<float>& current_end_lines) {
  auto min_size =
      std::min_element(current_end_lines.begin(), current_end_lines.end());
  return static_cast<int>(std::distance(current_end_lines.begin(), min_size));
}

int StaggeredGridLayoutManager::BiggestColumn(
    const std::vector<float>& current_lines) {
  auto min_size = std::max_element(current_lines.begin(), current_lines.end());
  return static_cast<int>(std::distance(current_lines.begin(), min_size));
}

bool StaggeredGridLayoutManager::HasRemainSpace(LayoutState& layout_state) {
  // return true if need real fill
  if (layout_state.layout_direction_ == list::LayoutDirection::kLayoutToEnd) {
    return HasUnfilledEndLines(layout_state);
  } else {
    return HasUnfilledStartLines(layout_state);
  }
}

bool StaggeredGridLayoutManager::HasUnfilledEndLines(
    LayoutState& layout_state) {
  bool has_remain_space{false};
  if (layout_state.end_index.empty() || layout_state.end_lines.empty()) {
    return false;
  }

  if (list_container_->list_adapter()->HasFullSpanItems()) {
    // List-rows may cause unfilled rows, requiring special consideration
    int max_end_index = *std::max_element(layout_state.end_index.begin(),
                                          layout_state.end_index.end());
    int next_end_index = max_end_index + 1;
    if (next_end_index < list_container_->GetDataCount() &&
        list_container_->list_adapter()->IsFullSpanAtIndex(next_end_index)) {
      // judge whether the biggest element has cross end line if this
      // next_end_index is full-span as it will show up in a new row
      float max_end_line = *std::max_element(layout_state.end_lines.begin(),
                                             layout_state.end_lines.end());
      return CurrentLineHasUnfilledEnd(max_end_line);
    } else {
      // judge whether the smallest element has cross end line
      float min_end_line = *std::min_element(layout_state.end_lines.begin(),
                                             layout_state.end_lines.end());
      return CurrentLineHasUnfilledEnd(min_end_line);
    }
  }
  for (int i = 0; i < span_count_ && i < list_container_->GetDataCount(); ++i) {
    auto end_line = layout_state.end_lines[i];
    if (CurrentLineHasUnfilledEnd(end_line)) {
      has_remain_space = true;
      break;
    }
  }
  return has_remain_space;
}

bool StaggeredGridLayoutManager::HasUnfilledStartLines(
    LayoutState& layout_state) {
  bool has_remain_space{false};
  if (layout_state.start_index.empty() || layout_state.start_lines.empty()) {
    return false;
  }
  if (list_container_->list_adapter()->HasFullSpanItems()) {
    // List-rows may cause unfilled rows, requiring special consideration
    int next_start_index = FindNextIndexToBindToStart(layout_state);
    if (next_start_index < 0 &&
        next_start_index >= list_container_->GetDataCount()) {
      return false;
    }
    if (list_container_->list_adapter()->IsFullSpanAtIndex(next_start_index)) {
      // judge whether the biggest element has cross end line if this
      // next_start_index is full-span as it will show up in a new row
      float min_start_line = *std::min_element(layout_state.start_lines.begin(),
                                               layout_state.start_lines.end());
      return min_start_line > content_offset_ &&
             min_start_line > list_orientation_helper_->GetStartAfterPadding();
    } else {
      // judge whether the smallest element has cross end line
      auto next_item_holder =
          list_container_->GetItemHolderForIndex(next_start_index);
      return IntersectVisibleArea(next_item_holder);
    }
  }
  for (int i = 0; i < span_count_ && i < list_container_->GetDataCount(); ++i) {
    auto start_line = layout_state.start_lines[i];
    if (start_line > content_offset_ &&
        start_line > list_orientation_helper_->GetStartAfterPadding()) {
      has_remain_space = true;
      break;
    }
  }
  return has_remain_space;
}

bool StaggeredGridLayoutManager::CurrentLineHasUnfilledEnd(float end_line) {
  return end_line <
             std::min(
                 content_offset_ + list_orientation_helper_->GetMeasurement(),
                 content_size_ - list_orientation_helper_->GetEndPadding()) &&
         std::fabs(end_line - content_offset_) > 10e-6;
}

int StaggeredGridLayoutManager::FindNextIndexToBindToStart(
    LayoutState& layout_state) {
  int max_start_col = BiggestColumn(layout_state.start_lines);
  int max_start_position = layout_state.start_index[max_start_col];
  auto find_index =
      std::find(column_indexes_[max_start_col].begin(),
                column_indexes_[max_start_col].end(), max_start_position);
  int index_in_layout_column = static_cast<int>(
      std::distance(column_indexes_[max_start_col].begin(), find_index));
  if (index_in_layout_column <= 0 ||
      index_in_layout_column >=
          static_cast<int>(column_indexes_[max_start_col].size())) {
    return -1;
  } else {
    return column_indexes_[max_start_col][index_in_layout_column - 1];
  }
}

}  // namespace tasm
}  // namespace lynx
