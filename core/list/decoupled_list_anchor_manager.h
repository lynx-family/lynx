// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_ANCHOR_MANAGER_H_
#define CORE_LIST_DECOUPLED_LIST_ANCHOR_MANAGER_H_

#include "core/list/decoupled_list_adapter.h"
#include "core/list/decoupled_list_children_helper.h"
#include "core/list/decoupled_list_orientation_helper.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

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
  void ClearDiffReference() {
    first_valid_item_holder_below_screen_ = nullptr;
    last_valid_item_holder_up_screen_ = nullptr;
  }
  void SetAnchorAlignToBottom(bool anchor_align_to_bottom) {
    anchor_align_to_bottom_ = anchor_align_to_bottom;
  }
  void SetAnchorVisibility(list::AnchorVisibility anchor_visibility) {
    anchor_visibility_ = anchor_visibility;
  }
  void SetAnchorPriorityFromBegin(bool anchor_priority_from_begin) {
    anchor_priority_from_begin_ = anchor_priority_from_begin;
  }
  void SetListContainer(ListContainerImpl* list_container) {
    list_container_ = list_container;
  }
  void SetListAdapter(ListAdapter* list_adapter) {
    list_adapter_ = list_adapter;
  }
  void SetListChildrenHelper(ListChildrenHelper* children_helper) {
    list_children_helper_ = children_helper;
  }
  void SetInitialScrollIndex(int initial_scroll_index) {
    initial_scroll_index_ = initial_scroll_index;
  }
  void MarkScrolledInitialScrollIndex();
  void RetrieveAnchorInfoBeforeLayout(AnchorInfo& anchor_info,
                                      int finishing_binding_index);
  void AdjustAnchorInfoAfterLayout(AnchorInfo& anchor_info);
  //  void UpdateDiffAnchorReference();
  bool IsValidInitialScrollIndex() const;
  int initial_scroll_index() const { return initial_scroll_index_; }
  void InitScrollToPositionParam(ItemHolder* item_holder, int index,
                                 float offset, int align, bool smooth);
  float CalculateTargetScrollingOffset(ItemHolder* item_holder);
  void InvalidateScrollInfoPosition() { scrolling_info_.InvalidatePosition(); }
  void ResetScrollInfo() { scrolling_info_.Reset(); }
  bool IsValidSmoothScrollInfo() {
    return scrolling_info_.scrolling_target_ != list::kInvalidIndex &&
           scrolling_info_.scrolling_smooth_;
  }
  void AdjustContentOffsetWithAnchor(AnchorInfo& anchor_info,
                                     float content_offset);
  const ScrollingInfo& scrolling_info() const { return scrolling_info_; }

 private:
  void FindAnchor(AnchorInfo& anchor_info, bool from_begin,
                  int finishing_binding_index);
  //  void UpdateAnchorInfoWithoutDiff(AnchorInfo& anchor_info,
  //                                   int component_index /* = -1 */);
  void UpdateAnchorWithItemHolder(AnchorInfo& anchor_info,
                                  ItemHolder& item_holder);
  void AdjustAnchorAlignment(AnchorInfo& anchor_info);

 private:
  bool anchor_align_to_bottom_{false};
  bool anchor_priority_from_begin_{true};
  list::AnchorVisibility anchor_visibility_{
      list::AnchorVisibility::kAnchorVisibilityNoAdjustment};
  int initial_scroll_index_{-1};
  ItemHolder* first_valid_item_holder_below_screen_{nullptr};
  ItemHolder* last_valid_item_holder_up_screen_{nullptr};
  ListContainerImpl* list_container_{nullptr};
  ListAdapter* list_adapter_{nullptr};
  ListLayoutManager* list_layout_manager_{nullptr};
  ListChildrenHelper* list_children_helper_{nullptr};
  ListOrientationHelper* list_orientation_helper_{nullptr};
  ScrollingInfo scrolling_info_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_ANCHOR_MANAGER_H_
