// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
#include "core/list/decoupled_item_holder.h"

#include <algorithm>

#include "base/include/float_comparison.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/list/decoupled_list_orientation_helper.h"

namespace lynx {
namespace list {

ItemHolder::ItemHolder(int index, const std::string& item_key)
    : index_(index), item_key_(item_key) {}

void ItemHolder::UpdateLayoutFromItemDelegate() {
  UpdateLayoutFromItemDelegate(item_delegate_);
}

void ItemHolder::UpdateLayoutFromItemDelegate(
    list::ItemElementDelegate* item_delegate) {
  if (item_delegate) {
    // Update layout info from starlight. Here we don't update left and top's
    // value which are always zero for list's child element.
    width_ = item_delegate->GetWidth();
    height_ = item_delegate->GetHeight();
    borders_ = item_delegate->GetBorders();
    paddings_ = item_delegate->GetPaddings();
    margins_ = item_delegate->GetMargins();
  }
}

void ItemHolder::UpdateLayoutToPlatform(float content_size,
                                        float container_size) {
  UpdateLayoutToPlatform(content_size, container_size, item_delegate_);
}

void ItemHolder::UpdateLayoutToPlatform(
    float content_size, float container_size,
    list::ItemElementDelegate* item_delegate) {
  if (item_delegate_) {
    if (direction_ == list::Direction::kRTL) {
      item_delegate_->UpdateLayoutToPlatform(
          GetRTLLeft(content_size, container_size), top_);
    } else {
      item_delegate_->UpdateLayoutToPlatform(left_, top_);
    }
  }
}

void ItemHolder::UpdateLayoutFromManager(float left, float top) {
  // Update left and top's value from list's layout manager.
  left_ = left;
  top_ = top;
}

float ItemHolder::height() const {
  if (orientation_ == list::Orientation::kHorizontal) {
    return base::FloatsLargerOrEqual(height_, 0.f) ? height_ : 0.f;
  }
  return GetSizeInMainAxis();
}

float ItemHolder::width() const {
  if (orientation_ == list::Orientation::kVertical) {
    return base::FloatsLargerOrEqual(width_, 0.f) ? width_ : 0.f;
  }
  return GetSizeInMainAxis();
}

float ItemHolder::GetSizeInMainAxis() const {
  // If the ItemHolder is never bound, we use estimated size or container size.
  float main_axis_size =
      orientation_ == list::Orientation::kVertical ? height_ : width_;
  return base::FloatsLargerOrEqual(main_axis_size, 0.f)
             ? main_axis_size
             : (base::FloatsLargerOrEqual(estimated_size_, 0.f)
                    ? estimated_size_
                    : (base::FloatsLarger(container_size_, 0.f)
                           ? container_size_
                           : list::kDefaultMainAxisItemSize));
}

float ItemHolder::GetBorder(list::FrameDirection frame_direction) const {
  return borders_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetPadding(list::FrameDirection frame_direction) const {
  return paddings_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetMargin(list::FrameDirection frame_direction) const {
  return margins_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetRTLLeft(float content_size, float container_size) const {
  if (orientation_ == list::Orientation::kHorizontal) {
    return std::max(content_size, container_size) - left_ - width_;
  } else {
    return container_size - left_ - width_;
  }
}

bool ItemHolder::IsAtStickyPosition(float content_offset, float list_height,
                                    float content_size, float sticky_offset,
                                    float start, float end) const {
  if (sticky_top_ && start < content_offset + sticky_offset) {
    return true;
  } else if (sticky_bottom_ &&
             end >= std::min(content_offset + list_height - sticky_offset,
                             content_size)) {
    return true;
  } else {
    return false;
  }
}

bool ItemHolder::VisibleInList(ListOrientationHelper* orientation_helper,
                               float content_offset) const {
  if (!orientation_helper) {
    return false;
  }
  float container_size = orientation_helper->GetMeasurement();
  float list_start = content_offset;
  float list_end = list_start + container_size;
  float start = orientation_helper->GetDecoratedStart(this);
  float end = orientation_helper->GetDecoratedEnd(this);
  return ((base::FloatsLarger(list_start, start) &&
           base::FloatsLarger(end, list_start)) ||
          (base::FloatsLarger(list_end, start) &&
           base::FloatsLarger(end, list_end)) ||
          (base::FloatsLargerOrEqual(start, list_start) &&
           base::FloatsLargerOrEqual(list_end, end)));
}

}  // namespace list
}  // namespace lynx
