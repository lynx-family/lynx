// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_anchor_manager.h"

#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/list_layout_manager.h"

namespace lynx {
namespace tasm {

ListAnchorManager::ListAnchorManager(ListLayoutManager* list_layout_manager)
    : list_layout_manager_(list_layout_manager) {}

// Update anchor info when layout children.
void ListAnchorManager::RetrieveAnchorInfoBeforeLayout(
    AnchorInfo& anchor_info, int finishing_binding_index) {
  if (IsValidInitialScrollIndex()) {
    // initial-scroll-index
    //      Use initial-scroll-index as anchor and fill a screen size area
    // Since item_holders' layout not finished, do not set coordinate here.
    ItemHolder* item_holder =
        list_adapter_->GetItemHolderForIndex(initial_scroll_index_);
    if (!item_holder) {
      return;
    }
    anchor_info.valid_ = true;
    anchor_info.index_ = initial_scroll_index_;
    anchor_info.item_holder_ = item_holder;
    anchor_info.start_alignment_delta_ = 0;
  } else if (scrolling_info_.IsValidNonSmoothScrollTarget()) {
    ItemHolder* item_holder =
        list_adapter_->GetItemHolderForIndex(scrolling_info_.scrolling_target_);
    if (!item_holder) {
      return;
    }
    anchor_info.valid_ = true;
    anchor_info.index_ = scrolling_info_.scrolling_target_;
    anchor_info.item_holder_ = item_holder;
    anchor_info.start_alignment_delta_ = 0;
  } else {
    FindAnchor(anchor_info, !anchor_priority_from_begin_,
               finishing_binding_index);
    ClearDiffReference();
  }
}

void ListAnchorManager::AdjustAnchorInfoAfterLayout(AnchorInfo& anchor_info) {
  if (IsValidInitialScrollIndex()) {
    // initial-scroll-index
    //      Use initial-scroll-index as anchor and fill a screen size area
    float start =
        list_orientation_helper_->GetDecoratedStart(anchor_info.item_holder_);
    list_layout_manager_->SetContentOffset(start);
    anchor_info.start_offset_ = start;
  } else if (scrolling_info_.IsValidNonSmoothScrollTarget()) {
    // un-smoothed scroll
    //      Use target index as anchor and fill a screen size area
    float start =
        list_orientation_helper_->GetDecoratedStart(anchor_info.item_holder_);
    list_layout_manager_->SetContentOffset(start);
    anchor_info.start_offset_ = start;
  } else {
    // Note: Need update anchor layout coordinate after invoking
    // LayoutInvalidItemHolder(0) because it is possible to insert or delete
    // some items before the anchor.
    float coordinate =
        list_orientation_helper_->GetDecoratedStart(anchor_info.item_holder_);
    list_layout_manager_->SetContentOffset(
        list_layout_manager_->content_offset() + coordinate -
        anchor_info.start_offset_);
    // Rest anchor's coordinate
    anchor_info.start_offset_ = coordinate;
  }
}

void ListAnchorManager::AdjustContentOffsetWithAnchor(
    ListAnchorManager::AnchorInfo& anchor_info, float content_offset) {
  if (anchor_info.valid_) {
    list_layout_manager_->SetContentOffset(
        list_orientation_helper_->GetStart(anchor_info.item_holder_) -
        anchor_info.start_alignment_delta_);
  } else {
    // Note: due to the update in content_size, here needs invoke
    // SetContentOffset() again to trim the content_offset.
    list_layout_manager_->SetContentOffset(content_offset);
  }
}

void ListAnchorManager::UpdateAnchorWithItemHolder(AnchorInfo& anchor_info,
                                                   ItemHolder& item_holder) {
  anchor_info.index_ = item_holder.index();
  anchor_info.valid_ = true;
  anchor_info.item_holder_ = &item_holder;
  AdjustAnchorAlignment(anchor_info);
}

void ListAnchorManager::FindAnchor(AnchorInfo& anchor_info, bool from_end,
                                   int finishing_binding_index) {
  if (list_container_->GetDataCount() == 0) {
    anchor_info.Reset();
    return;
  }
  // The priority level is:
  // 1. No dirty item_holder
  // 2. No dirty but binding item_holder(multi-thread)
  // 3. The item_holder finishing binding which cause this layout
  // 4. Updated item_holder (Not removed or newly inserted)
  // 5. Closest outside screen item_holder (above or below screen)
  // Find these item_holders can be chosen from.
  ItemHolder* updated_item_holder = nullptr;
  ItemHolder* binding_item_holder = nullptr;
  ItemHolder* finishing_binding_item_holder = nullptr;
  list_children_helper_->ForEachChild(
      list_children_helper_->on_screen_children(),
      [this, &updated_item_holder, &binding_item_holder, &anchor_info,
       finishing_binding_index,
       &finishing_binding_item_holder](ItemHolder* item_holder) {
        // Sticky item_holder can not be choosen as anchor
        if (!IsItemHolderNotSticky(item_holder)) {
          return false;
        }
        int index = item_holder->index();
        if ((list_adapter_->IsFinishedBinding(item_holder) ||
             list_adapter_->IsRecycled(item_holder)) &&
            index != finishing_binding_index) {
          UpdateAnchorWithItemHolder(anchor_info, *item_holder);
          return true;
        } else if (!list_adapter_->IsDirty(item_holder) &&
                   list_adapter_->IsBinding(item_holder) &&
                   !binding_item_holder) {
          binding_item_holder = item_holder;
        } else if (!list_adapter_->IsDirty(item_holder) &&
                   index == finishing_binding_index) {
          // Only when this finishing_binding_item_holder is in
          // on_screen_children can it be anchor.
          finishing_binding_item_holder = item_holder;
        } else if (list_adapter_->IsDirty(item_holder) &&
                   list_adapter_->IsUpdated(item_holder) &&
                   !updated_item_holder) {
          updated_item_holder = item_holder;
        }
        return false;
      },
      from_end);
  // Find Updated binding_item_holder as the second-best anchor.
  if (!anchor_info.valid_ && binding_item_holder) {
    UpdateAnchorWithItemHolder(anchor_info, *binding_item_holder);
    return;
  }
  // Find finishing_binding_item_holder as the third-best anchor.
  if (!anchor_info.valid_ && finishing_binding_item_holder) {
    UpdateAnchorWithItemHolder(anchor_info, *finishing_binding_item_holder);
    return;
  }
  // Find Updated item_holders as the forth-best anchor.
  if (!anchor_info.valid_ && updated_item_holder) {
    UpdateAnchorWithItemHolder(anchor_info, *updated_item_holder);
    return;
  }
  // If all item_holders are invalid, find outside anchor.
  ItemHolder* outside_bounds_item_holder =
      from_end ? first_valid_item_holder_below_screen_
               : last_valid_item_holder_up_screen_;
  if (!anchor_info.valid_) {
    if (outside_bounds_item_holder) {
      anchor_info.valid_ = true;
      anchor_info.index_ = outside_bounds_item_holder->index();
      anchor_info.item_holder_ = outside_bounds_item_holder;
      anchor_info.start_offset_ = list_orientation_helper_->GetDecoratedStart(
          outside_bounds_item_holder);
      anchor_visibility_ = list::AnchorVisibility::kAnchorVisibilityHide;
      AdjustAnchorAlignment(anchor_info);
    } else {
      // no anchor found. layout from list padding top
      anchor_info.valid_ = true;
      anchor_info.index_ = 0;
      anchor_info.item_holder_ = list_adapter_->GetItemHolderForIndex(0);
      anchor_info.AssignCoordinateFromPadding(list_orientation_helper_);
      anchor_info.start_alignment_delta_ = anchor_info.start_offset_;
    }
  }
}

void ListAnchorManager::InitScrollToPositionParam(ItemHolder* item_holder,
                                                  int index, float offset,
                                                  int align, bool smooth) {
  scrolling_info_.scrolling_target_ = index;
  scrolling_info_.scrolling_offset_ = offset;
  scrolling_info_.scrolling_align_ = (list::ScrollingInfoAlignment)align;
  scrolling_info_.scrolling_smooth_ = smooth;
  scrolling_info_.item_holder_ = item_holder;
}

float ListAnchorManager::CalculateTargetScrollingOffset(
    ItemHolder* item_holder) {
  return scrolling_info_.CalcScrollingOffset(
      list_orientation_helper_->GetMeasurement(),
      list_layout_manager_->content_size(),
      list_orientation_helper_->GetStart(item_holder),
      list_orientation_helper_->GetDecoratedMeasurement(item_holder));
}

void ListAnchorManager::AdjustAnchorAlignment(AnchorInfo& anchor_info) {
  switch (anchor_visibility_) {
    case list::AnchorVisibility::kAnchorVisibilityNoAdjustment:
      if (anchor_align_to_bottom_) {
        anchor_info.start_offset_ = list_orientation_helper_->GetDecoratedStart(
            anchor_info.item_holder_);
        anchor_info.start_alignment_delta_ =
            list_orientation_helper_->GetDecoratedEnd(
                anchor_info.item_holder_) -
            list_layout_manager_->content_offset();
      } else {
        anchor_info.start_offset_ = list_orientation_helper_->GetDecoratedStart(
            anchor_info.item_holder_);
        anchor_info.start_alignment_delta_ =
            list_orientation_helper_->GetStart(anchor_info.item_holder_) -
            list_layout_manager_->content_offset();
      }
      break;
    case list::AnchorVisibility::kAnchorVisibilityHide:
      anchor_info.start_offset_ =
          list_orientation_helper_->GetDecoratedStart(anchor_info.item_holder_);
      anchor_info.start_alignment_delta_ =
          -list_orientation_helper_->GetDecoratedMeasurement(
              anchor_info.item_holder_);
      break;
    case list::AnchorVisibility::kAnchorVisibilityShow:
      anchor_info.start_offset_ = 0;
      anchor_info.start_alignment_delta_ = 0;
    default:
      break;
  }
}

void ListAnchorManager::UpdateDiffAnchorReference() {
  ClearDiffReference();
  const ItemHolderSet& on_screen_children =
      list_children_helper_->on_screen_children();
  ItemHolder* first_visible_item_holder = list_children_helper_->GetFirstChild(
      on_screen_children, [this](const ItemHolder* item_holder) {
        return !list_adapter_->IsRemoved(item_holder) &&
               IsItemHolderNotSticky(item_holder);
      });
  ItemHolder* last_visible_item_holder = list_children_helper_->GetLastChild(
      on_screen_children, [this](const ItemHolder* item_holder) {
        return !list_adapter_->IsRemoved(item_holder) &&
               IsItemHolderNotSticky(item_holder);
      });
  if (!first_visible_item_holder || !last_visible_item_holder ||
      !list_children_helper_) {
    return;
  }
  // TODO(fangzhou) provide more options here
  list_children_helper_->ForEachChild([this, first_visible_item_holder,
                                       last_visible_item_holder](
                                          ItemHolder* item_holder) {
    if (!list_adapter_->IsDirty(item_holder)) {
      if (item_holder->index() < first_visible_item_holder->index() &&
          (!last_valid_item_holder_up_screen_ ||
           item_holder->index() > last_valid_item_holder_up_screen_->index())) {
        last_valid_item_holder_up_screen_ = item_holder;
      }
      if (item_holder->index() > last_visible_item_holder->index() &&
          (!first_valid_item_holder_below_screen_ ||
           item_holder->index() <
               first_valid_item_holder_below_screen_->index())) {
        first_valid_item_holder_below_screen_ = item_holder;
      }
    }
    return false;
  });
}

bool ListAnchorManager::IsItemHolderNotSticky(
    const ItemHolder* item_holder) const {
  int sticky_offset = list_container_->sticky_offset();
  bool sticky_enabled = list_container_->sticky_enabled();
  return !sticky_enabled || !item_holder->sticky() ||
         !item_holder->IsAtStickyPosition(
             list_layout_manager_->content_offset(),
             list_layout_manager_->GetHeight(),
             list_layout_manager_->content_size(), sticky_offset,
             list_orientation_helper_->GetDecoratedStart(item_holder),
             list_orientation_helper_->GetDecoratedEnd(item_holder));
}

bool ListAnchorManager::IsValidInitialScrollIndex() {
  return initial_scroll_index_ >= 0 &&
         initial_scroll_index_ < list_adapter_->GetDataCount() &&
         initial_scroll_index_status_ == list::InitialScrollIndexStatus::kSet;
}

void ListAnchorManager::MarkScrolledInitialScrollIndex() {
  if (IsValidInitialScrollIndex()) {
    initial_scroll_index_status_ = list::InitialScrollIndexStatus::kScrolled;
  }
}

// Calculate scroll offset by alignment
float ListAnchorManager::ScrollingInfo::CalcScrollingOffset(
    float list_size, float list_content_size, float item_offset,
    float item_size) {
  float estimated_offset = 0;
  if (scrolling_align_ == list::ScrollingInfoAlignment::kTop) {
    estimated_offset = item_offset - scrolling_offset_;
  } else if (scrolling_align_ == list::ScrollingInfoAlignment::kMiddle) {
    estimated_offset =
        item_offset - (list_size - item_size) / 2.0 - scrolling_offset_;
  } else if (scrolling_align_ == list::ScrollingInfoAlignment::kBottom) {
    estimated_offset = item_offset - list_size + item_size - scrolling_offset_;
  }
  // Clamp estimated_offset to [0.f, max content offset].
  if (base::FloatsLargerOrEqual(0.f, estimated_offset) ||
      base::FloatsLargerOrEqual(list_size, list_content_size)) {
    estimated_offset = 0.f;
  } else if (estimated_offset > list_content_size - list_size) {
    estimated_offset = list_content_size - list_size;
  }
  return estimated_offset;
}

}  // namespace tasm
}  // namespace lynx
