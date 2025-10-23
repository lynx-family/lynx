// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_layout_manager_linear.h"

#include <algorithm>
#include <optional>
#include <string>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/ui/common/macros.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_length_cache.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/list/macros.h"

namespace clay {
namespace {

constexpr float kMaxScrollFactor = 1.f / 3.f;

// Calculate the accumulated length of items from position 0 to `to_position`
// based on the cached length sections.
// Item at `to_position` is not included.
float CalculateAccumulatedLength(const ListItemLengthCache::Sections& sections,
                                 int to_position, float main_axis_gap) {
  float total_length = 0.f;

  int last_position = -1;

  // For item without cached length, we estimate its length with the closest
  // item having valid length. It's possible that items in the first section
  // have no cached length. If that's the case, we continue the iteration
  // without accumulating their lengths and go back when we have the valid
  // length of the next section.
  int unknown_count = 0;
  int known_count = 0;
  for (const auto& section : sections) {
    last_position = section.to_pos;
    const int end_pos = std::min(section.to_pos, to_position);
    if (section.length == ListItemLengthCache::kInvalidLength) {
      unknown_count += end_pos - section.from_pos;
    } else {
      total_length += (end_pos - section.from_pos) * section.length;
      known_count += end_pos - section.from_pos;
    }

    if (section.to_pos >= to_position) {
      break;
    }
  }

  if (to_position > last_position) {
    unknown_count += to_position - last_position;
  }

  // It is possible that the previous loop may end with unknown_count is not
  // resolved. For example, Items at [0, 100) are unknown and the input position
  // is 50. Then we should find the first section with valid length and use it
  // to calculate the unknown count.
  if (unknown_count > 0) {
    if (known_count == 0) {
      return 0;
    }
    float avg_length = total_length / known_count;
    total_length += unknown_count * avg_length;
  }
  // add gaps between first item and to_position item.
  total_length += to_position * main_axis_gap;
  return total_length;
}

}  // namespace

void ListLayoutManagerLinear::AnchorInfo::Reset() {
  valid_ = false;
  layout_from_end_ = false;
  position_ = ListItemViewHolder::kNoPosition;
  coordinate_ = kInvalidOffset;
}

void ListLayoutManagerLinear::AnchorInfo::AssignCoordinateFromPadding(
    const ListOrientationHelper& helper, const LayoutState& state) {
  coordinate_ = layout_from_end_ ? helper.GetEndAfterPadding()
                                 : helper.GetStartAfterPadding();
}

void ListLayoutManagerLinear::AnchorInfo::AssignFromChild(
    ListItemViewHolder* child, const ListOrientationHelper& helper,
    const LayoutState& state) {
  if (state.layout_direction_ == ListLayoutDirection::kToStart) {
    coordinate_ = helper.GetDecoratedEnd(child);
  } else {
    coordinate_ = helper.GetDecoratedStart(child);
  }
  position_ = child->GetLayoutPosition();
}

ListLayoutManagerLinear::ListLayoutManagerLinear(ScrollDirection orientation)
    : ListLayoutManager(orientation) {
  ResetLayoutState();
  anchor_info_.Reset();
}

ListLayoutManagerLinear::~ListLayoutManagerLinear() = default;

void ListLayoutManagerLinear::OnLayoutChildren(ListRecycler& recycler,
                                               const ListViewState& state) {
  LIST_LOG << "------ LayoutChildren Start ------";

  if (pending_scroll_position_ != ListItemViewHolder::kNoPosition &&
      state.item_count == 0) {
    RemoveAndRecycleAllViews(recycler);
    return;
  }

  ResetLayoutState();
  layout_state_.recycle_enabled_ = false;

  if (!anchor_info_.valid_ ||
      pending_scroll_position_ != ListItemViewHolder::kNoPosition) {
    anchor_info_.Reset();
    UpdateAnchorInfo(recycler, state);
    anchor_info_.valid_ = true;
  }

  LIST_LOG << " anchor_info: position = " << anchor_info_.position_
           << ", coordinate = " << anchor_info_.coordinate_;

  int extra_for_start = orientation_helper_->GetStartAfterPadding();
  int extra_for_end = orientation_helper_->GetEndPadding();

  ListItemDirection first_layout_direction = anchor_info_.layout_from_end_
                                                 ? ListItemDirection::kToHead
                                                 : ListItemDirection::kToTail;

  bool is_prime_direction =
      first_layout_direction == ListItemDirection::kToTail;

  OnAnchorReady(recycler, state, anchor_info_, is_prime_direction);
  RemoveAndScrapChildren(recycler);
  float start_offset = 0.f, end_offset = 0.f;

  if (anchor_info_.layout_from_end_) {
    // fill towards start
    UpdateLayoutStateToFillStart(anchor_info_.position_,
                                 anchor_info_.coordinate_);
    layout_state_.extra_ = extra_for_start;
    Fill(recycler, state);
    start_offset = layout_state_.offset_;
    if (layout_state_.available_ > 0) {
      extra_for_end += layout_state_.available_;
    }

    // fill towards end
    UpdateLayoutStateToFillEnd(anchor_info_.position_,
                               anchor_info_.coordinate_);
    layout_state_.extra_ = extra_for_end;
    layout_state_.current_position_ +=
        static_cast<int>(layout_state_.item_direction_);
    Fill(recycler, state);
    end_offset = layout_state_.offset_;
  } else {
    // fill towards end
    UpdateLayoutStateToFillEnd(anchor_info_.position_,
                               anchor_info_.coordinate_);
    layout_state_.extra_ = extra_for_end;
    LIST_LOG << "Layout to the end.";
    Fill(recycler, state);
    end_offset = layout_state_.offset_;
    if (layout_state_.available_ > 0) {
      extra_for_start += layout_state_.available_;
    }

    // fill towards start
    UpdateLayoutStateToFillStart(anchor_info_.position_,
                                 anchor_info_.coordinate_);
    layout_state_.extra_ = extra_for_start;
    layout_state_.current_position_ +=
        static_cast<int>(layout_state_.item_direction_);
    LIST_LOG << "Layout to the start.";
    Fill(recycler, state);
    start_offset = layout_state_.offset_;
  }

  // changes may cause gaps on the UI, try to fix them.
  if (GetChildCount() > 0) {
    // because layout from end may be changed by scroll to position we
    // re-calculate it. find which side we should check for gaps.
    float fix_offset = FixLayoutStartGap(start_offset, recycler, state, true);
    start_offset += fix_offset;
    end_offset += fix_offset;
    fix_offset = FixLayoutEndGap(end_offset, recycler, state, false);
    start_offset += fix_offset;
    end_offset += fix_offset;
  }

  orientation_helper_->OnLayoutCompleted();
  LIST_LOG << "------ LayoutChildren End ------";
}

void ListLayoutManagerLinear::OnLayoutCompleted(ListRecycler& recycler,
                                                const ListViewState& state) {
  ListLayoutManager::OnLayoutCompleted(recycler, state);
  anchor_info_.Reset();
}

bool ListLayoutManagerLinear::LayoutState::HasMore(const ListViewState& state) {
  return current_position_ >= 0 &&
         static_cast<size_t>(current_position_) < state.item_count;
}

ListItemViewHolder* ListLayoutManagerLinear::LayoutState::Next(
    ListRecycler& recycler) {
  ListItemViewHolder* item = recycler.GetItemForPosition(current_position_);
  if (item == nullptr) {
    LIST_LOG << "GetItemForPosition:" << current_position_ << " failed";
  }
  current_position_ += static_cast<int>(item_direction_);
  return item;
}

std::string ListLayoutManagerLinear::LayoutState::ToString() const {
  std::string res;
#if (DEBUG_LIST)
  res += "infinite: ";
  res += infinite_ ? "true" : "false";
  res += "\n";

  res += "available: ";
  res += std::to_string(available_);
  res += "\n";

  res += "offset: ";
  res += std::to_string(offset_);
  res += "\n";

  res += "current_position: ";
  res += std::to_string(current_position_);
  res += "\n";

  res += "scrolling_offset: ";
  res += std::to_string(scrolling_offset_.value_or(0.f));
  res += "\n";

  res += "item_direction: ";
  res += item_direction_ == ListItemDirection::kToHead ? "Head" : "Tail";
  res += "\n";

  res += "layout_direction: ";
  res += layout_direction_ == ListLayoutDirection::kToStart ? "Start" : "End";
  res += "\n";

  res += "extra: ";
  res += std::to_string(extra_);
  res += "\n";
#endif
  return res;
}

void ListLayoutManagerLinear::ResetLayoutState() {
  layout_state_.infinite_ = false;
  layout_state_.recycle_enabled_ = true;
  layout_state_.available_ = 0.f;
  layout_state_.offset_ = 0.f;
  layout_state_.current_position_ = 0;
  layout_state_.item_direction_ = ListItemDirection::kToTail;
  layout_state_.layout_direction_ = ListLayoutDirection::kToEnd;
  layout_state_.scrolling_offset_.reset();
  layout_state_.last_scroll_delta_ = 0.f;
}

void ListLayoutManagerLinear::UpdateLayoutState(
    ListLayoutDirection layout_direction, float required_space,
    bool can_use_existing_space, const ListViewState& state) {
  FML_DCHECK(GetChildCount());

  layout_state_.layout_direction_ = layout_direction;
  float scrolling_offset = 0.f;
  if (layout_direction == ListLayoutDirection::kToEnd) {
    layout_state_.extra_ = orientation_helper_->GetEndPadding();
    const ListItemViewHolder* child = GetLastChild();
    layout_state_.item_direction_ = ListItemDirection::kToTail;
    layout_state_.current_position_ =
        child->GetLayoutPosition() +
        static_cast<int>(layout_state_.item_direction_);
    layout_state_.offset_ = orientation_helper_->GetDecoratedEnd(child);
    // Calculate how much we can scroll without adding new children (independent
    // of layout)
    int gap =
        (child->GetLayoutPosition() != static_cast<int>(state.item_count - 1))
            ? main_axis_gap_
            : 0;
    scrolling_offset = orientation_helper_->GetDecoratedEnd(child) + gap -
                       orientation_helper_->GetEndAfterPadding();
  } else {
    const ListItemViewHolder* child = GetFirstChild();
    layout_state_.extra_ = orientation_helper_->GetStartAfterPadding();
    layout_state_.item_direction_ = ListItemDirection::kToHead;
    layout_state_.current_position_ =
        child->GetLayoutPosition() +
        static_cast<int>(layout_state_.item_direction_);
    layout_state_.offset_ = orientation_helper_->GetDecoratedStart(child);
    int gap = child->GetLayoutPosition() == 0 ? 0 : main_axis_gap_;
    scrolling_offset = -orientation_helper_->GetDecoratedStart(child) + gap +
                       orientation_helper_->GetStartAfterPadding();
  }
  layout_state_.available_ = required_space;
  if (can_use_existing_space) {
    layout_state_.available_ -= scrolling_offset;
  }
  layout_state_.scrolling_offset_ = scrolling_offset;
}

void ListLayoutManagerLinear::UpdateLayoutStateToFillStart(int item_pos,
                                                           float offset) {
  layout_state_.available_ =
      offset - orientation_helper_->GetStartAfterPadding();
  layout_state_.current_position_ = item_pos;
  layout_state_.item_direction_ = ListItemDirection::kToHead;
  layout_state_.layout_direction_ = ListLayoutDirection::kToStart;
  layout_state_.offset_ = offset;
  layout_state_.scrolling_offset_.reset();
}

void ListLayoutManagerLinear::UpdateLayoutStateToFillEnd(int item_pos,
                                                         float offset) {
  layout_state_.available_ = orientation_helper_->GetEndAfterPadding() - offset;
  layout_state_.current_position_ = item_pos;
  layout_state_.item_direction_ = ListItemDirection::kToTail;
  layout_state_.layout_direction_ = ListLayoutDirection::kToEnd;
  layout_state_.offset_ = offset;
  layout_state_.scrolling_offset_.reset();
}

void ListLayoutManagerLinear::UpdateAnchorInfo(ListRecycler& recycler,
                                               const ListViewState& state) {
  if (UpdateAnchorInfoFromPendingData(state)) {
    LIST_LOG << "Updated anchor info from pending data.";
    return;
  }
  if (UpdateAnchorInfoFromChildren(recycler, state)) {
    LIST_LOG << "Updated anchor info from children";
    return;
  }
  anchor_info_.AssignCoordinateFromPadding(*orientation_helper_, layout_state_);
  anchor_info_.position_ = 0;
}

bool ListLayoutManagerLinear::UpdateAnchorInfoFromPendingData(
    const ListViewState& state) {
  if (pending_scroll_position_ == ListItemViewHolder::kNoPosition) {
    return false;
  }
  // validate scroll position
  if (pending_scroll_position_ < 0 ||
      pending_scroll_position_ >= static_cast<int>(state.item_count)) {
    LIST_LOG << "Pending scroll position invalid: " << pending_scroll_position_;
    pending_scroll_position_ = ListItemViewHolder::kNoPosition;
    return false;
  }

  // Here we just ensure the target item is visible. The next operations are
  // handled in |ListScroller::ScrollImmediately|
  ListItemViewHolder* child = FindChildByPosition(pending_scroll_position_);
  if (child != nullptr) {
    pending_scroll_position_ = ListItemViewHolder::kNoPosition;
    return false;
  } else {  // item is not visible.
    anchor_info_.position_ = pending_scroll_position_;
    if (GetChildCount() > 0) {
      // get position of any child, does not matter
      int pos = GetFirstChild()->GetPosition();
      anchor_info_.layout_from_end_ = pending_scroll_position_ >= pos;
    }
    switch (pending_scroll_align_to_) {
      case AlignTo::kNone:
      case AlignTo::kMiddle: {
        anchor_info_.coordinate_ =
            anchor_info_.layout_from_end_ ? orientation_helper_->GetEnd() : 0;
        break;
      }
      case AlignTo::kStart: {
        anchor_info_.coordinate_ = 0;
        anchor_info_.layout_from_end_ = false;
        break;
      }
      case AlignTo::kEnd: {
        anchor_info_.coordinate_ = orientation_helper_->GetEnd();
        anchor_info_.layout_from_end_ = true;
        break;
      }
      default:
        break;
    }
  }
  pending_scroll_position_ = ListItemViewHolder::kNoPosition;
  return true;
}

bool ListLayoutManagerLinear::UpdateAnchorInfoFromChildren(
    ListRecycler& recycler, const ListViewState& state) {
  if (GetChildCount() == 0) {
    return false;
  }
  // TODO(Xietong): anchor should be determined by the focus if any.
  ListItemViewHolder* reference = FindReferenceChild(recycler, state);
  if (reference) {
    anchor_info_.AssignFromChild(reference, *orientation_helper_,
                                 layout_state_);
    return true;
  }
  return false;
}

ListItemViewHolder* ListLayoutManagerLinear::FindReferenceChild(
    ListRecycler& recycler, const ListViewState& state) {
  ListItemViewHolder* invalid_match = nullptr;
  ListItemViewHolder* best_first_find = nullptr;
  ListItemViewHolder* best_second_find = nullptr;
  ListItemViewHolder* in_bound = nullptr;

  children_helper_->ForEach(
      [this, &invalid_match, &best_first_find, &best_second_find, &in_bound,
       item_count = state.item_count,
       bounds_start = orientation_helper_->GetStartAfterPadding(),
       bounds_end = orientation_helper_->GetEndAfterPadding()](
          ListItemViewHolder* child) -> bool {
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

          // Usually if child_start >= bounds_end the child is out
          // of bounds, except if the child is 0 pixels!
          const bool out_of_bounds_before =
              child_end <= bounds_start && child_start < bounds_start;
          const bool out_of_bounds_after =
              child_start >= bounds_end && child_end > bounds_end;
          if (out_of_bounds_before || out_of_bounds_after) {
            // The item is out of bounds.
            // We want to find the items closest to the in bounds items and
            // because we are always going through the items linearly, the 2
            // items we want are the last out of bounds item on the side we
            // start searching on, and the first out of bounds item on the side
            // we are ending on.  The side that we are ending on ultimately
            // takes priority because we want items later in the layout to move
            // forward if no in bounds anchors are found.
            if (layout_state_.layout_direction_ ==
                ListLayoutDirection::kToStart) {
              if (out_of_bounds_after) {
                best_first_find = child;
              } else if (best_second_find == nullptr) {
                best_second_find = child;
              }
            } else {
              if (out_of_bounds_before) {
                best_first_find = child;
              } else if (best_second_find == nullptr) {
                best_second_find = child;
              }
            }
          } else {
            in_bound = child;
            return true;  // Found in bound. Should break the iteration.
          }
        }
        return false;
      });

