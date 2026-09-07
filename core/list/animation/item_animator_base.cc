// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator_base.h"

namespace lynx {
namespace list {

bool ItemAnimatorBase::AnimateDisappearance(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info) {
  // In the current Recorder model, a disappearing target has only a PRE
  // layout snapshot.
  return AnimateRemoveImpl(target, pre_layout_info);
}

bool ItemAnimatorBase::AnimateAppearance(
    AnimationTarget* target, const ItemLayoutInfo& post_layout_info) {
  // In the current Recorder model, an appearing target has only a POST layout
  // snapshot.
  return AnimateAddImpl(target, post_layout_info);
}

bool ItemAnimatorBase::AnimatePersistence(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info,
    const ItemLayoutInfo& post_layout_info) {
  return AnimateMoveImpl(target, pre_layout_info, post_layout_info);
}

bool ItemAnimatorBase::AnimateChange(AnimationTarget* target,
                                     const ItemLayoutInfo& pre_layout_info,
                                     const ItemLayoutInfo& post_layout_info) {
  return AnimateChangeImpl(target, pre_layout_info, post_layout_info);
}

}  // namespace list
}  // namespace lynx
