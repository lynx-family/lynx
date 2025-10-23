// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_layout_manager_staggered_grid.h"

#include <algorithm>
#include <deque>
#include <map>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

namespace {
constexpr int kInvalidSpanID = -1;
constexpr float kInvalidLine = std::numeric_limits<float>::lowest();
}  // namespace

// We keep information about full span items because they may create gaps in
// the UI.
class FullSpanItem {
 public:
  float GetGapForSpan(int index) const {
    if (gap_per_span_.size() <= static_cast<size_t>(index)) {
      return 0;
    }
    return gap_per_span_[index];
  }

  int position_;
  ListLayoutDirection gap_dir_;
  std::vector<int> gap_per_span_;
  // A full span may be laid out in primary direction but may have gaps due to
  // invalidation of views after it. This is recorded during a reverse scroll
  // and if view is still on the screen after scroll stops, we have to
  // recalculate layout
  bool has_unwanted_gap_after_;
};

class LazySpanLookup {
 public:
  int ForceInvalidateAfter(size_t position) {
    auto iter = full_span_items_.lower_bound(position);
    while (iter != full_span_items_.end()) {
      full_span_items_.erase(iter);
      iter = full_span_items_.lower_bound(position);
    }
    return InvalidateAfter(position);
  }

  int InvalidateAfter(size_t position) {
    if (position >= data_.size()) {
      return ListItemViewHolder::kNoPosition;
    }
    int end_position = InvalidateFullSpansAfter(position);
    if (end_position == ListItemViewHolder::kNoPosition) {
      std::fill(data_.begin() + position, data_.end(), kInvalidSpanID);
      return data_.size();
    } else {
      // Just invalidate items in between `position` and the next full span
      // item, or the end of the tracked spans in `data_` if it's not been
      // lengthened yet.
      size_t invalidate_to_index =
          std::min(static_cast<size_t>(end_position + 1), data_.size());
      std::fill(data_.begin() + position, data_.begin() + invalidate_to_index,
                kInvalidSpanID);
      return invalidate_to_index;
    }
  }

  int GetSpan(size_t position) const {
    if (data_.size() <= position) {
      return kInvalidSpanID;
    }
    return data_[position];
  }

  void SetSpan(size_t position, int index) {
    EnsureSize(position);
    data_[position] = index;
  }

  void AddFullSpanItem(std::unique_ptr<FullSpanItem> full_span_item) {
    auto iter = full_span_items_.find(full_span_item->position_);
    FML_DCHECK(iter == full_span_items_.end())
        << "two full span has same position";
    full_span_items_.insert(
        {full_span_item->position_, std::move(full_span_item)});
  }

  FullSpanItem* GetFullSpanItem(size_t position) {
    if (full_span_items_.empty()) {
      return nullptr;
    }
    auto iter = full_span_items_.find(position);
    if (iter != full_span_items_.end()) {
      return iter->second.get();
    }
    return nullptr;
  }

  FullSpanItem* GetFirstFullSpanItemInRange(int min_pos, int max_pos,
                                            ListLayoutDirection gap_dir,
                                            bool has_unwanted_gap_after) {
    if (full_span_items_.empty()) {
      return nullptr;
    }
    auto iter = full_span_items_.lower_bound(min_pos);
    while (iter != full_span_items_.end()) {
      if (iter->first >= max_pos) {
        return nullptr;
      }
      if (gap_dir == iter->second->gap_dir_ ||
          (has_unwanted_gap_after && iter->second->has_unwanted_gap_after_)) {
        return iter->second.get();
      }
      ++iter;
    }
    return nullptr;
  }

  void OffsetForAddition(size_t position_start, int item_count) {
    if (position_start >= data_.size()) {
      return;
    }
    EnsureSize(position_start + item_count);
    std::copy_backward(data_.begin() + position_start, data_.end() - item_count,
                       data_.end());
    std::fill(data_.begin() + position_start,
              data_.begin() + position_start + item_count, kInvalidSpanID);
    OffsetFullSpansForAddition(position_start, item_count);
  }

  void OffsetForRemoval(size_t position_start, int item_count) {
    if (position_start >= data_.size()) {
      return;
    }

    EnsureSize(position_start + item_count);
    std::copy(data_.begin() + position_start + item_count, data_.end(),
              data_.begin() + position_start);
    std::fill(data_.end() - item_count, data_.end(), kInvalidSpanID);
    OffsetFullSpansForRemoval(position_start, item_count);
  }

  void Clear() {
    data_.assign(data_.size(), kInvalidSpanID);
    full_span_items_.clear();
  }

 private:
  int InvalidateFullSpansAfter(int position) {
    full_span_items_.erase(position);
    auto iter = full_span_items_.lower_bound(position);
    if (iter != full_span_items_.end()) {
      int position = iter->first;
      full_span_items_.erase(iter);
      return position;
    } else {
      return ListItemViewHolder::kNoPosition;
    }
  }

  void EnsureSize(size_t target_position) {
    size_t old_size = data_.size();
    if (old_size <= target_position) {
      // growth strategy
      size_t new_size = target_position + old_size + 1;
      data_.resize(new_size);
      std::fill(data_.begin() + old_size, data_.end(), kInvalidSpanID);
    }
  }

  void OffsetFullSpansForAddition(int position_start, int item_count) {
    if (full_span_items_.empty()) {
      return;
    }

    auto iter = full_span_items_.begin();
    std::vector<std::unique_ptr<FullSpanItem>> pending_items;
    while (iter != full_span_items_.end()) {
      if (iter->first < position_start) {
        ++iter;
      } else {
        iter->second->position_ += item_count;
        pending_items.emplace_back(std::move(iter->second));
        iter = full_span_items_.erase(iter);
      }
    }

    for (auto& item : pending_items) {
      full_span_items_.insert({item->position_, std::move(item)});
    }
  }

  void OffsetFullSpansForRemoval(int position_start, int item_count) {
    if (full_span_items_.empty()) {
      return;
    }
    int end = position_start + item_count;

    auto iter = full_span_items_.begin();
    std::vector<std::unique_ptr<FullSpanItem>> pending_items;
    while (iter != full_span_items_.end()) {
      if (iter->first >= position_start) {
        if (iter->first >= end) {
          iter->second->position_ -= item_count;
          pending_items.emplace_back(std::move(iter->second));
        }
        iter = full_span_items_.erase(iter);
      } else {
        ++iter;
      }
    }

    for (auto& item : pending_items) {
      full_span_items_.insert({item->position_, std::move(item)});
    }
  }

  std::vector<int> data_;
  std::map<int, std::unique_ptr<FullSpanItem>> full_span_items_;
};

class Span {
  using LayoutParam = ListLayoutManagerStaggeredGrid::LayoutParam;

 public:
  explicit Span(int index, ListLayoutManagerStaggeredGrid* manager)
      : index_(index), manager_(manager) {}

  /*
   * return cached_start_ if present, otherwise calculate and return
   * cached_start_ if items_ is not empty, otherwise return default_line
   */
  float GetStartLine(float default_line = 0.f) {
    if (cached_start_ != kInvalidLine) {
      return cached_start_;
    } else if (!items_.empty()) {
      CalculateStart();
      return cached_start_;
    }
    return default_line;
  }

  /*
   * return cached_end_ if present, otherwise calculate and return cached_end_
   * if items_ is not empty, otherwise return default_line
   */
  float GetEndLine(float default_line = 0.f) {
    if (cached_end_ != kInvalidLine) {
      return cached_end_;
    } else if (!items_.empty()) {
      CalculateEnd();
      return cached_end_;
    } else {
      return default_line;
    }
  }

  // prepend item to self and invalidate cached_start_( and cached_end_ if
  // needed)
  void PrependToSpan(ListItemViewHolder* item) {
    LayoutParam& param = manager_->GetLayoutParam(item);
    param.span_ = this;
    cached_start_ = kInvalidLine;
    if (items_.empty()) {
      cached_end_ = kInvalidLine;
    }
    if (item->IsRemoved() || item->NeedsUpdate()) {
      deleted_size_ += manager_->orientation_helper_->GetDecoratedMeasure(item);
    }

    items_.emplace_front(item);
  }

