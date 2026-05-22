// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_layout_manager_grid.h"

#include <algorithm>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_length_cache.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

namespace {

inline size_t GetRowIndex(size_t index, size_t columns) {
  return index / columns;
}

// Calculate the accumulated length of items from position 0 to `to_position`
// based on the cached length sections.
// Item at `to_position` is not included.
float CalculateAccumulatedLength(const ListItemLengthCache::Sections& sections,
                                 int to_position, size_t columns,
                                 float main_axis_gap = 0) {
  if (to_position <= 0) {
    return 0;
  }

  FML_DCHECK(columns >= 1);
  size_t rows = GetRowIndex(to_position - 1, columns) + 1;
  std::vector<float> rows_height(rows, -1);  // -1 means unknown

  for (const auto& section : sections) {
    const int end_pos = std::min(section.to_pos, to_position);
    if (section.length != ListItemLengthCache::kInvalidLength) {
      for (size_t start = GetRowIndex(section.from_pos, columns),
                  end = GetRowIndex(end_pos, columns);
           start <= end; start++) {
        if (start < rows) {
          rows_height[start] =
              std::max<float>(rows_height[start], section.length);
        }
      }
    }
    if (section.to_pos >= to_position) {
      break;
    }
  }

  float known_height = 0;
  size_t known_rows = 0;
  for (auto height : rows_height) {
    if (height >= 0) {
      known_rows++;
      known_height += height + main_axis_gap;
    }
  }

  if (known_rows == 0) {
    return 0;
  } else if (known_rows < rows_height.size()) {
    float estimated_unknown_height =
        known_height / known_rows * (rows_height.size() - known_rows);
    return known_height + estimated_unknown_height;
  } else {
    return known_height;
  }
}

}  // namespace

void CalculateItemBorders(std::vector<float>& cached_borders, size_t span_count,
                          float total_space) {
  if (cached_borders.size() != span_count + 1) {
    cached_borders.resize(span_count + 1);
  }
  if (cached_borders[span_count] == total_space) {
    // if space didn't change, we don't need calculate again
    return;
  }
  FML_DCHECK(span_count);
  float size_per_span = total_space / span_count;
  float consumed_pixels = 0;
  for (size_t i = 0; i <= span_count; ++i) {
    cached_borders[i] = consumed_pixels;
    consumed_pixels += size_per_span;
  }
}

class FullSpanEnabledSpanSizeLookup
    : public ListLayoutManagerGrid::SpanSizeLookup {
 public:
  explicit FullSpanEnabledSpanSizeLookup(ListLayoutManagerGrid* manager)
      : SpanSizeLookup(true), manager_(manager) {}
  int GetSpanSize(int position) const override {
    if (manager_->IsItemFullSpan(position)) {
      return manager_->GetSpanCount();
    }
    return 1;
  }

 private:
  ListLayoutManagerGrid* manager_;
};

ListLayoutManagerGrid::SpanSizeLookup::SpanSizeLookup(bool enable_cache)
    : enable_cache_(enable_cache) {}

int ListLayoutManagerGrid::SpanSizeLookup::GetCachedSpanIndex(int position,
                                                              int span_count) {
  if (!enable_cache_) {
    return GetSpanIndex(position, span_count);
  }
  auto iter = span_index_cache_.find(position);
  if (iter != span_index_cache_.end()) {
    return iter->second;
  }
  int value = GetSpanIndex(position, span_count);
  span_index_cache_.insert({position, value});
  return value;
}

int ListLayoutManagerGrid::SpanSizeLookup::GetCachedSpanGroupIndex(
    int adapter_position, int span_count) {
  if (!enable_cache_) {
    return GetSpanGroupIndex(adapter_position, span_count);
  }
  auto iter = span_group_index_cache_.find(adapter_position);
  if (iter != span_group_index_cache_.end()) {
    return iter->second;
  }
  int value = GetSpanIndex(adapter_position, span_count);
  span_group_index_cache_.insert({adapter_position, value});
  return value;
}

void ListLayoutManagerGrid::SpanSizeLookup::InvalidCaches() {
  span_group_index_cache_.clear();
  span_index_cache_.clear();
}

void ListLayoutManagerGrid::SpanSizeLookup::SetEnableCache(bool enable) {
  if (!enable) {
    InvalidCaches();
  }
  enable_cache_ = enable;
}