  if (in_bound) {
    return in_bound;
  } else if (best_second_find) {
    return best_second_find;
  } else if (best_first_find) {
    return best_first_find;
  } else {
    return invalid_match;
  }
}

float ListLayoutManagerLinear::Fill(ListRecycler& recycler,
                                    const ListViewState& state) {
  LIST_LOG << "---- Filling Start ----";
  // max offset we should set is mFastScroll + available
  const float start = layout_state_.available_;
  if (layout_state_.scrolling_offset_) {
    if (layout_state_.available_ < 0) {
      *(layout_state_.scrolling_offset_) += layout_state_.available_;
    }
    RecycleByLayoutState(recycler);
  }

  float remaining = layout_state_.available_ + layout_state_.extra_;
  while ((layout_state_.infinite_ || remaining > 0) &&
         layout_state_.HasMore(state)) {
    LIST_LOG << "Layout State before layout item:\n"
             << layout_state_.ToString();

    const float consumed = LayoutChunk(recycler, state);
    layout_state_.offset_ +=
        consumed * static_cast<int>(layout_state_.layout_direction_);

    layout_state_.available_ -= consumed;
    // we keep a separate remaining space because available_ is important for
    // recycling
    remaining -= consumed;

    if (layout_state_.scrolling_offset_) {
      *(layout_state_.scrolling_offset_) += consumed;
      if (layout_state_.available_ < 0) {
        *(layout_state_.scrolling_offset_) += layout_state_.available_;
      }
      RecycleByLayoutState(recycler);
    }
  }
  LIST_LOG << "---- Filling End ----";

  return start - layout_state_.available_;
}