  // append item to self and invalidate cached_end_( and cached_start_ if
  // needed)
  void AppendToSpan(ListItemViewHolder* item) {
    LayoutParam& param = manager_->GetLayoutParam(item);
    param.span_ = this;
    cached_end_ = kInvalidLine;
    if (items_.empty()) {
      cached_start_ = kInvalidLine;
    }
    if (item->IsRemoved() || item->NeedsUpdate()) {
      deleted_size_ += manager_->orientation_helper_->GetDecoratedMeasure(item);
    }

    items_.emplace_back(item);
  }

  void OnOffset(float dt) {
    if (cached_end_ != kInvalidLine) {
      cached_end_ += dt;
    }
    if (cached_start_ != kInvalidLine) {
      cached_start_ += dt;
    }
  }

  void SetLine(float line) { cached_end_ = cached_start_ = line; }

  void PopStart() {
    FML_DCHECK(!items_.empty());
    ListItemViewHolder* item = items_.front();
    items_.pop_front();
    LayoutParam& param = manager_->GetLayoutParam(item);
    param.span_ = nullptr;
    if (item->IsRemoved() || item->NeedsUpdate()) {
      deleted_size_ -= manager_->orientation_helper_->GetDecoratedMeasure(item);
    }
    if (items_.empty()) {
      cached_end_ = kInvalidLine;
    }
    cached_start_ = kInvalidLine;
  }

  void PopEnd() {
    FML_DCHECK(!items_.empty());
    ListItemViewHolder* item = items_.back();
    items_.pop_back();
    LayoutParam& param = manager_->GetLayoutParam(item);
    param.span_ = nullptr;
    if (item->IsRemoved() || item->NeedsUpdate()) {
      deleted_size_ -= manager_->orientation_helper_->GetDecoratedMeasure(item);
    }
    if (items_.empty()) {
      cached_start_ = kInvalidLine;
    }
    cached_end_ = kInvalidLine;
  }

  std::vector<int> GetVisibleItemPositions() {
    int start = FindOneVisibleChildIndex(0, items_.size(), false);
    int last = FindOneVisibleChildIndex(items_.size() - 1, -1, false);

    if (start == ListItemViewHolder::kNoPosition) {
      return {};
    }
    FML_DCHECK(last != ListItemViewHolder::kNoPosition && start <= last);
    std::vector<int> positions;
    positions.reserve(last - start + 1);
    while (start <= last) {
      positions.push_back(items_[start]->GetPosition());
      start++;
    }
    return positions;
  }

  float GetDeletedSize() const {
    // TODO(hanhaoshen): Fix wrong deleted_size_ during relayout.
    return 0.f;
    // return deleted_size_;
  }

  void CacheReferenceLineAndClear(bool reverse_layout, float offset) {
    float reference;
    if (reverse_layout) {
      reference = GetEndLine(kInvalidLine);
    } else {
      reference = GetStartLine(kInvalidLine);
    }
    Clear();
    if (reference == kInvalidLine) {
      return;
    }
    if (!reverse_layout &&
        reference > manager_->orientation_helper_->GetStartAfterPadding()) {
      return;
    }
    if (reverse_layout &&
        reference < manager_->orientation_helper_->GetEndAfterPadding()) {
      return;
    }
    if (offset != ListLayoutManagerStaggeredGrid::AnchorInfo::kInvalidOffset) {
      reference += offset;
    }
    SetLine(reference);
  }

  void Clear() {
    items_.clear();
    SetLine(kInvalidLine);
    deleted_size_ = 0.f;
  }

  std::string DebugStatus() {
    std::stringstream s;
    s << "index: " << index_ << "\n";
    s << "items: " << items_.size() << "\n";
    s << "deleted: " << deleted_size_ << "\n";
    s << "cached_start: " << cached_start_ << "\n";
    s << "cached_end: " << cached_end_ << "\n";
    return s.str();
  }

  size_t size() const { return items_.size(); }
  int index() const { return index_; }

 private:
  void CalculateStart() {
    FML_DCHECK(!items_.empty());

    ListItemViewHolder* item = items_.front();
    const LayoutParam& param = manager_->GetLayoutParam(item);
    cached_start_ = manager_->orientation_helper_->GetDecoratedStart(item);
    if (!param.full_span_) {
      return;
    }
    FullSpanItem* full_span_item =
        manager_->lazy_span_lookup_->GetFullSpanItem(item->GetPosition());
    if (full_span_item != nullptr &&
        full_span_item->gap_dir_ == ListLayoutDirection::kToStart) {
      cached_start_ -= full_span_item->GetGapForSpan(index_);
    }
  }

  void CalculateEnd() {
    FML_DCHECK(!items_.empty());

    ListItemViewHolder* item = items_.back();
    cached_end_ = manager_->orientation_helper_->GetDecoratedEnd(item);
    const LayoutParam& param = manager_->GetLayoutParam(item);
    if (!param.full_span_) {
      return;
    }
    FullSpanItem* full_span_item =
        manager_->lazy_span_lookup_->GetFullSpanItem(item->GetPosition());
    if (full_span_item != nullptr &&
        full_span_item->gap_dir_ == ListLayoutDirection::kToEnd) {
      cached_end_ += full_span_item->GetGapForSpan(index_);
    }
  }

  int FindOneVisibleChildIndex(int from_index, int to_index,
                               bool complete_visible) {
    ListOrientationHelper* orientation_helper =
        manager_->GetOrientationHelper();
    float start = orientation_helper->GetStartAfterPadding();
    float end = orientation_helper->GetEndAfterPadding();
    int diff = to_index > from_index ? 1 : -1;
    for (int i = from_index; i != to_index; i += diff) {
      ListItemViewHolder* item = items_[i];
      if (orientation_helper->GetDecoratedMeasure(item) == 0.f) {
        continue;
      }
      float child_start = orientation_helper->GetDecoratedStart(item);
      float child_end = orientation_helper->GetDecoratedEnd(item);
      // Intersect, partially visible
      if (child_start <= end && child_end >= start) {
        if (complete_visible) {
          if (child_start <= start && child_end >= end) {
            return i;
          }
        } else {
          return i;
        }
      }
    }
    return ListItemViewHolder::kNoPosition;
  }

  const int index_;

  ListLayoutManagerStaggeredGrid* manager_;
  float cached_start_ = kInvalidLine;
  float cached_end_ = kInvalidLine;
  float deleted_size_ = 0.f;
  std::deque<ListItemViewHolder*> items_;
};

void ListLayoutManagerStaggeredGrid::AnchorInfo::Reset() {
  position_ = ListItemViewHolder::kNoPosition;
  offset_ = kInvalidOffset;
  layout_from_end_ = false;
  invalidate_offsets_ = false;
  valid_ = false;
  span_reference_lines_.assign(span_reference_lines_.size(), kInvalidSpanID);
}

void ListLayoutManagerStaggeredGrid::AnchorInfo::SaveSpanReferenceLines(
    std::vector<std::unique_ptr<Span>>& spans) {
  size_t span_count = spans.size();
  if (span_reference_lines_.size() < span_count) {
    span_reference_lines_.resize(span_count);
  }
  for (size_t i = 0; i < span_count; ++i) {
    span_reference_lines_[i] = spans[i]->GetStartLine(kInvalidLine);
  }
}

std::string ListLayoutManagerStaggeredGrid::AnchorInfo::ToString() const {
#if (DEBUG_LIST)
  std::stringstream s;
  s << "position :" << position_ << "\t";
  s << "offset :" << offset_ << "\t";
  s << "layout_from_end : " << layout_from_end_ << "\t";
  s << "invalidate_offsets_ : " << invalidate_offsets_ << "\t";
  s << "valid_ : " << valid_ << "\t";
  return s.str();
#else
  return "";
#endif
}

bool ListLayoutManagerStaggeredGrid::LayoutState::HasMore(
    const ListViewState& state) {
  return current_position_ >= 0 &&
         static_cast<size_t>(current_position_) < state.item_count;
}

ListItemViewHolder* ListLayoutManagerStaggeredGrid::LayoutState::Next(
    ListRecycler& recycler) {
  ListItemViewHolder* item = recycler.GetItemForPosition(current_position_);
  current_position_ += static_cast<int>(item_direction_);
  return item;
}