int ListLayoutManagerGrid::SpanSizeLookup::GetSpanIndex(int position,
                                                        int span_count) {
  int position_span_size = GetSpanSize(position);
  if (position_span_size == span_count) {
    return 0;
  }
  int span = 0;
  int start_pos = 0;

  for (int i = start_pos; i < position; ++i) {
    int size = GetSpanSize(i);
    span += size;
    if (span == span_count) {
      span = 0;
    } else if (span > span_count) {
      span = size;
    }
  }
  if (span + position_span_size <= span_count) {
    return span;
  }
  return 0;
}

int ListLayoutManagerGrid::SpanSizeLookup::GetSpanGroupIndex(
    int adapter_position, int span_count) {
  int span = 0;
  int group = 0;
  int start = 0;

  int position_span_size = GetSpanSize(adapter_position);
  for (int i = start; i < adapter_position; ++i) {
    int size = GetSpanSize(i);
    span += size;
    if (span == span_count) {
      span = 0;
      group++;
    } else if (span > span_count) {
      span = size;
      group++;
    }
  }
  if (span + position_span_size > span_count) {
    group++;
  }
  return group;
}

ListLayoutManagerGrid::ListLayoutManagerGrid(int span_count,
                                             ScrollDirection orientation)
    : ListLayoutManagerLinear(orientation),
      span_count_(span_count),
      cached_borders_({}),
      span_size_lookup_(std::make_unique<FullSpanEnabledSpanSizeLookup>(this)) {
  FML_CHECK(span_count > 0);
}

ListLayoutManagerGrid::~ListLayoutManagerGrid() = default;

int ListLayoutManagerGrid::GetSpanSize(ListRecycler& recycler,
                                       const ListViewState& state,
                                       int pos) const {
  return span_size_lookup_->GetSpanSize(pos);
}

int ListLayoutManagerGrid::GetSpanIndex(ListRecycler& recycler,
                                        const ListViewState& state,
                                        int pos) const {
  return span_size_lookup_->GetCachedSpanIndex(pos, span_count_);
}

int ListLayoutManagerGrid::GetSpanGroupIndex(ListRecycler& recycler,
                                             const ListViewState& state,
                                             int pos) const {
  return span_size_lookup_->GetCachedSpanGroupIndex(pos, span_count_);
}

float ListLayoutManagerGrid::ScrollVerticallyBy(float dy,
                                                ListRecycler& recycler,
                                                const ListViewState& state) {
  UpdateMeasurements();
  return ListLayoutManagerLinear::ScrollVerticallyBy(dy, recycler, state);
}

float ListLayoutManagerGrid::ScrollHorizontallyBy(float dx,
                                                  ListRecycler& recycler,
                                                  const ListViewState& state) {
  UpdateMeasurements();
  return ListLayoutManagerLinear::ScrollHorizontallyBy(dx, recycler, state);
}

void ListLayoutManagerGrid::OnItemsChanged(BaseListView* list_view) {
  span_size_lookup_->InvalidCaches();
}

void ListLayoutManagerGrid::OnItemsAdded(BaseListView* list_view,
                                         int position_start, int item_count) {
  ListLayoutManager::OnItemsAdded(list_view, position_start, item_count);
  span_size_lookup_->InvalidCaches();
}
void ListLayoutManagerGrid::OnItemsRemoved(BaseListView* list_view,
                                           int position_start, int item_count) {
  ListLayoutManager::OnItemsRemoved(list_view, position_start, item_count);
  span_size_lookup_->InvalidCaches();
}
void ListLayoutManagerGrid::OnItemsUpdated(BaseListView* list_view,
                                           int position_start, int item_count) {
  ListLayoutManager::OnItemsUpdated(list_view, position_start, item_count);
  span_size_lookup_->InvalidCaches();
}
void ListLayoutManagerGrid::OnItemsMoved(BaseListView* list_view, int from,
                                         int to, int item_count) {
  ListLayoutManager::OnItemsMoved(list_view, from, to, item_count);
  span_size_lookup_->InvalidCaches();
}

void ListLayoutManagerGrid::SetSpanCount(int span_count) {
  if (span_count == span_count_) {
    return;
  }
  FML_CHECK(span_count > 0) << "Span count must greater than 0!";
  span_count_ = span_count;
  span_size_lookup_->InvalidCaches();
  RequestLayout();
}

