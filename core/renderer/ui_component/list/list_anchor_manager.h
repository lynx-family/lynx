// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_ANCHOR_MANAGER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_ANCHOR_MANAGER_H_

#include "core/renderer/ui_component/list/list_adapter.h"
#include "core/renderer/ui_component/list/list_children_helper.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {
class ListLayoutManager;
class ListContainerImpl;

class ListAnchorManager {
 public:
  class AnchorInfo {
   public:
    void AssignCoordinateFromPadding(
        const ListOrientationHelper* list_orientation_helper) {
      start_offset_ = list_orientation_helper
                          ? list_orientation_helper->GetStartAfterPadding()
                          : 0.f;
    }

    void Reset() {
      valid_ = false;
      index_ = list::kInvalidIndex;
      start_offset_ = 0.f;
      start_alignment_delta_ = 0.f;
      item_holder_ = nullptr;
    }

    bool valid_{false};
    int index_{list::kInvalidIndex};
    // The top of anchor item_holder when this anchor_info first generated.
    float start_offset_{0.f};
    // The delta between anchor item_holder's top and content_offset when this
    // anchor_info first generated.
    float start_alignment_delta_{0.f};
    ItemHolder* item_holder_{nullptr};
  };

  class ScrollingInfo {
   public:
    float CalcScrollingOffset(float list_size, float list_content_size,
                              float item_offset, float item_size);
    void Reset() {
      scrolling_target_ = list::kInvalidIndex;
      scrolling_align_ = list::ScrollingInfoAlignment::kTop;
      scrolling_offset_ = 0;
      scrolling_smooth_ = false;
      item_holder_ = nullptr;
    };
    void InvalidatePosition() { scrolling_target_ = list::kInvalidIndex; }
    bool IsValidNonSmoothScrollTarget() const {
      return scrolling_target_ != list::kInvalidIndex && !scrolling_smooth_;
    }

    int scrolling_target_{list::kInvalidIndex};
    list::ScrollingInfoAlignment scrolling_align_{
        list::ScrollingInfoAlignment::kTop};
    float scrolling_offset_{0.f};
    bool scrolling_smooth_{false};
    ItemHolder* item_holder_{nullptr};
  };

 public:
  ListAnchorManager(ListLayoutManager* list_layout_manager);
  void SetListOrientationHelper(
      ListOrientationHelper* list_orientation_helper) {
    list_orientation_helper_ = list_orientation_helper;
  }
  void SetListAdapter(ListAdapter* list_adapter) {
    list_adapter_ = list_adapter;
  }
  void ClearDiffReference() {
    first_valid_item_holder_below_screen_ = nullptr;
    last_valid_item_holder_up_screen_ = nullptr;
  }
  void MarkScrolledInitialScrollIndex();
  void SetAnchorAlignToBottom(bool anchor_align_to_bottom) {
    anchor_align_to_bottom_ = anchor_align_to_bottom;
  }
  void SetAnchorVisibility(list::AnchorVisibility anchor_visibility) {
    anchor_visibility_ = anchor_visibility;
  }
  void SetAnchorPriorityFromBegin(bool anchor_priority_from_begin) {
    anchor_priority_from_begin_ = anchor_priority_from_begin;
  }

  void RetrieveAnchorInfoBeforeLayout(AnchorInfo& anchor_info,
                                      int finishing_binding_index);
  void AdjustAnchorInfoAfterLayout(AnchorInfo& anchor_info);
  void UpdateDiffAnchorReference();

  void SetInitialScrollIndex(int initial_scroll_index) {
    initial_scroll_index_ = initial_scroll_index;
    initial_scroll_index_status_ = list::InitialScrollIndexStatus::kSet;
  }
  void SetListContainer(ListContainerImpl* list_container) {
    list_container_ = list_container;
  }

  int initial_scroll_index() const { return initial_scroll_index_; }

  void SetListChildrenHelper(ListChildrenHelper* children_helper) {
    list_children_helper_ = children_helper;
  }
  bool IsValidInitialScrollIndex();
  void SetInitialScrollIndexStatus(list::InitialScrollIndexStatus status) {
    initial_scroll_index_status_ = status;
  }
  list::InitialScrollIndexStatus initial_scroll_index_status() const {
    return initial_scroll_index_status_;
  }
  void InitScrollToPositionParam(ItemHolder* item_holder, int index,
                                 float offset, int align, bool smooth);
  float CalculateTargetScrollingOffset(ItemHolder* item_holder);
  void InvalidateScrollInfoPosition() { scrolling_info_.InvalidatePosition(); }
  void ResetScrollInfo() { scrolling_info_.Reset(); }
  bool IsValidSmoothScrollInfo() {
    return scrolling_info_.scrolling_target_ != list::kInvalidIndex &&
           scrolling_info_.scrolling_smooth_;
  }
  bool IsValidScrollTarget() {
    return list_adapter_->GetItemHolderForIndex(
        scrolling_info_.scrolling_target_);
  }
  void AdjustContentOffsetWithAnchor(AnchorInfo& anchor_info,
                                     float content_offset);
  void AdjustAnchorAlignment(AnchorInfo& anchor_info);
  const ScrollingInfo& scrolling_info() const { return scrolling_info_; }

 private:
  void FindAnchor(AnchorInfo& anchor_info, bool from_begin,
                  int finishing_binding_index);
  void UpdateAnchorInfoWithoutDiff(AnchorInfo& anchor_info,
                                   int component_index /* = -1 */);
  bool IsItemHolderNotSticky(const ItemHolder* item_holder) const;
  void UpdateAnchorWithItemHolder(AnchorInfo& anchor_info,
                                  ItemHolder& item_holder);

 private:
  int initial_scroll_index_{-1};
  ScrollingInfo scrolling_info_;
  list::InitialScrollIndexStatus initial_scroll_index_status_;
  ListChildrenHelper* list_children_helper_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ItemHolder* first_valid_item_holder_below_screen_{nullptr};
  ItemHolder* last_valid_item_holder_up_screen_{nullptr};
  list::AnchorVisibility anchor_visibility_{
      list::AnchorVisibility::kAnchorVisibilityNoAdjustment};
  ListContainerImpl* list_container_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListOrientationHelper* list_orientation_helper_{nullptr};
  bool anchor_align_to_bottom_{false};
  bool anchor_priority_from_begin_{true};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_ANCHOR_MANAGER_H_
