// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/grid_layout_manager.h"

#include <algorithm>
#include <vector>

#include "core/renderer/ui_component/list/item_holder.h"
#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

GridLayoutManager::GridLayoutManager(ListContainerImpl* list_container_impl)
    : LinearLayoutManager(list_container_impl) {}

void GridLayoutManager::UpdateLayoutStateToFillPreloadBuffer(
    LayoutState& layout_state, int index, float offset,
    list::LayoutDirection layout_direction) {
  ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);
  // calculate the index of the first column in this row
  if (item_holder && item_holder->item_col_index() > 0) {
    layout_state.next_bind_index_ = index - item_holder->item_col_index();
  } else {
    layout_state.next_bind_index_ = index;
  }
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = layout_direction;
}

int GridLayoutManager::GetTargetIndexForPreloadBuffer(
    int start_index, list::LayoutDirection layout_direction) {
  if (!ValidPreloadBufferCount()) {
    return list::kInvalidIndex;
  }
  const int data_count = list_container_->GetDataCount();
  int target_index = list::kInvalidIndex;
  if (layout_direction == list::LayoutDirection::kLayoutToEnd) {
    // Layout to end
    for (int i = start_index;
         i < data_count && i < start_index + preload_buffer_count_; ++i) {
      ItemHolder* item_holder = list_container_->GetItemHolderForIndex(i);
      if (item_holder) {
        if (item_holder->item_full_span()) {
          target_index = std::max(target_index, item_holder->index());
        } else if (item_holder->item_col_index() >= 0) {
          // Note: here target_index may out of data count.
          target_index =
              std::max(target_index, item_holder->index() -
                                         item_holder->item_col_index() +
                                         span_count_ - 1);
        }
      }
    }
    if (target_index == list::kInvalidIndex) {
      return list::kInvalidIndex;
    }
  } else {
    // Layout to start
    target_index = data_count;
    for (int i = start_index; i >= 0 && i > start_index - preload_buffer_count_;
         --i) {
      ItemHolder* item_holder = list_container_->GetItemHolderForIndex(i);
      if (item_holder) {
        if (item_holder->item_full_span()) {
          target_index = std::min(target_index, item_holder->index());
        } else if (item_holder->item_col_index() >= 0) {
          target_index =
              std::min(target_index,
                       item_holder->index() - item_holder->item_col_index());
        }
      }
    }
    if (target_index == data_count) {
      return list::kInvalidIndex;
    }
  }
  // clamp target_index to range
  return std::max(0, std::min(target_index, data_count - 1));
}