float ListLayoutManagerLinear::LayoutChunk(ListRecycler& recycler,
                                           const ListViewState& state) {
  LIST_LOG << "------ LayoutChunk Start ------";
  ListItemViewHolder* item = layout_state_.Next(recycler);
  DCHECK_RET1(item, 0);
  LIST_LOG << "Item To layout. view_id:" << item->GetView()->id()
           << " name:" << item->GetView()->GetName()
           << " pos:" << item->GetPosition();

  if (layout_state_.layout_direction_ == ListLayoutDirection::kToEnd) {
    AddItem(item, GetChildCount());
  } else {
    AddItem(item, 0);
  }
  MeasureConstraint constraint;
  if (orientation_ == ScrollDirection::kVertical) {
    constraint.width_mode = MeasureMode::kDefinite;
    constraint.height_mode = MeasureMode::kIndefinite;
    constraint.width = GetWidth();
    if (!item->FullSpan()) {
      *constraint.width -= GetPaddingLeft() + GetPaddingRight();
    }
    *constraint.width = std::max(*constraint.width, 0.f);
  } else {
    constraint.height_mode = MeasureMode::kDefinite;
    constraint.width_mode = MeasureMode::kIndefinite;
    constraint.height = GetHeight();
    if (!item->FullSpan()) {
      *constraint.height -= GetPaddingTop() + GetPaddingBottom();
    }
    *constraint.height = std::max(*constraint.height, 0.f);
  }
  MeasureResult res = MeasureItem(item, constraint);
  LIST_LOG << "Measured an item. res.w:" << res.width
           << " res.h:" << res.height;
  float consumed, left = 0.f, top = 0.f;
  // when layout from start to end, offset is the bottom of the last item
  // when layout from end to start, offset is the top of the item
  float main_axis_gap_layout_to_end =
      item->GetPosition() > 0 ? main_axis_gap_ : 0;
  float gap = layout_state_.layout_direction_ == ListLayoutDirection::kToEnd
                  ? main_axis_gap_layout_to_end
                  : main_axis_gap_;
  if (orientation_ == ScrollDirection::kVertical) {
    consumed = res.height + gap;
    left = 0.f;
    if (!item->FullSpan()) {
      left += GetPaddingLeft();
    }
    top = layout_state_.layout_direction_ == ListLayoutDirection::kToEnd
              ? layout_state_.offset_ + gap
              : layout_state_.offset_ - consumed;
  } else {
    consumed = res.width + gap;
    left = layout_state_.layout_direction_ == ListLayoutDirection::kToEnd
               ? layout_state_.offset_ + gap
               : layout_state_.offset_ - consumed;
    top = 0.f;
    if (!item->FullSpan()) {
      top += GetPaddingTop();
    }
  }
  LayoutItem(item, FloatPoint(left, top));
  LIST_LOG << "Laid out an item to (" << left << "," << top
           << "). Consumed:" << consumed;
  LIST_LOG << "------ LayoutChunk End ------";
  return consumed;
}