float ListLayoutManagerGrid::GetScrollOffset(const ListViewState& state) {
  if (!HasSpaceToStart(state)) {
    return 0.f;
  } else if (!HasSpaceToEnd(state)) {
    return GetTotalLength(state) - orientation_helper_->GetTotalSpace();
  }

  ListItemViewHolder* first = GetFirstInBoxChild(state);
  if (first == nullptr) {
    return 0.f;
  }
  const ListItemLengthCache::Sections& sections = length_cache_->GetSections();
  float offset = CalculateAccumulatedLength(sections, first->GetPosition(),
                                            span_count_, main_axis_gap_);
  offset -= orientation_helper_->GetDecoratedStart(first);
  return offset;
}

float ListLayoutManagerGrid::GetTotalLength(const ListViewState& state) {
  const ListItemLengthCache::Sections& sections = length_cache_->GetSections();
  return CalculateAccumulatedLength(sections, state.item_count, span_count_,
                                    main_axis_gap_);
}

void ListLayoutManagerGrid::CollectPrefetchPositionForLayoutState(
    const ListViewState& list_view_state, int abs_delta, int max_limit,
    LayoutPrefetchRegistry* layout_prefetch_registry) {
  int remaining_span_count = span_count_;
  int scrolling_offset = layout_state_.scrolling_offset_.value_or(0);
  if (GetChildCount() != 0 && scrolling_offset != 0) {
    while (layout_state_.HasMore(list_view_state) && remaining_span_count > 0 &&
           max_limit > 0) {
      max_limit--;
      int position = layout_state_.current_position_;
      layout_prefetch_registry->AddPosition(
          position,
          std::max(0, static_cast<int>(
                          layout_state_.scrolling_offset_.value_or(0))));
      int span_size = span_size_lookup_->GetSpanSize(position);
      remaining_span_count -= span_size;
      layout_state_.current_position_ +=
          static_cast<int>(layout_state_.item_direction_);
    }
  }
}