void GridLayoutManager::LayoutChunk(LayoutChunkResult& result,
                                    LayoutState& layout_state,
                                    bool preload_section) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GridLayoutManager::LayoutChunk", "index",
              base::FormatString("%d", layout_state.next_bind_index_));
  if (!list_container_ || !list_children_helper_ || !list_orientation_helper_) {
    result.consumed_ = 0.f;
    result.finished_ = true;
    return;
  }
  ItemHolder* current_item_holder =
      list_container_->GetItemHolderForIndex(layout_state.next_bind_index_);
  if (!current_item_holder || current_item_holder->item_col_index() != 0) {
    NLIST_LOGE(
        "GridLayoutManager::LayoutChunk: item holder is nullptr or it's column "
        "index is not 0");
    result.consumed_ = 0.f;
    result.finished_ = true;
    return;
  }
  const int data_count = list_container_->GetDataCount();
  int remainingSpan = span_count_;
  int count = 0;
  std::vector<ItemHolder*> item_holder_vector;
  // should fill a row or column during the layout chunk.
  while (count < span_count_ && remainingSpan > 0) {
    int index = layout_state.next_bind_index_ + count;
    int item_span_size = getSpanSize(index);
    if (item_span_size > span_count_) {
      NLIST_LOGE("GridLayoutManager::LayoutChunk: invalid item span size = "
                 << item_span_size);
      result.consumed_ = 0.f;
      result.finished_ = true;
      return;
    }
    remainingSpan -= item_span_size;
    // item did not fit into this row or column
    if (remainingSpan < 0) {
      break;
    }
    // if the index here exceeds the count, should break the loop.
    if (index >= data_count) {
      break;
    }
    ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);
    if (!item_holder) {
      NLIST_LOGE(
          "GridLayoutManager::LayoutChunk: item holder is nullptr with "
          "index = "
          << index);
    } else {
      list_container_->list_adapter()->BindItemHolder(item_holder, index,
                                                      preload_section);
      if (item_holder->item_full_span()) {
        remainingSpan = 0;
      }
      item_holder_vector.push_back(item_holder);
    }
    count++;
  }
  if (count == 0 || item_holder_vector.empty()) {
    // If the count of this line's items is 0, mark result.finished_ and
    // directly return.
    result.finished_ = true;
    return;
  } else if (count != static_cast<int32_t>(item_holder_vector.size())) {
    NLIST_LOGE(
        "GridLayoutManager::LayoutChunk: fail to get all item holders in this "
        "line, directly return.");
    result.consumed_ = 0.f;
    result.finished_ = true;
    return;
  }
  // Update current index of layout state.
  int start_index_of_next_row = GetStartIndexOfNextRow(
      layout_state.layout_direction_, layout_state.next_bind_index_);
  if (start_index_of_next_row != list::kInvalidIndex) {
    layout_state.next_bind_index_ = start_index_of_next_row;
  } else {
    layout_state.next_bind_index_ +=
        static_cast<int32_t>(layout_state.layout_direction_) * count;
  }
  // Layout item holders in one line.
  // Note: item_holder_vector should be checked not empty when using
  // std::max_element
  ItemHolder* max_item_holder = *std::max_element(
      item_holder_vector.begin(), item_holder_vector.end(),
      [this](ItemHolder* a, ItemHolder* b) {
        return list_orientation_helper_->GetDecoratedMeasurement(a) <
               list_orientation_helper_->GetDecoratedMeasurement(b);
      });
  float max_size =
      list_orientation_helper_->GetDecoratedMeasurement(max_item_holder);
  result.consumed_ = max_size;
  // Calculate item holder's main offset.
  float main_offset = 0.f, cross_offset = 0.f;
  if (list::LayoutDirection::kLayoutToStart == layout_state.layout_direction_) {
    main_offset = layout_state.next_layout_offset_ - max_size +
                  max_item_holder->top_inset();
  } else {
    main_offset = layout_state.next_layout_offset_;
  }

  // Calculate item holder's cross offset.
  for (ItemHolder* item_holder : item_holder_vector) {
    if (item_holder) {
      int item_col_index = item_holder->item_col_index();
      float item_cross_size =
          list_orientation_helper_->GetDecoratedMeasurementInOther(item_holder);
      if (base::FloatsLargerOrEqual(0.f, item_cross_size)) {
        // If get invalid item size in cross axis.
        float list_cross_size =
            list_orientation_helper_->GetMeasurementInOtherWithoutPadding();
        float total_cross_gap_size = (span_count_ - 1) * cross_axis_gap_;
        item_cross_size =
            (list_cross_size - total_cross_gap_size) / span_count_;
      }
      cross_offset = list_orientation_helper_->GetStartAfterPaddingInOther() +
                     item_col_index * (item_cross_size + cross_axis_gap_);
      if (orientation_ == list::Orientation::kVertical) {
        item_holder->UpdateLayoutFromManager(cross_offset, main_offset);
      } else {
        item_holder->UpdateLayoutFromManager(main_offset, cross_offset);
      }
    }
  }
}

int GridLayoutManager::GetStartIndexOfNextRow(list::LayoutDirection direction,
                                              const int start_index) const {
  if (!list_container_) {
    return list::kInvalidIndex;
  }
  int index = list::kInvalidIndex;
  ItemHolder* item_holder = nullptr;
  if (direction == list::LayoutDirection::kLayoutToEnd) {
    const int data_count = list_container_->GetDataCount();
    for (int i = start_index + 1; i < data_count; ++i) {
      item_holder = list_container_->GetItemHolderForIndex(i);
      if (!item_holder) {
        NLIST_LOGE("GridLayoutManager::GetStartIndexOfNextRow "
                   << "null item holder");
        return list::kInvalidIndex;
      } else if (item_holder->item_full_span() ||
                 item_holder->item_col_index() == 0) {
        // If iterate to the first item holder in the next line, break directly.
        index = i;
        break;
      }
    }
  } else {
    for (int i = start_index - 1; i >= 0; --i) {
      item_holder = list_container_->GetItemHolderForIndex(i);
      if (!item_holder) {
        NLIST_LOGE("GridLayoutManager::GetStartIndexOfNextRow "
                   << "null item holder");
        return list::kInvalidIndex;
      } else if (item_holder->item_full_span() ||
                 item_holder->item_col_index() == 0) {
        // If iterate to the first item holder in the prev line, break directly.
        index = i;
        break;
      }
    }
  }
  return index;
}

