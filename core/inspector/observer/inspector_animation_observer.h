// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_INSPECTOR_OBSERVER_INSPECTOR_ANIMATION_OBSERVER_H_
#define CORE_INSPECTOR_OBSERVER_INSPECTOR_ANIMATION_OBSERVER_H_

namespace lynx {
namespace animation {
class Animation;
}  // namespace animation

namespace tasm {

// Observer for animation lifecycle events used by the Inspector (Animation CDP
// MVP). All methods have default empty implementations so that concrete
// observers only override the events they care about. When the Inspector is
// disabled, no observer is installed and the call sites compile out via
// EXEC_EXPR_FOR_INSPECTOR, so these callbacks are never invoked.
//
// Lifecycle mapping to CDP Animation events:
//   OnAnimationCreated  -> animationCreated (animation object created)
//   OnAnimationStarted  -> animationStarted  (first real sample)
//   OnAnimationUpdated  -> animationUpdated  (pause / natural finish)
//   OnAnimationCanceled -> animationCanceled (cancel / replace / destroy)
class InspectorAnimationObserver {
 public:
  virtual ~InspectorAnimationObserver() = default;

  virtual void OnAnimationCreated(animation::Animation* animation) {}
  virtual void OnAnimationStarted(animation::Animation* animation) {}
  virtual void OnAnimationUpdated(animation::Animation* animation) {}
  virtual void OnAnimationCanceled(animation::Animation* animation) {}
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_INSPECTOR_OBSERVER_INSPECTOR_ANIMATION_OBSERVER_H_