std::string ListLayoutManagerStaggeredGrid::LayoutState::ToString() const {
#if (DEBUG_LIST)
  std::stringstream res;
  res << "recycle: " << (recycle_ ? "true" : "false") << "\n";
  res << "infinite: " << (infinite_ ? "true" : "false") << "\n";
  res << "available: " << std::to_string(available_) << "\n";
  res << "offset: " << std::to_string(offset_) << "\n";

  res << "current_position: " << std::to_string(current_position_) << "\n";

  res << "item_direction: "
      << (item_direction_ == ListItemDirection::kToHead ? "Head" : "Tail")
      << "\n";

  res << "layout_direction: "
      << (layout_direction_ == ListLayoutDirection::kToStart ? "Start" : "End")
      << "\n";

  res << "start_line: " << start_line_ << "\n";
  res << "end_line: " << end_line_;

  return res.str();
#else
  return "";
#endif
}

ListLayoutManagerStaggeredGrid::ListLayoutManagerStaggeredGrid(
    int span_count, ScrollDirection orientation)
    : ListLayoutManager(orientation) {
  lazy_span_lookup_ = std::make_unique<LazySpanLookup>();
  SetSpanCount(span_count);
  anchor_info_.Reset();
}

ListLayoutManagerStaggeredGrid::~ListLayoutManagerStaggeredGrid() = default;

void ListLayoutManagerStaggeredGrid::SetSpanCount(int span_count) {
  if (span_count == span_count_) {
    return;
  }
  FML_CHECK(span_count > 0) << "Span count must greater than 0!";
  lazy_span_lookup_->Clear();
  span_count_ = span_count;
  remaining_spans_.resize(span_count);
  spans_.reserve(span_count);
  for (int i = 0; i < span_count; ++i) {
    spans_.emplace_back(std::make_unique<Span>(i, this));
  }
  RequestLayout();
}

bool ListLayoutManagerStaggeredGrid::SetOrientation(
    ScrollDirection orientation) {
  if (ListLayoutManager::SetOrientation(orientation)) {
    RequestLayout();
    return true;
  }
  return false;
}

void ListLayoutManagerStaggeredGrid::OnFocusSearchFailed(
    bool to_end, ListRecycler& recycler, const ListViewState& state) {
  if (GetChildCount() == 0) {
    return;
  }
  int next_position;
  if (to_end) {
    next_position = GetLastChildPosition() + 1;
    SetLayoutStateDirection(ListLayoutDirection::kToEnd);
  } else {
    next_position = GetFirstChildPosition() - 1;
    SetLayoutStateDirection(ListLayoutDirection::kToStart);
  }
  LIST_LOG << "OnFocusSearchFailed try to reach " << next_position;
  UpdateLayoutState(next_position, state);
  Fill(recycler, state);
}

void ListLayoutManagerStaggeredGrid::OnScrollStateChange(
    Scrollable::ScrollStatus state) {
  if (state == Scrollable::ScrollStatus::kIdle) {
    CheckForGaps();
  }
}

void ListLayoutManagerStaggeredGrid::ScrollToPosition(int position,
                                                      AlignTo align_to) {
  pending_scroll_position_ = position;
  pending_scroll_align_to_ = align_to;
  LIST_LOG << "ScrollToPosition " << position;
}

FloatSize ListLayoutManagerStaggeredGrid::ScrollToRect(const FloatRect& rect,
                                                       AlignTo align_to) {
  const float rect_start = orientation_helper_->GetRectStart(rect);
  const float rect_end = orientation_helper_->GetRectEnd(rect);

  // Ignore padding because we don't clip the padding.
  const float bound_start = 0.f;
  const float bound_end = orientation_helper_->GetEnd();

  bool totally_in_view = rect_start >= bound_start && rect_end <= bound_end;
  if (totally_in_view && align_to == AlignTo::kNone) {
    return FloatSize();
  }

  float offset = 0.f;
  if (align_to == AlignTo::kMiddle) {
    offset = (rect_end + rect_start - (bound_end + bound_start)) / 2;
  } else if (align_to == AlignTo::kStart) {
    offset = rect_start - bound_start;
  } else if (align_to == AlignTo::kEnd) {
    offset = rect_end - bound_end;
  } else {
    auto start_offset = rect_start - bound_start;
    auto end_offset = rect_end - bound_end;
    if (start_offset < 0) {
      if (end_offset <= 0) {
        // item is [partially] above list, but not larger than list.
        offset = start_offset;
      }
    } else if (end_offset > 0) {
      // item is [partially] under list
      offset = end_offset;
    }
  }

  if (orientation_ == ScrollDirection::kHorizontal) {
    return {offset, 0.f};
  } else {
    return {0.f, offset};
  }
}

bool ListLayoutManagerStaggeredGrid::HasSpaceToStart(
    const ListViewState& state) {
  if (GetChildCount() == 0) {
    return false;
  }
  ListItemViewHolder* first = GetFirstChild();
  if (first->GetPosition() != 0) {
    return true;
  }
  return orientation_helper_->GetDecoratedStart(first);
}

bool ListLayoutManagerStaggeredGrid::HasSpaceToEnd(const ListViewState& state) {
  if (GetChildCount() == 0) {
    return false;
  }
  ListItemViewHolder* last = GetLastChild();
  if (last->GetPosition() != static_cast<int>(state.item_count - 1)) {
    return true;
  }
  return orientation_helper_->GetDecoratedEnd(last) >
         orientation_helper_->GetEnd();
}

ListItemViewHolder* ListLayoutManagerStaggeredGrid::GetFirstInBoxChild(
    const ListViewState& state) {
  ListItemViewHolder* invalid_match = nullptr;
  ListItemViewHolder* last_out_of_bounds = nullptr;
  // in bounds means, the start of the child is out of the visible area but the
  // end of the child is below the start of the list view.
  ListItemViewHolder* in_bounds = nullptr;
  ListItemViewHolder* in_box = nullptr;
  children_helper_->ForEach(
      [this, &invalid_match, &last_out_of_bounds, &in_bounds, &in_box,
       item_count = state.item_count, start = 0.f,
       end = orientation_helper_->GetEnd()](clay::ListItemViewHolder* child) {
        const int position = child->GetLayoutPosition();
        const int child_start = orientation_helper_->GetDecoratedStart(child);
        const int child_end = orientation_helper_->GetDecoratedEnd(child);
        if (position >= 0 && position < static_cast<int>(item_count)) {
          if (child->IsRemoved()) {
            if (invalid_match == nullptr) {
              invalid_match = child;  // removed item, least preferred
            }
            return false;
          }

          if (child_end <= start) {
            last_out_of_bounds = child;
            return false;
          }

          if (child_start < start && child_end > start) {
            in_bounds = child;

            // We may still find the in box child, continue the iteration.
            return false;
          }

          if (child_start >= start) {
            in_box = child;
            return true;
          }

          if (child_start >= end) {
            return true;
          }

          FML_UNREACHABLE();
        }
        return false;
      });

  if (in_box) {
    return in_box;
  } else if (in_bounds) {
    return in_bounds;
  } else if (last_out_of_bounds) {
    return last_out_of_bounds;
  } else {
    return invalid_match;
  }
}

ListItemViewHolder* ListLayoutManagerStaggeredGrid::GetLastInBoxChild(
    const ListViewState& state) {
  ListItemViewHolder* invalid_match = nullptr;
  ListItemViewHolder* last_out_of_bounds = nullptr;
  ListItemViewHolder* in_bounds = nullptr;
  ListItemViewHolder* in_box = nullptr;
  children_helper_->ReversedForEach(
      [this, &invalid_match, &last_out_of_bounds, &in_bounds, &in_box,
       item_count = state.item_count, start = 0.f,
       end = orientation_helper_->GetEnd()](clay::ListItemViewHolder* child) {
        const int position = child->GetLayoutPosition();
        const int child_start = orientation_helper_->GetDecoratedStart(child);
        const int child_end = orientation_helper_->GetDecoratedEnd(child);
        if (position >= 0 && position < static_cast<int>(item_count)) {
          if (child->IsRemoved()) {
            if (invalid_match == nullptr) {
              invalid_match = child;  // removed item, least preferred
            }
            return false;
          }

          if (child_start >= end) {
            last_out_of_bounds = child;
            return false;
          }

          if (child_end > end && child_start < end) {
            in_bounds = child;

            // We may still find the in box child, continue the iteration.
            return false;
          }

          if (child_end <= end) {
            in_box = child;
            return true;
          }

          if (child_end <= start) {
            return true;
          }

          FML_UNREACHABLE();
        }
        return false;
      });

  if (in_box) {
    return in_box;
  } else if (in_bounds) {
    return in_bounds;
  } else if (last_out_of_bounds) {
    return last_out_of_bounds;
  } else {
    return invalid_match;
  }
}

