// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/animation_item_holder.h"

#include <algorithm>

#include "base/include/float_comparison.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"

namespace lynx {
namespace tasm {

AnimationItemHolder::AnimationItemHolder(int index, const std::string& item_key)
    : ItemHolder(index, item_key){};

void AnimationItemHolder::DoAnimationFrame(float progress) {
  if (element_ && element_->element_container() &&
      (animation_origin_left_ != -1.f && animation_origin_top_ != -1.f)) {
    float l =
        animation_origin_left_ + (left() - animation_origin_left_) * progress;
    float t =
        animation_origin_top_ + (top() - animation_origin_top_) * progress;
    element_->UpdateLayout(
        GetRTLLeft(content_size_, container_size_, l, width_), t);
    element_->element_container()->UpdateLayout(
        GetRTLLeft(content_size_, container_size_, l, width_), t);
    element_->painting_context()->UpdateLayoutPatching();
    element_->painting_context()->OnNodeReady(element_->impl_id());
    element_->painting_context()->UpdateNodeReadyPatching();
    element_->painting_context()->Flush();
  }
}

void AnimationItemHolder::EndAnimation() {
  animation_origin_left_ = -1.f;
  animation_origin_top_ = -1.f;
}

void AnimationItemHolder::UpdateLayoutFromManager(float left, float top) {
  // Update left and top's value from list's layout manager.
  if (animation_delegate_ && animation_delegate_->InAnimationProcess() &&
      (left_ != left || top_ != top)) {
    animation_origin_left_ = left_;
    animation_origin_top_ = top_;
  }
  left_ = left;
  top_ = top;
}

void AnimationItemHolder::SetAnimationDelegate(
    ItemHolder::AnimationDelegate* delegate) {
  animation_delegate_ = delegate;
}

float AnimationItemHolder::GetRTLLeft(float content_size, float container_size,
                                      float left, float width) {
  if (orientation_ == list::Orientation::kHorizontal) {
    return std::max(content_size, container_size) - left - width;
  } else {
    return container_size - left - width;
  }
}

void AnimationItemHolder::UpdateLayoutToPlatform(float content_size,
                                                 float container_size,
                                                 Element* element) {
  if (element && element->element_container()) {
    if (animation_delegate_ && animation_delegate_->InAnimationProcess()) {
      content_size_ = content_size;
      container_size_ = container_size;
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
  }
}

}  // namespace tasm
}  // namespace lynx