float ListLayoutManagerLinear::FixLayoutStartGap(float start_offset,
                                                 ListRecycler& recycler,
                                                 const ListViewState& state,
                                                 bool can_offset_children) {
  float gap = start_offset - orientation_helper_->GetStartAfterPadding();
  float fix_offset = 0.f;
  if (gap > 0.f) {
    // check if we should fix this gap.
    if (orientation_ == ScrollDirection::kVertical) {
      fix_offset = -ScrollVerticallyBy(gap, recycler, state);
    } else {
      fix_offset = -ScrollHorizontallyBy(gap, recycler, state);
    }
  } else {
    return 0.f;  // nothing to fix
  }
  start_offset += fix_offset;
  if (can_offset_children) {
    // re-calculate gap, see if we could fix it
    gap = start_offset - orientation_helper_->GetStartAfterPadding();
    if (gap > 0.f) {
      orientation_helper_->OffsetChildren(-gap);
      return fix_offset - gap;
    }
  }
  return fix_offset;
}

float ListLayoutManagerLinear::FixLayoutEndGap(float end_offset,
                                               ListRecycler& recycler,
                                               const ListViewState& state,
                                               bool can_offset_children) {
  float gap = orientation_helper_->GetEndAfterPadding() - end_offset;
  float fix_offset = 0.f;
  if (gap > 0.f) {
    if (orientation_ == ScrollDirection::kVertical) {
      fix_offset = -ScrollVerticallyBy(-gap, recycler, state);
    } else {
      fix_offset = -ScrollHorizontallyBy(-gap, recycler, state);
    }
  } else {
    return 0.f;  // nothing to fix
  }
  // move offset according to scroll amount
  end_offset += fix_offset;
  if (can_offset_children) {
    // re-calculate gap, see if we could fix it
    gap = orientation_helper_->GetEndAfterPadding() - end_offset;
    if (gap > 0.f) {
      orientation_helper_->OffsetChildren(gap);
      return gap + fix_offset;
    }
  }
  return fix_offset;
}