float ListLayoutManagerStaggeredGrid::GetScrollOffset(
    const ListViewState& state) {
  if (!HasSpaceToStart(state)) {
    return 0.f;
  } else if (!HasSpaceToEnd(state)) {
    return GetTotalLength(state) - orientation_helper_->GetTotalSpace();
  }

  ListItemViewHolder* first = GetFirstInBoxChild(state);
  if (first == nullptr) {
    return 0.f;
  }
  float offset = CalculateAccumulatedLength(first->GetPosition());
  offset -= orientation_helper_->GetDecoratedStart(first);
  return offset;
}

float ListLayoutManagerStaggeredGrid::GetTotalLength(
    const ListViewState& state) {
  return CalculateAccumulatedLength(state.item_count);
}

float ListLayoutManagerStaggeredGrid::CalculateAccumulatedLength(
    int to_position) {
  if (to_position <= 0) {
    return 0.f;
  }

  FML_DCHECK(to_position > 0);
  int target_span_idx = lazy_span_lookup_->GetSpan(to_position);
  std::vector<int> known_item(span_count_, 0);
  std::vector<float> heights(span_count_, 0.f);
  int unassigned_span_item = 0;
  for (int i = 0; i < to_position; ++i) {
    int cur_span = lazy_span_lookup_->GetSpan(i);
    bool is_full_span = (lazy_span_lookup_->GetFullSpanItem(i) != nullptr);
    std::optional<int> length = length_cache_->GetLength(i);
    if (length.has_value() &&
        length.value() != ListItemLengthCache::kInvalidLength) {
      if (is_full_span) {
        std::for_each(known_item.begin(), known_item.end(),
                      [](int& counter) { counter++; });
        std::for_each(heights.begin(), heights.end(),
                      [&length, this](float& h) {
                        h += length.value() + main_axis_gap_;
                      });
      } else {
        if (cur_span == kInvalidSpanID) {
          unassigned_span_item += 1;
        } else {
          known_item[cur_span] += 1;
          heights[cur_span] += length.value();
          heights[cur_span] += main_axis_gap_;
        }
      }
    } else {
      unassigned_span_item += (is_full_span ? span_count_ : 1);
    }
  }
  if (unassigned_span_item == 0 && target_span_idx != kInvalidSpanID) {
    return heights[target_span_idx];
  } else {
    float item_avg_height =
        std::accumulate(heights.begin(), heights.end(), 0.f) /
        std::accumulate(known_item.begin(), known_item.end(), 0);
    float estimated_height =
        item_avg_height * unassigned_span_item / span_count_;
    if (target_span_idx != kInvalidSpanID) {
      return (known_item[target_span_idx] + estimated_height);
    } else {
      return (*std::max_element(heights.begin(), heights.end()) +
              estimated_height);
    }
  }
}

void ListLayoutManagerStaggeredGrid::OnLayoutChildren(
    ListRecycler& recycler, const ListViewState& state) {
  if (state.structure_changed) {
    auto first_child = GetFirstChild();
    if (first_child) {
      lazy_span_lookup_->InvalidateAfter(first_child->GetPosition());
    }
  }
  OnLayoutChildrenInternal(recycler, state, true);
}

void ListLayoutManagerStaggeredGrid::CollectAdjacentPrefetchPositions(
    int delta_x, int delta_y, int max_limit,
    const ListViewState& list_view_state,
    LayoutPrefetchRegistry* layout_prefetch_registry) {
  int delta = (orientation_ == ScrollDirection::kVertical ? delta_y : delta_x);
  if (GetChildCount() == 0 || delta == 0) {
    return;
  }
  int remaining_span_count = span_count_;
  while (layout_state_.HasMore(list_view_state) && remaining_span_count > 0 &&
         max_limit > 0) {
    max_limit--;
    int position = layout_state_.current_position_;
    layout_prefetch_registry->AddPosition(
        position,
        std::max(0, static_cast<int>(GetScrollOffset(list_view_state))));

    int span_size = (lazy_span_lookup_->GetFullSpanItem(position) != nullptr)
                        ? span_count_
                        : 1;
    remaining_span_count -= span_size;
    layout_state_.current_position_ +=
        static_cast<int>(layout_state_.item_direction_);
  }
}

void ListLayoutManagerStaggeredGrid::OnLayoutChildrenInternal(
    ListRecycler& recycler, const ListViewState& state,
    bool should_check_for_gaps) {
  if (pending_scroll_position_ != ListItemViewHolder::kNoPosition &&
      state.item_count == 0) {
    RemoveAndRecycleAllViews(recycler);
    anchor_info_.Reset();
    return;
  }

  bool recalculate_anchor =
      !anchor_info_.valid_ ||
      pending_scroll_position_ != ListItemViewHolder::kNoPosition;
  if (recalculate_anchor) {
    anchor_info_.Reset();
    ResolveShouldLayoutReverse();
    anchor_info_.layout_from_end_ = should_reverse_layout_;
    UpdateAnchorInfo(state);
    anchor_info_.valid_ = true;
  }

  if (pending_scroll_position_ == ListItemViewHolder::kNoPosition) {
    if (anchor_info_.layout_from_end_ != last_layout_from_end_) {
      lazy_span_lookup_->Clear();
      anchor_info_.invalidate_offsets_ = true;
    }
  }

  if (GetChildCount() > 0) {
    if (anchor_info_.invalidate_offsets_) {
      // clear cells'layout data in origin spans and re-layout from computed
      // anchor_info_.offset_ as cached_start_&cached_end_
      for (auto& span : spans_) {
        span->Clear();
        if (anchor_info_.offset_ != AnchorInfo::kInvalidOffset) {
          span->SetLine(anchor_info_.offset_);
        }
      }
    } else {
      if (recalculate_anchor || anchor_info_.span_reference_lines_.empty()) {
        for (auto& span : spans_) {
          // clear cells'layout data in origin spans and re-layout from
          // cached_start_&cached_end_ plus anchor_info_.offset_
          span->CacheReferenceLineAndClear(should_reverse_layout_,
                                           anchor_info_.offset_);
        }
        anchor_info_.SaveSpanReferenceLines(spans_);
      } else {
        for (int i = 0; i < span_count_; ++i) {
          spans_[i]->Clear();
          spans_[i]->SetLine(anchor_info_.span_reference_lines_[i]);
        }
      }
    }
  }

  // update recycler
  RemoveAndScrapChildren(recycler);
  layout_state_.recycle_ = false;
  laid_out_invalid_full_span_ = false;
  UpdateMeasureSpecs(orientation_helper_->GetSecondaryTotalSpace());
  // set anchor_info_.position_ as start point of layout
  UpdateLayoutState(anchor_info_.position_, state);

  // start fill
  if (anchor_info_.layout_from_end_) {
    // Layout start.
    SetLayoutStateDirection(ListLayoutDirection::kToStart);
    Fill(recycler, state);
    // Layout end.
    SetLayoutStateDirection(ListLayoutDirection::kToEnd);
    layout_state_.current_position_ =
        anchor_info_.position_ +
        static_cast<int>(layout_state_.item_direction_);
    Fill(recycler, state);
  } else {
    // Layout end.
    SetLayoutStateDirection(ListLayoutDirection::kToEnd);
    Fill(recycler, state);
    // Layout start.
    SetLayoutStateDirection(ListLayoutDirection::kToStart);
    layout_state_.current_position_ =
        anchor_info_.position_ +
        static_cast<int>(layout_state_.item_direction_);
    Fill(recycler, state);
  }

  // Fix Gaps
  if (GetChildCount() > 0) {
    FixLayoutStartGap(recycler, state, true);
    FixLayoutEndGap(recycler, state, false);
  }

  bool has_gaps = false;
  if (should_check_for_gaps) {
    if (GetChildCount() > 0 &&
        (laid_out_invalid_full_span_ || HasGapsToFix() != nullptr)) {
      has_gaps = CheckForGaps();
    }
  }
  last_layout_from_end_ = anchor_info_.layout_from_end_;
  if (has_gaps) {
    anchor_info_.Reset();
    OnLayoutChildrenInternal(recycler, state, false);
  }
}