void GridLayoutManager::UpdateLayoutStateToFillStart(
    LayoutState& layout_state,
    const ListAnchorManager::AnchorInfo& anchor_info) {
  // anchor_info.coordinate_ is the top value of anchor item holder, which is
  // including main axis gap. For example, item_holder's height == 100 and
  // main_axis_gap == 10, the top value of item_holder_1 is 110.
  float top_inset =
      anchor_info.item_holder_ ? anchor_info.item_holder_->top_inset() : 0.f;
  float offset = anchor_info.start_offset_ - top_inset;
  // calculate the index of the first column in the row of anchor.
  int first_col_index = anchor_info.index_;
  ItemHolder* item_holder =
      list_container_->GetItemHolderForIndex(first_col_index);
  if (item_holder && item_holder->item_col_index() > 0) {
    first_col_index -= item_holder->item_col_index();
  }
  // calculate the index of  the first column in the pre row.
  int next_bind_index =
      first_col_index +
      static_cast<int32_t>(list::LayoutDirection::kLayoutToStart);
  item_holder = list_container_->GetItemHolderForIndex(next_bind_index);
  if (item_holder && item_holder->item_col_index() > 0) {
    next_bind_index -= item_holder->item_col_index();
  }
  layout_state.next_bind_index_ = next_bind_index;
  layout_state.available_ = offset - content_offset_ -
                            list_orientation_helper_->GetStartAfterPadding();
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = list::LayoutDirection::kLayoutToStart;
}

void GridLayoutManager::UpdateLayoutStateToFillEnd(
    LayoutState& layout_state,
    const ListAnchorManager::AnchorInfo& anchor_info) {
  int index = anchor_info.index_;
  float offset = anchor_info.start_offset_;
  ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);
  // calculate the index of the first column in this row
  if (item_holder && item_holder->item_col_index() > 0) {
    layout_state.next_bind_index_ = index - item_holder->item_col_index();
  } else {
    layout_state.next_bind_index_ = index;
  }
  layout_state.available_ =
      list_orientation_helper_->GetEndAfterPadding() + content_offset_ - offset;
  layout_state.next_layout_offset_ = offset;
  layout_state.layout_direction_ = list::LayoutDirection::kLayoutToEnd;
}

void GridLayoutManager::LayoutInvalidItemHolder(int first_invalid_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GridLayoutManager::LayoutInvalidItemHolder",
              "first_invalid_index", std::to_string(first_invalid_index));
  if (!list_container_ || !list_children_helper_ || first_invalid_index < 0 ||
      first_invalid_index >= list_container_->GetDataCount()) {
    return;
  }
  // Update span size and column index.
  // Note: we should update column index and span size firstly because the span
  // size is important info in calculate item holder's layout offset.
  list_children_helper_->ForEachChild(
      [this, first_invalid_index](ItemHolder* item_holder) {
        const int index = item_holder->index();
        if (index >= first_invalid_index) {
          if (item_holder->item_full_span()) {
            item_holder->SetItemSpanSize(span_count_);
            item_holder->SetItemColIndex(0);
          } else {
            ItemHolder* prev_item_holder =
                list_container_->GetItemHolderForIndex(index - 1);
            ItemHolder* next_item_holder =
                list_container_->GetItemHolderForIndex(index + 1);
            if (prev_item_holder) {
              if (prev_item_holder->item_full_span() ||
                  prev_item_holder->item_col_index() == span_count_ - 1) {
                // If the last item holder is full span or the last one in it's
                // line, the current item holder's column index is 0.
                item_holder->SetItemColIndex(0);
              } else {
                item_holder->SetItemColIndex(
                    prev_item_holder->item_col_index() + 1);
              }
            } else {
              // The current item holder is the first one.
              item_holder->SetItemColIndex(0);
            }
            if (next_item_holder) {
              if (next_item_holder->item_full_span()) {
                // If the next item holder is full span, the span size of
                // current item holder should be (span_count - col_index)
                item_holder->SetItemSpanSize(span_count_ -
                                             item_holder->item_col_index());
              } else {
                item_holder->SetItemSpanSize(1);
              }
            } else {
              // The current item holder is the last one.
              item_holder->SetItemSpanSize(span_count_ -
                                           item_holder->item_col_index());
            }
          }
        }
        return false;
      });
  list_children_helper_->ForEachChild([this, first_invalid_index](
                                          ItemHolder* item_holder) {
    item_holder->SetOrientation(orientation());
    int index = item_holder->index();
    if (index >= first_invalid_index) {
      float main_axis = 0.f, cross_axis = 0.f;
      ItemHolder* prev_item_holder = nullptr;
      if (index > 0) {
        prev_item_holder = list_container_->GetItemHolderForIndex(index - 1);
      }
      if (prev_item_holder) {
        if (prev_item_holder->item_full_span() ||
            prev_item_holder->item_col_index() == span_count_ - 1 ||
            item_holder->item_full_span()) {
          // previous item_holder already hit the end
          // current item_holder should layout from begin
          main_axis =
              LargestMainSizeInRowWithItemHolder(prev_item_holder) +
              main_axis_gap_ +
              list_orientation_helper_->GetItemHolderMainMargin(item_holder);
          // add main_axis_gap to item_holder
          item_holder->SetTopInset(main_axis_gap_);
          cross_axis =
              list_orientation_helper_->GetStartAfterPaddingInOther() +
              list_orientation_helper_->GetItemHolderCrossMargin(item_holder);
        } else {
          // previous item holder is not the last one in the current row.
          if (orientation() == list::Orientation::kVertical) {
            main_axis = prev_item_holder->top();
            cross_axis =
                list_orientation_helper_->GetDecoratedMeasurementInOther(
                    prev_item_holder) +
                prev_item_holder->left();
          } else {
            main_axis = prev_item_holder->left();
            cross_axis =
                list_orientation_helper_->GetDecoratedMeasurementInOther(
                    prev_item_holder) +
                prev_item_holder->top();
          }
          cross_axis += cross_axis_gap_;
          item_holder->SetTopInset(prev_item_holder->top_inset());
        }
      } else {
        // this is the first item_holder
        main_axis =
            list_orientation_helper_->GetStartAfterPadding() +
            list_orientation_helper_->GetItemHolderMainMargin(item_holder);
        cross_axis =
            list_orientation_helper_->GetStartAfterPaddingInOther() +
            list_orientation_helper_->GetItemHolderCrossMargin(item_holder);
      }
      if (orientation_ == list::Orientation::kVertical) {
        item_holder->UpdateLayoutFromManager(cross_axis, main_axis);
      } else {
        item_holder->UpdateLayoutFromManager(main_axis, cross_axis);
      }
    }
    return false;
  });
}

