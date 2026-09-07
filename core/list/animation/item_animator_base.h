// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ITEM_ANIMATOR_BASE_H_
#define CORE_LIST_ANIMATION_ITEM_ANIMATOR_BASE_H_

#include "core/list/animation/item_animator.h"

namespace lynx {
namespace list {

// Provides the common conversion from high-level layout animation semantics to
// the primitive Add, Remove, Move, and single-target Change operations.
class ItemAnimatorBase : public ItemAnimator {
 public:
  ItemAnimatorBase() = default;
  ~ItemAnimatorBase() override = default;

  bool AnimateDisappearance(AnimationTarget* target,
                            const ItemLayoutInfo& pre_layout_info) override;
  bool AnimateAppearance(AnimationTarget* target,
                         const ItemLayoutInfo& post_layout_info) override;
  bool AnimatePersistence(AnimationTarget* target,
                          const ItemLayoutInfo& pre_layout_info,
                          const ItemLayoutInfo& post_layout_info) override;
  bool AnimateChange(AnimationTarget* target,
                     const ItemLayoutInfo& pre_layout_info,
                     const ItemLayoutInfo& post_layout_info) override;

 protected:
  virtual bool AnimateRemoveImpl(AnimationTarget* target,
                                 const ItemLayoutInfo& pre_layout_info) = 0;
  virtual bool AnimateAddImpl(AnimationTarget* target,
                              const ItemLayoutInfo& post_layout_info) = 0;
  virtual bool AnimateMoveImpl(AnimationTarget* target,
                               const ItemLayoutInfo& pre_layout_info,
                               const ItemLayoutInfo& post_layout_info) = 0;
  // Change receives one target and its PRE/POST snapshots. The interface does
  // not model separate old and new targets.
  virtual bool AnimateChangeImpl(AnimationTarget* target,
                                 const ItemLayoutInfo& pre_layout_info,
                                 const ItemLayoutInfo& post_layout_info) = 0;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ITEM_ANIMATOR_BASE_H_