void ListLayoutManagerStaggeredGrid::OnLayoutCompleted(
    ListRecycler& recycler, const ListViewState& state) {
  ListLayoutManager::OnLayoutCompleted(recycler, state);

  anchor_info_.Reset();
  pending_scroll_position_ = ListItemViewHolder::kNoPosition;
}

float ListLayoutManagerStaggeredGrid::ScrollVerticallyBy(
    float dy, ListRecycler& recycler, const ListViewState& state) {
  if (orientation_ != ScrollDirection::kVertical) {
    return 0;
  }
  return ScrollBy(dy, recycler, state);
}

float ListLayoutManagerStaggeredGrid::ScrollHorizontallyBy(
    float dx, ListRecycler& recycler, const ListViewState& state) {
  if (orientation_ != ScrollDirection::kHorizontal) {
    return 0;
  }
  return ScrollBy(dx, recycler, state);
}

void ListLayoutManagerStaggeredGrid::OffsetChildren(float dx, float dy) {
  // Can only be one direction to offset
  FML_DCHECK(dx == 0.f || dy == 0.f);
  ListLayoutManager::OffsetChildren(dx, dy);
  for (auto& span : spans_) {
    span->OnOffset(dx + dy);
  }
}

void ListLayoutManagerStaggeredGrid::OnRemoveItem(ListItemViewHolder* item) {
  layout_params_.erase(item->GetView()->id());
}

void ListLayoutManagerStaggeredGrid::OnItemsChanged(BaseListView* list_view) {
  lazy_span_lookup_->Clear();
}

void ListLayoutManagerStaggeredGrid::OnItemsAdded(BaseListView* list_view,
                                                  int position_start,
                                                  int item_count) {
  ListLayoutManager::OnItemsAdded(list_view, position_start, item_count);
  HandleUpdate(position_start, item_count, ListAdapterHelper::Type::kInsert);
}

void ListLayoutManagerStaggeredGrid::OnItemsRemoved(BaseListView* list_view,
                                                    int position_start,
                                                    int item_count) {
  ListLayoutManager::OnItemsRemoved(list_view, position_start, item_count);
  HandleUpdate(position_start, item_count, ListAdapterHelper::Type::kRemove);
}

void ListLayoutManagerStaggeredGrid::OnItemsUpdated(BaseListView* list_view,
                                                    int position_start,
                                                    int item_count) {
  ListLayoutManager::OnItemsUpdated(list_view, position_start, item_count);
  HandleUpdate(position_start, item_count, ListAdapterHelper::Type::kChange);
}

void ListLayoutManagerStaggeredGrid::OnItemsMoved(BaseListView* list_view,
                                                  int from, int to,
                                                  int item_count) {
  ListLayoutManager::OnItemsMoved(list_view, from, to, item_count);
  HandleUpdate(from, to, ListAdapterHelper::Type::kMove);
}

void ListLayoutManagerStaggeredGrid::PrepareLayoutStateForDelta(
    float delta, const ListViewState& state) {
  int reference_child_position;
  ListLayoutDirection layout_dir;
  if (delta > 0.f) {
    layout_dir = ListLayoutDirection::kToEnd;
    reference_child_position = GetLastChildPosition();
  } else {
    layout_dir = ListLayoutDirection::kToStart;
    reference_child_position = GetFirstChildPosition();
  }
  layout_state_.recycle_ = true;
  UpdateLayoutState(reference_child_position, state);
  SetLayoutStateDirection(layout_dir);
  layout_state_.current_position_ =
      reference_child_position +
      static_cast<int>(layout_state_.item_direction_);
  layout_state_.available_ = std::abs(delta);
}

float ListLayoutManagerStaggeredGrid::ScrollBy(float delta,
                                               ListRecycler& recycler,
                                               const ListViewState& state) {
  if (delta == 0.f || GetChildCount() == 0) {
    return 0.f;
  }
  PrepareLayoutStateForDelta(delta, state);
  const float consumed = Fill(recycler, state);
  float available = layout_state_.available_;
  float total_scroll;
  if (available < consumed) {
    total_scroll = delta;
  } else if (delta < 0) {
    total_scroll = -consumed;
  } else {
    total_scroll = consumed;
  }
  LIST_LOG << "Scroll Asked: " << delta << " Scrolled: " << total_scroll;

  orientation_helper_->OffsetChildren(-total_scroll);
  last_layout_from_end_ = should_reverse_layout_;
  layout_state_.available_ = 0.f;
  Recycle(recycler);
  return total_scroll;
}

float ListLayoutManagerStaggeredGrid::Fill(ListRecycler& recycler,
                                           const ListViewState& state) {
  remaining_spans_.assign(span_count_, true);
  // The target position we are trying to reach.
  float target_line;

  if (layout_state_.infinite_) {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToEnd) {
      target_line = std::numeric_limits<float>::max();
    } else {
      target_line = std::numeric_limits<float>::lowest();
    }
  } else {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToEnd) {
      target_line = layout_state_.end_line_ + layout_state_.available_;
    } else {
      target_line = layout_state_.start_line_ - layout_state_.available_;
    }
  }
  int remaining_span_count =
      span_count_ - CalculateNumberOfUnavailableSpans(
                        layout_state_.layout_direction_, target_line);
  LIST_LOG << "---- Filling Start ----\n"
           << "target line: " << target_line << ", direction: "
           << static_cast<int>(layout_state_.layout_direction_)
           << ", remaining_span_count: " << remaining_span_count
           << "\nstate: " << layout_state_.ToString();

  int default_new_view_line = orientation_helper_->GetStartAfterPadding();
  bool added = false;
  bool laying_out_to_end =
      layout_state_.layout_direction_ == ListLayoutDirection::kToEnd;
  while (layout_state_.HasMore(state) &&
         (layout_state_.infinite_ || remaining_span_count != 0)) {
    if (IsItemFullSpan(layout_state_.current_position_) &&
        remaining_span_count < span_count_) {
      break;
    }

    ListItemViewHolder* item = layout_state_.Next(recycler);
    FML_DCHECK(item);

    int position = item->GetPosition();
    int span_index = lazy_span_lookup_->GetSpan(position);
    LayoutParam& param = GetLayoutParam(item);
    param.full_span_ = item->FullSpan();
    LIST_LOG << "start fill one child at pos: " << position
             << ", id: " << item->GetView()->id();

    Span* current_span;
    bool assign_span = span_index == kInvalidSpanID;
    if (assign_span) {
      current_span = param.full_span_ ? spans_.front().get() : GetNextSpan();
      LIST_LOG << " newly assign to span " << current_span->index();

      lazy_span_lookup_->SetSpan(position, current_span->index());
    } else {
      LIST_LOG << "already exist in span " << span_index;
      current_span = spans_[span_index].get();
    }

    param.span_ = current_span;

    if (laying_out_to_end) {
      AddItem(item, GetChildCount());
    } else {
      AddItem(item, 0);
    }

    MeasureConstraint constraint;
    if (param.full_span_) {
      if (orientation_ == ScrollDirection::kVertical) {
        constraint.width_mode = MeasureMode::kDefinite;
        constraint.height_mode = MeasureMode::kIndefinite;
        constraint.width = full_size_spec_;
      } else {
        constraint.height_mode = MeasureMode::kDefinite;
        constraint.width_mode = MeasureMode::kIndefinite;
        constraint.height = full_size_spec_;
      }
    } else {
      if (orientation_ == ScrollDirection::kVertical) {
        constraint.width_mode = MeasureMode::kDefinite;
        constraint.height_mode = MeasureMode::kIndefinite;
        constraint.width = size_per_span_;
      } else {
        constraint.height_mode = MeasureMode::kDefinite;
        constraint.width_mode = MeasureMode::kIndefinite;
        constraint.height = size_per_span_;
      }
    }
    MeasureItem(item, constraint);

    int start, end;
    if (laying_out_to_end) {
      start = param.full_span_
                  ? GetMaxEnd(default_new_view_line)
                  : current_span->GetEndLine(default_new_view_line);
      if (position >= span_count_) {
        // not first row
        start += main_axis_gap_;
      }
      end = start + orientation_helper_->GetDecoratedMeasure(item);
      if (assign_span && param.full_span_) {
        std::unique_ptr<FullSpanItem> full_span_item =
            CreateFullSpanItemFromEnd(start);
        full_span_item->gap_dir_ = ListLayoutDirection::kToStart;
        full_span_item->position_ = position;
        lazy_span_lookup_->AddFullSpanItem(std::move(full_span_item));
      }
    } else {
      end = param.full_span_
                ? GetMinStart(default_new_view_line)
                : current_span->GetStartLine(default_new_view_line);
      // when fill from end, always consider gaps
      end -= main_axis_gap_;
      start = end - orientation_helper_->GetDecoratedMeasure(item);

      if (assign_span && param.full_span_) {
        std::unique_ptr<FullSpanItem> full_span_item =
            CreateFullSpanItemFromStart(end);

        full_span_item->gap_dir_ = ListLayoutDirection::kToEnd;
        full_span_item->position_ = position;
        lazy_span_lookup_->AddFullSpanItem(std::move(full_span_item));
      }
    }

    // check if this item may create gaps in the future
    if (param.full_span_ &&
        layout_state_.item_direction_ == ListItemDirection::kToHead) {
      if (assign_span) {
        laid_out_invalid_full_span_ = true;
      } else {
        bool has_invalid_gap;
        if (layout_state_.layout_direction_ == ListLayoutDirection::kToEnd) {
          has_invalid_gap = !CheckAllEndsEqual();
        } else {
          has_invalid_gap = !CheckAllStartsEqual();
        }
        if (has_invalid_gap) {
          FullSpanItem* item = lazy_span_lookup_->GetFullSpanItem(position);
          if (item != nullptr) {
            item->has_unwanted_gap_after_ = true;
          }
          laid_out_invalid_full_span_ = true;
        }
      }
    }

    AttachItemToSpans(item, param, laying_out_to_end);
    int other_start = 0, other_end;
    ALLOW_UNUSED_LOCAL(other_end);

    // full_span has no padding
    if (!param.full_span_) {
      other_start = orientation_helper_->GetSecondaryStartAfterPadding();
      other_start += current_span->index() * (size_per_span_ + cross_axis_gap_);
    }
    other_end =
        other_start + orientation_helper_->GetSecondaryDecoratedMeasure(item);

    FloatPoint layout_position;
    if (orientation_ == ScrollDirection::kVertical) {
      layout_position = FloatPoint(other_start, start);
    } else {
      layout_position = FloatPoint(start, other_start);
    }

    LIST_LOG << "layout item to " << layout_position.x() << "\t"
             << layout_position.y();
    LayoutItem(item, layout_position);
    if (param.full_span_) {
      remaining_span_count -= CalculateNumberOfUnavailableSpans(
          layout_state_.layout_direction_, target_line);
    } else {
      remaining_span_count -= UpdateRemainingSpans(
          current_span, layout_state_.layout_direction_, target_line);
    }
    Recycle(recycler);
    added = true;
  }

  LIST_LOG << "---- Filling End ----";

  if (!added) {
    Recycle(recycler);
  }
  float diff;
  if (!laying_out_to_end) {
    int min_start = GetMinStart(orientation_helper_->GetStartAfterPadding());
    diff = orientation_helper_->GetStartAfterPadding() - min_start;
  } else {
    int max_end = GetMaxEnd(orientation_helper_->GetEndAfterPadding());
    diff = max_end - orientation_helper_->GetEndAfterPadding();
  }

  float consumed = diff > 0 ? std::min(layout_state_.available_, diff) : 0;

  LIST_LOG << "Filling with " << consumed << "\tdiff " << diff << "\nminstart: "
           << GetMinStart(orientation_helper_->GetStartAfterPadding())
           << " start " << orientation_helper_->GetStartAfterPadding()
           << "\nmaxend: "
           << GetMaxEnd(orientation_helper_->GetEndAfterPadding()) << " end "
           << orientation_helper_->GetEndAfterPadding();
  return consumed;
}

