// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_TARGET_H_
#define CORE_LIST_ANIMATION_ANIMATION_TARGET_H_

#include <string>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/list/animation/animation_types.h"
#include "core/renderer/css/css_property_id.h"
#include "core/renderer/css/css_value.h"

namespace lynx {
namespace list {

class AnimationTarget;

using WeakAnimationTarget = fml::WeakPtr<AnimationTarget>;

// An abstract item node that can participate in list item animations.
//
// AnimationTarget does not own the underlying ItemHolder or platform node. The
// animation pipeline generally tracks targets through weak pointers and must
// tolerate their expiration. Removed ItemHolders are the exception: the active
// transaction owns deferred holders until the transaction completes or is
// cancelled.
class AnimationTarget {
 public:
  virtual ~AnimationTarget() = default;

  // Returns a weak pointer to this animation target.
  //
  // AnimationTarget does not create the WeakPtr itself. Each concrete class
  // creates it through its own EnableWeakFromThis implementation.
  virtual WeakAnimationTarget GetWeakAnimationTarget() const = 0;

  // Used for stable identity diagnostics. Animation matching must not use the
  // adapter index because it may have already changed before layout starts.
  virtual const std::string& GetAnimationKey() const = 0;

  // Returns the latest adapter index. This is not necessarily the index from
  // the last completed layout.
  virtual int GetAnimationIndex() const = 0;

  // Returns the target's current logical layout bounds.
  virtual float GetAnimationLeft() const = 0;

  virtual float GetAnimationTop() const = 0;

  virtual float GetAnimationWidth() const = 0;

  virtual float GetAnimationHeight() const = 0;

  // Marks the target as entering its animation lifecycle. Implementations may
  // use this to defer recycling or prevent non-animation paths from overwriting
  // the current presentation state.
  virtual void PrepareForAnimation(ItemAnimationType animation_type) = 0;

  // Ends the animation lifecycle while the target is still safe to access.
  // This is called after normal completion, pending-animation cancellation, and
  // running-animation cancellation outside destroy mode. Implementations must
  // be idempotent.
  virtual void FinishAnimation() = 0;

  // Applies an animated opacity value to the target's platform node.
  virtual void UpdateAnimationOpacity(float opacity,
                                      bool flush_immediately) = 0;

  // Updates only the target's presentation position. This must not overwrite
  // the final logical position stored by ItemHolder. Coordinate conversion,
  // such as RTL conversion, is handled by the concrete target.
  virtual void UpdateAnimationPosition(float left, float top,
                                       bool flush_immediately) = 0;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_TARGET_H_