void ListLayoutManagerLinear::RecycleByLayoutState(ListRecycler& recycler) {
  if (!layout_state_.recycle_enabled_) {
    return;
  }

  if (layout_state_.layout_direction_ == ListLayoutDirection::kToStart) {
    RecycleViewsFromEnd(recycler,
                        layout_state_.scrolling_offset_.value_or(0.f));
  } else {
    RecycleViewsFromStart(recycler,
                          layout_state_.scrolling_offset_.value_or(0.f));
  }
}

void ListLayoutManagerLinear::RecycleViewsFromEnd(ListRecycler& recycler,
                                                  float dt) {
  if (dt <= 0.f) {
    return;
  }
  const float limit = orientation_helper_->GetEnd() - dt;
  const int child_count = GetChildCount();

  LIST_LOG << "---- RecycleFromEnd Start ----";
  LIST_LOG << "Children before recycle:" << children_helper_->ToString();

  for (int i = child_count - 1; i >= 0; i--) {
    ListItemViewHolder* child = GetChildAt(i);
    // Find first child whose top is above `limit`.
    if (orientation_helper_->GetDecoratedStart(child) < limit) {
      LIST_LOG << "Recycle from:" << child_count - 1 << " to:" << i;

      // stop here
      RecycleChildren(recycler, child_count - 1, i);  // (child_count-1, i]
      LIST_LOG << "---- RecycleFromEnd End ----";
      return;
    }
  }
}