void ListLayoutManagerStaggeredGrid::FixLayoutStartGap(
    ListRecycler& recycler, const ListViewState& state,
    bool can_offset_children) {
  static float kMaxMinLine = std::numeric_limits<float>::max();
  float min_start_line = GetMinStart(kMaxMinLine);
  if (min_start_line == kMaxMinLine) {
    return;
  }
  float gap = min_start_line - orientation_helper_->GetStartAfterPadding();
  if (gap <= 0.f) {
    return;
  }

  gap -= ScrollBy(gap, recycler, state);
  if (can_offset_children && gap > 0.f) {
    orientation_helper_->OffsetChildren(gap);
  }
}

void ListLayoutManagerStaggeredGrid::FixLayoutEndGap(ListRecycler& recycler,
                                                     const ListViewState& state,
                                                     bool can_offset_children) {
  float max_end_line = GetMaxEnd(kInvalidLine);
  if (max_end_line == kInvalidLine) {
    return;
  }
  float gap = orientation_helper_->GetEndAfterPadding() - max_end_line;
  if (gap <= 0.f) {
    return;
  }

  gap += ScrollBy(-gap, recycler, state);
  if (can_offset_children && gap > 0.f) {
    orientation_helper_->OffsetChildren(gap);
  }
}

bool ListLayoutManagerStaggeredGrid::CheckForGaps() {
  if (GetChildCount() == 0) {
    return false;
  }
  int min_pos = GetFirstChildPosition();
  int max_pos = GetLastChildPosition();

  if (min_pos == 0) {
    ListItemViewHolder* item = HasGapsToFix();
    if (item != nullptr) {
      lazy_span_lookup_->Clear();
      LIST_LOG << "Has gap relayout";
      RequestLayout();
      return true;
    }
  }

  if (!laid_out_invalid_full_span_) {
    return false;
  }
  ListLayoutDirection invalid_gap_dir = ListLayoutDirection::kToEnd;
  FullSpanItem* invalid_item = lazy_span_lookup_->GetFirstFullSpanItemInRange(
      min_pos, max_pos + 1, invalid_gap_dir, true);
  if (invalid_item == nullptr) {
    laid_out_invalid_full_span_ = false;
    lazy_span_lookup_->ForceInvalidateAfter(max_pos + 1);
    return false;
  }
  FullSpanItem* valid_item = lazy_span_lookup_->GetFirstFullSpanItemInRange(
      min_pos, invalid_item->position_,
      static_cast<ListLayoutDirection>(static_cast<int>(invalid_gap_dir) * -1),
      true);
  if (valid_item == nullptr) {
    lazy_span_lookup_->ForceInvalidateAfter(invalid_item->position_);
  } else {
    lazy_span_lookup_->ForceInvalidateAfter(valid_item->position_ + 1);
  }
  LIST_LOG << "Has gap relayout";
  RequestLayout();
  return true;
}

ListItemViewHolder* ListLayoutManagerStaggeredGrid::HasGapsToFix() {
  int child_count = GetChildCount();
  remaining_spans_.assign(span_count_, true);

  bool preferred_span_dir = orientation_ == ScrollDirection::kVertical;
  int idx = -1;
  ListItemViewHolder* ret = nullptr;
  children_helper_->ForEach([this, &idx, &ret, child_count, preferred_span_dir](
                                ListItemViewHolder* item) -> bool {
    LayoutParam& param = GetLayoutParam(item);
    ++idx;
    if (remaining_spans_[param.span_->index()]) {
      remaining_spans_[param.span_->index()] = false;
    }
    if (param.full_span_) {
      return false;
    }

    if (idx + 1 != child_count) {
      ListItemViewHolder* next_item = GetChildAt(idx + 1);
      bool compare_spans = false;
      int cur_start = orientation_helper_->GetDecoratedStart(item);
      int next_start = orientation_helper_->GetDecoratedStart(next_item);
      if (cur_start > next_start) {
        ret = item;
        return true;
      } else if (cur_start == next_start) {
        compare_spans = true;
      }

      if (compare_spans) {
        LayoutParam& next_param = GetLayoutParam(next_item);
        if (param.span_->index() >= next_param.span_->index() ==
            preferred_span_dir) {
          ret = item;
          return true;
        }
      }
    }
    return false;
  });
  return ret;
}

std::unique_ptr<FullSpanItem>
ListLayoutManagerStaggeredGrid::CreateFullSpanItemFromEnd(int new_item_top) {
  std::unique_ptr<FullSpanItem> item = std::make_unique<FullSpanItem>();
  item->gap_per_span_.resize(span_count_);
  for (int i = 0; i < span_count_; ++i) {
    item->gap_per_span_[i] = new_item_top - spans_[i]->GetEndLine(new_item_top);
  }
  return item;
}