// Fill one row every time called.
float ListLayoutManagerGrid::LayoutChunk(ListRecycler& recycler,
                                         const ListViewState& state) {
  LIST_LOG << "------ LayoutChunk Start ------";
  std::vector<ListItemViewHolder*> temp_items;
  layout_params_.clear();
  bool laying_out_in_primary_direction =
      layout_state_.item_direction_ == ListItemDirection::kToTail;

  int count = 0;
  int remaining_span;
  if (laying_out_in_primary_direction) {
    remaining_span = span_count_;
  } else {
    int item_span_index =
        GetSpanIndex(recycler, state, layout_state_.current_position_);
    int item_span_size =
        GetSpanSize(recycler, state, layout_state_.current_position_);
    remaining_span = item_span_index + item_span_size;
  }
  while (count < span_count_ && layout_state_.HasMore(state) &&
         remaining_span > 0) {
    int pos = layout_state_.current_position_;
    int span_size = GetSpanSize(recycler, state, pos);
    if (span_size > span_count_) {
      // invalid case
      FML_DLOG(ERROR) << "Item at position " << pos << " requires " << span_size
                      << " spans but GridLayoutManager has only " << span_count_
                      << " spans. Skipping this item";
      continue;
    }
    remaining_span -= span_size;
    // When remaining_span is insufficient, finish loop instead of retrieving
    // item.
    if (remaining_span < 0) {
      break;
    }
    ListItemViewHolder* item = layout_state_.Next(recycler);
    if (item == nullptr) {
      break;
    }

    LIST_LOG << "Add an Item To layout. view_id:" << item->GetView()->id()
             << " name:" << item->GetView()->GetName()
             << " pos:" << item->GetPosition() << " span_size: " << span_size;
    temp_items.push_back(item);
    count++;
  }

  LIST_LOG << "Total items to layout " << count;

  if (count == 0) {
    return 0;
  }

  float max_size = 0.f;
  // same constraint for the row
  // first time we measure with constraint
  MeasureConstraint constraint;
  if (orientation_ == ScrollDirection::kVertical) {
    constraint.width_mode = MeasureMode::kDefinite;
    constraint.height_mode = MeasureMode::kIndefinite;
  } else {
    constraint.height_mode = MeasureMode::kDefinite;
    constraint.width_mode = MeasureMode::kIndefinite;
  }

  AssignSpan(recycler, state, count, laying_out_in_primary_direction,
             temp_items);
  for (ListItemViewHolder* item : temp_items) {
    if (laying_out_in_primary_direction) {
      AddItem(item, GetChildCount());
    } else {
      AddItem(item, 0);
    }

    auto param = GetLayoutParam(item);
    int total_space_in_other =
        GetSpaceForSpanRange(param.span_index_, param.span_size_);
    FML_DCHECK(total_space_in_other >= 0.f)
        << "Invalid span! span_index " << param.span_index_ << "\tspan_size "
        << param.span_size_;
    if (orientation_ == ScrollDirection::kVertical) {
      constraint.width = total_space_in_other;
    } else {
      constraint.height = total_space_in_other;
    }

    MeasureResult res = MeasureItem(item, constraint);
    if (orientation_ == ScrollDirection::kVertical) {
      if (res.height > max_size) {
        max_size = res.height;
      }
    } else {
      if (res.width > max_size) {
        max_size = res.width;
      }
    }
  }

  // MeasureAgain make sure they have same space in one row
  if (orientation_ == ScrollDirection::kVertical) {
    constraint.height = max_size;
    constraint.height_mode = MeasureMode::kDefinite;
  } else {
    constraint.width = max_size;
    constraint.width_mode = MeasureMode::kDefinite;
  }

  LIST_LOG << "Measure Items with constraint width: "
           << constraint.width.value_or(-1)
           << " height: " << constraint.height.value_or(-1);
  for (ListItemViewHolder* item : temp_items) {
    MeasureItem(item, constraint);
  }

  float left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;
  bool first_line = temp_items[0]->GetPosition() < span_count_;
  bool fill_to_start =
      layout_state_.layout_direction_ == ListLayoutDirection::kToStart;
  float expected_main_axis_gap =
      fill_to_start ? main_axis_gap_ : (first_line ? 0 : main_axis_gap_);
  if (orientation_ == ScrollDirection::kVertical) {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToStart) {
      bottom = layout_state_.offset_;
      bottom -= expected_main_axis_gap;
      top = bottom - max_size;
    } else {
      top = layout_state_.offset_;
      top += expected_main_axis_gap;
      bottom = top + max_size;
    }
  } else {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToStart) {
      right = layout_state_.offset_;
      right -= expected_main_axis_gap;
      left = right - max_size;
    } else {
      left = layout_state_.offset_;
      left += expected_main_axis_gap;
      right = left + max_size;
    }
  }

  for (ListItemViewHolder* item : temp_items) {
    auto param = GetLayoutParam(item);
    if (orientation_ == ScrollDirection::kVertical) {
      left = cached_borders_[param.span_index_];
      if (!item->FullSpan()) {
        left += GetPaddingLeft();
        left += cross_axis_gap_ * (param.span_index_ % span_count_);
      }
      right = left + orientation_helper_->GetSecondaryDecoratedMeasure(item);
    } else {
      top = cached_borders_[param.span_index_];
      if (!item->FullSpan()) {
        top += GetPaddingTop();
        top += cross_axis_gap_ * (param.span_index_ % span_count_);
      }
      bottom = right + orientation_helper_->GetSecondaryDecoratedMeasure(item);
    }
    LayoutItem(item, FloatPoint(left, top));

    LIST_LOG << "Laid out an item to (" << left << "," << top << ").";
  }

  LIST_LOG << "Grid Laid out a raw of item. consumed " << max_size << " items "
           << temp_items.size();
  return max_size + expected_main_axis_gap;
}

void ListLayoutManagerGrid::UpdateMeasurements() {
  float total_space = orientation_helper_->GetSecondaryTotalSpace();
  CalculateItemBorders(cached_borders_, span_count_, total_space);
}

void ListLayoutManagerGrid::OnAnchorReady(ListRecycler& recycler,
                                          const ListViewState& state,
                                          AnchorInfo& anchor_info,
                                          bool is_prime_direction) {
  ListLayoutManagerLinear::OnAnchorReady(recycler, state, anchor_info,
                                         is_prime_direction);
  UpdateMeasurements();
  if (state.item_count > 0) {
    EnsureAnchorSpanPosition(recycler, state, anchor_info, is_prime_direction);
  }
}

