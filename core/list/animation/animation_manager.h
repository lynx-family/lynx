// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_MANAGER_H_
#define CORE_LIST_ANIMATION_ANIMATION_MANAGER_H_

#include <memory>

#include "core/list/animation/animation_types.h"

namespace lynx {
namespace list {

class ListContainerImpl;

// Defines the integration contract between ListContainerImpl and the list item
// animation pipeline. The concrete implementation is selected at build time by
// CreateAnimationManager().
class AnimationManager {
 public:
  AnimationManager() = default;
  virtual ~AnimationManager() = default;

  AnimationManager(const AnimationManager&) = delete;
  AnimationManager& operator=(const AnimationManager&) = delete;

  // Applies the complete runtime configuration for update animations.
  // Disabling animations cancels the active transaction immediately. Enabling
  // animations only affects eligible future data updates. While animations
  // remain enabled, duration changes are also applied to pending animations in
  // the active transaction, but do not affect BasicAnimator instances that are
  // already running.
  virtual void SetUpdateAnimationConfig(
      const UpdateAnimationConfig& config) = 0;

  // Called before the adapter updates its children and holder map to the latest
  // data source. The manager uses the diff validity, whether the diff is
  // expected to animate, and whether the first layout has completed to decide
  // whether to preserve or cancel the active transaction and whether to create
  // a new one.
  virtual void BeforeDataUpdate(bool has_valid_diff,
                                bool has_expected_diff_animation,
                                bool has_completed_first_layout) = 0;

  // Called before the list starts logical layout. Consumes the transaction's
  // PRE target snapshot and records PRE layout information for each live
  // target.
  virtual void BeforeLayout() = 0;

  // Called after logical layout completes but before its results are flushed to
  // the platform. Records POST layout information, transfers ownership of
  // removed holders to the transaction, and prepares pending animations.
  virtual void AfterLayoutBeforeFlush() = 0;

  // Called after the regular layout patches for this pass are flushed. Starts
  // pending animations, then flushes the animation property updates accumulated
  // while the batch was starting.
  virtual void AfterFlush() = 0;

  virtual void CancelAnimationTransaction(AnimationCancelReason reason) = 0;

  virtual void Destroy() = 0;
};

std::unique_ptr<AnimationManager> CreateAnimationManager(
    ListContainerImpl* list_container);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_MANAGER_H_