std::unique_ptr<FullSpanItem>
ListLayoutManagerStaggeredGrid::CreateFullSpanItemFromStart(
    int new_item_bottom) {
  std::unique_ptr<FullSpanItem> item = std::make_unique<FullSpanItem>();
  item->gap_per_span_.resize(span_count_);
  for (int i = 0; i < span_count_; ++i) {
    item->gap_per_span_[i] =
        spans_[i]->GetStartLine(new_item_bottom) - new_item_bottom;
  }
  return item;
}

void ListLayoutManagerStaggeredGrid::UpdateAnchorInfo(
    const ListViewState& state) {
  if (UpdateAnchorInfoFromPendingData(state)) {
    return;
  }
  UpdateAnchorInfoFromChildren(state);
}

bool ListLayoutManagerStaggeredGrid::UpdateAnchorInfoFromPendingData(
    const ListViewState& state) {
  if (pending_scroll_position_ == ListItemViewHolder::kNoPosition) {
    return false;
  }

  if (pending_scroll_position_ < 0 ||
      pending_scroll_position_ >= static_cast<int>(state.item_count)) {
    pending_scroll_position_ = ListItemViewHolder::kNoPosition;
    return false;
  }

  // Here we just ensure the target item is visible. The next operations are
  // handled in |ListScroller::ScrollImmediately|
  ListItemViewHolder* item =
      children_helper_->FindChildByPosition(pending_scroll_position_);
  if (item != nullptr) {  // item is visible
    pending_scroll_position_ = ListItemViewHolder::kNoPosition;
    return false;
  } else {
    anchor_info_.position_ = pending_scroll_position_;
    ListLayoutDirection direction =
        CalculateScrollDirectionForPosition(anchor_info_.position_);
    anchor_info_.layout_from_end_ = ListLayoutDirection::kToEnd == direction;
    switch (pending_scroll_align_to_) {
      case AlignTo::kNone:
      case AlignTo::kMiddle:
        anchor_info_.offset_ =
            anchor_info_.layout_from_end_ ? orientation_helper_->GetEnd() : 0;
        break;
      case AlignTo::kStart:
        anchor_info_.offset_ = 0;
        break;
      case AlignTo::kEnd:
        anchor_info_.offset_ = orientation_helper_->GetEnd();
        break;
      default:
        break;
    }
    anchor_info_.invalidate_offsets_ = true;
  }
  return true;
}

bool ListLayoutManagerStaggeredGrid::UpdateAnchorInfoFromChildren(
    const ListViewState& state) {
  anchor_info_.position_ =
      FindReferenceChildPosition(state.item_count, last_layout_from_end_);
  anchor_info_.offset_ = AnchorInfo::kInvalidOffset;
  return true;
}

ListLayoutDirection
ListLayoutManagerStaggeredGrid::CalculateScrollDirectionForPosition(
    int position) {
  if (GetChildCount() == 0) {
    return ListLayoutDirection::kToEnd;
  }
  float first_child_pos = GetFirstChildPosition();
  if (position < first_child_pos) {
    return ListLayoutDirection::kToStart;
  } else {
    return ListLayoutDirection::kToEnd;
  }
}

void ListLayoutManagerStaggeredGrid::UpdateMeasureSpecs(float total_space) {
  size_per_span_ = total_space / span_count_;
  full_size_spec_ = total_space;
}

// attach item to specified span or all spans, invalidate cached_start_ and
// cached_end_
void ListLayoutManagerStaggeredGrid::AttachItemToSpans(ListItemViewHolder* item,
                                                       LayoutParam& param,
                                                       bool laying_out_to_end) {
  if (laying_out_to_end) {
    if (param.full_span_) {
      AppendItemToAllSpans(item);
    } else {
      param.span_->AppendToSpan(item);
    }
  } else {
    if (param.full_span_) {
      PrependItemToAllSpans(item);
    } else {
      param.span_->PrependToSpan(item);
    }
  }
}

void ListLayoutManagerStaggeredGrid::AppendItemToAllSpans(
    ListItemViewHolder* item) {
  // traverse in reverse so that we end up assigning full span items to 0
  for (int i = span_count_ - 1; i >= 0; --i) {
    spans_[i]->AppendToSpan(item);
  }
}

void ListLayoutManagerStaggeredGrid::PrependItemToAllSpans(
    ListItemViewHolder* item) {
  // traverse in reverse so that we end up assigning full span items to 0
  for (int i = span_count_ - 1; i >= 0; --i) {
    spans_[i]->PrependToSpan(item);
  }
}

void ListLayoutManagerStaggeredGrid::HandleUpdate(int position_start,
                                                  int item_count_or_to_position,
                                                  ListAdapterHelper::Type cmd) {
  // Note: currently every update will trigger layout on list, so we don't check
  // whether should relayout in here
  int affected_range_start, affected_range_end;
  ALLOW_UNUSED_LOCAL(affected_range_end);
  if (cmd == ListAdapterHelper::Type::kMove) {
    if (position_start < item_count_or_to_position) {
      affected_range_end = item_count_or_to_position + 1;
      affected_range_start = position_start;
    } else {
      affected_range_end = position_start + 1;
      affected_range_start = item_count_or_to_position;
    }
  } else {
    affected_range_start = position_start;
    affected_range_end = position_start + item_count_or_to_position;
  }

  lazy_span_lookup_->InvalidateAfter(affected_range_start);
  switch (cmd) {
    case ListAdapterHelper::Type::kInsert:
      lazy_span_lookup_->OffsetForAddition(position_start,
                                           item_count_or_to_position);
      break;
    case ListAdapterHelper::Type::kRemove:
      lazy_span_lookup_->OffsetForRemoval(position_start,
                                          item_count_or_to_position);
      break;
    case ListAdapterHelper::Type::kMove:
      lazy_span_lookup_->OffsetForRemoval(position_start, 1);
      lazy_span_lookup_->OffsetForAddition(item_count_or_to_position, 1);
      break;
    default:
      break;
  }
}

void ListLayoutManagerStaggeredGrid::Recycle(ListRecycler& recycler) {
  if (!layout_state_.recycle_ || layout_state_.infinite_) {
    return;
  }

  if (layout_state_.available_ == 0.f) {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToStart) {
      RecycleFromEnd(recycler, layout_state_.end_line_);
    } else {
      RecycleFromStart(recycler, layout_state_.start_line_);
    }
  } else {
    if (layout_state_.layout_direction_ == ListLayoutDirection::kToStart) {
      float scrolled =
          layout_state_.start_line_ - GetMaxStart(layout_state_.start_line_);
      float line = layout_state_.end_line_;
      if (scrolled >= 0.f) {
        line -= std::min(scrolled, layout_state_.available_);
      }
      RecycleFromEnd(recycler, line);
    } else {
      float scrolled =
          GetMinEnd(layout_state_.end_line_) - layout_state_.end_line_;
      float line = layout_state_.start_line_;
      if (scrolled >= 0.f) {
        line += std::min(scrolled, layout_state_.available_);
      }
      RecycleFromStart(recycler, line);
    }
  }
}

void ListLayoutManagerStaggeredGrid::RecycleFromStart(ListRecycler& recycler,
                                                      float line) {
  while (GetChildCount() > 0) {
    ListItemViewHolder* item = GetFirstChild();
    if (orientation_helper_->GetDecoratedEnd(item) <= line) {
      const LayoutParam& param = GetLayoutParam(item);
      // Don't recycle the last View in a span not to lose span's start/end
      // lines
      if (param.full_span_) {
        for (const auto& span : spans_) {
          if (span->size() == 1) {
            return;
          }
        }
        for (auto& span : spans_) {
          span->PopStart();
        }
      } else {
        FML_DCHECK(param.span_);
        if (param.span_->size() == 1) {
          return;
        }
        param.span_->PopStart();
      }
      RemoveAndRecycleViewAt(recycler, 0);
    } else {
      return;  // done for iterate
    }
  }
}

void ListLayoutManagerStaggeredGrid::RecycleFromEnd(ListRecycler& recycler,
                                                    float line) {
  while (GetChildCount() > 0) {
    ListItemViewHolder* item = GetLastChild();
    if (orientation_helper_->GetDecoratedStart(item) >= line) {
      const LayoutParam& param = GetLayoutParam(item);
      // Don't recycle the last View in a span not to lose span's start/end
      // lines
      if (param.full_span_) {
        for (const auto& span : spans_) {
          if (span->size() == 1) {
            return;
          }
        }
        for (auto& span : spans_) {
          span->PopEnd();
        }
      } else {
        FML_DCHECK(param.span_);
        if (param.span_->size() == 1) {
          return;
        }
        param.span_->PopEnd();
      }

      RemoveAndRecycleViewAt(recycler, GetChildCount() - 1);
    } else {
      return;  // done for iterate
    }
  }
}