void ListLayoutManagerGrid::EnsureAnchorSpanPosition(
    ListRecycler& recycler, const ListViewState& state, AnchorInfo& anchor_info,
    bool is_prime_direction) const {
  int span = GetSpanIndex(recycler, state, anchor_info.position_);
  if (is_prime_direction) {
    while (span > 0 && anchor_info.position_ > 0) {
      anchor_info.position_--;
      span = GetSpanIndex(recycler, state, anchor_info.position_);
    }
  } else {
    int index_limit = state.item_count - 1;
    int pos = anchor_info.position_;
    int best_span = span;
    while (pos < index_limit) {
      int next = GetSpanIndex(recycler, state, pos + 1);
      if (next > best_span) {
        pos++;
        best_span = next;
      } else {
        break;
      }
    }
    anchor_info.position_ = pos;
  }
}

ListItemViewHolder* ListLayoutManagerGrid::FindReferenceChild(
    ListRecycler& recycler, const ListViewState& state) {
  ListItemViewHolder* invalid_match = nullptr;
  ListItemViewHolder* out_of_bounds_match = nullptr;
  ListItemViewHolder* in_bounds_match = nullptr;

  children_helper_->ForEach(
      [this, &invalid_match, &out_of_bounds_match, &in_bounds_match, &recycler,
       &state, item_count = state.item_count,
       bounds_start = orientation_helper_->GetStartAfterPadding(),
       bounds_end = orientation_helper_->GetEndAfterPadding()](
          ListItemViewHolder* child) -> bool {
        const int position = child->GetLayoutPosition();
        const int child_start = orientation_helper_->GetDecoratedStart(child);
        const int child_end = orientation_helper_->GetDecoratedEnd(child);
        if (position >= 0 && position < static_cast<int>(item_count)) {
          int span = GetSpanIndex(recycler, state, position);
          if (span != 0) {
            return false;
          }

          if (child->IsRemoved()) {
            if (invalid_match == nullptr) {
              invalid_match = child;  // removed item, least preferred
            }
            return false;
          }

          // Usually if child_start >= bounds_end the child is out
          // of bounds, except if the child is 0 pixels!
          const bool out_of_bounds_before =
              child_end <= bounds_start && child_start < bounds_start;
          const bool out_of_bounds_after =
              child_start >= bounds_end && child_end > bounds_end;
          if (out_of_bounds_before || out_of_bounds_after) {
            if (out_of_bounds_match == nullptr) {
              out_of_bounds_match = child;
              return false;
            }
          } else {
            in_bounds_match = child;
            return true;  // Found in bound. Should break the iteration.
          }
        }
        return false;
      });
  if (in_bounds_match) {
    return in_bounds_match;
  }
  if (out_of_bounds_match) {
    return out_of_bounds_match;
  }
  return invalid_match;
}

void ListLayoutManagerGrid::AssignSpan(
    ListRecycler& recycler, const ListViewState& state, int count,
    bool laying_out_in_primary_direction,
    const std::vector<ListItemViewHolder*>& temp_items) {
  int span_index = 0, start, end, diff;
  int span_size = 0;
  // make sure we traverse from min position to max position
  if (laying_out_in_primary_direction) {
    start = 0;
    end = count;
    diff = 1;
  } else {
    start = count - 1;
    end = -1;
    diff = -1;
  }
  for (int i = start; i != end; i += diff) {
    ListItemViewHolder* item = temp_items[i];
    span_size = GetSpanSize(recycler, state, item->GetPosition());
    layout_params_.insert({item->GetView()->id(), {span_index, span_size}});
    span_index += span_size;
  }
}

float ListLayoutManagerGrid::GetSpaceForSpanRange(int start_span,
                                                  int span_size) const {
  return cached_borders_[start_span + span_size] - cached_borders_[start_span];
}

const ListLayoutManagerGrid::LayoutParam& ListLayoutManagerGrid::GetLayoutParam(
    ListItemViewHolder* item) const {
  FML_DCHECK(item);

  int view_id = item->GetView()->id();
  auto iter = layout_params_.find(view_id);

  // ensure get layout param in a valid item.
  FML_DCHECK(iter != layout_params_.end())
      << " grid layout fail to get layout param for " << item->GetView()->id();
  return iter->second;
}

}  // namespace clay
