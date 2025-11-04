// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/animation_item_holder.h"

#include <algorithm>
#include <cmath>

#include "base/include/float_comparison.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"

namespace lynx::tasm {

AnimationItemHolder::AnimationItemHolder(int index, const std::string& item_key,
                                         AnimationDelegate* delegate)
    : ItemHolder(index, item_key), animation_delegate_(delegate) {
  DCHECK(delegate);
}

void AnimationItemHolder::DoAnimationFrame(float progress) {
  if (element_ && element_->element_container()) {
    if (animation_type_ == list::ItemHolderAnimationType::kNone) {
      return;
    }
    DCHECK((animation_type_ == list::ItemHolderAnimationType::kTransform) ==
               !std::isnan(animation_origin_left_) &&
           (animation_type_ == list::ItemHolderAnimationType::kTransform) ==
               !std::isnan(animation_origin_top_));
    DCHECK((animation_type_ == list::ItemHolderAnimationType::kOpacity) ==
           !std::isnan(animation_origin_opacity_));
    if (animation_type_ == list::ItemHolderAnimationType::kTransform &&
        !std::isnan(animation_origin_left_) &&
        !std::isnan(animation_origin_top_)) {
      float l =
          animation_origin_left_ + (left() - animation_origin_left_) * progress;
      const float t =
          animation_origin_top_ + (top() - animation_origin_top_) * progress;
      if (direction_ == list::Direction::kRTL) {
        l = GetRTLLeft(content_size_, container_width_, l, width_);
      }
      element_->UpdateLayout(l, t);
      element_->element_container()->UpdateLayout(l, t);
      element_->element_container()->UpdateLayoutPatching();
      element_->OnNodeReady();
      element_->element_container()->UpdateNodeReadyPatching();
      element_->element_container()->Flush();
    } else if (animation_type_ == list::ItemHolderAnimationType::kOpacity &&
               !std::isnan(animation_origin_opacity_)) {
      element_->FlushAnimatedStyle(
          CSSPropertyID::kPropertyIDOpacity,
          tasm::CSSValue(static_cast<double>(
                             std::fabs(animation_origin_opacity_ - progress)),
                         tasm::CSSValuePattern::NUMBER));
      element_->OnNodeReady();
      element_->element_container()->UpdateNodeReadyPatching();
      element_->element_container()->Flush();
    }
  }
}

void AnimationItemHolder::EndAnimation() {
  if (animation_type_ == list::ItemHolderAnimationType::kTransform) {
    if (element_ && element_->element_container() &&
        (left_ != element_->left() || top_ != element_->top())) {
      if (direction_ == list::Direction::kRTL) {
        element_->UpdateLayout(
            GetRTLLeft(content_size_, container_width_, left_, width_), top_);
        element_->element_container()->UpdateLayout(
            GetRTLLeft(content_size_, container_width_, left_, width_), top_);
      } else {
        element_->UpdateLayout(left_, top_);
        element_->element_container()->UpdateLayout(left_, top_);
      }
    }
    animation_origin_left_ = std::numeric_limits<float>::quiet_NaN();
    animation_origin_top_ = std::numeric_limits<float>::quiet_NaN();
    content_size_ = std::numeric_limits<float>::quiet_NaN();
    container_width_ = std::numeric_limits<float>::quiet_NaN();
  } else if (animation_type_ == list::ItemHolderAnimationType::kOpacity) {
    if (element_) {
      element_->FlushAnimatedStyle(
          CSSPropertyID::kPropertyIDOpacity,
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

  animation_type_ = list::ItemHolderAnimationType::kNone;
}

void AnimationItemHolder::UpdateLayoutFromManager(float left, float top) {
  if (base::FloatsEqual(left, left_) && base::FloatsEqual(top, top_)) {
    return;
  }
  if (animation_delegate_->UpdateAnimation() &&
      animation_delegate_->AnimationType() !=
          list::ListContainerAnimationType::kNone &&
      std::isnan(animation_origin_left_) && std::isnan(animation_origin_top_)) {
    animation_origin_left_ = left_;
    animation_origin_top_ = top_;
    animation_type_ = list::ItemHolderAnimationType::kTransform;
    // NOTE: In an insert animation, the item could be set with opacity
    // animation first, currently we cover it with a transform animation.
    animation_origin_opacity_ = std::numeric_limits<float>::quiet_NaN();
  }
  // NOTE: When the coordinates are assigned multiple times during the
  // animation, we take the last assignment as the animation’s target position.
  left_ = left;
  top_ = top;
}

// Animation here means the `animation` of *this* item holder, nor all the list
// animations.
void AnimationItemHolder::RecycleAfterAnimation(
    list::ItemHolderAnimationType type) {
  if (!animation_delegate_->UpdateAnimation()) {
    return;
  }
  animation_type_ = type;
  if (type == list::ItemHolderAnimationType::kOpacity) {
    animation_origin_opacity_ = 1.f;
  }
  animation_delegate_->DeferredDestroyItemHolder(this);
}

float AnimationItemHolder::GetRTLLeft(float content_size, float container_size,
                                      float left, float width) const {
  if (orientation_ == list::Orientation::kHorizontal) {
    return std::max(content_size, container_size) - left - width;
  }
  return container_size - left - width;
}

void AnimationItemHolder::MarkInsertOpacity() {
  DCHECK(animation_type_ == list::ItemHolderAnimationType::kNone);
  animation_type_ = list::ItemHolderAnimationType::kOpacity;
  animation_origin_opacity_ = 0.f;
}

void AnimationItemHolder::UpdateLayoutToPlatform(float content_size,
                                                 float container_size,
                                                 Element* element) {
  if (element && element->element_container()) {
    if (animation_delegate_->UpdateAnimation() &&
        animation_type_ == list::ItemHolderAnimationType::kTransform) {
      // NOTE: In the remove animation, a new item holder is created, and we
      // need the new item holder to also run the transform animation.
    } else {
      if (direction_ == list::Direction::kRTL) {
        element->UpdateLayout(
            GetRTLLeft(content_size, container_size, left_, width_), top_);
        element->element_container()->UpdateLayout(
            GetRTLLeft(content_size, container_size, left_, width_), top_);
      } else {
        element->UpdateLayout(left_, top_);
        element->element_container()->UpdateLayout(left_, top_);
      }
    }
    content_size_ = content_size;
    container_width_ = container_size;
  }
}

}  // namespace lynx::tasm