// Return span will being placed the cell
Span* ListLayoutManagerStaggeredGrid::GetNextSpan() {
  bool prefer_last_span =
      (layout_state_.layout_direction_ == ListLayoutDirection::kToStart);
  int start_idx = 0, end_idx = span_count_, diff = 1;
  Span* span = nullptr;
  if (prefer_last_span) {
    start_idx = span_count_ - 1;
    end_idx = -1;
    diff = -1;
  }

  if (layout_state_.layout_direction_ == ListLayoutDirection::kToEnd) {
    float min_line = std::numeric_limits<float>::max();
    float default_line = orientation_helper_->GetStartAfterPadding();
    for (; start_idx != end_idx; start_idx += diff) {
      Span* other = spans_[start_idx].get();
      float other_line = other->GetEndLine(default_line);
      if (other_line < min_line) {
        span = other;
        min_line = other_line;
      }
    }
  } else {
    float max_line = std::numeric_limits<float>::lowest();
    float default_line = orientation_helper_->GetEndAfterPadding();
    for (; start_idx != end_idx; start_idx += diff) {
      Span* other = spans_[start_idx].get();
      float other_line = other->GetStartLine(default_line);
      if (other_line > max_line) {
        span = other;
        max_line = other_line;
      }
    }
  }
  FML_DCHECK(span != nullptr);
  return span;
}

std::vector<int> ListLayoutManagerStaggeredGrid::GetVisibleItemPositions() {
  std::vector<int> visible_positions;
  for (auto& span : spans_) {
    std::vector<int> positions = span->GetVisibleItemPositions();
    visible_positions.insert(visible_positions.end(), positions.begin(),
                             positions.end());
  }
  return visible_positions;
}

bool ListLayoutManagerStaggeredGrid::CheckAllEndsEqual() {
  float end = spans_.front()->GetEndLine(kInvalidLine);
  for (auto& span : spans_) {
    if (span->GetEndLine(kInvalidLine) != end) {
      return false;
    }
  }
  return true;
}

bool ListLayoutManagerStaggeredGrid::CheckAllStartsEqual() {
  float start = spans_.front()->GetStartLine(kInvalidLine);
  for (auto& span : spans_) {
    if (span->GetStartLine(kInvalidLine) != start) {
      return false;
    }
  }
  return true;
}

int ListLayoutManagerStaggeredGrid::FindReferenceChildPosition(int item_count,
                                                               bool reversed) {
  // Different with android, We prefer a visible child.
  int visible_ret = ListItemViewHolder::kNoPosition;
  int invisible_ret = ListItemViewHolder::kNoPosition;
  // If no child be found at all, default position is 0.
  int ret = 0;
  auto finder = [&visible_ret, &invisible_ret,
                 item_count](ListItemViewHolder* item) -> bool {
    int position = item->GetPosition();
    if (position >= 0 && position < item_count) {
      if (item->GetViewVisible()) {
        visible_ret = position;
        return true;
      } else if (invisible_ret == ListItemViewHolder::kNoPosition) {
        invisible_ret = position;
      }
      return false;
    }
    return false;
  };
  if (!reversed) {
    children_helper_->ForEach(finder);
  } else {
    children_helper_->ReversedForEach(finder);
  }
  if (visible_ret != ListItemViewHolder::kNoPosition) {
    ret = visible_ret;
  } else if (invisible_ret != ListItemViewHolder::kNoPosition) {
    ret = invisible_ret;
  }
  return ret;
}

int ListLayoutManagerStaggeredGrid::GetLastChildPosition() {
  FML_DCHECK(GetChildCount() != 0);
  return GetLastChild()->GetPosition();
}

int ListLayoutManagerStaggeredGrid::GetFirstChildPosition() {
  FML_DCHECK(GetChildCount() != 0);
  return GetFirstChild()->GetPosition();
}

// find min start in spans_
float ListLayoutManagerStaggeredGrid::GetMinStart(float default_line) {
  FML_DCHECK(!spans_.empty());
  float min_start = spans_.front()->GetStartLine(default_line);
  for (auto& span : spans_) {
    float span_start = span->GetStartLine(default_line);
    if (span_start < min_start) {
      min_start = span_start;
    }
  }
  return min_start;
}

float ListLayoutManagerStaggeredGrid::GetMaxStart(float default_line) {
  FML_DCHECK(!spans_.empty());
  float max_start = spans_.front()->GetStartLine(default_line);
  for (auto& span : spans_) {
    float span_start = span->GetStartLine(default_line);
    if (span_start > max_start) {
      max_start = span_start;
    }
  }
  return max_start;
}

float ListLayoutManagerStaggeredGrid::GetMinEnd(float default_line) {
  FML_DCHECK(!spans_.empty());
  float min_end = spans_.front()->GetEndLine(default_line);
  for (auto& span : spans_) {
    float span_end = span->GetEndLine(default_line);
    if (span_end < min_end) {
      min_end = span_end;
    }
  }
  return min_end;
}

// find max end in spans_
float ListLayoutManagerStaggeredGrid::GetMaxEnd(float default_line) {
  FML_DCHECK(!spans_.empty());
  float max_end = spans_.front()->GetEndLine(default_line);
  for (auto& span : spans_) {
    float span_end = span->GetEndLine(default_line);
    if (span_end > max_end) {
      max_end = span_end;
    }
  }
  return max_end;
}

int ListLayoutManagerStaggeredGrid::CalculateNumberOfUnavailableSpans(
    ListLayoutDirection layout_dir, float target_line) {
  int updated = 0;
  for (auto& span : spans_) {
    if (span->size() == 0) {
      continue;
    }
    updated += UpdateRemainingSpans(span.get(), layout_dir, target_line);
  }
  return updated;
}

// check whether there is available space in span with layout_dir & target_line,
// and set remaining_spans_
int ListLayoutManagerStaggeredGrid::UpdateRemainingSpans(
    Span* span, ListLayoutDirection layout_dir, float target_line) {
  float deleted_size = span->GetDeletedSize();
  FML_DCHECK(span->size() != 0);
  if (layout_dir == ListLayoutDirection::kToStart) {
    float line = span->GetStartLine();
    if (line + deleted_size <= target_line && remaining_spans_[span->index()]) {
      // set false meaning current span is full
      remaining_spans_[span->index()] = false;
      return 1;
    }
  } else {
    float line = span->GetEndLine();
    if (line - deleted_size >= target_line && remaining_spans_[span->index()]) {
      remaining_spans_[span->index()] = false;
      return 1;
    }
  }
  return 0;
}

void ListLayoutManagerStaggeredGrid::UpdateLayoutState(
    int anchor_position, const ListViewState& state) {
  layout_state_.available_ = 0;
  layout_state_.current_position_ = anchor_position;

  layout_state_.end_line_ = orientation_helper_->GetEnd();
  layout_state_.start_line_ = 0.f;

  layout_state_.recycle_ = true;
  layout_state_.infinite_ = false;
}

ListLayoutManagerStaggeredGrid::LayoutParam&
ListLayoutManagerStaggeredGrid::GetLayoutParam(ListItemViewHolder* item) {
  FML_DCHECK(item);
  int view_id = item->GetView()->id();
  auto iter = layout_params_.find(view_id);

  if (iter == layout_params_.end()) {
    LayoutParam param = {false, nullptr};
    iter = layout_params_.insert({view_id, param}).first;
  }
  return iter->second;
}

void ListLayoutManagerStaggeredGrid::SetLayoutStateDirection(
    ListLayoutDirection direction) {
  layout_state_.layout_direction_ = direction;
  layout_state_.item_direction_ = direction == ListLayoutDirection::kToStart
                                      ? ListItemDirection::kToHead
                                      : ListItemDirection::kToTail;
}

void ListLayoutManagerStaggeredGrid::ResolveShouldLayoutReverse() {
  // TODO(hanhaoshen) : more support to ltr and reversed layout
  should_reverse_layout_ = false;
}

}  // namespace clay