void ListLayoutManagerLinear::RecycleViewsFromStart(ListRecycler& recycler,
                                                    float dt) {
  if (dt <= 0.f) {
    return;
  }

  LIST_LOG << "---- RecycleFromStart Start ----";
  LIST_LOG << "Children before recycle:" << children_helper_->ToString();

  const float limit = dt;
  const int child_count = GetChildCount();
  for (int i = 0; i < child_count; i++) {
    ListItemViewHolder* child = GetChildAt(i);
    // Find first child whose bottom is below `limit`.
    if (orientation_helper_->GetDecoratedEnd(child) > limit) {
      LIST_LOG << "Recycle from:0 to:" << i;

      // stop here
      RecycleChildren(recycler, 0, i);  // [0, i)
      LIST_LOG << "---- RecycleFromStart End ----";
      return;
    }
  }
}

void ListLayoutManagerLinear::RecycleChildren(ListRecycler& recycler,
                                              int start_index, int end_index) {
  if (start_index == end_index) {
    return;
  }
  if (end_index > start_index) {
    for (int i = end_index - 1; i >= start_index; i--) {
      RemoveAndRecycleViewAt(recycler, i);
    }
  } else {
    for (int i = start_index; i > end_index; i--) {
      RemoveAndRecycleViewAt(recycler, i);
    }
  }
}

