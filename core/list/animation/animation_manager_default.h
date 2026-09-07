// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_MANAGER_DEFAULT_H_
#define CORE_LIST_ANIMATION_ANIMATION_MANAGER_DEFAULT_H_

#include "core/list/animation/animation_manager.h"

namespace lynx {
namespace list {

// No-op implementation used by builds that compile out list item animation.
class AnimationManagerDefault final : public AnimationManager {
 public:
  AnimationManagerDefault() = default;
  ~AnimationManagerDefault() override;

  void SetUpdateAnimationConfig(const UpdateAnimationConfig& config) override;

  void BeforeDataUpdate(bool has_valid_diff, bool has_expected_diff_animation,
                        bool has_completed_first_layout) override;
  void BeforeLayout() override;
  void AfterLayoutBeforeFlush() override;
  void AfterFlush() override;
  void CancelAnimationTransaction(AnimationCancelReason reason) override;
  void Destroy() override;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_MANAGER_DEFAULT_H_
