// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_

#include <memory>

#include "base/include/float_comparison.h"
#include "core/renderer/ui_component/list/list_anchor_manager.h"
#include "core/renderer/ui_component/list/list_children_helper.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"

namespace lynx {
namespace tasm {

class ListContainerImpl;

// Basic list layout manager, inherited by LinearLayoutManager /
// StaggeredGridLayoutManager.
class ListLayoutManager {
 public:
  ListLayoutManager(ListContainerImpl* list_container_impl);
  virtual ~ListLayoutManager() = default;
  void InitLayoutManager(ListChildrenHelper* list_children_helper,
                         list::Orientation list_orientation) {
    SetListChildrenHelper(list_children_helper);
    SetOrientation(list_orientation);
    SetListAnchorManager(list_children_helper_);
  }
  virtual void PreloadSection() {}
  // Init layout state.
  virtual void InitLayoutState() {}
  // Render and layout child nodes. This function will be invoked within
  // OnListElementUpdated() if list has new diff result or list self-updated. In
  // PART_ON_LAYOUT or MULTI_THREAD, this function will also be invoked within
  // OnComponentFinished().
  virtual void OnLayoutChildren(bool is_component_finished = false,
                                int component_index = -1) = 0;
  virtual void OnBatchLayoutChildren() {}
  void SetOrientation(list::Orientation orientation);
  void SetListAnchorManager(ListChildrenHelper* list_children_helper);
  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y);
  void ScrollToPosition(int index, float offset, int align, bool smooth);
  void ScrollStopped();

  float GetWidth() const;
  float GetHeight() const;
  float GetHeightAfterPadding() const;
  float GetPaddingLeft() const;
  float GetPaddingRight() const;
  float GetPaddingTop() const;
  float GetPaddingBottom() const;
  void SetListLayoutInfoToAllItemHolders();
  bool CanScrollHorizontally() const {
    return orientation_ == list::Orientation::kHorizontal;
  }
  bool CanScrollVertically() const {
    return orientation_ == list::Orientation::kVertical;
  }
  void SetListChildrenHelper(ListChildrenHelper* list_children_helper) {
    list_children_helper_ = list_children_helper;
  }
  void SetMainAxisGap(float main_axis_gap) { main_axis_gap_ = main_axis_gap; }
  void SetCrossAxisGap(float cross_axis_gap) {
    cross_axis_gap_ = cross_axis_gap;
  }

  int GetInitialScrollIndex() const {
    return list_anchor_manager_->initial_scroll_index();
  }

  list::InitialScrollIndexStatus GetInitialScrollIndexStatus() const {
    return list_anchor_manager_->initial_scroll_index_status();
  }

  void SetInitialScrollStatus(list::InitialScrollIndexStatus status) {
    list_anchor_manager_->SetInitialScrollIndexStatus(status);
  }

  void SetInitialScrollIndex(int initial_scroll_index) {
    list_anchor_manager_->SetInitialScrollIndex(initial_scroll_index);
  }
  void SetAnchorPriorityFromBegin(bool anchor_priority_from_begin) {
    list_anchor_manager_->SetAnchorPriorityFromBegin(
        anchor_priority_from_begin);
  }
  void SetAnchorAlignToBottom(bool anchor_align_to_bottom) {
    list_anchor_manager_->SetAnchorAlignToBottom(anchor_align_to_bottom);
  }
  void SetAnchorVisibility(list::AnchorVisibility anchor_visibility) {
    list_anchor_manager_->SetAnchorVisibility(anchor_visibility);
  }
  bool SetPreloadBufferCount(int count) {
    bool count_changed = false;
    if (count < 0) {
      NLIST_LOGE(
          "ListLayoutManager::SetPreloadBufferCount: invalid preload buffer "
          "count = "
          << count);
      count_changed = preload_buffer_count_ != 0;
      preload_buffer_count_ = 0;
    } else {
      count_changed = preload_buffer_count_ != count;
      preload_buffer_count_ = count;
    }
    return count_changed;
  }
  void SetEnablePreloadSection(bool value) { enable_preload_section_ = value; }
  int preload_buffer_count() const { return preload_buffer_count_; }
  void SetSpanCount(int span_count);
  int span_count() const { return span_count_; }
  float content_size() const { return content_size_; }
  list::Orientation orientation() const { return orientation_; }
  float main_axis_gap() const { return main_axis_gap_; }
  float cross_axis_gap() const { return cross_axis_gap_; }
  void UpdateDiffAnchorReference() {
    list_anchor_manager_->UpdateDiffAnchorReference();
  }
  float content_offset() const { return content_offset_; }
  // Only use this in UpdateListLayoutManager.
  void SetContentOffset(float content_offset) {
    content_offset_ = ClampContentOffsetToEdge(content_offset, content_size_);
  }
  bool IsHorizontal() const {
    return orientation_ == list::Orientation::kHorizontal;
  }
  bool ItemHolderVisibleInList(ItemHolder* item_holder);
  void ResetContentOffsetAndContentSize(float content_offset,
                                        float content_size) {
    content_size_ = content_size;
    content_offset_ = content_offset;
  }

 protected:
  // Init AnchorInfo and layout all item_holders.
  void InitLayoutAndAnchor(ListAnchorManager::AnchorInfo& anchor_info,
                           int finishing_binding_index);
  // Handle scrolling events from the platform.
  virtual void ScrollByInternal(float content_offset, float original_offset,
                                bool from_platform) = 0;
  // Layout ItemHolder from specified index to end.
  virtual void LayoutInvalidItemHolder(int first_invalid_index) = 0;
  // Get list's content size.
  virtual float GetTargetContentSize() = 0;
  virtual bool ShouldRecycleItemHolder(ItemHolder* item_holder);
  void RecycleOffScreenItemHolders();
  void FlushContentSizeAndOffsetToPlatform(
      float content_offset_before_adjustment);
  void OnLayoutCompleted();
  void SendLayoutCompleteEvent(float scroll_delta);
  void SendScrollEvents(float scroll_delta, float original_offset,
                        list::EventSource event_source);
  // Sticky methods
  int UpdateStickyItems();
  bool IsItemHolderNotSticky(const ItemHolder* item_holder) const;
  void UpdateStickyItemsAfterLayout(ListAnchorManager::AnchorInfo& anchor_info);
  // Calculated content_offset must be clamped to [0, content_size], or Android
  // may stop at an over-edge index
  float ClampContentOffsetToEdge(float content_offset, float content_size);
  bool ValidPreloadBufferCount() const {
    return preload_buffer_count_ > 0 && !enable_preload_section_;
  }
  void OnPrepareForLayoutChildren();
  void SendAnchorDebugInfo(ListAnchorManager::AnchorInfo& anchor_info);
  void HandleLayoutOrScrollResult(bool is_layout);

 private:
  bool UpdateStickyItemsInternal(int& layout_changed_position,
                                 float sticky_offset, int index);
  void FlushScrollInfoToPlatformIfNeeded();

 public:
  std::unique_ptr<ListOrientationHelper> list_orientation_helper_;

 protected:
  list::Orientation orientation_{list::Orientation::kVertical};
  int span_count_{1};
  float content_size_{0.f};
  float content_offset_{0.f};
  float last_content_offset_{0.f};
  float main_axis_gap_{0.f};
  float cross_axis_gap_{0.f};
  int preload_buffer_count_{0};
  std::unique_ptr<ListAnchorManager> list_anchor_manager_;
  bool is_scroll_to_position_{false};
  ListContainerImpl* list_container_{nullptr};
  ListChildrenHelper* list_children_helper_{nullptr};
  bool enable_preload_section_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_