float ListLayoutManagerLinear::ScrollVerticallyBy(float dy,
                                                  ListRecycler& recycler,
                                                  const ListViewState& state) {
  if (orientation_ != ScrollDirection::kVertical) {
    return 0;
  }
  return ScrollBy(dy, recycler, state);
}

float ListLayoutManagerLinear::ScrollHorizontallyBy(
    float dx, ListRecycler& recycler, const ListViewState& state) {
  if (orientation_ != ScrollDirection::kHorizontal) {
    return 0;
  }
  return ScrollBy(dx, recycler, state);
}

float ListLayoutManagerLinear::ScrollBy(float delta, ListRecycler& recycler,
                                        const ListViewState& state) {
  LIST_LOG << "ScrollBy: " << delta;

  if (delta == 0.f || GetChildCount() == 0) {
    return 0.f;
  }
  layout_state_.recycle_enabled_ = true;

  const ListLayoutDirection direction =
      delta > 0.f ? ListLayoutDirection::kToEnd : ListLayoutDirection::kToStart;
  const float absDelta = std::abs(delta);
  UpdateLayoutState(direction, absDelta, true, state);

  LIST_LOG << "\n" << layout_state_.ToString();
  const float consumed =
      layout_state_.scrolling_offset_.value() + Fill(recycler, state);
  if (consumed < 0.f) {
    return 0.f;
  }
  const float scrolled =
      absDelta > consumed ? static_cast<int>(direction) * consumed : delta;
  orientation_helper_->OffsetChildren(-scrolled);

  layout_state_.last_scroll_delta_ = scrolled;

  return scrolled;
}

bool ListLayoutManagerLinear::SetOrientation(ScrollDirection orientation) {
  if (ListLayoutManager::SetOrientation(orientation)) {
    ResetLayoutState();
    RequestLayout();
    return true;
  }
  return false;
}

void ListLayoutManagerLinear::OnFocusSearchFailed(bool to_end,
                                                  ListRecycler& recycler,
                                                  const ListViewState& state) {
  if (GetChildCount() == 0) {
    return;
  }

  ListLayoutDirection layout_dir =
      to_end ? ListLayoutDirection::kToEnd : ListLayoutDirection::kToStart;

  const float max_scroll =
      kMaxScrollFactor * orientation_helper_->GetTotalSpace();
  UpdateLayoutState(layout_dir, max_scroll, false, state);
  layout_state_.scrolling_offset_.reset();
  layout_state_.recycle_enabled_ = false;
  Fill(recycler, state);
}

void ListLayoutManagerLinear::ScrollToPosition(int position, AlignTo align_to) {
  pending_scroll_position_ = position;
  pending_scroll_align_to_ = align_to;
}

