// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator.h"

#include "base/include/log/logging.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

ItemLayoutInfo ItemAnimator::GetItemLayoutInfo(const AnimationTarget& target) {
  ItemLayoutInfo layout_info;
  layout_info.left_ = target.GetAnimationLeft();
  layout_info.top_ = target.GetAnimationTop();
  layout_info.right_ = layout_info.left_ + target.GetAnimationWidth();
  layout_info.bottom_ = layout_info.top_ + target.GetAnimationHeight();
  return layout_info;
}

ItemLayoutInfo ItemAnimator::GetPreItemLayoutInfo(
    const AnimationTarget* target) {
  if (!target) {
    DLIST_LOGE("[" << this << "] ItemAnimator::GetPreItemLayoutInfo: "
                   << "target is nullptr");
    DCHECK(target);
    return {};
  }
  return GetItemLayoutInfo(*target);
}

ItemLayoutInfo ItemAnimator::GetPostItemLayoutInfo(
    const AnimationTarget* target) {
  if (!target) {
    DLIST_LOGE("[" << this << "] ItemAnimator::GetPostItemLayoutInfo: "
                   << "target is nullptr");
    DCHECK(target);
    return {};
  }
  return GetItemLayoutInfo(*target);
}

}  // namespace list
}  // namespace lynx
