// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_ITEM_HOLDER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_ITEM_HOLDER_H_

#include <array>
#include <string>

#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {

class Element;
class ListOrientationHelper;

class ItemHolder {
 public:
  ItemHolder(int index, const std::string& item_key);

  void UpdateLayoutFromElement();
  void UpdateLayoutFromElement(Element* element);
  void UpdateLayoutToPlatform(float content_size, float container_size);
  void UpdateLayoutToPlatform(float content_size, float container_size,
                              Element* element);
  void UpdateLayoutFromManager(float left, float top);
  bool IsAtStickyPosition(float content_offset, float list_height,
                          float content_size, float sticky_offset, float start,
                          float end) const;
  bool VisibleInList(ListOrientationHelper* orientation_helper,
                     float content_offset) const;
  float GetBorder(list::FrameDirection frame_direction) const;
  float GetPadding(list::FrameDirection frame_direction) const;
  float GetMargin(list::FrameDirection frame_direction) const;
  void SetIndex(int index) { index_ = index; }
  void SetItemKey(const std::string& item_key) { item_key_ = item_key; }
  void SetItemSpanSize(int item_span_size) { item_span_size_ = item_span_size; }
  void SetItemColIndex(int item_col_index) { item_col_index_ = item_col_index; }
  void SetItemFullSpan(bool item_full_span) {
    item_full_span_ = item_full_span;
  }
  void SetTopInset(float top_inset) { top_inset_ = top_inset; }
  void SetTop(float top) { top_ = top; }
  void SetLeft(float left) { left_ = left; }
  void SetEstimatedSize(float estimated_size) {
    estimated_size_ = estimated_size;
  }
  void SetContainerSize(float container_size) {
    container_size_ = container_size;
  }
  void SetOrientation(list::Orientation orientation) {
    orientation_ = orientation;
  }
  void SetDirection(list::Direction direction) { direction_ = direction; }
  void SetSticky(bool sticky_top, bool sticky_bottom) {
    sticky_top_ = sticky_top;
    sticky_bottom_ = sticky_bottom;
  }

  const std::string& item_key() const { return item_key_; }
  int index() const { return index_; }
  int item_col_index() const { return item_col_index_; }
  int item_span_size() const { return item_span_size_; }
  bool item_full_span() const { return item_full_span_; }
  float top_inset() const { return top_inset_; }
  float left() const { return left_; }
  float top() const { return top_; }
  float height() const;
  float width() const;
  bool sticky() const { return sticky_top_ || sticky_bottom_; }
  bool sticky_top() const { return sticky_top_; }
  bool sticky_bottom() const { return sticky_bottom_; }

  // Note: The comparator of ItemHolder should allow objects with the same
  // index but different addresses to exist and it should meet the
  // requirements that the comparator of std::set should possess, specifically,
  // it needs to determine that there's a strict weak ordering among elements.
  class Compare {
   public:
    bool operator()(const ItemHolder* lhs, const ItemHolder* rhs) const {
      if (lhs && rhs) {
        if (lhs == rhs) {
          return false;
        } else if (lhs->index_ == rhs->index_) {
          return lhs < rhs;
        } else {
          return lhs->index_ < rhs->index_;
        }
      } else if (!lhs) {  // nullptr is less than any non-null pointer
        return rhs != nullptr;
      } else {  // any non-null pointer is not less than nullptr
        return false;
      }
    }
  };

 private:
  float GetRTLLeft(float content_size, float container_size) const;
  float GetSizeInMainAxis() const;
  void SetElement(Element* element) { element_ = element; }
  void SetOperationId(int64_t operation_id) { operation_id_ = operation_id; }
  void MarkDiffStatus(list::DiffStatus status) { diff_status_ = status; }
  void MarkRemoved(bool value) { removed_ = value; }
  void MarkDirty(bool value) { dirty_ = value; }
  void MarkVirtualDomPreloaded(bool value) { virtual_dom_preloaded_ = value; }
  Element* element() const { return element_; }
  bool removed() const { return removed_; }
  bool is_updated() const {
    return diff_status_ == list::DiffStatus::kUpdateTo;
  }
  bool BoundWithElement() { return !dirty_ && element_ && operation_id_ == 0; }
  bool BoundWithoutElement() {
    return !dirty_ && !element_ && operation_id_ == 0;
  }
  bool dirty() const { return dirty_; }
  int64_t operation_id() const { return operation_id_; }
  bool virtual_dom_preloaded() const { return virtual_dom_preloaded_; }

 private:
  friend class DefaultListAdapter;
  // All status Info of item holder.
  // Whether the ItemHolder is need to bind in the next layout. If the
  // ItemHolder is bound, the dirty_ will be set to false.
  bool dirty_{true};
  // Whether the ItemHolder is removed.
  bool removed_{false};
  // kValid / kRemoved / kUpdateTo / kUpdatedFrom / kMoveTo / kMoveFrom
  list::DiffStatus diff_status_{list::DiffStatus::kValid};
  // The id of every binding operation, which is used to determine the binding
  // relationship with ItemHolder and child element.
  int64_t operation_id_{0};
  // The element's ptr bound with ItemHolder. If the ItemHolder is recycled, the
  // element_ will be reset to nullptr;
  Element* element_{nullptr};
  bool virtual_dom_preloaded_{false};

  // All layout Info of item holder.
  // The ItemHolder's index in data source.
  int index_{list::kInvalidIndex};
  // The ItemHold's key.
  std::string item_key_;
  // Whether the ItemHolder is a sticky top item.
  bool sticky_top_{false};
  // Whether the ItemHolder is a sticky bottom item.
  bool sticky_bottom_{false};
  // Whether the ItemHolder is full span item.
  bool item_full_span_{false};
  // The ItemHolder's column index.
  int item_col_index_{0};
  // The number of columns occupied by each ItemHolder. For example, a full-span
  // ItemHolder, the span_size is equal to the list's column count.
  int item_span_size_{1};
  float left_{0.f};
  float top_{0.f};
  float width_{list::kInvalidDimensionSize};
  float height_{list::kInvalidDimensionSize};
  // The ItemHolder's main axis-gap.
  float top_inset_{0.f};
  // The container's size in main axis.
  float container_size_{list::kInvalidDimensionSize};
  // The ItemHolder's estimated size (ppx).
  float estimated_size_{list::kInvalidDimensionSize};
  // The layout's orientation. (kVertical / kHorizontal)
  list::Orientation orientation_{list::Orientation::kVertical};
  // The layout's direction. (kNormal / kRTL)
  list::Direction direction_{list::Direction::kNormal};
  std::array<float, 4> paddings_{};
  std::array<float, 4> borders_{};
  std::array<float, 4> margins_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_ITEM_HOLDER_H_
