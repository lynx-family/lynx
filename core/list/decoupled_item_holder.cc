// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_item_holder.h"

#include <algorithm>

#include "base/include/float_comparison.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/list/decoupled_list_orientation_helper.h"

namespace lynx {
namespace list {

ItemHolder::ItemHolder(int index, const std::string& item_key,
                       AnimationDelegate* delegate)
    : index_(index), item_key_(item_key), animation_delegate_(delegate) {
  DCHECK(delegate);
}

void ItemHolder::UpdateLayoutFromItemDelegate() {
  UpdateLayoutFromItemDelegate(item_delegate_);
}

void ItemHolder::UpdateLayoutFromItemDelegate(
    ItemElementDelegate* item_delegate) {
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
                                        float container_width) {
  UpdateLayoutToPlatform(content_size, container_width, item_delegate_);
}

void ItemHolder::UpdateLayoutToPlatform(float content_size,
                                        float container_width,
                                        ItemElementDelegate* item_delegate) {
  if (item_delegate) {
    if (animation_delegate_->UpdateAnimation() &&
        animation_type_ == ItemHolderAnimationType::kTransform) {
      // NOTE: In the remove animation, a new item holder is created, and we
      // need the new item holder to also run the transform animation.
    } else {
      if (direction_ == Direction::kRTL) {
        item_delegate->UpdateLayoutToPlatform(
            GetRTLLeft(content_size, container_width, left_, width_), top_);
      } else {
        item_delegate->UpdateLayoutToPlatform(left_, top_);
      }
    }
    // Record current content size and container width.
    content_size_ = content_size;
    container_width_ = container_width;
  }
}

void ItemHolder::UpdateLayoutFromManager(float left, float top) {
  // Update left and top's value from list's layout manager.
  if (base::FloatsEqual(left, left_) && base::FloatsEqual(top, top_)) {
    return;
  }
  if (animation_delegate_->UpdateAnimation() &&
      animation_delegate_->AnimationType() != ListAnimationType::kNone &&
      std::isnan(animation_origin_left_) && std::isnan(animation_origin_top_)) {
    animation_origin_left_ = left_;
    animation_origin_top_ = top_;
    animation_type_ = ItemHolderAnimationType::kTransform;
    // NOTE: In an insert animation, the item could be set with opacity
    // animation first, currently we cover it with a transform animation.
    animation_origin_opacity_ = std::numeric_limits<float>::quiet_NaN();
  }
  // NOTE: When the coordinates are assigned multiple times during the
  // animation, we take the last assignment as the animation’s target position.
  left_ = left;
  top_ = top;
}

void ItemHolder::DoAnimationFrame(float progress) {
  ItemElementDelegate* item_delegate =
      animation_delegate_->GetItemElementDelegate(this);
  if (item_delegate) {
    if (animation_type_ == ItemHolderAnimationType::kNone) {
      return;
    }
    DCHECK((animation_type_ == ItemHolderAnimationType::kTransform) ==
               !std::isnan(animation_origin_left_) &&
           (animation_type_ == ItemHolderAnimationType::kTransform) ==
               !std::isnan(animation_origin_top_));
    DCHECK((animation_type_ == ItemHolderAnimationType::kOpacity) ==
           !std::isnan(animation_origin_opacity_));
    if (animation_type_ == ItemHolderAnimationType::kTransform &&
        !std::isnan(animation_origin_left_) &&
        !std::isnan(animation_origin_top_)) {
      float l =
          animation_origin_left_ + (left_ - animation_origin_left_) * progress;
      const float t =
          animation_origin_top_ + (top_ - animation_origin_top_) * progress;
      if (direction_ == Direction::kRTL) {
        l = GetRTLLeft(content_size_, container_width_, l, width_);
      }
      item_delegate->UpdateLayoutToPlatform(l, t);
      item_delegate->FlushPatching();
    } else if (animation_type_ == ItemHolderAnimationType::kOpacity &&
               !std::isnan(animation_origin_opacity_)) {
      item_delegate->FlushAnimatedStyle(
          tasm::CSSPropertyID::kPropertyIDOpacity,
          tasm::CSSValue(std::fabs(animation_origin_opacity_ - progress),
                         tasm::CSSValuePattern::NUMBER));
    }
  }
}

void ItemHolder::EndAnimation() {
  ItemElementDelegate* item_delegate =
      animation_delegate_->GetItemElementDelegate(this);
  if (animation_type_ == ItemHolderAnimationType::kTransform) {
    if (item_delegate && (left_ != item_delegate->GetLeft() ||
                          top_ != item_delegate->GetTop())) {
      if (direction_ == Direction::kRTL) {
        item_delegate->UpdateLayoutToPlatform(
            GetRTLLeft(content_size_, container_width_, left_, width_), top_);
      } else {
        item_delegate->UpdateLayoutToPlatform(left_, top_);
      }
    }
    animation_origin_left_ = std::numeric_limits<float>::quiet_NaN();
    animation_origin_top_ = std::numeric_limits<float>::quiet_NaN();
    content_size_ = std::numeric_limits<float>::quiet_NaN();
    container_width_ = std::numeric_limits<float>::quiet_NaN();
  } else if (animation_type_ == ItemHolderAnimationType::kOpacity) {
    if (item_delegate) {
      item_delegate->FlushAnimatedStyle(
          tasm::CSSPropertyID::kPropertyIDOpacity,
          tasm::CSSValue(1, tasm::CSSValuePattern::NUMBER));
    }
    if (animation_origin_opacity_ == 1.f) {
      animation_delegate_->RecycleItemHolder(this);
    }
    animation_origin_opacity_ = std::numeric_limits<float>::quiet_NaN();
  }
  DCHECK(std::isnan(animation_origin_top_));
  DCHECK(std::isnan(animation_origin_left_));
  DCHECK(std::isnan(animation_origin_opacity_));
  animation_type_ = ItemHolderAnimationType::kNone;
}

// Animation here means the `animation` of *this* item holder, nor all the list
// animations.
void ItemHolder::RecycleAfterAnimation(ItemHolderAnimationType type) {
  if (!animation_delegate_->UpdateAnimation()) {
    return;
  }
  animation_type_ = type;
  if (type == ItemHolderAnimationType::kOpacity) {
    animation_origin_opacity_ = 1.f;
  }
  animation_delegate_->DeferredDestroyItemHolder(this);
}

void ItemHolder::MarkInsertOpacity() {
  DCHECK(animation_type_ == ItemHolderAnimationType::kNone);
  animation_type_ = ItemHolderAnimationType::kOpacity;
  animation_origin_opacity_ = 0.f;
}

float ItemHolder::height() const {
  if (orientation_ == Orientation::kHorizontal) {
    return base::FloatsLargerOrEqual(height_, 0.f) ? height_ : 0.f;
  }
  return GetSizeInMainAxis();
}

float ItemHolder::width() const {
  if (orientation_ == Orientation::kVertical) {
    return base::FloatsLargerOrEqual(width_, 0.f) ? width_ : 0.f;
  }
  return GetSizeInMainAxis();
}

float ItemHolder::GetSizeInMainAxis() const {
  // If the ItemHolder is never bound, we use estimated size or container size.
  float main_axis_size =
      orientation_ == Orientation::kVertical ? height_ : width_;
  return base::FloatsLargerOrEqual(main_axis_size, 0.f)
             ? main_axis_size
             : (base::FloatsLargerOrEqual(estimated_size_, 0.f)
                    ? estimated_size_
                    : (base::FloatsLarger(container_size_, 0.f)
                           ? container_size_
                           : kDefaultMainAxisItemSize));
}

float ItemHolder::GetBorder(FrameDirection frame_direction) const {
  return borders_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetPadding(FrameDirection frame_direction) const {
  return paddings_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetMargin(FrameDirection frame_direction) const {
  return margins_[static_cast<uint32_t>(frame_direction)];
}

float ItemHolder::GetRTLLeft(float content_size, float container_width,
                             float left, float width) const {
  if (orientation_ == Orientation::kHorizontal) {
    return std::max(content_size, container_width) - left - width;
  }
  return container_width - left - width;
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