FloatSize ListLayoutManagerLinear::ScrollToRect(const FloatRect& rect,
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

std::optional<ListLayoutManagerLinear::PositionAndRelativeRect>
ListLayoutManagerLinear::GetChildPositionByRect(const FloatRect& rect) {
  ListLayoutManagerLinear::PositionAndRelativeRect res;
  bool find_value = false;
  auto* orientation_helper = GetOrientationHelper();
  children_helper_->ForEach(
      [&res, &rect, &find_value, orientation_helper](ListItemViewHolder* item) {
        auto start = orientation_helper->GetDecoratedStart(item);
        auto end = orientation_helper->GetDecoratedEnd(item);
        auto rect_start = orientation_helper->GetRectStart(rect);
        auto rect_end = orientation_helper->GetRectEnd(rect);
        if (start <= rect_start && rect_end <= end) {
          find_value = true;
          res.first = item->GetPosition();
          // Convert rect relative to item.
          res.second =
              FloatRect(rect.x() - item->GetLeft(), rect.y() - item->GetTop(),
                        rect.width(), rect.height());
          return true;
        }
        return false;
      });
  if (find_value) {
    return res;
  } else {
    return std::nullopt;
  }
}

bool ListLayoutManagerLinear::HasSpaceToStart(const ListViewState& state) {
  if (GetChildCount() == 0) {
    return false;
  }
  ListItemViewHolder* first = GetFirstChild();
  if (first->GetPosition() != 0) {
    return true;
  }
  return orientation_helper_->GetDecoratedStart(first);
}

bool ListLayoutManagerLinear::HasSpaceToEnd(const ListViewState& state) {
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

ListItemViewHolder* ListLayoutManagerLinear::GetFirstInBoxChild(
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

ListItemViewHolder* ListLayoutManagerLinear::GetLastInBoxChild(
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

float ListLayoutManagerLinear::GetScrollOffset(const ListViewState& state) {
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
                                            main_axis_gap_);
  offset -= orientation_helper_->GetDecoratedStart(first);
  return offset;
}

float ListLayoutManagerLinear::GetTotalLength(const ListViewState& state) {
  // TODO(Xietong): can have a cache mechanism so that we don't need to
  // accumulate the length every time.
  const ListItemLengthCache::Sections& sections = length_cache_->GetSections();
  return CalculateAccumulatedLength(sections, state.item_count, main_axis_gap_);
}

void ListLayoutManagerLinear::CollectAdjacentPrefetchPositions(
    int delta_x, int delta_y, int max_limit,
    const ListViewState& list_view_state,
    LayoutPrefetchRegistry* layout_prefetch_registry) {
  int delta = (orientation_ == ScrollDirection::kVertical ? delta_y : delta_x);
  if (children_helper_->GetChildCount() == 0 || delta == 0) {
    return;
  }
  ListLayoutDirection direction =
      (delta > 0 ? ListLayoutDirection::kToEnd : ListLayoutDirection::kToStart);
  UpdateLayoutState(direction, std::abs(delta), true, list_view_state);

  CollectPrefetchPositionForLayoutState(list_view_state, std::abs(delta),
                                        max_limit, layout_prefetch_registry);
}

void ListLayoutManagerLinear::CollectPrefetchPositionForLayoutState(
    const ListViewState& list_view_state, int abs_velocity, int max_limit,
    LayoutPrefetchRegistry* layout_prefetch_registry) {
  int position = layout_state_.current_position_;
  int scrolling_offset = layout_state_.scrolling_offset_.value_or(0);
  while (0 <= position &&
         position < static_cast<int>(list_view_state.item_count) &&
         0 < scrolling_offset && max_limit > 0) {
    max_limit--;
    // Only when item will come into viewport in next frame, then prefetch it.
    layout_prefetch_registry->AddPosition(position, scrolling_offset);
    position += static_cast<int>(layout_state_.item_direction_);
  }
}

/*
 *This method is only called when a ListView is nested in another
 *ListView. To be implemented.
 */
void ListLayoutManagerLinear::CollectInitialPrefetchPositions(
    int adapter_items_count, const ListViewState& list_view_state,
    LayoutPrefetchRegistry* layout_prefetch_registry) {}

std::vector<int> ListLayoutManagerLinear::GetVisibleItemPositions() {
  int start_pos = FindOneVisibleChildPosition(false, false);
  int last_pos = FindOneVisibleChildPosition(true, false);
  if (start_pos == ListItemViewHolder::kNoPosition) {
    return {};
  }
  FML_DCHECK(last_pos != ListItemViewHolder::kNoPosition &&
             start_pos <= last_pos);
  std::vector<int> positions;
  positions.reserve((last_pos - start_pos + 1));
  while (start_pos <= last_pos) {
    positions.push_back(start_pos);
    start_pos++;
  }
  return positions;
}

int ListLayoutManagerLinear::FindOneVisibleChildPosition(
    bool reverse, bool complete_visible) {
  float start = 0;
  float end = orientation_helper_->GetEnd();
  int position = ListItemViewHolder::kNoPosition;
  auto item_visiter = [this, start, end, complete_visible,
                       &position](ListItemViewHolder* item) -> bool {
    if (orientation_helper_->GetDecoratedMeasure(item) != 0.f) {
      float child_start = orientation_helper_->GetDecoratedStart(item);
      float child_end = orientation_helper_->GetDecoratedEnd(item);
      // Intersect, partially visible
      if (child_start <= end && child_end >= start) {
        if (complete_visible) {
          if (child_start >= start && child_end <= end) {
            position = item->GetPosition();
            return true;
          }
        } else {
          position = item->GetPosition();
          return true;
        }
      }
      return false;
    } else {
      return false;
    }
  };
  if (reverse) {
    children_helper_->ReversedForEach(item_visiter);
  } else {
    children_helper_->ForEach(item_visiter);
  }
  return position;
}

}  // namespace clay