bool GridLayoutManager::ShouldRecycleItemHolder(ItemHolder* item_holder) {
  if (!item_holder) {
    return false;
  }
  return LargestMainSizeInRowWithItemHolder(item_holder) < content_offset_ ||
         list_orientation_helper_->GetDecoratedStart(item_holder) >
             content_offset_ + list_orientation_helper_->GetMeasurement();
}

int GridLayoutManager::getSpanSize(int index) {
  if (list_container_) {
    // full_span item should fill the whole row, so its size should be equal to
    // the span_count.
    ListAdapter* adapter = list_container_->list_adapter();
    if (adapter && adapter->IsFullSpanAtIndex(index)) {
      return span_count_;
    }
    ItemHolder* item_holder = list_container_->GetItemHolderForIndex(index);
    if (item_holder) {
      return item_holder->item_span_size();
    }
  }
  return 1;
}

float GridLayoutManager::GetTargetContentSize() {
  // Note: content size == padding top + sum of children's height + padding
  // bottom
  if (!list_children_helper_ || !list_orientation_helper_) {
    return 0.f;
  }
  int last_element_index = list_container_->list_adapter()->GetDataCount() - 1;
  return LargestMainSizeInRowWithItemHolder(
             list_container_->GetItemHolderForIndex(last_element_index)) +
         list_orientation_helper_->GetEndPadding();
}

float GridLayoutManager::LargestMainSizeInRowWithItemHolder(
    ItemHolder* item_holder) {
  if (!item_holder || !list_container_ || !list_orientation_helper_) {
    return 0.f;
  }
  if (item_holder->item_full_span()) {
    return list_orientation_helper_->GetDecoratedEnd(item_holder);
  }
  const int data_count = list_container_->GetDataCount();
  int start_index = item_holder->index();
  if (item_holder->item_col_index() > 0) {
    start_index = start_index - item_holder->item_col_index();
  }
  if (start_index < 0 || start_index >= data_count - 1) {
    NLIST_LOGE("GridLayoutManager::LargestMainSizeInRowWithItemHolder "
               << "invalid start index " << start_index);
    return list_orientation_helper_->GetDecoratedEnd(item_holder);
  }
  float largest_main_size = 0.f;
  // Iterate all item holders in the current line.
  for (int i = start_index; i < data_count; ++i) {
    ItemHolder* current_item_holder = list_container_->GetItemHolderForIndex(i);
    if (!current_item_holder) {
      NLIST_LOGE("GridLayoutManager::LargestMainSizeInRowWithItemHolder "
                 << "null item holder");
      break;
    } else if (i != start_index &&
               (current_item_holder->item_full_span() ||
                current_item_holder->item_col_index() == 0)) {
      // If iterate to the first item holder in the next line, break directly.
      break;
    }
    largest_main_size = std::max(
        largest_main_size,
        list_orientation_helper_->GetDecoratedEnd(current_item_holder));
  }
  return largest_main_size;
}

}  // namespace tasm
}  // namespace lynx
